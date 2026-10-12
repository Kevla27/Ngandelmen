# Rangkuman Komprehensif Fitur Program Firmware Smart Meter 3-Fasa TrustZone

Dokumen ini menyajikan inventarisasi lengkap seluruh arsitektur, modul subsistem, dan fitur operasional yang telah dirancang dan diimplementasikan pada firmware **Smart Meter 3-Fasa Berbasis ARM Cortex-M33 TrustZone (STM32U575ZIT6Q)**, memenuhi standar **SPLN D3.006: 2021 + SUP-1: 2022 + SUP-2: 2023** dan standar tampilan **SPLN D3.006-1: 2017 (Gambar 4)**.

---

## 1. Arsitektur Sistem Terintegrasi (Multi-Layer & Dual-Domain)

Firmware dibangun menggunakan arsitektur modular berlapis (*layered architecture*) yang memisahkan ranah keamanan (*TrustZone Security*), manajemen tugas (*RTOS*), protokol komunikasi (*DLMS/COSEM*), pengolahan metrologi (*Metrology Adapter*), dan antarmuka pengguna (*HMI & Alarm Actuation*).

```
+---------------------------------------------------------------------------------------+
|                                    HARDWARE MCU                                       |
|                    STM32U575ZIT6Q (2 MB Dual-Bank Flash, 786 KB SRAM)                 |
+-------------------------------------------+-------------------------------------------+
|          SECURE WORLD (Bank 1)            |          NON-SECURE WORLD (Bank 2)        |
|          Flash: 0x08000000 (512 KB)       |          Flash: 0x08100000 (1024 KB)      |
+-------------------------------------------+-------------------------------------------+
| - Root of Trust & System Clock (160 MHz)  | - FreeRTOS Kernel (Preemptive Scheduler)   |
| - Security Attribution Unit (SAU)         | - Subsystem Metrologi (E1 Adapter)        |
| - TrustZone Security Controller (TZSC)    | - Subsystem Protokol DLMS/COSEM (E2)      |
| - Secure EXTI13 Interupsi Tombol PC13     | - Subsystem Display HMI SPLN Gambar 4 (E3)|
| - Secure Gateway Non-Secure Callable(NSC) | - Subsystem Tamper & Alarm Sabotase (E3)  |
| - Delegasi Pin Non-Secure: PB7, PB8, PB9, | - Subsystem Profil Beban 15-Menit (E3)    |
|   PA2, PA3, PA9, PA10                     | - Driver BSP: OLED, Buzzer/LED, LPUART    |
+-------------------------------------------+-------------------------------------------+
```

---

## 2. Inventarisasi Fitur Utama per Subsistem

### A. Fitur Keamanan Perangkat Keras (TrustZone Security Core)
1. **Pemisahan Fisik Dual-Bank Flash**:
   - Secure World dialokasikan di Bank 1 (`0x08000000` s.d. `0x0807FFFF`).
   - Non-Secure World dialokasikan di Bank 2 (`0x08100000` s.d. `0x081FFFFF`), menjamin kode aplikasi umum tidak dapat memanipulasi kunci enkripsi atau vektor proteksi aman.
2. **Secure Non-Secure Callable (NSC) Interface**:
   - Pintu gerbang aman berbasis instruksi `SG` (Secure Gateway) untuk registrasi callback interupsi (`SECURE_RegisterCallback`).
   - Melindungi vektor sabotase pembukaan tutup meter (*Meter Cover Open*) dari injeksi perangkat lunak tak berizin.
3. **Pendelegasian Dinamis Periferal I/O (GTZC / TZSC)**:
   - Peripheral komunikasi (USART1, LPUART1, I2C1, DMA1) dan pin GPIO aktuasi (PB7, PB8, PB9) dialokasikan secara eksplisit ke ranah Non-Secure agar dapat dikelola langsung oleh task-task FreeRTOS.

---

### B. Fitur Metrologi & Konversi Sinyal (Subsystem E1 - Metrology)
1. **Akuisisi Sinyal 3-Fasa 4-Kawat (ADE9000 Interface)**:
   - Komunikasi SPI berkecepatan tinggi dengan validasi checksum CRC-16 hardware.
2. **Kalkulasi Parameter Kelistrikan Lengkap**:
   - **Tegangan Fasa-Netral**: $V_R, V_S, V_T$ (skala $0.1\text{ V}$ / desivolt, resolusi $230.0\text{ V}$).
   - **Arus Tiap Fasa & Netral**: $I_R, I_S, I_T, I_N$ dengan **presisi tinggi 3 desimal** ($X.XXX\text{ A}$, misal `11.946 A`) sesuai standar SPLN Tabel E.1.
   - **Daya Aktif ($W$), Reaktif ($\text{var}$), Semu ($\text{VA}$)**: Pengukuran daya fasa maupun total.
   - **Faktor Daya ($\cos \phi$)**: Resolusi per mil ($0.001$).
   - **Frekuensi Jaringan**: Skala milli-Hertz ($50.00\text{ Hz}$).
   - **Akumulasi Energi Aktif Total**: Satuan Watt-hour ($Wh$) dan registrasi OBIS kiloWatt-hour ($0.01\text{ kWh}$).
3. **Thread-Safe Measurement Cache**:
   - Dilindungi menggunakan FreeRTOS Mutex (`xMeasMutex`) untuk menjamin konsistensi data saat diakses bersamaan oleh display, load profile, dan DLMS stack.

---

### C. Fitur Tampilan & HMI Standar PLN (Subsystem E3 - Display HMI)
Sesuai standar **SPLN D3.006-1: 2017 Lampiran Gambar 4** dan **SPLN D3.006: 2021 Pasal 6.11**:
1. **Format Layar 3-Baris Standard Gambar 4**:
   - **Baris 1 (Header Status & Indeks Carousel)**:
     - Ikon status alarm: Menampilkan `OK` saat normal dan `!ALM` saat terjadi gangguan/sabotase.
     - Indeks urutan display 2 digit (`zz`: `01` s.d. `15` saat normal, atau `AL` saat alarm).
   - **Baris 2 (Nilai Utama & Satuan Unit)**:
     - Angka nilai pengukuran font besar (7-segment custom font) atau font 7x10.
     - Satuan resmi tersemat di kanan: `V`, `A`, `W`, `var`, `VA`, `Hz`, `kWh`, `WIB`.
   - **Baris 3 (Kode Register OBIS Ringkas PLN)**:
     - Kode standar PLN (`96.01`, `32.07`, `31.07`, `01.07`, `01.08`, `01.00`, `96.50`, dll).
2. **Siklus Carousel Otomatis 15 Halaman Regulasi**:
   - **Page 01 (`zz=01`)**: ID Pelanggan / Nomor Seri Meter (OBIS `96.01`).
   - **Page 02-04 (`zz=02..04`)**: Tegangan Fasa R, S, T (OBIS `32.07`, `52.07`, `72.07`).
   - **Page 05-08 (`zz=05..08`)**: Arus Fasa R, S, T, dan Netral 3 desimal (OBIS `31.07`, `51.07`, `71.07`, `91.07`).
   - **Page 09 (`zz=09`)**: Daya Aktif Total (OBIS `01.07`).
   - **Page 10 (`zz=10`)**: Daya Reaktif Total (OBIS `03.07`).
   - **Page 11 (`zz=11`)**: Daya Semu Total (OBIS `09.07`).
   - **Page 12 (`zz=12`)**: Faktor Daya / Power Factor (OBIS `13.07`).
   - **Page 13 (`zz=13`)**: Frekuensi Sistem (OBIS `14.07`).
   - **Page 14 (`zz=14`)**: Total Energi Aktif Impor (OBIS `01.08`).
   - **Page 15 (`zz=15`)**: Waktu & Jam Nyata / RTC (OBIS `01.00`, format `HH:MM:SS WIB`).
3. **Interval Scrolling 5000 ms**:
   - Selang waktu pergeseran otomatis diatur minimal 5 detik sesuai Pasal 6.11 SPLN D3.006.
4. **I2C Bus Recovery Otomatis**:
   - Rutin recovery clock pulsing 9x pada jalur SCL/SDA untuk memulihkan modul OLED jika mengalami hang saat boot.
5. **Output Layar Ganda (Dual Display Engine)**:
   - Mendukung rendering grafis piksel ke modul **OLED SSD1306 128x32 I2C**.
   - Mendukung rendering teks ASCII frame ke **Terminal Konsol Serial UART** (`display_render_frame`).

---

### D. Fitur Deteksi Sabotase & Alarm Darurat (Subsystem E3 - Tamper & Security)
Memenuhi **SPLN D3.006: 2021 Tabel 4 (Alarm Internal Hardware)** dan **Tabel 6 (Respons Ketidaknormalan & Tampering)**:
1. **Dukungan 9 Vektor Sabotase Eksternal**:
   - **Case Open (Tutup Meter Terbuka)**: Banner `!ALM`, indeks `AL`, teks `RUSAK`, OBIS `96.50`.
   - **Terminal Cover Open**: Teks `PERIKSA ERR20`.
   - **Wrong Phase Sequence (Urutan Fasa Terbalik)**: Teks `PERIKSA ERR21`.
   - **Cross Phase Current (Arus Fasa Bersilangan)**: Teks `PERIKSA ERR22`.
   - **Neutral Missing / Ground Tamper**: Teks `PERIKSA ERR23`.
   - **Missing Voltage (Tegangan Hilang)**: Teks `PERIKSA ERR24`.
   - **Strong Magnetic Field (Medan Magnet Abnormal)**: Teks `PERIKSA ERR25`.
   - **Reverse Active Power (Arah Arus Terbalik)**: Teks `REVERSE ERR26`.
   - **Low Power Factor Quad 4**: Teks `PERIKSA ERR27`.
2. **Dukungan 8 Alarm Internal Hardware**:
   - Flash ROM Error (`ERR00`), RAM Error (`ERR01`), RTC Clock Loss (`ERR02`), Low Battery (`ERR03`), MCU Failure (`ERR04`), ADC Error (`ERR05`), Supercapacitor Fail (`ERR06`), Relay Hardware Fail (`ERR07`).
3. **Pemicu Sabotase Dual-Channel (Hardware & Serial)**:
   - **Hardware Button PC13**: Tombol biru fisik B1 pada board Nucleo via Secure EXTI13.
   - **UART Virtual Trigger**: Tombol keyboard `'t'`, `'T'`, `'s'`, `'S'`, atau spasi melalui terminal serial ST-LINK VCP.
4. **Respons Instan (*Instant OLED Refresh*)**:
   - Begitu sabotase terjadi, layar langsung beralih ke halaman sabotase tanpa menunggu selesainya timer scrolling 5 detik.
   - Tombol ditekan sekali lagi untuk memulihkan status ke kondisi normal (`OK`).

---

### E. Fitur Aktuasi Buzzer & Indikator Alarm SPLN (Pin PB7)
Memenuhi **SPLN D3.006: 2021 Butir 6.14 (Indikator Alarm Audio & Visual)**:
1. **Abstraksi Driver Hardware BSP (`bsp_alarm_buzzer`)**:
   - Mengendalikan pin GPIO **PB7** (terhubung ke User LED2 Blue board Nucleo dan jalur driver piezo buzzer eksternal).
   - Menyediakan simulasi state mock otomatis saat dikompilasi pada host PC (tanpa dependensi HAL).
2. **Irama Peringatan Beeper (Beep Cadence)**:
   - Saat alarm aktif: Pin PB7 berkedip/berbunyi dengan irama **200 ms ON / 200 ms OFF**.
   - Saat alarm pulih: Pin PB7 otomatis direset ke kondisi **LOW / Mati**.

---

### F. Fitur Protokol Komunikasi DLMS/COSEM (Subsystem E2 - Communication)
1. **Lapisan Fisik & Data Link HDLC (IEC 62056-46)**:
   - Transmisi serial non-blocking melalui LPUART1 dengan DMA Circular Buffer.
   - Handshake koneksi: Frame SNRM (Set Normal Response Mode), UA (Unnumbered Acknowledge), dan DISC (Disconnect).
   - Validasi integritas frame berbasis HDLC CRC-16 Frame Check Sequence (FCS).
2. **Lapisan Aplikasi DLMS/COSEM (IEC 62056-53 & 62056-62)**:
   - Manajemen Asosiasi: `AARQ` (Application Association Request) dan `AARE` (Association Response) untuk sesi Public Client (Read-Only) dan Management Client.
   - Layanan Pesan: `Get-Request-Normal` dan `Get-Response-Normal`.
3. **Kamus Register OBIS DLMS Terintegrasi**:
   - Pengukuran Tegangan fasa R, S, T (`1.0.32.7.0.255`, `1.0.52.7.0.255`, `1.0.72.7.0.255`).
   - Pengukuran Arus fasa R, S, T, N (`1.0.31.7.0.255`, `1.0.51.7.0.255`, `1.0.71.7.0.255`, `1.0.91.7.0.255`).
   - Energi Aktif Total (`1.0.1.8.0.255`).
   - Clock RTC (`0.0.1.0.0.255`).
   - Profil Buku Tamper Log Generic (`0.0.99.98.0.255`) dengan penomoran event counter individual (`0.0.96.20.X.255`).
4. **Perekaman Tamper Otomatis ke DLMS Buffer**:
   - Setiap pemicuan sabotase dari Task E3 secara instan dikonversi menjadi record DLMS ber-timestamp epoch UNIX dan status flag (1 = Aktif, 0 = Selesai).

---

### G. Fitur Profil Beban & Penyimpanan Persisten (Subsystem E3 - Load Profile & NVRAM)
1. **Load Profile Generic 15-Menit**:
   - Penjadwalan pembekuan snapshot data kelistrikan tiap 900 detik (15 menit) sesuai standar grid profil beban.
   - Parameter snapshot mencakup seluruh tegangan, arus, daya, energi, dan frekuensi sesaat.
2. **Flash NVRAM Persistence**:
   - Emulasi penyimpanan data penting (tamper counters dan event log) ke sektor memori flash internal agar data tetap tersimpan saat listrik padam (*power cycle resilience*).

---

### H. Fitur Multitasking & Manajemen Tugas (FreeRTOS Lead)
Empat task independen yang diatur secara preemptive berbasis prioritas:

| Nama Task | Prioritas | Frekuensi / Interval | Deskripsi Tanggung Jawab |
|---|:---:|:---:|---|
| **`task_tamper_emergency`** | **4 (Tinggi)** | 200 ms | Memantau pemicu sabotase, mengirim event ke antrean, menggerakkan irama beeper/LED PB7 (200ms ON/OFF). |
| **`task_metrology_profiling`**| **3 (Sedang)** | 1000 ms / 15 menit | Menjalankan sampling metrologi 1 Hz, sinkronisasi mutex, dan membekukan snapshot profil beban 15 menit. |
| **`task_dlms_server`** | **2 (Normal)** | Event-Driven | Memproses paket data link HDLC dan APDU DLMS/COSEM yang diterima dari LPUART DMA. |
| **`task_oled_carousel`** | **1 (Rendah)** | 100 ms loop (5s cycle) | Mengatur transisi 15 halaman layar standar SPLN Gambar 4 dan merender tampilan grafis OLED. |

---

### I. Fitur Konsol Universal & Pengujian Otomatis
1. **Konsol Serial Universal**:
   - Menampilkan boot banner informatif hardware clock (160 MHz), peripheral map, dan status inisialisasi FreeRTOS saat startup.
   - Memberikan laporan debug waktu-nyata untuk setiap perubahan halaman OLED, sampling metrologi, dan pencatatan event DLMS.
2. **Infrastruktur Pengujian Otomatis (Host & Target)**:
   - **15 Test Suite CTest**: Verifikasi unit terotomatisasi mencakup HDLC, APDU, Tamper Log, OBIS Dictionary, FSM Statechart, Load Profile, dan FreeRTOS Tasks.
   - **Uji Integrasi End-to-End (`test_e1_to_e2_integration.exe`)**: Pengujian alur utuh dari modul Metrologi E1 $\rightarrow$ Aplikasi E3 $\rightarrow$ Transaksi Paket DLMS E2.
   - **Uji Kepatuhan Tampilan SPLN (`test_spln_display_tamper.exe`)**: Verifikasi otomatis terhadap 9 kode sabotase Tabel 6 dan 8 kode alarm hardware Tabel 4 SPLN D3.006.

---

## 3. Matriks Kepatuhan Standar Regulasi

| No | Regulasi / Standar Acuan | Butir / Klausul | Status Implementasi |
|:--:|---|---|:---:|
| 1 | **SPLN D3.006: 2021** | Butir 6.11 (Format Waktu RTC `00.00` s.d `23.59`) | **100% Sesuai** (Page 15 OBIS `01.00`) |
| 2 | **SPLN D3.006: 2021** | Butir 6.11 (Interval Carousel Otomatis $\ge$ 5 detik) | **100% Sesuai** (Interval 5000 ms) |
| 3 | **SPLN D3.006: 2021** | Butir 6.14 (Indikator Alarm Audio Buzzer & Visual LED) | **100% Sesuai** (Aktuasi PB7 200ms ON/OFF) |
| 4 | **SPLN D3.006: 2021** | Tabel 4 (Alarm Internal Hardware ERR00..ERR07) | **100% Sesuai** (Dukungan 8 kode alarm) |
| 5 | **SPLN D3.006: 2021** | Tabel 6 (Tamper & Ketidaknormalan ERR20..ERR27, RUSAK, REVERSE) | **100% Sesuai** (Dukungan 9 vektor sabotase) |
| 6 | **SPLN D3.006: 2021** | Lampiran E Tabel E.1 (Arus Fasa & Netral 3 Desimal) | **100% Sesuai** (Format `X.XXX A`) |
| 7 | **SPLN D3.006-1: 2017** | Gambar 4 (Tata Letak Layar 3-Baris: Status/zz/Nilai/OBIS) | **100% Sesuai** (Header, Font Besar, Baris OBIS) |
| 8 | **IEC 62056 / DLMS UA** | Blue Book & Green Book (HDLC, APDU, Association, OBIS Profile) | **100% Sesuai** (Terverifikasi End-to-End) |
| 9 | **Arm TrustZone-M** | Isolasi Keamanan Perangkat Keras Dual-World Cortex-M33 | **100% Sesuai** (SAU, GTZC, Bank 2 Flash Jump) |

