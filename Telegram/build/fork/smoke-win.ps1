param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [string]$ScreenshotDirectory = ''
)

$ErrorActionPreference = 'Stop'
$executable = (Resolve-Path (Join-Path $BuildDirectory 'Telegram.exe')).Path
$profile = Join-Path ([IO.Path]::GetTempPath()) ('AuroraGlass-smoke-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $profile | Out-Null
$client = $null
if ($ScreenshotDirectory) {
    New-Item -ItemType Directory -Path $ScreenshotDirectory -Force | Out-Null
    Add-Type -AssemblyName System.Drawing
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class AuroraWindowCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr after,
        int x, int y, int width, int height, uint flags);
}
'@
}

function Save-WindowCapture([IntPtr]$Window, [string]$Name) {
    $rect = New-Object AuroraWindowCapture+Rect
    if (-not [AuroraWindowCapture]::GetWindowRect($Window, [ref]$rect)) {
        throw 'Could not measure the client window for capture.'
    }
    $bitmap = New-Object Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $dc = $graphics.GetHdc()
        try {
            if (-not [AuroraWindowCapture]::PrintWindow($Window, $dc, 2)) {
                throw 'The Windows runner could not capture the native client window.'
            }
        } finally { $graphics.ReleaseHdc($dc) }
        $colors = [Collections.Generic.HashSet[int]]::new()
        for ($y = 0; $y -lt $bitmap.Height; $y += 7) {
            for ($x = 0; $x -lt $bitmap.Width; $x += 7) {
                [void]$colors.Add($bitmap.GetPixel($x, $y).ToArgb())
            }
        }
        if ($colors.Count -lt 16) { throw 'The captured window was blank or incomplete.' }
        $bitmap.Save((Join-Path (Resolve-Path $ScreenshotDirectory).Path $Name), [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

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
    if ($ScreenshotDirectory) {
        try {
            [AuroraWindowCapture]::SetWindowPos($client.MainWindowHandle, [IntPtr]::Zero, 40, 40, 1040, 760, 0x14) | Out-Null
            Start-Sleep -Seconds 2
            Save-WindowCapture $client.MainWindowHandle 'aurora-login-desktop.png'
            [AuroraWindowCapture]::SetWindowPos($client.MainWindowHandle, [IntPtr]::Zero, 40, 40, 460, 720, 0x14) | Out-Null
            Start-Sleep -Seconds 2
            Save-WindowCapture $client.MainWindowHandle 'aurora-login-narrow.png'
            'Real unauthenticated Windows client captures; authenticated screens remain unverified.' |
                Set-Content (Join-Path $ScreenshotDirectory 'capture-status.txt')
        } catch {
            Write-Warning $_
            "Screenshot verification pending: $_" | Set-Content (Join-Path $ScreenshotDirectory 'capture-status.txt')
        }
    }
    Write-Output 'The unauthenticated client created a native window and stayed responsive in an isolated profile.'
} finally {
    if ($client -and -not $client.HasExited) {
        Stop-Process -Id $client.Id -Force
        $client.WaitForExit()
    }
    Remove-Item -LiteralPath $profile -Recurse -Force
}
