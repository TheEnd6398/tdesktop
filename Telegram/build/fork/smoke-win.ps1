param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory
)

$ErrorActionPreference = 'Stop'
$executable = (Resolve-Path (Join-Path $BuildDirectory 'Telegram.exe')).Path
$profile = Join-Path ([IO.Path]::GetTempPath()) ('LiquidGlass-smoke-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $profile | Out-Null
$client = $null
try {
    $client = Start-Process -FilePath $executable -ArgumentList @(
        '-workdir', ('"' + $profile + '"'), '-noupdate'
    ) -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(60)
    do {
        Start-Sleep -Milliseconds 500
        $client.Refresh()
        if ($client.HasExited) {
            throw "The isolated client exited during startup (code $($client.ExitCode))."
        }
    } while ($client.MainWindowHandle -eq 0 -and [DateTime]::UtcNow -lt $deadline)
    if ($client.MainWindowHandle -eq 0) {
        throw 'The isolated client did not create a native window within 60 seconds.'
    }
    Start-Sleep -Seconds 10
    $client.Refresh()
    if ($client.HasExited -or -not $client.Responding) {
        throw 'The isolated client did not remain responsive after startup.'
    }
    if (-not (Test-Path (Join-Path $profile 'tdata'))) {
        throw 'The isolated client did not initialize the requested profile.'
    }
    Write-Output 'The unauthenticated client created a native window and stayed responsive in an isolated profile.'
} finally {
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force
        $client.WaitForExit()
    }
    Remove-Item -LiteralPath $profile -Recurse -Force
}
