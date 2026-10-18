# Offline hook check: weaves the agent into IsoGameCharacter without starting the game.
#   powershell -ExecutionPolicy Bypass -File agent\smoke-test.ps1
param(
    [string]$JdkHome = $(if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Program Files\Java\jdk-27' }),
    [string]$PzHome = 'C:\Program Files (x86)\Steam\steamapps\common\ProjectZomboid'
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$out = Join-Path $root 'build\smoke'
$dump = Join-Path $out 'dump'
if (Test-Path $dump) { Remove-Item -Recurse -Force $dump }
New-Item -ItemType Directory -Force $dump | Out-Null

& "$JdkHome\bin\javac.exe" --release 25 -d $out (Join-Path $root 'src\test\java\HookSmokeTest.java')
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

# Run on the game's own runtime so class-file support matches what the game uses.
& "$PzHome\jre64\bin\java.exe" "-javaagent:$root\build\pz-animator-agent.jar=home=$out" `
    "-Dnet.bytebuddy.dump=$dump" -cp "$out;$PzHome\projectzomboid.jar" HookSmokeTest
if ($LASTEXITCODE -ne 0) { throw "smoke run failed" }

$woven = Get-ChildItem $dump -Recurse -Filter 'zombie.characters.IsoGameCharacter.*.class' |
    Where-Object { [IO.File]::ReadAllText($_.FullName, [Text.Encoding]::GetEncoding(28591)).Contains('pzanimator/agent/Bridge') }
if ($woven) { Write-Host "PASS: postupdate() calls pzanimator.agent.Bridge ($($woven[0].Name))" }
else { throw "FAIL: Bridge call not found in transformed IsoGameCharacter" }
