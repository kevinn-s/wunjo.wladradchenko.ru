#requires -Version 7
<#
  Local Wunjo dev runner.
  Builds incrementally, installs into C:\CraftRoot, launches installed exe.
  Usage:
    ./run-wunjo.ps1              # build + install + run
    ./run-wunjo.ps1 -BuildOnly   # build + install, no launch
    ./run-wunjo.ps1 -RunOnly     # launch only, no build/install
#>
[CmdletBinding()]
param(
    [switch]$BuildOnly,
    [switch]$RunOnly,
    [int]$Parallel = 2
)

$ErrorActionPreference = 'Stop'

$repo = 'C:\Users\ACER\Desktop\goat'
$build = Join-Path $repo 'build-windows'
$prefix = 'C:\CraftRoot'
$wunjoExe = Join-Path $prefix 'bin\wunjo.exe'

Get-Process wunjo -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$env:PATH = ($env:PATH -split ';' | Where-Object {
        $_ -notmatch 'mingw' -and $_ -notmatch 'Strawberry' -and $_ -notmatch 'Git\\bin'
    }) -join ';'
$env:PATH = "$prefix\bin;$env:PATH"
$env:MLT_PREFIX = $prefix
$env:MLT_REPOSITORY = "$prefix\lib\mlt"
$env:MLT_DATA = "$prefix\share\mlt"
$env:MLT_PROFILES_PATH = "$prefix\share\mlt\profiles"
$env:FREI0R_PATH = "$prefix\lib\frei0r-1"

function Clear-Host {}

& "$prefix\craft\craftenv.ps1"

if (-not $RunOnly) {
    cmake --build "$build" --parallel $Parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }

    cmake --install "$build" --prefix "$prefix"
    if ($LASTEXITCODE -ne 0) { throw "Install failed: $LASTEXITCODE" }
}

if (-not (Test-Path -LiteralPath $wunjoExe)) {
    throw "Missing installed exe: $wunjoExe"
}

if ($BuildOnly) {
    Write-Host "Installed OK: $wunjoExe"
    return
}

Start-Process -FilePath $wunjoExe
Write-Host "Launched: $wunjoExe"
