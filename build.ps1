$ErrorActionPreference = 'Stop'
$compiler = Get-Command clang++ -ErrorAction SilentlyContinue
if (-not $compiler) {
    $compiler = Get-Command g++ -ErrorAction Stop
}

$source_dir = Join-Path $PSScriptRoot 'src'
$sources = Get-ChildItem -LiteralPath $source_dir -Recurse -File -Filter '*.cpp' |
    ForEach-Object FullName
$exe = Join-Path $PSScriptRoot 'CampusPlatform.exe'

& $compiler.Source -std=c++11 -O2 -Wall -Wextra -static @sources -o $exe -lbcrypt
if ($LASTEXITCODE -ne 0) {
    throw "C++ build failed with exit code $LASTEXITCODE."
}
Write-Output $exe
