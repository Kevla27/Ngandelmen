# Script PowerShell untuk Flash STM32U575 TrustZone via USB DFU
$ErrorActionPreference = "Stop"

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "   STM32U575 TRUSTZONE FIRMWARE FLASH UTILITY (POWERSHELL / USB DFU)   " -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

$CLI = "STM32_Programmer_CLI.exe"
$SecureHex = "Secure\build\Uji_Coba_Integrasi_TZ_S.hex"
$NonSecureHex = "NonSecure\build\Uji_Coba_Integrasi_TZ_NS.hex"

# 1. Cek keberadaan file HEX
if (-not (Test-Path $SecureHex) -or -not (Test-Path $NonSecureHex)) {
    Write-Host "[ERROR] Binary .hex belum dibuat! Jalankan 'cmake --build --preset Debug' terlebih dahulu." -ForegroundColor Red
    exit 1
}

# 2. Cek Koneksi USB DFU
Write-Host "[1/4] Mendeteksi board STM32U575 di port usb1..." -ForegroundColor Yellow
& $CLI -c port=usb1
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[PERHATIAN] Board tidak terdeteksi di port USB DFU!" -ForegroundColor Red
    Write-Host "Langkah masuk mode DFU:" -ForegroundColor Yellow
    Write-Host " 1. Tahan tombol BOOT0 (atau hubungkan pin BOOT0 ke 3.3V)." -ForegroundColor Yellow
    Write-Host " 2. Tekan dan lepas tombol RESET (B2)." -ForegroundColor Yellow
    Write-Host " 3. Lepas tombol BOOT0." -ForegroundColor Yellow
    exit 1
}
Write-Host "[OK] Board terhubung di port USB1." -ForegroundColor Green
Write-Host ""

# 3. Flash Secure World
Write-Host "[2/4] Flashing Secure World ($SecureHex ke 0x08000000)..." -ForegroundColor Yellow
& $CLI -c port=usb1 -d $SecureHex -v
if ($LASTEXITCODE -ne 0) {
    Write-Host "[GAGAL] Gagal flashing Secure World!" -ForegroundColor Red
    Write-Host "Pastikan TZEN=1 aktif dengan perintah: STM32_Programmer_CLI -c port=usb1 -ob TZEN=1" -ForegroundColor Yellow
    exit 1
}
Write-Host "[OK] Secure World terverifikasi." -ForegroundColor Green
Write-Host ""

# 4. Flash Non-Secure World
Write-Host "[3/4] Flashing Non-Secure World ($NonSecureHex ke 0x08080000)..." -ForegroundColor Yellow
& $CLI -c port=usb1 -d $NonSecureHex -v
if ($LASTEXITCODE -ne 0) {
    Write-Host "[GAGAL] Gagal flashing Non-Secure World!" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Non-Secure World terverifikasi." -ForegroundColor Green
Write-Host ""

# 5. Reset target
Write-Host "[4/4] Mereset target MCU..." -ForegroundColor Yellow
& $CLI -c port=usb1 -rst

Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Green
Write-Host " [SUKSES] Flash selesai! Buka serial terminal di COM17 @ 115200 bps." -ForegroundColor Green
Write-Host "=======================================================================" -ForegroundColor Green
