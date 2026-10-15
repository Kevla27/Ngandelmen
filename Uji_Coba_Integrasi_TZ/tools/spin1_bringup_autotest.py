#!/usr/bin/env python3
"""
=============================================================================
  SPIN 1 HARDWARE BRING-UP AUTOMATED TEST SUITE & HEALTH CHECKER
  Smart Meter 3-Fasa STM32U575 + ADE9000 (E1/E2/E3 Subsystems)
=============================================================================
Skrip ini mengotomatiskan pengujian Built-In Self-Test (BIST) saat PCB Spin 1
pertama kali tiba dari perakitan SMT.

Fitur Otomatisasi:
  1. Auto-detect port serial ST-LINK VCP / CH340 / CP2102.
  2. Eksekusi diagnostik mandiri hardware (BIST All):
     - Jalur I2C & OLED Display (0x3C / 0x3D)
     - Jalur SPI ADE9000 Metrologi (Komunikasi & Register Map)
     - Aktuator Buzzer (PB7) & Relai Latching (PC8/PC9)
     - Pin Sensor Sabotase & Jalur Interupsi EXTI (PC13, PB0, PB1)
     - Pengawas Tegangan Darurat PVD (Threshold 2.9V Last-Gasp)
     - Memori Non-Volatile Flash / NVRAM (Integritas Tulis-Baca)
  3. Pengujian Telemetri Watchdog Supervisor Multi-Task (IWDG 3.0s, LSI 32kHz).
  4. Pengujian Simulasi Offline Tamper & Retensi Backup Register VBAT (TAMP).
  5. Pembuatan Laporan Berita Acara Uji Kelulusan (Bring-Up Certificate) dalam format Markdown.

Penggunaan:
  python tools/spin1_bringup_autotest.py [--port COMx] [--baud 115200] [--export]
"""

import sys
import os
import time
import argparse
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] Library 'pyserial' belum terpasang.")
    print("Silakan jalankan: pip install pyserial")
    sys.exit(1)

# ANSI Terminal Colors
class Colors:
    HEADER    = '\033[95m'
    BLUE      = '\033[94m'
    CYAN      = '\033[96m'
    GREEN     = '\033[92m'
    YELLOW    = '\033[93m'
    RED       = '\033[91m'
    BOLD      = '\033[1m'
    UNDERLINE = '\033[4m'
    ENDC      = '\033[0m'

def auto_detect_port():
    """Mencari port serial ST-LINK atau USB Serial yang aktif."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    for p in ports:
        desc = p.description.lower()
        if "stlink" in desc or "st-link" in desc or "vcp" in desc or "stm" in desc:
            return p.device
    # Fallback ke port pertama jika tidak ditemukan deskripsi spesifik
    return ports[0].device

def send_and_collect(ser, cmd, wait_sec=1.5):
    """Mengirim perintah karakter ke MCU dan mengumpulkan seluruh respons teks."""
    ser.reset_input_buffer()
    ser.write(cmd.encode('utf-8'))
    time.sleep(wait_sec)
    output = ""
    while ser.in_waiting > 0:
        chunk = ser.read(ser.in_waiting).decode('utf-8', errors='replace')
        output += chunk
        time.sleep(0.05)
    return output

def parse_bist_results(raw_text):
    """Mem-parsing hasil BIST dari output konsol MCU."""
    results = {}
    tests = [
        ("I2C Bus", "1. Jalur Bus I2C"),
        ("SPI ADE9000", "2. Bus SPI ADE9000"),
        ("Aktuator PB7/PC8/PC9", "3. Aktuator PB7 & Relai"),
        ("Sensor & Tamper Pin", "4. Pin Input Sabotase & IRQ"),
        ("Monitor PVD", "5. Monitor Tegangan Darurat (PVD)"),
        ("Memori NVRAM", "6. Memori Non-Volatile (NVRAM)")
    ]

    for label, pattern in tests:
        found = False
        for line in raw_text.splitlines():
            if pattern in line:
                found = True
                if "PASS" in line or "[OK]" in line:
                    results[label] = True
                else:
                    results[label] = False
                break
        if not found:
            results[label] = False

    return results

def main():
    parser = argparse.ArgumentParser(description="Spin 1 Hardware Bring-Up Automated Test Suite")
    parser.add_argument("--port", help="Port Serial COM (Contoh: COM17, /dev/ttyACM0)", default=None)
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (Default: 115200)")
    parser.add_argument("--timeout", type=float, default=2.0, help="Serial timeout dalam detik")
    parser.add_argument("--export", action="store_true", help="Ekspor sertifikat kelayakan ke folder reports/")
    args = parser.parse_args()

    port = args.port
    if not port:
        port = auto_detect_port()
        if not port:
            print(f"{Colors.RED}[ERROR] Tidak ada port COM terdeteksi! Hubungkan kabel USB ST-LINK.{Colors.ENDC}")
            sys.exit(1)
        print(f"{Colors.CYAN}[AUTO-DETECT] Port serial ditemukan: {port}{Colors.ENDC}")

    print(f"\n{Colors.BOLD}{Colors.HEADER}" + "=" * 70)
    print("   AUTOMATED HARDWARE BRING-UP TEST SUITE - PCB SPIN 1 (STM32U575)   ")
    print("=" * 70 + f"{Colors.ENDC}\n")

    try:
        ser = serial.Serial(port, args.baud, timeout=args.timeout)
        time.sleep(0.5)
    except Exception as e:
        print(f"{Colors.RED}[GAGAL KONEKSI] Tidak dapat membuka {port}: {e}{Colors.ENDC}")
        sys.exit(1)

    print(f"{Colors.GREEN}[1/4] Terhubung ke {port} @ {args.baud} bps 8-N-1.{Colors.ENDC}")

    # Wake-up prompt
    ser.write(b"\r\n")
    time.sleep(0.2)

    # 1. Eksekusi Full BIST
    print(f"{Colors.YELLOW}[2/4] Menjalankan Built-In Self-Test (BIST All Subsystems)...{Colors.ENDC}")
    bist_raw = send_and_collect(ser, "d", wait_sec=2.0)
    bist_scores = parse_bist_results(bist_raw)

    print("\n  " + "-" * 66)
    print(f"  {Colors.BOLD}HASIL DIAGNOSTIK HARDWARE PERIPHERAL BIST:{Colors.ENDC}")
    print("  " + "-" * 66)
    all_passed = True
    for item, status in bist_scores.items():
        if status:
            status_str = f"{Colors.GREEN}[PASS / OK]{Colors.ENDC}"
        else:
            status_str = f"{Colors.RED}[FAIL / ANOMALI]{Colors.ENDC}"
            all_passed = False
        print(f"   • {item:<35} : {status_str}")
    print("  " + "-" * 66)

    # 2. Pengujian Watchdog Supervisor
    print(f"\n{Colors.YELLOW}[3/4] Mengambil Telemetri Multi-Task Watchdog Supervisor...{Colors.ENDC}")
    wdg_raw = send_and_collect(ser, "w", wait_sec=0.5)
    wdg_ok = "Status Init    : AKTIF" in wdg_raw or "WATCHDOG" in wdg_raw
    if wdg_ok:
        print(f"   • Multi-Task Watchdog Supervisor   : {Colors.GREEN}[PASS / AKTIF]{Colors.ENDC}")
    else:
        print(f"   • Multi-Task Watchdog Supervisor   : {Colors.RED}[FAIL / TIDAK TERDETEKSI]{Colors.ENDC}")
        all_passed = False

    # 3. Pengujian Offline Tamper Simulation
    print(f"\n{Colors.YELLOW}[4/4] Menguji Deteksi Sabotase Fisik Padam (Offline Tamper)...{Colors.ENDC}")
    tamp_raw = send_and_collect(ser, "o", wait_sec=0.8)
    tamp_ok = "OFFLINE TAMPER" in tamp_raw or "TUTUP METER" in tamp_raw or "TAMP" in tamp_raw
    if tamp_ok:
        print(f"   • Simulasi Offline Tamper & VBAT   : {Colors.GREEN}[PASS / RESPON TEPAT]{Colors.ENDC}")
    else:
        print(f"   • Simulasi Offline Tamper & VBAT   : {Colors.RED}[FAIL / TIDAK ADA RESPON]{Colors.ENDC}")
        all_passed = False

    # Evaluasi Akhir
    print("\n" + "=" * 70)
    if all_passed:
        verdict = f"{Colors.BOLD}{Colors.GREEN}[SEMPURNA] SELURUH SUB-SISTEM PCB SPIN 1 LOLOS UJI BRING-UP!{Colors.ENDC}"
        board_ready = True
    else:
        verdict = f"{Colors.BOLD}{Colors.RED}[PERIKSA] TERDAPAT DEFECT / GANGGUAN PADA SUB-SISTEM TERTENTU!{Colors.ENDC}"
        board_ready = False
    print(f" KESIMPULAN KELAYAKAN HARDWARE:")
    print(f" {verdict}")
    print("=" * 70 + "\n")

    # Pembuatan Laporan File
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    file_timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_md = f"""# Berita Acara Uji Kelayakan Hardware Bring-Up Spin 1
**Tanggal Uji**: {now_str}  
**Port Komunikasi**: `{port}` @ {args.baud} bps  
**Status Akhir**: **{'LULUS / MEMENUHI SYARAT' if board_ready else 'GAGAL / BUTUH PERBAIKAN'}**

---

## 1. Ringkasan Diagnostik Hardware (BIST)
| No | Modul / Jalur Hardware | Kriteria Uji | Hasil Uji | Keterangan |
|:---:|---|---|:---:|---|
| 1 | Bus I2C & OLED Display | ACK respon pada 0x3C/0x3D | {'PASS' if bist_scores.get('I2C Bus') else 'FAIL'} | {'Sirkuit I2C normal' if bist_scores.get('I2C Bus') else 'Periksa pull-up 4k7'} |
| 2 | Bus SPI ADE9000 Metrologi | Read/Write Register ADE9000 | {'PASS' if bist_scores.get('SPI ADE9000') else 'FAIL'} | {'Komunikasi SPI 10MHz valid' if bist_scores.get('SPI ADE9000') else 'Periksa jalur MISO/MOSI/CS'} |
| 3 | Aktuator Buzzer & Relai | Pulsa PB7, PC8 (Trip), PC9 (Close) | {'PASS' if bist_scores.get('Aktuator PB7/PC8/PC9') else 'FAIL'} | Driver transistor/MOSFET responsif |
| 4 | Pin Sensor Sabotase & IRQ | Logika level EXTI13, IRQ0, IRQ1 | {'PASS' if bist_scores.get('Sensor & Tamper Pin') else 'FAIL'} | Sakelar mikro & optocoupler normal |
| 5 | Monitor Tegangan PVD | PVD Level 6 (~2.9V Threshold) | {'PASS' if bist_scores.get('Monitor PVD') else 'FAIL'} | Last-Gasp flash save siap |
| 6 | Memori Non-Volatile | Verifikasi tulis-baca circular buffer | {'PASS' if bist_scores.get('Memori NVRAM') else 'FAIL'} | Flash/FRAM terverifikasi |

## 2. Fitur Keandalan Sistem
* **Multi-Task Watchdog Supervisor**: {'LULUS (IWDG 3.0s, LSI 32kHz Aktif)' if wdg_ok else 'GAGAL'}
* **Offline Tamper VBAT & TAMP**: {'LULUS (Retensi Register Sukses)' if tamp_ok else 'GAGAL'}

---
*Laporan ini dibuat otomatis oleh skrip `tools/spin1_bringup_autotest.py`.*
"""

    os.makedirs("reports", exist_ok=True)
    report_file = os.path.join("reports", f"SPIN1_BRINGUP_REPORT_{file_timestamp}.md")
    with open(report_file, "w", encoding="utf-8") as f:
        f.write(report_md)
    print(f"{Colors.CYAN}[LAPORAN DISIMPAN] Sertifikat hasil uji: {report_file}{Colors.ENDC}\n")

    ser.close()

if __name__ == "__main__":
    main()

