param(
    [string]$Port = "COM19",
    [int]$BaudRate = 115200
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " STM32U575 TRUSTZONE VIRTUAL COM PORT MONITOR ($Port)" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

$sp = New-Object System.IO.Ports.SerialPort $Port, $BaudRate
$sp.ReadTimeout = 2000

try {
    $sp.Open()
    Write-Host "Berhasil terhubung ke $Port ($BaudRate bps). Tekan Ctrl+C untuk keluar.`n" -ForegroundColor Green
    while ($true) {
        if ($sp.BytesToRead -gt 0) {
            $text = $sp.ReadExisting()
            Write-Host -NoNewline $text
        }
        Start-Sleep -Milliseconds 20
    }
} catch {
    Write-Host "`nError membuka port $Port : $_" -ForegroundColor Red
} finally {
    if ($sp.IsOpen) {
        $sp.Close()
        Write-Host "`nPort $Port ditutup." -ForegroundColor Yellow
    }
}

