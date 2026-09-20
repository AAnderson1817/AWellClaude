$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
New-Item -ItemType Directory -Force -Path .toolchain | Out-Null
$zigUrl = 'https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip'
$zigHash = '554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c'
if (!(Test-Path '.toolchain/zig-x86_64-windows-0.14.1/zig.exe')) {
    Invoke-WebRequest $zigUrl -OutFile '.toolchain/zig.zip'
    if ((Get-FileHash '.toolchain/zig.zip' -Algorithm SHA256).Hash.ToLower() -ne $zigHash) { throw 'Zig archive checksum mismatch' }
    Expand-Archive '.toolchain/zig.zip' -DestinationPath '.toolchain' -Force
}
if (!(Test-Path '.toolchain/raylib-5.5_win64_mingw-w64/include/raylib.h')) {
    Invoke-WebRequest 'https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip' -OutFile '.toolchain/raylib.zip'
    Expand-Archive '.toolchain/raylib.zip' -DestinationPath '.toolchain' -Force
}
Write-Host 'Portable C dependencies ready; no system installation or PATH changes.'
