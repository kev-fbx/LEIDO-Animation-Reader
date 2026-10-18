# LEIDO PZ Animation Exporter

Play Project Zomboid normally while a desktop app mirrors your character live. Press **F9** to
record and **F9** again to stop. The take is saved for Blender (GLB export is planned; see
Status below).

Design, findings and roadmap: [Feasibility study](https://claude.ai/code/artifact/8daa4f2f-9bd4-4220-9b7e-7c3f0f80c372).
Wire format: [docs/protocol.md](docs/protocol.md).

## Layout

| Path | What |
| --- | --- |
| `agent/` | Java agent loaded into the game's JVM. Hooks `IsoGameCharacter.postupdate()`, copies the player's bone matrices every tick, streams them over `127.0.0.1:47400`, writes raw `.pzrec` takes |
| `app/` | Desktop app: C++20, Qt 6.10 Quick 3D, Assimp. Live viewport, Record/Stop, F9 hotkey |
| `launcher/` | Starts PZ with the agent via `JAVA_TOOL_OPTIONS`; no game files are modified |
| `tools/pzstream.py` | Command-line client: live stats, scripted takes, `.pzrec` summaries (no app needed) |
| `docs/` | Protocol specification |
| `research/` | Vineflower output of the game's `zombie.*` classes (git-ignored, reading only) |

## Requirements

* Project Zomboid Build 42.21.0 (revision `4a0e9546ec`), single-player
* JDK 25 or newer for building the agent (`C:\Program Files\Java\jdk-27` is used by default)
* Visual Studio 2022 Build Tools (C++), CMake 3.24+
* Python 3.10+ for `tools/pzstream.py`
* Qt 6.10.3 (msvc2022_64 + Qt Quick 3D) in `tools/qt` and vcpkg in `tools/vcpkg` (see Setup)

## Setup (once)

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
powershell -ExecutionPolicy Bypass -File agent\smoke-test.ps1    # offline: checks the hook is woven

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
and put `jre64\bin` on `PATH`. The PATH part matters. The JVM's agent support (`instrument.dll`) imports
`jli.dll`, which exists only in `jre64\bin`. Without it, the game's JVM aborts with *"Could not find agent
library instrument ... Can't find dependent libraries"* and the game silently fails to start.

The agent log is `runtime\agent.log`.

To check an exported GLB without opening Blender's UI (prints what was imported and renders PNGs):

```powershell
& "C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe" -b --factory-startup `
    --python tools\blender_check.py -- "<take.glb>" research\exports\renders
``` It reports the game build, the hook, and timing every 600 ticks.

## Status

| Piece | State |
| --- | --- |
| Agent: hook, pose sampling, streaming, `.pzrec` takes, multiplayer guard | Built; hook verified offline; needs an in-game run (Experiment 0) |
| `pzstream.py` | Working against the agent (HELLO, STATUS, record start/stop, take reader) |
| App: connection, live bone view, Record/Stop, F9, output folder, Assimp body probe | Builds clean (Release); connects to the agent; parses `MaleBody.x` (2,748 vertices, 27 weighted bones). Log: `%LOCALAPPDATA%\LEIDO PZ Animation Exporter\LEIDO PZ Animation Exporter\app.log` |
| GLB export on Stop (textured PZ body, skin, world-space root motion) | Working; verified headless in Blender 4.5 and 5.2. Also `app\build\Release\pzexport.exe <take.pzrec>` |
| Takes panel: list of `.pzrec` takes, batch export to GLB, export options (fps, root motion, scale, auto-export) | Working |
| Windows 7 (Aero) look: `qml/Theme.qml` + `qml/Aero*.qml` controls | Done |
| Real skinned body mesh in the viewport | Next |
| Held items, obstacles | Planned (protocol types reserved) |
