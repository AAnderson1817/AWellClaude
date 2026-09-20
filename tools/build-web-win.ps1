param(
    [switch]$RebuildRaylib,
    [switch]$NoWrap,
    [string]$Output = 'build/game.js',
    [string]$HtmlOutput = 'build/play.html'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$sdk = Join-Path $projectRoot '.toolchain/emsdk-main'
$raylibSource = Join-Path $projectRoot '.toolchain/raylib-5.5/src'
$python = Join-Path $sdk 'python/3.13.3_64bit/python.exe'
$emcc = Join-Path $sdk 'upstream/emscripten/emcc.py'
$emar = Join-Path $sdk 'upstream/emscripten/emar.py'
foreach ($required in @($python, $emcc, $emar, "$raylibSource/raylib.h")) {
    if (!(Test-Path -LiteralPath $required)) { throw "Web dependency missing: $required. This build uses an existing emsdk 6.0.8 and raylib 5.5 source installation." }
}
if (!(Test-Path -LiteralPath "$sdk/.emscripten")) {
    & $python "$sdk/emsdk.py" activate 6.0.8
    if ($LASTEXITCODE -ne 0) { throw 'SDK-local Emscripten activation failed' }
}
$previousConfig = $env:EM_CONFIG
$previousCache = $env:EM_CACHE
$previousSdk = $env:EMSDK
try {
    $env:EMSDK = $sdk
    $env:EM_CONFIG = Join-Path $sdk '.emscripten'
    $env:EM_CACHE = Join-Path $sdk 'upstream/emscripten/cache'
    $objectDirectory = Join-Path $projectRoot '.toolchain/raylib-web-objects'
    New-Item -ItemType Directory -Force -Path $objectDirectory, 'lib', 'build' | Out-Null
    $library = Join-Path $projectRoot 'lib/libraylib_web.a'
    if ($RebuildRaylib -or !(Test-Path -LiteralPath $library)) {
        $objects = @()
        # Match raylib 5.5 src/Makefile PLATFORM_WEB, including models and audio.
        foreach ($module in @('rcore','rshapes','rtextures','rtext','utils','rmodels','raudio')) {
            $object = Join-Path $objectDirectory "$module.o"
            & $python $emcc -c "$raylibSource/$module.c" -o $object -Os -std=gnu99 -D_GNU_SOURCE -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -fno-strict-aliasing -Wno-missing-braces -Werror=pointer-arith -I $raylibSource -I "$raylibSource/external/glfw/include"
            if ($LASTEXITCODE -ne 0) { throw "Web raylib compilation failed: $module" }
            $objects += $object
        }
        & $python $emar rcs $library $objects
        if ($LASTEXITCODE -ne 0) { throw 'Web raylib archive creation failed' }
    }
    $sources = Get-ChildItem src/*.c | ForEach-Object { $_.FullName }
    # Keep the original tools/build.sh web ABI, heap exports and single-file output.
    & $python $emcc $sources -o $Output -I $raylibSource $library -Os -DPLATFORM_WEB -s USE_GLFW=3 -s ASYNCIFY -s SINGLE_FILE=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=RL -s ENVIRONMENT=web -s EXPORTED_RUNTIME_METHODS=HEAPF32,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32
    if ($LASTEXITCODE -ne 0) { throw 'Web game compilation failed' }
    if (!$NoWrap) {
        & $python tools/web/wrap.py $Output $HtmlOutput 'The Vault and the City Under It'
        if ($LASTEXITCODE -ne 0) { throw 'Web HTML wrapping failed' }
    }
    Get-Item -LiteralPath $Output | Select-Object FullName, Length
    if (!$NoWrap) { Get-Item -LiteralPath $HtmlOutput | Select-Object FullName, Length }
} finally {
    $env:EM_CONFIG = $previousConfig
    $env:EM_CACHE = $previousCache
    $env:EMSDK = $previousSdk
}
