param([string]$OutputDirectory = 'docs/evidence/city-responses/native')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$zig = Join-Path $projectRoot '.toolchain/zig-x86_64-windows-0.14.1/zig.exe'
$raylib = Join-Path $projectRoot '.toolchain/raylib-5.5_win64_mingw-w64'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '.toolchain/zig-cache'
$source = Get-ChildItem src/*.c | Where-Object Name -ne 'main.c' | ForEach-Object FullName
& $zig cc -std=c99 -O2 tools/tests/city_capture.c $source -I "$raylib/include" "$raylib/lib/libraylib.a" -lopengl32 -lgdi32 -lwinmm -o build/city-capture.exe
if ($LASTEXITCODE -ne 0) { throw 'City native review build failed' }
$folder = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
New-Item -ItemType Directory -Force -Path $folder | Out-Null
foreach ($case in @('face','mural-dark','mural-lit','window-lit','window-dark','shoal-capital','shoal-lamp')) {
    $log = Join-Path $folder "$case.txt"
    $errorLog = Join-Path $folder "$case.stderr.txt"
    $process = Start-Process -FilePath (Join-Path $projectRoot 'build/city-capture.exe') -ArgumentList @($case,('"'+$folder+'"')) -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $log -RedirectStandardError $errorLog
    Get-Content -LiteralPath $log
    if ($process.ExitCode -ne 0) { Get-Content -LiteralPath $errorLog; throw "City capture failed: $case" }
}
