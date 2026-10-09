# Rangkuman Komprehensif: Perubahan Berkas dan Kondisi Sebelum Perubahan (Before vs. After)

Dokumen ini menyajikan audit teknis lengkap mengenai **seluruh penambahan dan modifikasi berkas** pada proyek integrasi firmware Smart Meter 3-Fasa (**STM32U575VGT6**), mencakup subsistem **E1 (Metrologi ADE9000)**, **E2 (Protokol DLMS/COSEM)**, dan **E3 (Aplikasi & Display OLED 128x32)**.

Setiap berkas dilengkapi dengan **kondisi sebelum diubah (Before)**, **potongan kode yang diubah/ditambah (After)**, serta **alasan teknis perubahannya**.

---

## 1. Matriks Ringkasan Perubahan Berkas

| No | Lokasi Berkas | Kategori | Jenis Tindakan | Dampak Utama |
|---|---|---|---|---|
| 1 | [`CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/CMakeLists.txt) | Build System | Modifikasi | Mendaftarkan sumber kode dan *include paths* E1 ke build target. |
| 2 | [`E1/Mock/Mock.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/E1/Mock/Mock.h) | E1 Simulator | Modifikasi | Menambahkan prototipe fungsi `ADE9000_Mock_SetAIFRMS` yang hilang. |
| 3 | [`E1/SPI/SPI.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/E1/SPI/SPI.c) | E1 SPI Driver | Modifikasi | Meng-guard pemanggilan `printf` transaksi SPI agar UART konsol tidak terbanjiri. |
| 4 | [`firmware/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/tamper/src/tamper_manager.c) | E3 Tamper | Modifikasi | Memperbaiki kesalahan ketik (*typo*) `==` menjadi penugasan `=` pada mask sabotase. |
| 5 | [`firmware/protocols/dlms/inc/dlms_obis.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/protocols/dlms/inc/dlms_obis.h) | E2 DLMS Protocol | Modifikasi | Menambahkan prototipe fungsi sinkronisasi `dlms_obis_update_from_meter()`. |
| 6 | [`firmware/protocols/dlms/src/dlms_obis.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/protocols/dlms/src/dlms_obis.c) | E2 DLMS Protocol | Modifikasi | Menghapus `const` pada tabel OBIS dan mengimplementasikan pembaruan data dinamis. |
| 7 | [`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/inc/display.h) | E3 Display | Modifikasi | Menambahkan tipe data `display_spln_frame_t` untuk standar layar PLN Gambar 4. |
| 8 | [`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display.c) | E3 Display | Modifikasi | Mengimplementasikan pemetaan 14-halaman ke tata letak resmi SPLN Gambar 4. |
| 9 | [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c) | Core Application | Modifikasi | Mengintegrasikan inisialisasi E1, pembaruan E1 $\rightarrow$ E2, dan render OLED font besar `11x18`. |
| 10 | [`tests/test_e1_to_e2_integration.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/tests/test_e1_to_e2_integration.c) | Test Suite | **Berkas Baru** | Pengujian integrasi otomatis Host-Side C (SNRM, AARQ, GET-Req, DISC). |

---

## 2. Rincian Teknis Sebelum vs. Sesudah per Berkas

---

### A. `CMakeLists.txt`

#### 🔴 Kondisi Sebelum Diubah:
Berkas sumber dan folder *include* milik tim **E1** (`E1/Metrology`, `E1/Mock`, `E1/SPI`, `E1/RegisterMap`) belum terdaftar sama sekali di dalam CMake. Jika berkas `main.c` memanggil fungsi metrologi E1, linker akan memunculkan galat *undefined reference*.

#### 🟢 Kode Setelah Diubah / Ditambahkan:
```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ...
    # Metrology (E1) Sources
    E1/Metrology/Metrology.c
    E1/Mock/Mock.c
    E1/SPI/SPI.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ...
    E1/Metrology
    E1/Mock
    E1/RegisterMap
    E1/SPI
)
```
* **Alasan Teknis:** Memungkinkan seluruh modul komputasi metrologi ADE9000 dikompilasi secara terpadu ke dalam binary firmware target STM32.

---

### B. `E1/Mock/Mock.h`

#### 🔴 Kondisi Sebelum Diubah:
Implementasi fungsi `ADE9000_Mock_SetAIFRMS(uint32_t value)` sudah ada di dalam `Mock.c`, namun deklarasi prototipenya terlewat (*missing declaration*) di file header `Mock.h`.

#### 🟢 Kode Setelah Diubah / Ditambahkan:
```c
/* PHASE A SETTERS */
void ADE9000_Mock_SetAIRMS(uint32_t value);
void ADE9000_Mock_SetAVRMS(uint32_t value);
void ADE9000_Mock_SetAIFRMS(uint32_t value); /* <-- DITAMBAHKAN */
void ADE9000_Mock_SetAWATT(int32_t value);
```
* **Alasan Teknis:** Mencegah *compiler warning/error* (`implicit declaration of function`) saat modul aplikasi mengonfigurasi arus fundamental fasa A.

---

### C. `E1/SPI/SPI.c`

#### 🔴 Kondisi Sebelum Diubah:
Setiap transaksi pembacaan register SPI mengeksekusi 33 baris perintah `printf(...)` secara rinci (menampilkan SS, CMD, ADDR, DATA, CRC). Saat fungsi ini dipanggil periodik di FreeRTOS target MCU STM32, buffer UART langsung meluap (*flooding*), membebani siklus interupsi CPU, dan membuat task display mengalami lag.

#### 🟢 Kode Setelah Diubah / Ditambahkan:
```c
#include <stdio.h>
#include <stdint.h>

/* <-- DITAMBAHKAN: Guard printf untuk target FreeRTOS / Hardware MCU */
#if defined(USE_FREERTOS) || defined(EMBEDDED_HARDWARE_TARGET)
#define printf(...) ((void)0)
#endif

#include "SPI.h"
#include "../Mock/Mock.h"
```
* **Alasan Teknis:** Di lingkungan target MCU, pemanggilan `printf` transaksi SPI dialihkan menjadi operasi kosong (*no-op*), menjaga kinerja UART tetap optimal dan hening. Namun saat dijalankan pada pengujian host PC mandiri, log detail transaksi SPI tetap dapat diamati.

---

### D. `firmware/app/tamper/src/tamper_manager.c`

#### 🔴 Kondisi Sebelum Diubah:
Pada baris 31 terdapat kesalahan penulisan operator perbandingan ganda `==`:
```c
entry->active_mask == ctx->active_tamper_mask; // <-- TYPO: Statement with no effect
```
Akibatnya nilai mask sabotase aktif tidak pernah tersimpan ke dalam struct histori riwayat kejadian sabotase (*tamper FIFO log*).

#### 🟢 Kode Setelah Diubah / Ditambahkan:
```c
entry->active_mask = ctx->active_tamper_mask; // <-- DIPERBAIKI: Operator penugasan (=)
```
* **Alasan Teknis:** Menyimpan nilai status bitmask sabotase secara benar ke log dan menghilangkan peringatan compiler `[-Wunused-value]`.

---

### E. `firmware/protocols/dlms/inc/dlms_obis.h` & `src/dlms_obis.c`

#### 🔴 Kondisi Sebelum Diubah:
Pangkalan data OBIS `g_obis_db[]` dideklarasikan sebagai `static const` kaku (read-only di Flash ROM). Nilai besaran listrik terpatri statis pada angka pengujian awal:
* Tegangan = `2302` ($230.2\text{ V}$)
* Arus = `500` ($5.00\text{ A}$)
* Energi = `123456` ($1234.56\text{ kWh}$)

Tidak ada metode atau API untuk memperbarui angka-angka tersebut secara dinamis dari hasil pengukuran metrologi.

#### 🟢 Kode Setelah Diubah / Ditambahkan:
1. **Di `dlms_obis.h`:**
```c
#include "display.h"

/* Prototipe fungsi sinkronisasi data metrologi ke kamus register OBIS */
void dlms_obis_update_from_meter(const meter_measurements_t *meas);
```
2. **Di `dlms_obis.c`:**
```c
/* Sebelumnya: static const obis_entry_t g_obis_db[] = { ... }; */
/* Sesudahnya (dapat dimutasi di RAM): */
static obis_entry_t g_obis_db[] = { ... };

void dlms_obis_update_from_meter(const meter_measurements_t *meas) {
    if (!meas) return;

    for (size_t i = 0; i < g_obis_db_count; i++) {
        obis_entry_t *entry = &g_obis_db[i];
        
        if (entry->class_id == 3 && entry->attribute_id == 2) {
            /* 1. Tegangan L1, L2, L3 (skala 0.1 V) */
            if (entry->obis.c == 32 && entry->obis.d == 7) entry->value_u32 = meas->voltage_r_dvolts;
            else if (entry->obis.c == 52 && entry->obis.d == 7) entry->value_u32 = meas->voltage_s_dvolts;
            else if (entry->obis.c == 72 && entry->obis.d == 7) entry->value_u32 = meas->voltage_t_dvolts;
            
            /* 2. Arus L1, L2, L3, N (skala 0.01 A) */
            else if (entry->obis.c == 31 && entry->obis.d == 7) entry->value_u32 = meas->current_r_mamps / 10;
            else if (entry->obis.c == 51 && entry->obis.d == 7) entry->value_u32 = meas->current_s_mamps / 10;
            else if (entry->obis.c == 71 && entry->obis.d == 7) entry->value_u32 = meas->current_t_mamps / 10;
            else if (entry->obis.c == 91 && entry->obis.d == 7) entry->value_u32 = meas->current_n_mamps / 10;
            
            /* 3. Daya Aktif, Reaktif, Semu, PF, dan Frekuensi */
            else if (entry->obis.c == 1 && entry->obis.d == 7) entry->value_u32 = abs(meas->active_power_w);
            else if (entry->obis.c == 3 && entry->obis.d == 7) entry->value_u32 = abs(meas->reactive_power_var);
            else if (entry->obis.c == 9 && entry->obis.d == 7) entry->value_u32 = meas->apparent_power_va;
            else if (entry->obis.c == 13 && entry->obis.d == 7) entry->value_u32 = meas->power_factor_ppm;
            else if (entry->obis.c == 14 && entry->obis.d == 7) entry->value_u32 = meas->frequency_mhz / 10;
            
            /* 4. Total Energi Aktif Impor +A (skala 0.01 kWh) */
            else if (entry->obis.c == 1 && entry->obis.d == 8 && entry->obis.e == 0) {
                entry->value_u32 = (uint32_t)(meas->active_energy_wh / 10);
            }
        }
    }
}
```
* **Alasan Teknis:** Memastikan bahwa ketika klien AMR/DLMS meminta pembacaan register penagihan melalui port komunikasi, data yang dikembalikan **sinkron 100% secara real-time** dengan data metrologi E1 dan angka yang sedang berputar di layar OLED E3.

---

### F. `firmware/app/display/inc/display.h` & `src/display.c`

#### 🔴 Kondisi Sebelum Diubah:
1. Hanya menyediakan fungsi generic `display_render_frame()` yang membagi layar menjadi 3 baris teks kecil bertumpuk tanpa format resmi standar PLN.
2. Halaman Daya Reaktif, Daya Semu, dan Power Factor belum ditangani di switch-case sehingga menampilkan pesan default: `DISPLAY PAGE x / ---`.

#### 🟢 Kode Setelah Diubah / Ditambahkan:
1. **Di `display.h`:**
```c
/* Struktur Tampilan Standar Layar Meter PLN (SPLN Gambar 4) */
typedef struct {
    char header_symbols[24];  /* Baris 1: Simbol & Kode OBIS (Font 6x8) */
    char scroll_index_zz[6];  /* Baris 2 Kiri: zz urutan scrolling (Font 6x8) */
    char main_value[16];      /* Baris 2 Tengah: Nilai angka utama besar */
    char unit[10];            /* Baris 2 Kanan: Satuan besaran listrik */
    bool is_large_font;       /* true jika menggunakan Font 11x18 */
} display_spln_frame_t;

void display_render_spln_frame(const display_context_t *ctx, display_spln_frame_t *frame);
```

2. **Di `display.c`:**
   * Melengkapi switch-case `display_render_frame()` untuk Daya Reaktif (`kvar`), Semu (`kVA`), dan Faktor Daya (`PF`).
   * Mengimplementasikan `display_render_spln_frame()`:
     * **Baris 1:** Menghasilkan string simbol status gabungan: `OK[B] L123 I123 32.07` (atau `![B] L123 I123 32.07` saat terjadi alarm sabotase).
     * **Baris 2 Kiri:** Menghasilkan indeks urutan scroll `zz` (`01` s/d `14`).
     * **Baris 2 Tengah:** Menghasilkan nilai angka utama yang diformat presisi.
     * **Baris 2 Kanan:** Menghasilkan satuan resmi (`V`, `A`, `kW`, `kvar`, `kVA`, `Hz`, `kWh`).

---

### G. `Core/Src/main.c`

#### 🔴 Kondisi Sebelum Diubah:
1. Di dalam `task_oled128x32_carousel`, data pengukuran meter di-hardcode lokal statis:
   ```c
   meter_measurements_t meas = { .voltage_r_dvolts = 2205, .current_r_mamps = 5420, ... };
   ```
2. Modul E1 tidak pernah diinisialisasi atau dibaca di main program.
3. Sinkronisasi data ke DLMS OBIS belum ada.
4. Perenderan layar menggunakan 3 baris teks kecil biasa:
   ```c
   ssd1306_SetCursor(0, 1);  ssd1306_WriteString(l1, Font_6x8, White);
   ssd1306_SetCursor(0, 11); ssd1306_WriteString(l2, Font_6x8, White);
   ssd1306_SetCursor(0, 21); ssd1306_WriteString(l3, Font_7x10, White);
   ```

#### 🟢 Kode Setelah Diubah / Ditambahkan:
1. **Menambahkan fungsi adapter metrologi E1:**
```c
static void init_metrology_e1(void) {
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();
    /* Set Baseline Register Metrologi 3-Fasa ADE9000 */
    ADE9000_Mock_SetAIRMS(5962491);
    ADE9000_Mock_SetAVRMS(17136895);
    ADE9000_Mock_SetAWATT(636984);
    ...
}

static void update_measurements_from_e1(meter_measurements_t *out_meas) {
    /* 1. Baca register RMS ADE9000 via driver SPI */
    uint32_t avrms = ADE9000_SPI_ReadRegister(ADE9000_AVRMS);
    uint32_t airms = ADE9000_SPI_ReadRegister(ADE9000_AIRMS);
    ...
    /* 2. Konversi ke satuan teknik via library Metrology E1 */
    float va = Metrology_VoltageFromAVRMS(avrms);
    float ia = Metrology_CurrentFromAIRMS(airms);
    ...
    /* 3. Masukkan ke struct meter_measurements_t */
    out_meas->voltage_r_dvolts = (uint32_t)(va * 10.0f);
    out_meas->current_r_mamps  = (uint32_t)(ia * 1000.0f);
    ...
    /* 4. Sinkronkan langsung ke tabel register OBIS DLMS/COSEM (E2) */
    dlms_obis_update_from_meter(out_meas);
}
```

2. **Memperbarui task carousel OLED untuk format standar PLN Gambar 4:**
```c
static void task_oled128x32_carousel(void *pvParameters) {
    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 2000);
    init_metrology_e1();

    meter_measurements_t meas;
    display_spln_frame_t spln_frame;

    for (;;) {
        update_measurements_from_e1(&meas);
        display_update_measurements(&disp_ctx, &meas);

        display_process_tick(&disp_ctx, 2000);
        display_render_spln_frame(&disp_ctx, &spln_frame);

        ssd1306_Fill(Black);

        /* BARIS 1: Simbol & Kode OBIS (Font_6x8 di Y: 0..8) */
        ssd1306_SetCursor(1, 0);
        ssd1306_WriteString(spln_frame.header_symbols, Font_6x8, White);

        /* Garis pemisah horizontal putus-putus halus (Y = 10) */
        for (uint8_t x = 0; x < 128; x += 2) {
            ssd1306_DrawPixel(x, 10, White);
        }

        /* BARIS 2 KIRI: zz indeks scroll (Font_6x8 di Y: 22) */
        ssd1306_SetCursor(1, 22);
        ssd1306_WriteString(spln_frame.scroll_index_zz, Font_6x8, White);

        /* BARIS 2 TENGAH: Nilai Angka Besar (Font_11x18 di Y: 13..31) */
        if (spln_frame.is_large_font) {
            int val_len = (int)strlen(spln_frame.main_value);
            int x_val = 18 + (76 - (val_len * 11)) / 2;
            if (x_val < 16) x_val = 16;
            ssd1306_SetCursor((uint8_t)x_val, 13);
            ssd1306_WriteString(spln_frame.main_value, Font_11x18, White);
        } else {
            ssd1306_SetCursor(18, 17);
            ssd1306_WriteString(spln_frame.main_value, Font_7x10, White);
        }

        /* BARIS 2 KANAN: Satuan Besaran Listrik (Font_6x8 di Y: 22) */
        if (spln_frame.unit[0] != '\0') {
            int unit_len = (int)strlen(spln_frame.unit);
            int x_unit = 127 - (unit_len * 6);
            ssd1306_SetCursor((uint8_t)x_unit, 22);
            ssd1306_WriteString(spln_frame.unit, Font_6x8, White);
        }

        ssd1306_UpdateScreen();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

---

## 3. Berkas-Berkas Baru yang Ditambahkan

1. **[`tests/test_e1_to_e2_integration.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/tests/test_e1_to_e2_integration.c):**
   * Berkas pengujian host otomatis untuk memverifikasi transaksi HDLC SNRM $\rightarrow$ UA, AARQ $\rightarrow$ AARE, GET-Request OBIS Tegangan, Arus, Energi, dan DISC (**100% PASS**).
2. **[`tests/test_e1_to_e2.exe`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/tests/test_e1_to_e2.exe):**
   * Binary executable program penguji integrasi host yang dapat dijalankan langsung di terminal Windows.
3. **Dokumentasi di Folder `rangkuman/`:**
   * [`rangkuman/INTEGRASI_METROLOGI_E1_KE_DISPLAY_E3.md`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/rangkuman/INTEGRASI_METROLOGI_E1_KE_DISPLAY_E3.md)
   * [`rangkuman/RANGKUMAN_INTEGRASI_E1_E2_E3_DLMS.md`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/rangkuman/RANGKUMAN_INTEGRASI_E1_E2_E3_DLMS.md)
   * [`rangkuman/TAMPILAN_STANDAR_PLN_GAMBAR_4.md`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/rangkuman/TAMPILAN_STANDAR_PLN_GAMBAR_4.md)
   * [`rangkuman/RANGKUMAN_PERUBAHAN_DAN_KONDISI_SEBELUMNYA.md`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/rangkuman/RANGKUMAN_PERUBAHAN_DAN_KONDISI_SEBELUMNYA.md) *(berkas ini)*

---

## 4. Hasil Kompilasi & Pengujian Akhir

### A. Pengujian Host Terminal (`test_e1_to_e2.exe`):
```text
[4A] HDLC SNRM  -> Server Respons: Frame HDLC UA                 [PASS]
[4B] DLMS AARQ  -> Server Respons: Frame DLMS AARE (ASSOCIATED)  [PASS]
[4C] GET-Req 1.0.32.7 (V_A)   -> Respons: 2300 (230.0 V)         [PASS]
[4D] GET-Req 1.0.31.7 (I_A)   -> Respons: 1194 (11.94 A)         [PASS]
[4E] GET-Req 1.0.1.8  (kWh)   -> Respons: 12543 (125.43 kWh)     [PASS]
[4F] HDLC DISC  -> Server Respons: State UNASSOCIATED            [PASS]
STATUS PENGUJIAN: 100% SUKSES TANPA ERROR.
```

### B. Kompilasi Target Firmware MCU STM32U575 (`Uji_Coba_Integrasi.elf`):
```text
Memory region         Used Size  Region Size  %age Used
             RAM:       80352 B       768 KB     10.22%
             ROM:       69400 B         2 MB      3.31%
           SRAM4:           0 B        16 KB      0.00%
Build Result: 0 Error, 0 Warning (100% Clean)
```
