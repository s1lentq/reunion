# Reunion [![C/C++ CI](https://github.com/s1lentq/reunion/actions/workflows/build.yml/badge.svg)](https://github.com/s1lentq/reunion/actions/workflows/build.yml) [![GitHub release (by tag)](https://img.shields.io/github/downloads/s1lentq/reunion/latest/total)](https://github.com/s1lentq/reunion/releases/latest) ![GitHub all releases](https://img.shields.io/github/downloads/s1lentq/reunion/total) [![Percentage of issues still open](http://isitmaintained.com/badge/open/s1lentq/reunion.svg)](http://isitmaintained.com/project/s1lentq/reunion "Percentage of issues still open") [![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

Metamod plugin that allows protocol 47 and 48 no-steam clients to connect to ReHLDS servers.<br/>
Reunion is a continuation of the DProto project adapted for ReHLDS.<br/>

You can try playing on one of many servers that are using Reunion: [Game Tracker](http://www.gametracker.com/search/?search_by=server_variable&search_by2=reu_version)

## Downloads
* [Release builds](https://github.com/s1lentq/reunion/releases)
* [Dev builds](https://github.com/s1lentq/reunion/actions/workflows/build.yml)

## Environment requirement
* ReHLDS API >= 3.10

## Installation

<details>
<summary>Click to expand</summary>

1. Go to `<gamedir>/addons/` and make new directory named reunion<br/>
`<gamedir>` - its a game directory; cstrike for Counter-Strike, valve for Half-Life, etc

2. Copy `reunion_mm.dll` or `reunion_mm_i386.so` to <gamedir>/addons/reunion/

3. Go to metamod installation directory (usually its <gamedir>/addons/metamod/) and edit plugins.ini:<br/>
add this line for windows<br/>
`win32 addons\reunion\reunion_mm.dll`<br/>
or this for linux<br/>
`linux addons/reunion/reunion_mm_i386.so`<br/>
at the beginning of the file<br/>

4. Copy `reunion.cfg` to server root or gamedir.

5. Start the server. When server loads, type `meta list` in console. You'll see something like this:
```
        Currently loaded plugins:
              description      stat pend  file              vers      src   load  unlod
         [ 1] Reunion          RUN   -    reunion_mm_i386.  v0.1.58   ini   Start Never
         [ 2] AMX Mod X        RUN   -    amxmodx_mm_i386.  v1.8.1.3  ini   Start ANY
        2 plugins, 2 running
```
6. Ready to use

If reunion doesn't work, meta list says this:
Currently loaded plugins:
      description      stat pend  file              vers      src   load  unlod
 [ 1] Reunion          fail load  reunion_mm_i386.  v0.1.65   ini   Start Never

Start server with `-console +log on +mp_logecho 1` parameters and look through console output. You'll find the reason there.
</details>

## FAQ
<details>
<summary>Click to expand</summary>

* `Q` I configured SteamIdHashSalt as well as in the dproto, but players get a different steamids. Why?<br/>
  `A` Reunion uses an another hashing algorythm with improved security. Knowing of someones's steamid before enabling SteamIdHashSalt doesn't help to get same id after hashing.

* `Q` Is it possible to do something against steamid changers?<br/>
  `A` No, idchangers generates a correct authorization tickets and it's impossible on serverside identify that steamid was changed. You can only set a SteamIdHashSalt option to prevent a substitution to specific steamid of another player.

* `Q` Why some server monitorings can't receive the players list from server?<br/>
  `A` They use an incorrect query format and must be rewritten using latest <a href="https://github.com/xPaw/PHP-Source-Query">PHP-Source-Query</a> script or equivalent.

* `Q` In dproto was option Game_Name, but in reunion it has not. How to change the game name?<br/>
  `A` Use plugin.

* `Q` Why has SmartSteamEmu3 support been removed?<br/>
  `A` To open-source the Reunion project, we had to remove some sensitive components.<br/>
   This includes the `SmartSteamEmu3` emulator's authorization code.
   Removing it won't significantly affect server online activity since this emulator is rare among non-steam clients.

</details>

## Build instructions
### Checking requirements
There are several software requirements for building reunion:

#### Windows
<pre>
Visual Studio 2015 (C++14 standard) and later
</pre>

#### Linux
<pre>
cmake >= 3.10
GCC >= 4.9.2 (Optional)
ICC >= 15.0.1 20141023 (Optional)
LLVM (Clang) >= 6.0 (Optional)
</pre>

### Building

#### Windows
Use `Visual Studio` to build, open `msvc/Reunion.sln` and just select from the solution configurations list `Release` or `Debug`

#### Linux

* Optional options using `build.sh --compiler=[gcc] --jobs=[N] -D[option]=[ON or OFF]` (without square brackets)

<pre>
-c=|--compiler=[icc|gcc|clang]  - Select preferred C/C++ compiler to build
-j=|--jobs=[N]                  - Specifies the number of jobs (commands) to run simultaneously (For faster building)

<sub>Definitions (-D)</sub>
DEBUG                           - Enables debugging mode
USE_STATIC_LIBSTDC              - Enables static linking library libstdc++
</pre>

* ICC          <pre>./build.sh --compiler=intel</pre>
* LLVM (Clang) <pre>./build.sh --compiler=clang</pre>
* GCC          <pre>./build.sh --compiler=gcc</pre>

##### Checking build environment (Debian / Ubuntu)

<details>
<summary>Click to expand</summary>

<ul>
<li>
Installing required packages
<pre>
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib
sudo apt-get install -y build-essential
sudo apt-get install -y libc6-dev libc6-dev-i386
</pre>
</li>

<li>
Select the preferred C/C++ Compiler installation
<pre>
1) sudo apt-get install -y gcc g++
2) sudo apt-get install -y clang
</pre>
</li>
</ul>
</details>

### Credits
* [@Crock](https://github.com/theCrock), [@Lev](https://github.com/LevShisterov), and other people such as [@PRoSToTeM@](https://github.com/WPMGPRoSToTeMa) who participated or helped with the development of [DProto](https://cs.rin.ru/forum/viewtopic.php?f=29&t=55986)
* [@theAsmodai](https://github.com/theAsmodai) for contributions as Reunion project former maintainer
* [@dreamstalker](https://github.com/dreamstalker) for the rehlds project, as well as the contributors involved in this project
* [@NordicWarrior](https://github.com/Nord1cWarr1or) for testing and feedback
* [@kazakh758](https://github.com/kazakh758) for testing a fix of issue related to the client hanging on connect
