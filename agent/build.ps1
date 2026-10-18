# Builds agent\build\pz-animator-agent.jar (agent classes + Byte Buddy, ready for -javaagent).
#
#   powershell -ExecutionPolicy Bypass -File agent\build.ps1
#
# Compiles with a JDK 25+ using --release 25 so the classes load in the game's bundled Java 25.

param(
    [string]$JdkHome = $(if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Program Files\Java\jdk-27' }),
    [string]$PzHome = 'C:\Program Files (x86)\Steam\steamapps\common\ProjectZomboid'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$byteBuddyVersion = '1.18.14'
$byteBuddySha1 = 'e26b6f5163f36a5ef91f84eea1befae54e11eb76'
$byteBuddy = Join-Path $root "lib\byte-buddy-$byteBuddyVersion.jar"

if (-not (Test-Path $byteBuddy)) {
    New-Item -ItemType Directory -Force (Split-Path $byteBuddy) | Out-Null
    $url = "https://repo1.maven.org/maven2/net/bytebuddy/byte-buddy/$byteBuddyVersion/byte-buddy-$byteBuddyVersion.jar"
    Write-Host "Downloading $url"
    Invoke-WebRequest -Uri $url -OutFile $byteBuddy -UseBasicParsing
}
$actualSha1 = (Get-FileHash -Algorithm SHA1 $byteBuddy).Hash.ToLower()
if ($actualSha1 -ne $byteBuddySha1) {
    Remove-Item $byteBuddy
    throw "Byte Buddy checksum mismatch ($actualSha1); deleted the download"
}
$gameJar = Join-Path $PzHome 'projectzomboid.jar'
$build = Join-Path $root 'build'
$classes = Join-Path $build 'classes'
$jarOut = Join-Path $build 'pz-animator-agent.jar'

foreach ($required in @("$JdkHome\bin\javac.exe", "$JdkHome\bin\jar.exe", $byteBuddy, $gameJar)) {
    if (-not (Test-Path $required)) { throw "Missing: $required" }
}

if (Test-Path $build) { Remove-Item -Recurse -Force $build }
New-Item -ItemType Directory -Force $classes | Out-Null

$sources = Get-ChildItem -Recurse -Filter *.java (Join-Path $root 'src\main\java') | ForEach-Object FullName
& "$JdkHome\bin\javac.exe" --release 25 -encoding UTF-8 -Xlint:all,-options,-classfile,-serial `
    -cp "$byteBuddy;$gameJar" -d $classes @sources
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

# Bundle Byte Buddy inside the agent jar (the game does not ship it).
Push-Location $classes
try {
    & "$JdkHome\bin\jar.exe" xf $byteBuddy
    if ($LASTEXITCODE -ne 0) { throw "extracting Byte Buddy failed" }
    Remove-Item -Recurse -Force 'META-INF' -ErrorAction SilentlyContinue
} finally {
    Pop-Location
}

& "$JdkHome\bin\jar.exe" --create --file $jarOut `
    --manifest (Join-Path $root 'src\main\resources\META-INF\MANIFEST.MF') -C $classes .
if ($LASTEXITCODE -ne 0) { throw "jar failed" }

Write-Host "Built $jarOut ($([math]::Round((Get-Item $jarOut).Length / 1MB, 1)) MB)"
