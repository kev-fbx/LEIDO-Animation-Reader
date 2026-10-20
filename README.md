# LEIDO PZ Animation Exporter

This software runs a Java agent that reads player animation data while the user plays the video game Project Zomboid. The purpose of this tool is to export player animation into a reusable format for cinematic use cases such as rendering in creative software.

* Project Zomboid Build 42.21.0, single-player
* JDK 25 or newer for building the agent
* Visual Studio 2022 Build Tools, CMake 3.24+
* Python 3.10+
* Qt 6.10.3 (msvc2022_64 + Qt Quick 3D) in `tools/qt` and vcpkg in `tools/vcpkg`

## Setup

```powershell
# Qt via aqtinstall (official mirrors, no account needed)
python -m venv tools\.venv
tools\.venv\Scripts\pip install aqtinstall
tools\.venv\Scripts\aqt install-qt windows desktop 6.10.3 win64_msvc2022_64 -m qtquick3d qtshadertools qtquicktimeline -O tools\qt

# vcpkg (Assimp is built from app\vcpkg.json on first configure)
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

## Build and run

```powershell
# 1. Agent (downloads and checksums Byte Buddy on first build)
powershell -ExecutionPolicy Bypass -File agent\build.ps1
powershell -ExecutionPolicy Bypass -File agent\smoke-test.ps1

# 2. Start PZ with the agent (Steam must be running), then load a single-player save
powershell -ExecutionPolicy Bypass -File launcher\Launch-PZ-With-Agent.ps1

# 3a. Check the stream from a terminal
python tools\pzstream.py live
python tools\pzstream.py live --record 10 --name walk_test
python tools\pzstream.py read runtime\takes\walk_test.pzrec --csv walk_test.csv

# 3b. Or run the desktop app
cmake --preset default -S app
cmake --build app\build --config Release
app\build\Release\PzAnimator.exe
```

Launch the game **from the app** (the *Launch Project Zomboid* button, or `PzAnimator.exe --launch`)
or from the PowerShell launcher, never from Steam's Play button: only these set `JAVA_TOOL_OPTIONS`
and put `jre64\bin` on `PATH`. The JVM's agent support (`instrument.dll`) imports
`jli.dll`, which exists only in `jre64\bin`. Without it, the game's JVM aborts with *"Could not find agent
library instrument ... Can't find dependent libraries"* and the game will fail to start.

The agent logs data at `runtime\agent.log`.
