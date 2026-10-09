# Harmonisasi Kode Error & Alarm Firmware Sesuai Standar SPLN D3.006:2021

Dokumen ini mendokumentasikan penyesuaian (*harmonization*) seluruh kode error, alarm internal, dan peringatan sabotase (*tamper*) pada firmware Smart Meter Fase Tiga agar patuh 100% terhadap spesifikasi resmi **SPLN D3.006: 2021 + SUP-1: 2022 + SUP-2: 2023 (CSV 2021.2): Meter Statik Pascabayar Fase Tiga**.

---

## 1. Landasan Standar SPLN D3.006: 2021

Berdasarkan dokumen acuan resmi PT PLN (Persero):
1. **Pasal 7.3 & Tabel 4 (Respons meter terhadap alarm internal / hardware):**
   * Meter mendeteksi kerusakan komponen internal dan menampilkan kode error yang berkedip.
2. **Pasal 7.5 & Tabel 6 (Respons meter terhadap ketidaknormalan dan penyalahgunaan / tampering):**
   * Meter menampilkan teks status khusus, kode respons standar, mengoperasikan relai pemutus jika disyaratkan, menyalakan LED kuning, dan mencatat rekaman ke log kejadian DLMS.
3. **Butir 6.11 (Format Layar Display Tampilan):**
   * Baris kedua menampilkan teks status sekurang-kurangnya 8 karakter dan kode respons resmi.

---

## 2. Matriks Kode Error Resmi SPLN D3.006

### A. Tabel 4: Respons Meter Terhadap Alarm Kerusakan Internal

| Kode Error | Jenis Kerusakan Komponen | Respons Meter | Teks Layar | Kode Layar | Indikator LED | Tindakan / Normalisasi |
|:---:|---|---|:---:|:---:|:---:|---|
| **`ERR00`** | Flash memory rusak/error | Rekam data, Relai buka | `-` | `ERR00` | Kuning Aktif | Penggantian meter |
| **`ERR01`** | RAM rusak/error | Rekam data, Relai buka | `-` | `ERR01` | Kuning Aktif | Penggantian meter |
| **`ERR02`** | Clock loss (kembali ke waktu awal chipset) | Rekam data, Relai buka | `-` | `ERR02` | Kuning Aktif | Sinkronisasi clock / Ganti meter |
| **`ERR03`** | Low battery (ambang batas < 2,7 V) | Rekam data, Simbol baterai kedip | `-` | `ERR03` | Kuning Aktif | Penggantian baterai |
| **`ERR04`** | Mikroprosesor tidak berfungsi normal | Rekam data, Relai buka | `-` | `ERR04` | Kuning Aktif | Penggantian meter |
| **`ERR05`** | Kegagalan sampling data pengukuran ADC | Rekam data, Relai buka | `-` | `ERR05` | Kuning Aktif | Penggantian meter |
| **`ERR06`** | Superkapasitor rusak/lepas | Rekam data | `-` | `ERR06` | Kuning Aktif | Penggantian meter |
| **`ERR07`** | Relai / shunt trip gagal membuka/menutup | Rekam data | `-` | `ERR07` | Kuning Aktif | Penggantian meter |

---

### B. Tabel 6: Respons Meter Terhadap Ketidaknormalan & Sabotase (Tampering)

| No | Jenis Sabotase / Ketidaknormalan | Respons Relai | Teks Layar (Tengah) | Kode Layar (Kanan) | LED Kuning | OBIS & DLMS Event | Normalisasi |
|:---:|---|:---:|:---:|:---:|:---:|:---:|---|
| **1** | **Pembukaan Tutup Meter (Case Open)** | **Membuka** | **`RUSAK`** | **`-`** | - | `0.0.96.20.1.255` (Code 0x02) | P2TL, Ganti meter |
| **2** | **Pembukaan Tutup Terminal** | **Membuka** | **`PERIKSA`** | **`ERR20`** | **Aktif** | `0.0.96.20.5.255` (Code 0x01) | P2TL, Remote reconnect |
| **3** | **Urutan Fase Terbalik (Wrong Sequence)** | **Membuka** | **`PERIKSA`** | **`ERR21`** | **Aktif** | Code 0x06 | P2TL, Remote reconnect |
| **4** | **Arus & Tegangan Tidak Sefase (Cross Phase)** | **Membuka** | **`PERIKSA`** | **`ERR22`** | **Aktif** | Code 0x07 | P2TL, Remote reconnect |
| **5** | **Kawat Netral Putus / Hilang (Neutral Missing)** | - | **`PERIKSA`** | **`ERR23`** | **Aktif** | `0.0.96.20.24.255` (Code 0x05) | P2TL, Perbaikan fisik |
| **6** | **Hilang Tegangan 1 atau 2 Fase** | **Membuka** | **`PERIKSA`** | **`ERR24`** | **Aktif** | Code 0x08 | P2TL, Perbaikan fisik |
| **7** | **Induksi Medan Magnet Eksternal ($\le 500\text{ mT}$)** | - *(Catatan 4)* | **`PERIKSA`** | **`ERR25`** | - | `0.0.96.20.26.255` (Code 0x03) | Perbaikan medan magnet |
| **8** | **Arus Terbalik (Reverse Power)** | - | **`REVERSE`** | **`ERR26`** | **Aktif** | `0.0.96.20.27.255` (Code 0x04) | P2TL, Perbaikan |
| **9** | **Pengukuran Kuadran 4 ($\text{PF} < 0,85$)** | - | **`PERIKSA`** | **`ERR27`** | **Aktif** | Code 0x09 | P2TL, Perbaikan |

> [!NOTE]
> * **Case Open (No 1):** Merupakan pelanggaran terberat. Layar menampilkan teks **`RUSAK`**, kode strip (`-`), dan relai trip permanen sehingga meter wajib diganti oleh tim P2TL PLN.
> * **Induksi Medan Magnet (No 7):** Sesuai Catatan 4 SPLN D3.006, meter harus tetap mengukur normal dan operasi relai tidak terpengaruh pada medan magnet sampai dengan $500\text{ mT}$.

---

## 3. Rincian Modifikasi File Firmware

1. **[`firmware/app/tamper/inc/tamper_manager.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/tamper/inc/tamper_manager.h):**
   * Memperluas enum `tamper_vector_t` menjadi 9 vektor lengkap (0..8) sesuai SPLN Tabel 6.
   * Menambahkan enum `spln_internal_alarm_t` untuk 8 jenis alarm kerusakan internal (Tabel 4: `ERR00`..`ERR07`).
2. **[`firmware/protocols/dlms/inc/dlms_tamper_log.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/protocols/dlms/inc/dlms_tamper_log.h):**
   * Memperluas enum `dlms_tamper_code_t` dari 0x00 hingga 0x09 agar mencakup seluruh kode kejadian DLMS Class 7.
3. **[`firmware/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/tamper/src/tamper_manager.c):**
   * Memperbarui logika pemutus relai alarm sesuai syarat SPLN Tabel 6: relai trip hanya pada Case Open, Terminal Open, Urutan Fase Terbalik, Cross Phase, dan Hilang Tegangan 1/2 Fase.
   * Melengkapi konversi dua arah `tamper_vector_to_dlms_code()` dan `dlms_code_to_tamper_vector()`.
4. **[`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/inc/display.h):**
   * Menambahkan field `active_tamper_mask` dan `active_alarm_mask` ke dalam `display_context_t`.
   * Mendeklarasikan fungsi `display_get_spln_tamper_info()`, `display_get_spln_alarm_info()`, `display_set_tamper_status()`, dan `display_set_internal_alarm()`.
5. **[`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display.c):**
   * Mengimplementasikan resolusi otomatis teks (`RUSAK`, `PERIKSA`, `REVERSE`, `-`) dan kode respons (`ERR00`..`ERR27`).
   * Memperbarui pemformatan halaman darurat `DISP_PAGE_TAMPER_ALARM` baik pada format grafis OLED SPLN Gambar 4 maupun tampilan terminal ASCII CLI.
6. **[`firmware/app/display/src/display_task.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display_task.c):**
   * Menyinkronkan `active_tamper_mask` saat terjadi interupsi tombol sabotase darurat PC13.
7. **[`firmware/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/CMakeLists.txt):**
   * Memasukkan `app/display/src/display_task.c` ke dalam pustaka `dlms_core`.
8. **[`tests/test_spln_display_tamper.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/tests/test_spln_display_tamper.c):**
   * Memperluas unit test host dengan verifikasi menyeluruh untuk ke-9 vektor sabotase (Tabel 6) dan ke-8 alarm internal (Tabel 4).

---

## 4. Hasil Pengujian & Verifikasi Kepatuhan

### A. Pengujian Layar SPLN & Seluruh Kode Error (`tests/test_spln_display_tamper.exe`)

```text
====================================================================
  TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & KODE ERROR SPLN D3.006   
====================================================================

[UJI 1] Verifikasi Tampilan Normal (alarm_icon_active = false)...
  > Page 01 (IDPEL): OK | zz: 01 | Val: 530000000001 | OBIS: 96.01 [PASS]
  > Page 02 (Volt R): OK | zz: 02 | Val: 230.0 V | OBIS: 32.07 [PASS]
  > Carousel Skip Test (Normal tidak menampilkan halaman sabotase): [PASS]

[UJI 2] Verifikasi Tampilan Alarm Sabotase Case Open (Tombol PC13)...
  > Banner Alarm Tersemat di Page 02: Respon Alarm = '!ALM' [PASS]
  > SPLN Tabel 6 No 1 (Case Open): Header: '!ALM' | zz: 'AL' | Teks: 'RUSAK' | Kode: '-' | OBIS: '96.50' [PASS]
  > Carousel Inclusion Test (Halaman sabotase masuk dalam siklus carousel): [PASS]
  > Carousel Loop Completion Test (Kembali ke Page 01): [PASS]

[UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)...
  > Status Layar Berhasil Pulih ke OK [PASS]

[UJI 4] Verifikasi Seluruh 9 Vektor Sabotase SPLN D3.006: 2021 Tabel 6...
  > 4.1 Case Open: Teks='RUSAK' | Kode='-' [PASS]
  > 4.2 Terminal Open: Teks='PERIKSA' | Kode='ERR20' [PASS]
  > 4.3 Wrong Sequence: Teks='PERIKSA' | Kode='ERR21' [PASS]
  > 4.4 Cross Phase: Teks='PERIKSA' | Kode='ERR22' [PASS]
  > 4.5 Neutral Missing: Teks='PERIKSA' | Kode='ERR23' [PASS]
  > 4.6 Voltage Loss: Teks='PERIKSA' | Kode='ERR24' [PASS]
  > 4.7 Magnetic Field: Teks='PERIKSA' | Kode='ERR25' [PASS]
  > 4.8 Reverse Power: Teks='REVERSE' | Kode='ERR26' [PASS]
  > 4.9 Quad 4 (Low PF): Teks='PERIKSA' | Kode='ERR27' [PASS]

[UJI 5] Verifikasi Seluruh 8 Alarm Hardware Internal SPLN D3.006: 2021 Tabel 4...
  > 5.0 Flash Error: Teks='-' | Kode='ERR00' [PASS]
  > 5.1 RAM Error: Teks='-' | Kode='ERR01' [PASS]
  > 5.2 Clock Loss RTC: Teks='-' | Kode='ERR02' [PASS]
  > 5.3 Low Battery: Teks='-' | Kode='ERR03' [PASS]
  > 5.4 MCU Error: Teks='-' | Kode='ERR04' [PASS]
  > 5.5 ADC Sampling: Teks='-' | Kode='ERR05' [PASS]
  > 5.6 Supercap Fail: Teks='-' | Kode='ERR06' [PASS]
  > 5.7 Relay Fail: Teks='-' | Kode='ERR07' [PASS]

[UJI 6] Verifikasi Format Tampilan ASCII CLI (display_render_frame)...
  > Case Open CLI Frame: L2='STATUS SABOTASE' | L3='RUSAK' [PASS]
  > Terminal Open CLI Frame: L3='PERIKSA ERR20' [PASS]
  > Reverse Power CLI Frame: L3='REVERSE ERR26' [PASS]

====================================================================
  HASIL: 100% SELURUH KODE ERROR SPLN D3.006 TABEL 4 & 6 LULUS!      
====================================================================
```

### B. Pengujian Integrasi Penuh E1 -> E3 -> E2 (`tests/test_e1_to_e2_integration.exe`)
* Status: **100% BERHASIL (PASS)** (Sinkronisasi metrologi ADE9000, pemetaan display E3, dan injeksi event DLMS).

### C. CTest Regression Suite (`ctest --test-dir firmware/build`)
* Status: **15/15 Test Suites Passed (100%)**.

### D. Kompilasi Target STM32U575 / STM32U585 (`cmake --build build/Debug`)
* Status: **0 Error, 0 Warning**.
* Output Firmware: `Uji_Coba_Integrasi.elf`, `Uji_Coba_Integrasi.hex`, `Uji_Coba_Integrasi.bin`.
* Alokasi Memori:
  * **RAM:** 81.288 B / 768 KB (10,34%)
  * **ROM:** 85.792 B / 2 MB (4,09%)

