# Rangkuman Implementasi: Hook Interupsi PVD (Last-Gasp) & Penyempurnaan Tamper Subsystem

**Tanggal:** 13 Oktober 2026  
**Peran:** E3 Lead (*Application, Display SPLN D3.006, Tamper Event Management & Safety*)  
**Target Hardware:** STM32U575VGT6 (ARM Cortex-M33 TrustZone)  
**Status Verifikasi:** 100% LULUS (19/19 CTest Suites, 0 Errors, 0 Warnings pada Kompilasi ARM GCC)

---

## 1. Ikhtisar Pekerjaan yang Dilakukan

Pada sesi kerja ini, fokus utama adalah merealisasikan arsitektur ketahanan sistem terhadap pemadaman listrik (*Power-Fail Resilience / Last-Gasp*) dan penyempurnaan menyeluruh pada subsistem pencatatan sabotase (*Tamper Event & Log*):

1. **Penyempurnaan Kamus OBIS Tamper (SPLN D3.006: 2021 Tabel 6):**
   * Melengkapi 4 pencacah kejadian sabotase yang sebelumnya belum terdaftar di kamus OBIS agar 100% dari 9 vektor sabotase SPLN memiliki register pencacah individu resmi:
     - Urutan Fase Terbalik (`0.0.96.20.6.255` - `ERR21`)
     - Pengawatan Silang / Cross Phase (`0.0.96.20.7.255` - `ERR22`)
     - Hilang Tegangan 1/2 Fase (`0.0.96.20.8.255` - `ERR24`)
     - Kuadran 4 / PF < 0.85 (`0.0.96.20.9.255` - `ERR27`)
   * Mengintegrasikan fungsi pencacah `increment`, `get`, dan `set` untuk seluruh 9 kode kejadian.

2. **Diagnostik & Monitoring Tamper Log Mandiri:**
   * Menambahkan API `rtos_tamper_dump_log()` untuk mencetak seluruh riwayat 30 rekaman FIFO DLMS Class 7 dalam bentuk tabel berformat rapi ke terminal serial.
   * Menambahkan API `rtos_tamper_record_event()` yang thread-safe untuk mempermudah pemicuan kejadian sabotase dari task mana pun.

3. **Implementasi Penuh Hook Interupsi PVD (*Last-Gasp Sequence*):**
   * Membangun driver BSP PVD (`bsp_pvd.h` & `bsp_pvd.c`) pada tingkat ambang batas **Level 6 (~2.9 V)** dengan mode `PWR_PVD_MODE_IT_RISING`.
   * Menghubungkan jalur interupsi ke `PVD_PVM_IRQn` dengan prioritas tertinggi (`NVIC Priority 0, 0`).
   * Mengatur pengalihan interupsi di TrustZone Secure World melalui `NVIC_SetTargetState(PVD_PVM_IRQn)` menuju Non-Secure World.
   * Mengimplementasikan ISR `PVD_PVM_IRQHandler()` pada `stm32u5xx_it.c`.
   * Memasang logika proteksi saat tegangan anjlok:
     1. Mematikan periferal boros daya (buzzer & LED alarm dimatikan seketika untuk menghemat muatan kapasitor catu daya).
     2. Melakukan *atomic flush* snapshot Tamper Log & Status FSM Relai Beban ke Flash NVRAM.
     3. Memanggil callback darurat pengguna.
     4. Mencegah eksekusi ganda jika tegangan berosilasi (*re-entry prevention*).

4. **Penambahan Fitur Uji Interaktif Konsol UART (ST-LINK VCP @ 115200 baud):**
   * Tombol `v` : Memicu simulasi pemadaman listrik darurat (PVD Last-Gasp Flash Save).
   * Tombol `V` : Memulihkan status catu daya kembali normal ($V_{DD} > 2.9\text{ V}$).
   * Tombol `l` : Menampilkan seluruh daftar 30 entri Tamper Log ke konsol.
   * Tombol `1`..`4` : Menstimulasi berbagai vektor sabotase SPLN (Terminal Cover, Magnet, Reverse, Netral).
   * Tombol `0` : Memulihkan status sabotase ke normal.

5. **Pengujian Otomatis & Pembersihan Sintaks:**
   * Memperbaiki kesalahan ketik aksidental (`k#include`) pada `test_dlms_tamper_log.c`.
   * Membuat test suite baru `test_bsp_pvd.c` untuk menguji perilaku PVD pada Host PC.
   * Memperbarui `CMakeLists.txt` pada folder `firmware/` dan `NonSecure/`.

---

## 2. Tabel Rincian Berkas yang Dibuat / Dimodifikasi

| No | Nama Berkas | Status | Rincian Modifikasi |
|:---:|---|:---:|---|
| **1** | [`firmware/drivers/bsp/inc/bsp_pvd.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/inc/bsp_pvd.h) | **Baru** | Definisi antarmuka modul PVD, fungsi inisialisasi, registrasi callback darurat, dan fungsi simulasi CLI/host. |
| **2** | [`firmware/drivers/bsp/src/bsp_pvd.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_pvd.c) | **Baru** | Implementasi driver PVD STM32 HAL Level 6 (~2.9V), NVIC priority 0, penanganan `HAL_PWR_PVDCallback()`, pematian buzzer, snapshot Flash, dan simulasi host CTest. |
| **3** | [`firmware/tests/drivers/test_bsp_pvd.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/drivers/test_bsp_pvd.c) | **Baru** | Unit test otomatis untuk memvalidasi state normal, trigger Last-Gasp, de-energisasi buzzer, eksekusi callback, proteksi anti-osilasi, dan restore daya. |
| **4** | [`firmware/protocols/dlms/src/dlms_obis.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/protocols/dlms/src/dlms_obis.c) | **Diubah** | Menambahkan 4 entri counter OBIS (`0.0.96.20.6/7/8/9.255`) dan memperluas fungsi `dlms_obis_increment/set/get_tamper_counter` untuk 9 vektor. |
| **5** | [`firmware/app/fsm/inc/rtos_tasks.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/inc/rtos_tasks.h) | **Diubah** | Menambahkan deklarasi `rtos_tamper_dump_log()` dan `rtos_tamper_record_event()`. |
| **6** | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/rtos_tasks.c) | **Diubah** | Mengimplementasikan pencetakan tabel 30 entri Tamper Log ke konsol dan pembungkus pengiriman event queue. |
| **7** | [`Secure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure/Core/Src/main.c) | **Diubah** | Menambahkan `NVIC_SetTargetState(PVD_PVM_IRQn);` untuk meneruskan interupsi PVD ke Non-Secure World. |
| **8** | [`NonSecure/Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/stm32u5xx_it.c) | **Diubah** | Menambahkan handler ISR `PVD_PVM_IRQHandler()` yang memanggil `HAL_PWR_PVD_IRQHandler()`. |
| **9** | [`NonSecure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c) | **Diubah** | Memanggil `bsp_pvd_init()`, menambahkan kontrol tombol UART `v`, `V`, `l`, `1`..`4`, `0`, dan memperbarui teks menu bantuan `h`. |
| **10**| [`firmware/tests/protocols/test_dlms_tamper_log.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/protocols/test_dlms_tamper_log.c) | **Diubah** | Memperbaiki karakter typo stray `k` pada baris pertama. |
| **11**| [`firmware/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/CMakeLists.txt) | **Diubah** | Menambahkan `bsp_pvd.c` ke pustaka `dlms_core` dan mendaftarkan test suite `test_bsp_pvd`. |
| **12**| [`NonSecure/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/CMakeLists.txt) | **Diubah** | Menambahkan `bsp_pvd.c` ke daftar source file build target STM32U575 Non-Secure. |
| **13**| [`rangkuman/ARSITEKTUR_HOOK_INTERUPSI_PVD_DAN_ADE9000_IRQ.md`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/rangkuman/ARSITEKTUR_HOOK_INTERUPSI_PVD_DAN_ADE9000_IRQ.md) | **Baru** | Dokumen arsitektur perbandingan teknis antara Hook PVD dan Hook ADE9000 IRQ. |

---

## 3. Hasil Pengujian & Metrik Verifikasi

### A. Host PC CTest (19/19 Suites Lulus 100%)
```text
Test project G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/build
      Start  1: test_dlms_association ............   Passed (0.04 s)
      Start  2: test_dlms_apdu ..................   Passed (0.03 s)
      Start  3: test_dlms_integration ............   Passed (0.03 s)
      Start  4: test_dlms_tamper_log .............   Passed (0.06 s)
      Start  5: test_dlms_hdlc ...................   Passed (0.03 s)
      Start  6: test_dlms_server .................   Passed (0.03 s)
      Start  7: test_dlms_task ...................   Passed (0.04 s)
      Start  8: test_dlms_nvm ....................   Passed (0.03 s)
      Start  9: test_bsp_lpuart ..................   Passed (0.03 s)
      Start 10: test_bsp_pvd .....................   Passed (0.03 s)  <-- NEW TEST
      Start 11: test_e3_app ......................   Passed (0.04 s)
      Start 12: test_fsm_statechart ..............   Passed (0.03 s)
      Start 13: test_load_profile ................   Passed (0.03 s)
      Start 14: test_load_profile_dlms_adapter ...   Passed (0.04 s)
      Start 15: test_nvram_storage ...............   Passed (0.04 s)
      Start 16: test_rtos_tasks ..................   Passed (0.03 s)
      Start 17: test_power_relay .................   Passed (0.03 s)
      Start 18: test_spln_display_tamper .........   Passed (0.06 s)
      Start 19: test_e1_to_e2_integration ........   Passed (0.06 s)

100% tests passed, 0 tests failed out of 19 (Waktu eksekusi: 0.76 s)
```

### B. Kompilasi Target Embedded ARM GCC (STM32U575)
```text
Memory region         Used Size  Region Size  %age Used
             RAM:       81176 B       512 KB     15.48%
             ROM:       91456 B         1 MB      8.72%
           SRAM4:           0 B        16 KB      0.00%
Linking C executable Uji_Coba_Integrasi_TZ_NS.elf
Generating physical flash HEX file: Uji_Coba_Integrasi_TZ_NS.hex
Status: 0 Errors, 0 Warnings
```

