# Starts Project Zomboid with the LEIDO PZ Animation Exporter agent attached, without editing any game files.
#
#   powershell -ExecutionPolicy Bypass -File launcher\Launch-PZ-With-Agent.ps1
#
# The agent is passed through JAVA_TOOL_OPTIONS, which the JVM inside ProjectZomboid64.exe reads
# at start-up. Only this launch is affected; starting PZ from Steam runs the game without the agent.
# Steam must already be running (the game folder's steam_appid.txt lets the exe start directly).

param(
    [int]$Port = 47400,
    [string]$PzHome = 'C:\Program Files (x86)\Steam\steamapps\common\ProjectZomboid',
    [string]$AgentHome = (Join-Path (Split-Path $PSScriptRoot -Parent) 'runtime'),
    [switch]$ShowConsole
)

$ErrorActionPreference = 'Stop'
$agentJar = Join-Path (Split-Path $PSScriptRoot -Parent) 'agent\build\pz-animator-agent.jar'

if (-not (Test-Path $agentJar)) { throw "Agent not built. Run agent\build.ps1 first." }
if (-not (Test-Path "$PzHome\ProjectZomboid64.exe")) { throw "Project Zomboid not found in $PzHome" }
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw "Start Steam first." }
if (Get-Process ProjectZomboid64 -ErrorAction SilentlyContinue) { throw "Project Zomboid is already running." }
if ("$agentJar$AgentHome" -match '\s') {
    throw "Paths with spaces cannot be passed through JAVA_TOOL_OPTIONS: $agentJar / $AgentHome"
}

New-Item -ItemType Directory -Force $AgentHome | Out-Null
$env:JAVA_TOOL_OPTIONS = "-javaagent:$agentJar=port=$Port,home=$AgentHome"
# instrument.dll imports jli.dll, which lives only in jre64\bin; without it on PATH the JVM aborts
# with "Could not find agent library instrument ... Can't find dependent libraries".
$env:PATH = "$PzHome\jre64\bin;$env:PATH"

$exe = if ($ShowConsole) { 'ProjectZomboid64ShowConsole.bat' } else { 'ProjectZomboid64.exe' }
Write-Host "Starting $exe with agent on 127.0.0.1:$Port (log: $AgentHome\agent.log)"
Start-Process -FilePath (Join-Path $PzHome $exe) -WorkingDirectory $PzHome
