# **Zombie Master: Reborn**
An FPS/RTS hybrid game on Source 2013 engine, supporting Linux and Windows. See more info at our [ModDB page](https://www.moddb.com/mods/zombie-master-reborn)!

## License
You can read it [here](https://github.com/zm-reborn/zmr-game/blob/master/LICENSE).

## Building

### Windows
1. Download [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) & [Python](https://www.python.org/).
2. In VS, install `Desktop development with C++` workload.
    * The required components are: **MSVC v143** and **Windows SDK 10.0** (10.0.20348.0 tested)
3. Run `creategameprojects.bat` in `src`-folder.
4. You now have a solution file `zmr-games.sln` which you can open and build.

### Linux

You should use the Steam Runtime SDK to compile. Here's an example using Docker:

```bash
docker run --rm -v .:/build/ -it "registry.gitlab.steamos.cloud/steamrt/sniper/sdk" /bin/bash
# You should now have a shell in the container.

cd /build/src
./creategameprojects
./build game
# Optionally strip debug symbols
./strip_debug_symbols
```

## Running

- Requires Source SDK Base 2013 Multiplayer to be downloaded from Steam. (In the Tools-page)
- Make sure you have cloned the game-folder submodule (`game/zombie_master_reborn`).


### Windows

Launch with the debugger (F5) in Visual Studio. :)

### Linux

You can create a symbolic link of `game/zombie_master_reborn` in your `<Steam>/steamapps/common/Source SDK Base 2013 Multiplayer`-folder and then run `hl2.exe` with arguments `-game zombie_master_reborn`

Start hl2 with the Steam runtime.
`~/.local/share/Steam/ubuntu12_32/steam-runtime/run.sh ./hl2.sh -game zombie_master_reborn`

## Final Build

Turning on the conditional `ZMR_FINAL` will enable FMOD sound engine and Discord RPC.

FMOD headers need to be placed in `src/public/fmod/` and library `fmod_vc.lib` in `src/lib/public/x64/`. These files cannot be distributed here.

Steam build is activated with the `ZMR_STEAM`-conditional.

```bash
# You might have to use /f to force a rebuild.
cd src && ./creategameprojects /ZMR_FINAL
```

You'll also need a copy of the binaries to run the game. (`discord_game_sdk.dll` & `fmod.dll`)

## FAQ

### Project refuses to compile, says something about wrong/missing toolset. / Opening the solution says something about upgrading.

Open project settings and make sure `Platform Toolset` is `v143` and `Windows SDK Version` is `10.0 (latest installed version)` or lower.
If it says one of them is missing, you need to download them from Visual Studio Installer (Tools -> Get Tools and Features)

### Other problems

Just [ask Mehis in Discord](https://discord.gg/tZTUex3).

## Contributing

**Please base all pull requests on the 'dev' branch.** If you're planning on something bigger or you want to work on one of the Issues, do contact us so your efforts don't go to waste.
