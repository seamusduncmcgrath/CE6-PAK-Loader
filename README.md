[center][size=6][b]Chrome Engine 6 PAK/RPACK Loader[/b][/size]
[size=4][i]For Dying Light, Dead Island Definitive, & Riptide Definitive[/i][/size][/center]

[line]

[size=5][b]Description[/b][/size]
This is a lightweight, open-source Mod Loader built from scratch for Chrome Engine 6 games. It uses simple engine hooks to load the mods, which is very unlikely to have issues

It is designed to work on Dying Light, Dead Island: Definitive Edition, and Dead Island Riptide: Definitive Edition

[size=5][b]Features[/b][/size]
[list]
[*] [b]Near Universal Support:[/b] Works natively with [i]Dying Light[/i], [i]Dead Island Definitive Edition[/i], and [i]Dead Island Riptide Definitive Edition [/i](No RPACK support for Dead Island yet).
[*] [b]Advanced File Loading:[/b] Supports loading standard [b].pak[/b] archives and [b].rpack[/b] (Resource Packs) for textures and meshes (Dying Light only).
[*] [b]Load Order System:[/b] Automatically generates a [font=Courier New]load_order.txt[/font] file in your mods folder. You can edit this file to change mod priority.
[*] [b]CRC Bypass:[/b] Automatically bypasses file integrity checks, allowing you to modify base game archives (like Data0.pak) without the game crashing.
[*] [b]Console and Logging:[/b] Includes a clean debug console and log file (toggleable) to verify exactly which mods loaded successfully.
[*] [b]Safe To Uninstall:[/b] Does not permanently modify any game executables. To uninstall, just delete the file.
[/list]

[line]

[size=5][b]Installation[/b][/size]
[list=1]
[*] Download the latest release.
[*] Extract the files into your game directory (the same folder as [font=Courier New]DyingLightGame.exe[/font] or [font=Courier New]DeadIslandGame.exe[/font]).
[*] Create a folder named [b]CustomMods[/b] in the same directory.
[*] Place your .pak or .rpack mod files inside [b]CustomMods[/b].
[*] Run the game!
[/list]

[size=5][b]How to Use Load Order[/b][/size]
On the first run, the loader will generate [font=Courier New]CustomMods\load_order.txt[/font].
[list]
[*] The loader reads this file from [b]Top to Bottom[/b].
[*] Files at the [b]Bottom[/b] have the highest priority (they overwrite files above them).
[*] New mods added to the folder are automatically appended to the end of the list.
[/list]

[line]

[size=5][b]Configuration[/b][/size]
You can configure the loader by editing [font=Courier New]ModLoader.ini[/font]:
[code]
[General]
EnableConsole=1   ; Set to 0 to hide the command window
EnableLogging=1   ; Set to 0 to disable log files
LoadRPacks=1      ; Set to 0 to disable RPACK injection
[/code]

[size=5][b]Credits[/b][/size]
[list]
[*]SteffenL for DIDE mod loader, which I used as a reference point. https://github.com/SteffenL/dide-mod
[*] Special thanks to Dadelues for general help and DLCE, which was also used as a reference. https://www.nexusmods.com/profile/12brendon34
[*]ThirteenAG for UltimateASI loader. https://github.com/ThirteenAG/Ultimate-ASI-Loader
[/list]
