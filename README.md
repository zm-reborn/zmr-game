# **Zombie Master: Reborn**
An FPS/RTS hybrid game on Source 2013 engine, supporting Linux and Windows. See more info at our [ModDB page](https://www.moddb.com/mods/zombie-master-reborn)!

## License
You can read it [here](https://github.com/zm-reborn/zmr-game/blob/master/LICENSE).

## Building

### Windows
1. Download [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)
2. Download components: **MSVC v143** and **Windows SDK 10.0** (10.0.20348.0 tested).
3. Run `mp/src/creategameprojects.bat`.
4. You now have a solution file `mp/src/zmr-games.sln`

### Linux

Follow [this tutorial](https://developer.valvesoftware.com/wiki/Source_SDK_2013) in Valve developer wiki for the steps.

## Running

- Requires Source SDK Base 2013 Multiplayer to be downloaded from Steam. (In the Tools-page)
- Make sure you have cloned the game-folder submodule (`mp/game/zombie_master_reborn`).


You can create a symbolic link of `mp/game/zombie_master_reborn` in your `<Steam>/steamapps/common/Source SDK Base 2013 Multiplayer`-folder and then run `hl2.exe` with arguments `-game zombie_master_reborn`

Or you can use the script below.

### Windows Script
Run `zmr_dev_setup.ps1` inside `mp/game/zombie_master_reborn`. There should now be a `zmr_dev.bat`-batch file in your `Source SDK Base 2013 Multiplayer`-folder which you can run to start the mod.

### Linux

Start hl2 with the Steam runtime.
`~/.local/share/Steam/ubuntu12_32/steam-runtime/run.sh ./hl2.sh -game zombie_master_reborn`

## Final Build

Turning on the conditional `ZMR_FINAL` in `mp/src/vpc_scripts/default.vgc` will enable FMOD sound engine and Discord RPC. FMOD headers need to be placed in `public/fmod/` (they cannot be distributed here).

You'll also need a copy of the binaries to run the game. (discord-rpc.dll & fmod.dll)

## FAQ

### Is compiling for Linux really this convoluted?

Yes.

### Project refuses to compile, says something about wrong/missing toolset. / Opening the solution says something about upgrading.

Open project settings and make sure `Platform Toolset` is `v143` and `Windows SDK Version` is `10.0 (latest installed version)` or lower.
If it says one of them is missing, you need to download them from Visual Studio Installer (Tools -> Get Tools and Features)

If you're having problems, just [ask Mehis in Discord](https://discord.gg/tZTUex3).

## Contributing

**Please base all pull requests on the 'dev' branch.** If you're planning on something bigger or you want to work on one of the Issues, do contact us so your efforts don't go to waste.
