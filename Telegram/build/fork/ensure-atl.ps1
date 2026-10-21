$ErrorActionPreference = 'Stop'
if (-not $env:VCToolsInstallDir -or -not $env:VSINSTALLDIR -or -not $env:GITHUB_ENV) {
    throw 'Initialize the MSVC developer environment before checking ATL.'
}
if ($env:VCToolsVersion -notlike '14.44.*') {
    throw 'The fork ATL bootstrap requires the pinned MSVC 14.44 toolset.'
}
$include = Join-Path $env:VCToolsInstallDir 'atlmfc/include'
$library = Join-Path $env:VCToolsInstallDir 'atlmfc/lib/x64'
$headers = @('atlbase.h', 'atlcomcli.h')
$missing = @($headers | Where-Object { -not (Test-Path (Join-Path $include $_)) })
if ($missing.Count -gt 0) {
    $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/setup.exe'
    if (-not (Test-Path $installer)) { throw 'The Visual Studio Installer was not found.' }
    $component = 'Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL'
    $installation = $env:VSINSTALLDIR.TrimEnd('\')
    $arguments = @('modify', '--installPath', ('"{0}"' -f $installation),
        '--add', $component, '--quiet', '--norestart', '--nocache')
    $process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -notin @(0, 3010)) {
        throw "Installing the MSVC 14.44 ATL component failed: $($process.ExitCode)."
    }
}
foreach ($header in $headers) {
    if (-not (Test-Path (Join-Path $include $header))) {
        throw "The matching ATL header is still missing: $header."
    }
}
if (-not (Test-Path $library)) { throw 'The x64 ATL library directory is missing.' }
"INCLUDE=$include;$env:INCLUDE" >> $env:GITHUB_ENV
"LIB=$library;$env:LIB" >> $env:GITHUB_ENV
Write-Output 'Verified ATL headers and libraries for MSVC 14.44.'
