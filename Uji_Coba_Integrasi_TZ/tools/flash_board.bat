@echo off
setlocal enabledelayedexpansion

echo =======================================================================
echo    STM32U575 TRUSTZONE FIRMWARE FLASH UTILITY (USB DFU MODE)
echo =======================================================================
echo.

set CLI="STM32_Programmer_CLI.exe"
set S_HEX="Secure\build\Uji_Coba_Integrasi_TZ_S.hex"
set NS_HEX="NonSecure\build\Uji_Coba_Integrasi_TZ_NS.hex"

:: 1. Verifikasi File Binary HEX
if not exist %S_HEX% (
    echo [ERROR] File %S_HEX% tidak ditemukan! Harap build proyek terlebih dahulu.
    pause
    exit /b 1
)

if not exist %NS_HEX% (
    echo [ERROR] File %NS_HEX% tidak ditemukan! Harap build proyek terlebih dahulu.
    pause
    exit /b 1
)

:: 2. Cek Koneksi USB DFU
echo [1/4] Mendeteksi board STM32U575 via USB DFU...
%CLI% -c port=usb1 > nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [PERHATIAN] Board tidak terdeteksi di port USB DFU!
    echo Langkah masuk mode DFU:
    echo  1. Tahan tombol BOOT0 (atau jumper BOOT0 ke 3.3V).
    echo  2. Tekan dan lepas tombol RESET (B2).
    echo  3. Lepas tombol BOOT0.
    echo.
    pause
    exit /b 1
)
echo [OK] Board STM32U575 terdeteksi di USB1.
echo.

:: 3. Flash Secure World Firmware
echo [2/4] Memprogram Firmware SECURE (%S_HEX% ke 0x08000000)...
%CLI% -c port=usb1 -d %S_HEX% -v
if %errorlevel% neq 0 (
    echo.
    echo [GAGAL] Gagal memprogram Secure World!
    echo Pastikan Option Byte TZEN=1 telah aktif dengan perintah:
    echo   STM32_Programmer_CLI -c port=usb1 -ob TZEN=1
    pause
    exit /b 1
)
echo [OK] Secure World berhasil diprogram dan diverifikasi.
echo.

:: 4. Flash Non-Secure World Firmware
echo [3/4] Memprogram Firmware NON-SECURE (%NS_HEX% ke 0x08080000)...
%CLI% -c port=usb1 -d %NS_HEX% -v
if %errorlevel% neq 0 (
    echo [GAGAL] Gagal memprogram Non-Secure World!
    pause
    exit /b 1
)
echo [OK] Non-Secure World berhasil diprogram dan diverifikasi.
echo.

:: 5. Reset MCU
echo [4/4] Mereset target MCU untuk menjalankan program...
%CLI% -c port=usb1 -rst
echo.
echo =======================================================================
echo [SUKSES] Seluruh firmware TrustZone berhasil diflash!
echo          Buka serial monitor pada COM17 (Baud: 115200 bps) untuk
echo          melihat boot banner dan log metrologi.
echo =======================================================================
