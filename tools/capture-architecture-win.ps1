param([string]$OutputDirectory='docs/evidence/architecture/native',[switch]$Materials)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$zig=Join-Path $projectRoot '.toolchain/zig-x86_64-windows-0.14.1/zig.exe'
$raylib=Join-Path $projectRoot '.toolchain/raylib-5.5_win64_mingw-w64'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $projectRoot '.toolchain/zig-cache'
$sources=Get-ChildItem src/*.c | Where-Object Name -ne 'main.c' | ForEach-Object FullName
$folder=[IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
$modes=if($Materials){@('with-materials','without-materials')}else{@('with-occlusion','without-occlusion')}
foreach($mode in $modes) {
    $flags=@()
    if($mode -eq 'without-occlusion'){$flags+= '-DAWELL_DEPTH_NO_AO'}
    if($mode -eq 'without-materials'){$flags+= '-DAWELL_DEPTH_NO_SURFACE_TEXTURES'}
    $exe=Join-Path $projectRoot "build/architecture-$mode.exe"
    & $zig cc -std=c99 -O2 @flags tools/tests/architecture_capture.c $sources -I "$raylib/include" "$raylib/lib/libraylib.a" -lopengl32 -lgdi32 -lwinmm -o $exe
    if($LASTEXITCODE -ne 0){throw 'Architecture native review build failed'}
    $caseFolder=Join-Path $folder $mode
    New-Item -ItemType Directory -Force -Path $caseFolder | Out-Null
    foreach($room in @(0,1)) {
        $log=Join-Path $caseFolder "room-$room.txt"
        $stderr=Join-Path $caseFolder "room-$room.stderr.txt"
        $process=Start-Process -FilePath $exe -ArgumentList @($room,('"'+$caseFolder+'"')) -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $log -RedirectStandardError $stderr
        if($process.ExitCode -ne 0){throw "Architecture capture failed $mode room$room"}
        Get-Content -LiteralPath $log
    }
}
$hashes=@{}
foreach($path in @('src/depth.c','src/generated/terrain_assets.h','src/generated/support_assets.h','src/generated/foliage_assets.h','src/generated/material_textures.h')){$hashes[$path]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower()}
$control=if($Materials){'surface textures'}else{'AO'}
[ordered]@{capturedUtc=(Get-Date).ToUniversalTime().ToString('o');sourceHashes=$hashes;scope="Same source/geometry, $control build control only; actual native images and local frame-loop wall timing. Not GPU-only timing or hardware-wide performance acceptance."} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $folder 'manifest.json') -Encoding utf8
