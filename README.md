# **Zombie Master: Reborn**
An FPS/RTS hybrid game on Source 2013 engine, supporting Linux and Windows.

## License
You can read it [here](https://github.com/zm-reborn/zmr-game/blob/master/LICENSE).

## Building
- Follow [this tutorial](https://developer.valvesoftware.com/wiki/Source_SDK_2013) in Valve wiki for **Linux building steps**.
- Download [Visual Studio 2019](https://visualstudio.microsoft.com/vs/)
- You need components: **MSVC v142** and **Windows SDK 10.0** .
- Run `mp/src/fix_vcxproj.bat` and `mp/src/creategameprojects.bat`, preferably in cmd with admin privileges. (right click Windows icon -> Windows Powershell (Admin) -> `cmd` -> cd your way to project folder)
- If you're having problems, just [ask Mehis in Discord](https://discord.gg/tZTUex3).

### FAQ
**Is Linux compiling really this convoluted?**
Yes.

**Opening the solution says something about upgrading.**
 Run `mp/src/fix_vcxproj.bat`. If it still complains about upgrading, see below.

**Project refuses to compile, says something about wrong/missing toolset.**
Open project settings and make sure `Platform Toolset` is `v142` and `Windows SDK Version` is `10.0` (other 10.X versions should also work).
If it says one of them is missing, you need to download them from Visual Studio Installer (Tools -> Get Tools and Features)

## Contributing

**Please base all pull requests on the 'dev' branch.** If you're planning on something bigger or you want to work on one of the Issues, do contact us so your efforts don't go to waste.
