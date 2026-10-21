param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$Destination
)

$ErrorActionPreference = 'Stop'
$source = (Resolve-Path $BuildDirectory).Path
if (-not (Test-Path (Join-Path $source 'Telegram.exe'))) {
    throw 'The Release build did not produce Telegram.exe.'
}
if (Test-Path $Destination) {
    throw 'The artifact destination already exists; preserve it or choose a new one.'
}
$package = Join-Path $Destination 'LiquidGlass'
New-Item -ItemType Directory -Path $package -Force | Out-Null
Get-ChildItem $source -File | Where-Object {
    $_.Extension -in '.exe', '.dll', '.rcc', '.bin', '.dat'
} | Where-Object {
    $_.Name -ne 'Updater.exe'
} | Copy-Item -Destination $package
if (Test-Path (Join-Path $source 'modules')) {
    Copy-Item (Join-Path $source 'modules') $package -Recurse
}
$checkout = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
Copy-Item (Join-Path $checkout 'LICENSE') $package
Copy-Item (Join-Path $checkout 'LEGAL') $package
Copy-Item (Join-Path $checkout 'docs/fork/BUILDING.md') (Join-Path $package 'README.md')
Copy-Item (Join-Path $checkout 'docs/fork/IMPLEMENTATION.md') $package
@'
@echo off
if not exist "%~dp0profile" mkdir "%~dp0profile"
start "Liquid Glass fork" "%~dp0Telegram.exe" -workdir "%~dp0profile" %*
'@ | Set-Content -Encoding ascii (Join-Path $package 'Start Liquid Glass.cmd')
$imports = & dumpbin /dependents (Join-Path $package 'Telegram.exe')
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect the executable imports.' }
$imports | Set-Content (Join-Path $package 'imports.txt')
$commit = & git -C $checkout rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-f]{40}$') {
    throw 'Could not identify the packaged source commit.'
}
[ordered]@{
    source_commit = $commit
    native_build_run = $env:GITHUB_RUN_ID
    executable_sha256 = (Get-FileHash (Join-Path $package 'Telegram.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
} | ConvertTo-Json | Set-Content -Encoding utf8NoBOM (Join-Path $package 'BUILD-INFO.json')
Compress-Archive -Path $package -DestinationPath (Join-Path $Destination 'LiquidGlass-Windows-x64.zip')
