Chrome Engine 6 PAK/RPACK Loader
For Dying Light, Dead Island Definitive, & Riptide Definitive




Description
This is a lightweight, open-source Mod Loader built from scratch for Chrome Engine 6 games. It uses simple engine hooks to load the mods, which is very unlikely to have issues

It is designed to work on Dying Light, Dead Island: Definitive Edition, and Dead Island Riptide: Definitive Edition

Features
Near Universal Support: Works natively with Dying Light, Dead Island Definitive Edition, and Dead Island Riptide Definitive Edition (No RPACK support for Dead Island yet).
Advanced File Loading: Supports loading standard .pak archives and .rpack (Resource Packs) for textures and meshes (Dying Light only).
Load Order System: Automatically generates a load_order.txt file in your mods folder. You can edit this file to change mod priority.
CRC Bypass: Automatically bypasses file integrity checks, allowing you to modify base game archives (like Data0.pak) without the game crashing.
Console and Logging: Includes a clean debug console and log file (toggleable) to verify exactly which mods loaded successfully.
Safe To Uninstall: Does not permanently modify any game executables. To uninstall, just delete the file.




Installation
Download the latest release.
Extract the files into your game directory (the same folder as DyingLightGame.exe or DeadIslandGame.exe).
Create a folder named CustomMods in the same directory.
Place your .pak or .rpack mod files inside CustomMods.
Run the game!


How to Use Load Order
On the first run, the loader will generate CustomMods\load_order.txt.
The loader reads this file from Top to Bottom.
Files at the Bottom have the highest priority (they overwrite files above them).
New mods added to the folder are automatically appended to the end of the list.




Configuration
You can configure the loader by editing ModLoader.ini:

[General]
EnableConsole=1; Set to 0 to hide the command window
EnableLogging=1; Set to 0 to disable log files
LoadRPacks=1; Set to 0 to disable RPACK injection


Credits
SteffenL for DIDE mod loader, which I used as a reference point. https://github.com/SteffenL/dide-mod
Special thanks to Dadelues for general help and DLCE, which was also used as a reference. https://www.nexusmods.com/profile/12brendon34
ThirteenAG for UltimateASI loader. https://github.com/ThirteenAG/Ultimate-ASI-Loader
