param(
    [Parameter(Mandatory = $true)][string]$LibrariesDirectory
)

$ErrorActionPreference = 'Stop'
$libraries = (Resolve-Path $LibrariesDirectory).Path
foreach ($stage in Get-ChildItem $libraries -Directory -Filter 'qt_*') {
    if ($stage.Name -notmatch '^qt_(6\.\d+\.\d+)$') {
        continue
    }
    $version = $Matches[1]
    $key = Join-Path $libraries "cache_keys/$($stage.Name)"
    if (-not (Test-Path $key -PathType Leaf)) {
        continue
    }
    $prefix = Join-Path $libraries "Qt-$version"
    foreach ($relative in @('bin/qmake.exe', 'lib/Qt6Core.lib', 'lib/cmake/Qt6/Qt6Config.cmake')) {
        if (-not (Test-Path (Join-Path $prefix $relative) -PathType Leaf)) {
            throw "Completed Qt stage has no installed $relative; refusing to trim its build tree."
        }
    }
    Get-ChildItem $stage.FullName -Force | Remove-Item -Recurse -Force
    'Installed Qt is preserved alongside this stage directory. Remove its cache key to rebuild.' |
        Set-Content -Encoding ascii (Join-Path $stage.FullName 'FORK_CACHE.txt')
    Write-Host "Retaining installed Qt $version and its stage key; removed duplicate source/build files."
}
