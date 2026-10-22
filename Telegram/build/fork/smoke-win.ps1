param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [string]$ScreenshotDirectory = '',
    [switch]$CaptureMotion
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
    [StructLayout(LayoutKind.Sequential)]
    public struct Point { public int X, Y; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr after,
        int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr window, uint message, IntPtr buttons, IntPtr position);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    public static extern bool GetCursorPos(out Point point);
    [DllImport("user32.dll")]
    public static extern bool ScreenToClient(IntPtr window, ref Point point);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
}

'@
}

function Send-ClientPointer([IntPtr]$Window, [int]$ScreenX, [int]$ScreenY, [uint32]$Message, [int]$Buttons) {
    $point = New-Object AuroraWindowCapture+Point
    $point.X = $ScreenX
    $point.Y = $ScreenY
    [AuroraWindowCapture]::SetCursorPos($ScreenX, $ScreenY) | Out-Null
    if (-not [AuroraWindowCapture]::ScreenToClient($Window, [ref]$point)) {
        throw 'Could not map the test pointer to the isolated client.'
    }
    $packed = (($point.Y -band 0xffff) -shl 16) -bor ($point.X -band 0xffff)
    if (-not [AuroraWindowCapture]::PostMessage($Window, $Message, [IntPtr]$Buttons, [IntPtr]$packed)) {
        throw 'Could not deliver pointer input to the isolated client.'
    }
}

function Save-MotionCaptures([IntPtr]$Window) {
    $rect = New-Object AuroraWindowCapture+Rect
    $cursor = New-Object AuroraWindowCapture+Point
    if (-not [AuroraWindowCapture]::GetWindowRect($Window, [ref]$rect)) {
        throw 'Could not measure the client window for motion capture.'
    }
    if (-not [AuroraWindowCapture]::GetCursorPos([ref]$cursor)) {
        throw 'The Windows runner does not expose a pointer for motion capture.'
    }
    [AuroraWindowCapture]::SetForegroundWindow($Window) | Out-Null
    $centerX = $rect.Left + [int](($rect.Right - $rect.Left) / 2)
    $buttonY = $rect.Top + [int](($rect.Bottom - $rect.Top) * 0.666)
    $outsideX = $rect.Left + 50
    $outsideY = $rect.Bottom - 50
    $frames = [Collections.Generic.List[object]]::new()
    $hashes = [Collections.Generic.HashSet[string]]::new()
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $motionDirectory = Join-Path $ScreenshotDirectory 'motion'
    New-Item -ItemType Directory -Path $motionDirectory -Force | Out-Null
    try {
        Send-ClientPointer $Window $outsideX $outsideY 0x0200 0
        Start-Sleep -Milliseconds 250
        foreach ($phase in @('hover', 'press', 'release')) {
            if ($phase -eq 'hover') { Send-ClientPointer $Window $centerX $buttonY 0x0200 0 }
            if ($phase -eq 'press') { Send-ClientPointer $Window $centerX $buttonY 0x0201 1 }
            if ($phase -eq 'release') {
                Send-ClientPointer $Window $outsideX $outsideY 0x0200 1
                Send-ClientPointer $Window $outsideX $outsideY 0x0202 0
            }
            for ($frame = 0; $frame -lt 12; $frame++) {
                $name = ('motion/{0}-{1:D2}.png' -f $phase, $frame)
                $before = $watch.Elapsed.TotalMilliseconds
                Save-WindowCapture $Window $name
                $captureMs = $watch.Elapsed.TotalMilliseconds - $before
                $path = Join-Path $ScreenshotDirectory $name
                $digest = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
                $hashes.Add($digest) | Out-Null
                $frames.Add([ordered]@{
                    file = $name; phase = $phase; elapsed_ms = $before
                    capture_pipeline_ms = $captureMs; sha256 = $digest
                })
                Start-Sleep -Milliseconds 16
            }
        }
    } finally {
        try {
            Send-ClientPointer $Window $outsideX $outsideY 0x0202 0
        } finally {
            [AuroraWindowCapture]::SetCursorPos($cursor.X, $cursor.Y) | Out-Null
        }
    }
    $report = [ordered]@{
        source_commit = $env:GITHUB_SHA
        scope = 'Real unauthenticated Windows welcome-button hover, press and release; PrintWindow PNG sequence.'
        status = $(if ($hashes.Count -gt 3) { 'interaction_frames_changed' } else { 'visual_change_unverified' })
        unique_frames = $hashes.Count
        limitation = 'Capture-pipeline timing is not application frame pacing or a 60/120 Hz performance result. Authenticated interactions remain unverified.'
        frames = $frames
    }
    $report | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $ScreenshotDirectory 'motion-capture.json')
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
            if ($CaptureMotion) {
                try {
                    Save-MotionCaptures $client.MainWindowHandle
                } catch {
                    Write-Warning $_
                    "Motion capture pending: $_" | Set-Content (Join-Path $ScreenshotDirectory 'motion-capture-pending.txt')
                }
            }
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
