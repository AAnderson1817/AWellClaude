param([switch]$Headless, [string]$Output = 'build/game.exe')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$zig = Join-Path $projectRoot '.toolchain/zig-x86_64-windows-0.14.1/zig.exe'
$raylibRoot = Join-Path $projectRoot '.toolchain/raylib-5.5_win64_mingw-w64'
if (!(Test-Path -LiteralPath $zig) -or !(Test-Path -LiteralPath "$raylibRoot/include/raylib.h")) {
    throw 'Portable dependencies missing. Run tools/setup-win.ps1 first.'
}
New-Item -ItemType Directory -Force -Path build | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '.toolchain/zig-cache'
$sources = Get-ChildItem src/*.c | ForEach-Object { $_.FullName }
$outputPath = $Output
if ($Headless) {
    if (!$PSBoundParameters.ContainsKey('Output')) { $outputPath = 'build/game-probe.exe' }
    $sources = @($sources | Where-Object { (Split-Path $_ -Leaf) -ne 'depth.c' })
    & $zig cc -std=c99 -O2 -DAWELL_HEADLESS $sources tools/tests/raylib_stubs.c -I "$raylibRoot/include" -o $outputPath
} else {
    & $zig cc -std=c99 -O2 $sources -I "$raylibRoot/include" "$raylibRoot/lib/libraylib.a" -lopengl32 -lgdi32 -lwinmm -o $outputPath
}
if ($LASTEXITCODE -ne 0) { throw "C build failed: $LASTEXITCODE" }
Write-Host "Built $outputPath from the existing C simulation."
