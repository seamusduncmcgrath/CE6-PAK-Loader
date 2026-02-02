#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <MinHook.h>

namespace fs = std::filesystem;

// GLOBALS
HMODULE g_hModule = nullptr;

// GAME DETECTION
enum class GameType {
    Unknown,
    DyingLight,
    DeadIsland_DE,
    DeadIsland_Riptide_DE
};

struct GameConfig {
    GameType type;
    std::string name;
    uintptr_t rpackOffset;
};

GameConfig CurrentGame = { GameType::Unknown, "Unknown Game", 0x0 };

void DetectGame() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exeName = fs::path(buffer).filename().string();
    std::transform(exeName.begin(), exeName.end(), exeName.begin(), ::tolower);

    if (exeName.find("dyinglight") != std::string::npos) {
        CurrentGame = { GameType::DyingLight, "Dying Light", 0x401a80 };
    } // add offsets to DIDE for rpacks
    else if (exeName.find("deadislandgame") != std::string::npos) {
        CurrentGame = { GameType::DeadIsland_DE, "Dead Island Definitive", 0x0 };
    }
    else if (exeName.find("deadislandriptidegame") != std::string::npos) {
        CurrentGame = { GameType::DeadIsland_Riptide_DE, "DI Riptide Definitive", 0x0 };
    }
}


// TYPEDEFS
enum class FFSAddSourceFlags : unsigned int { GENERIC = 0, DATA_FOLDER = 7, PAK_FILE = 9 };
enum class EIsGlobalPack : int { No = 0, Yes = 1 };
enum class EIsContentPack : int { No = 0, Yes = 1 };
enum class EUseCachePartition : int { No = 0, Yes = 1 };
enum class EIsCrossLevelPack : int { No = 0, Yes = 1 };
enum class EPackKind : int {};

typedef bool(__fastcall* fs_add_source_t)(const char* path, unsigned int flags);
typedef bool(__fastcall* fs_check_zip_crc_t)(void* _this);
typedef bool(__fastcall* LoadDataPack_t)(void* pRuntime, const char* path, void** outPack, EIsGlobalPack, EIsContentPack, EUseCachePartition, EIsCrossLevelPack, EPackKind);

fs_add_source_t fs_add_source_original = nullptr;
fs_check_zip_crc_t fs_check_zip_crc_original = nullptr;
LoadDataPack_t LoadDataPack_original = nullptr;

// STATE & HELPERS
struct {
    bool EnableConsole = true;
    bool EnableLogging = true;
    bool LoadRPacks = true;
} Config;

bool g_FilesystemReady = false;
bool g_RPacksLoaded = false;
bool g_FolderMounted = false;
std::ofstream g_LogFile;
HANDLE g_hConsole = nullptr;

enum class Color { White = 7, Green = 10, Red = 12, Yellow = 14, Cyan = 11 };
void SetColor(Color c) { if (g_hConsole) SetConsoleTextAttribute(g_hConsole, (WORD)c); }

void Log(const std::string& msg, Color color = Color::White, bool fileOnly = false) {
    if (Config.EnableLogging && g_LogFile.is_open()) {
        g_LogFile << msg << std::endl;
        g_LogFile.flush();
    }
    if (!fileOnly && Config.EnableConsole && g_hConsole) {
        SetColor(color);
        std::cout << "[MOD] " << msg << std::endl;
        SetColor(Color::White);
    }
}

std::string GetModLoaderDir() {
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(g_hModule, buffer, MAX_PATH)) {
        return fs::path(buffer).parent_path().string();
    }
    return fs::current_path().string(); // Fallback
}

void InitConsole() {
    if (!Config.EnableConsole) return;
    AllocConsole();
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    std::cout.clear();

    std::string title = "CE6 Mod Loader | " + CurrentGame.name;
    SetConsoleTitleA(title.c_str());

    SetColor(Color::Cyan);
    std::cout << "--- CE6 PAK/RPACK LOADER ---\n";
    std::cout << "Game: " << CurrentGame.name << "\n";
    std::cout << "----------------------------\n";
    SetColor(Color::White);
}

void LoadConfig() {
    std::string iniPath = GetModLoaderDir() + "\\ModLoader.ini";
    Config.EnableConsole = GetPrivateProfileIntA("General", "EnableConsole", 1, iniPath.c_str());
    Config.EnableLogging = GetPrivateProfileIntA("General", "EnableLogging", 1, iniPath.c_str());
    Config.LoadRPacks = GetPrivateProfileIntA("General", "LoadRPacks", 1, iniPath.c_str());
}

std::string RemoveSuffix(std::string str, const std::string& suffix) {
    if (str.length() < suffix.length()) return str;

    std::string strLower = str;
    std::string sufLower = suffix;
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    std::transform(sufLower.begin(), sufLower.end(), sufLower.begin(), ::tolower);

    if (strLower.compare(strLower.length() - sufLower.length(), sufLower.length(), sufLower) == 0) {
        return str.substr(0, str.length() - suffix.length());
    }
    return str;
}

// LOAD ORDER SYSTEM
void UpdateLoadOrderFile() {
    std::string modDir = GetModLoaderDir() + "\\CustomMods";
    std::string orderPath = modDir + "\\load_order.txt";

    if (!fs::exists(modDir)) {
        fs::create_directory(modDir);
        return;
    }

    // Look for all valid mod files in the folder
    std::vector<std::string> diskFiles;
    for (const auto& entry : fs::directory_iterator(modDir)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".pak" || ext == ".mpak" || ext == ".rpack") {
            diskFiles.push_back(entry.path().filename().string());
        }
    }

    // Read existing load_order.txt
    std::vector<std::string> existingList;
    if (fs::exists(orderPath)) {
        std::ifstream file(orderPath);
        std::string line;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) existingList.push_back(line);
        }
    }

    //Append New Files
    std::vector<std::string> newFiles;
    for (const auto& file : diskFiles) {
        bool found = false;
        // Case-insensitive search for file in list
        for (const auto& existing : existingList) {
            std::string a = file; std::string b = existing;
            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            if (a == b) { found = true; break; }
        }
        if (!found) newFiles.push_back(file);
    }

    if (!newFiles.empty()) {
        std::ofstream outfile(orderPath, std::ios_base::app);
        if (existingList.empty()) outfile << "; Auto-Generated Load Order\n; Top = Low Priority, Bottom = High Priority\n";
        else outfile << "\n";

        for (const auto& file : newFiles) {
            outfile << file << "\n";
            Log("Added new mod to load order: " + file, Color::Yellow);
        }
    }
    else if (!fs::exists(orderPath)) {
        std::ofstream outfile(orderPath);
        outfile << "; Auto-Generated Load Order\n; Top = Low Priority, Bottom = High Priority\n";
    }
}

std::vector<std::string> GetSortedModFiles(const std::string& extension) {
    std::string modDir = GetModLoaderDir() + "\\CustomMods";
    std::string orderPath = modDir + "\\load_order.txt";
    std::vector<std::string> sortedFiles;

    if (fs::exists(orderPath)) {
        std::ifstream file(orderPath);
        std::string line;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.length() > extension.length()) {
                std::string lineExt = line.substr(line.length() - extension.length());
                std::transform(lineExt.begin(), lineExt.end(), lineExt.begin(), ::tolower);

                std::string targetExt = extension;
                std::transform(targetExt.begin(), targetExt.end(), targetExt.begin(), ::tolower);

                if (lineExt == targetExt) {
                    if (fs::exists(modDir + "\\" + line)) {
                        sortedFiles.push_back(line);
                    }
                }
            }
        }
    }
    return sortedFiles;
}

// LOADERS
void LoadCustomRPacks(void* pRuntime) {
    if (!Config.LoadRPacks || CurrentGame.rpackOffset == 0) return;

    Log("Loading RPACKs...", Color::Cyan);
    std::string modDir = GetModLoaderDir() + "\\CustomMods";

    if (!g_FolderMounted) {
        g_FolderMounted = true;
        fs_add_source_original(modDir.c_str(), (unsigned int)FFSAddSourceFlags::DATA_FOLDER);
    }

    auto rpackFiles = GetSortedModFiles(".rpack");

    for (const auto& filename : rpackFiles) {
        //Some suffix stuff, could prob remove
        std::string cleanName = RemoveSuffix(filename, "_pc.rpack");
        if (cleanName == filename) cleanName = RemoveSuffix(filename, ".rpack");

        bool result = LoadDataPack_original(
            pRuntime, cleanName.c_str(), nullptr,
            EIsGlobalPack::Yes, EIsContentPack::No, EUseCachePartition::Yes,
            EIsCrossLevelPack::Yes, (EPackKind)0
        );

        if (result) Log(" [RPACK] Loaded: " + filename, Color::Green);
        else        Log(" [RPACK] Failed: " + filename, Color::Red);
    }
}

// HOOKS
bool __fastcall LoadDataPack_Detour(
    void* pRuntime, const char* path, void** outPack,
    EIsGlobalPack isGlobal, EIsContentPack isContent,
    EUseCachePartition isCache, EIsCrossLevelPack isCross, EPackKind kind)
{
    if (!g_RPacksLoaded) {
        g_RPacksLoaded = true;
        LoadCustomRPacks(pRuntime);
    }
    return LoadDataPack_original(pRuntime, path, outPack, isGlobal, isContent, isCache, isCross, kind);
}

bool __fastcall fs_add_source_detour(const char* path, unsigned int flags) {
    bool result = fs_add_source_original(path, flags);

    if (!g_FilesystemReady && path && strstr(path, "Data3.pak")) {
        g_FilesystemReady = true;
        Log("filesystem_x64_rwdi.dll Ready. Loading PAKs...", Color::Cyan);

        std::string modDir = GetModLoaderDir() + "\\CustomMods";

        if (fs::exists(modDir)) {
            if (!g_FolderMounted) {
                g_FolderMounted = true;
                fs_add_source_original(modDir.c_str(), (unsigned int)FFSAddSourceFlags::DATA_FOLDER);
            }

            // PAKS
            auto pakFiles = GetSortedModFiles(".pak");
            for (const auto& filename : pakFiles) {
                std::string fullPath = modDir + "\\" + filename;
                if (fs_add_source_original(fullPath.c_str(), (unsigned int)FFSAddSourceFlags::PAK_FILE)) {
                    Log(" [PAK]   Loaded: " + filename, Color::Green);
                }
                else {
                    Log(" [PAK]   Failed: " + filename, Color::Red);
                }
            }
        }
    }
    return result;
}

bool __fastcall check_zip_crc_detour(void* _this) { return true; } // CRC disable stuff

// INIT
void InitModLoader() {
    DetectGame();
    LoadConfig();
    InitConsole();

    if (Config.EnableLogging) {
        std::string logPath = GetModLoaderDir() + "\\mod_debug.log";
        g_LogFile.open(logPath);
        Log("Logger Initialized.", Color::White, true);
    }

    UpdateLoadOrderFile();

    HMODULE hFsModule = GetModuleHandleA("filesystem_x64_rwdi.dll");
    HMODULE hEngModule = GetModuleHandleA("engine_x64_rwdi.dll");

    if (!hFsModule || !hEngModule) {
        Log("FATAL: Game DLLs not found!", Color::Red);
        return;
    }

    MH_Initialize();

    void* addrAdd = GetProcAddress(hFsModule, "?add_source@fs@@YA_NPEBDW4ENUM@FFSAddSourceFlags@@@Z");
    void* addrCrc = GetProcAddress(hFsModule, "?check_zip_crc@izipped_buffer_file@fs@@UEAA_NXZ");

    if (addrAdd) { MH_CreateHook(addrAdd, &fs_add_source_detour, (LPVOID*)&fs_add_source_original); MH_EnableHook(addrAdd); }
    if (addrCrc) { MH_CreateHook(addrCrc, &check_zip_crc_detour, (LPVOID*)&fs_check_zip_crc_original); MH_EnableHook(addrCrc); }

    if (CurrentGame.rpackOffset != 0) {
        uintptr_t rpackAddr = (uintptr_t)hEngModule + CurrentGame.rpackOffset;
        MH_CreateHook((LPVOID)rpackAddr, &LoadDataPack_Detour, (LPVOID*)&LoadDataPack_original);
        MH_EnableHook((LPVOID)rpackAddr);
    }

    Log("Loader Active.", Color::White);

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)InitModLoader, nullptr, 0, nullptr);
    }
    return TRUE;
}