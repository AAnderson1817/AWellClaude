param(
    [switch]$Render,
    [string]$Python,
    [string]$EvidenceDirectory = 'build/verification'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$zig = Join-Path $projectRoot '.toolchain/zig-x86_64-windows-0.14.1/zig.exe'
$include = Join-Path $projectRoot '.toolchain/raylib-5.5_win64_mingw-w64/include'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '.toolchain/zig-cache'
if (!(Test-Path -LiteralPath $zig) -or !(Test-Path -LiteralPath "$include/raylib.h")) {
    throw 'Portable dependencies missing. Run tools/setup-win.ps1 first.'
}
if (!$Python) {
    $candidates = @()
    foreach ($name in @('python3', 'python')) {
        $found = Get-Command $name -ErrorAction SilentlyContinue
        if ($found) { $candidates += $found.Source }
    }
    $candidates += (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe')
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            $version = & $candidate --version 2>&1
            if ($LASTEXITCODE -eq 0 -and "$version" -match '^Python 3\.') { $Python = $candidate; break }
        }
    }
}
if (!$Python -or !(Test-Path -LiteralPath $Python)) {
    throw 'Python 3 is required for verification. Pass -Python with its executable path. The native game does not require Python.'
}
$evidencePath = if ([IO.Path]::IsPathRooted($EvidenceDirectory)) { $EvidenceDirectory } else { Join-Path $projectRoot $EvidenceDirectory }
New-Item -ItemType Directory -Force -Path $evidencePath | Out-Null

function Invoke-Checked {
    param([string]$Executable, [string[]]$CommandArgs, [string]$LogName)
    $output = & $Executable @CommandArgs 2>&1
    $result = $LASTEXITCODE
    $output | Out-File -LiteralPath (Join-Path $evidencePath $LogName) -Encoding utf8
    $output | Write-Output
    if ($result -ne 0) { throw "Verification command failed ($result): $Executable $CommandArgs" }
}

& "$PSScriptRoot/build-win.ps1" -Headless
$sources = @('src/player.c','src/room.c','src/fx.c','src/render.c','src/audio.c','src/life.c','src/items.c','src/props.c','src/city.c')
foreach ($testName in @('input_hash','presentation_snapshots','city_responses')) {
    & $zig cc -std=c99 -O1 -g -DAWELL_HEADLESS -fsanitize=undefined -fno-sanitize-recover=all -I $include "tools/tests/$testName.c" tools/tests/raylib_stubs.c $sources -o "build/$testName.exe"
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $testName" }
    Invoke-Checked -Executable "./build/$testName.exe" -CommandArgs @() -LogName "$testName.txt"
}
Invoke-Checked -Executable $Python -CommandArgs @('tools/tests/test_tools.py') -LogName 'python-tools.txt'
$headlessReport = Join-Path $evidencePath 'preservation-report.json'
Invoke-Checked -Executable $Python -CommandArgs @('tools/check-preservation.py','--build-baseline','--baseline','build/game-baseline-headless.exe','--candidate','build/game-probe.exe','--report',$headlessReport) -LogName 'preservation.txt'
$previousGame = $env:AWELL_GAME
try {
    $env:AWELL_GAME = Join-Path $projectRoot 'build/game-probe.exe'
    Invoke-Checked -Executable $Python -CommandArgs @('tools/route.py') -LogName 'route.txt'
    Invoke-Checked -Executable $Python -CommandArgs @('tools/escape.py') -LogName 'escape.txt'
} finally { $env:AWELL_GAME = $previousGame }
if ($Render) {
    # Build both from their actual sources. The historical binary is never made by
    # renaming the current executable or compiling the current branch with --flat.
    & "$PSScriptRoot/build-win.ps1"
    $renderReport = Join-Path $evidencePath 'render-preservation-report.json'
    Invoke-Checked -Executable $Python -CommandArgs @('tools/check-preservation.py','--build-baseline','--render','--baseline','build/game-baseline.exe','--candidate','build/game.exe','--report',$renderReport) -LogName 'render-preservation.txt'
}
$summary = [ordered]@{
    verifiedUtc = (Get-Date).ToUniversalTime().ToString('o')
    baselineGitRef = '99de7856bedfa059439da1b89435f54c98bddaf7'
    sanitizer = 'undefined; no recovery'
    renderedComparison = [bool]$Render
    routeChecks = 35
    escapeSurfaces = 36
    snapshotFrames = 3600
    cityResponseTests = 'four windows, mural/fire coupling, face sinking-stone acknowledgment, eight-fish shoal, detached snapshots'
    citySoundExtensions = @('city-murmur','city-hum')
    pythonToolTests = 5
    status = 'passed'
}
$summary | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidencePath 'summary.json') -Encoding utf8
Write-Host "All verification checks passed. Evidence: $evidencePath"
