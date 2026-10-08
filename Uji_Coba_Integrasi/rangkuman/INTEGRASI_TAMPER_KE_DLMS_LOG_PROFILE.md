# Laporan Teknis Integrasi: Tamper Event ke DLMS Log Profile Generic (0.0.99.98.0.255)

**Peran:** System Integrator & E3 Engineer (*Application, Display SPLN D3.022-1, Tamper Event Management & RTOS Orchestration*)  
**Target Hardware:** STM32U575VGT6 (Arm Cortex-M33) + ADE9000 Virtual Mock SPI (Blind Firmware Phase 2)  
**Standar Kepatuhan:** IEC 62056-6-2 (Class 7 Profile Generic), SPLN D3.006:2021, SPLN D3.022-1  
**Status Verifikasi:** 100% LULUS (15/15 CTest Suites, 10/10 Tamper Log Tests, End-to-End HDLC Pass, Clean ARM Build)

---

## 1. Arsitektur & Diagram Aliran Integrasi (Tamper Pipeline)

Ketika tombol pengguna PC13 ditekan atau pemicu sabotase metrologi (misal induksi magnet atau arus balik) terdeteksi, sistem mengeksekusi pipeline integrasi terpadu berikut:

```
[Hardware Push Button PC13 / EXTI13 / Sensor ADE9000]
                         │
                         ▼ (Interupsi Hardware ISR)
[Core/Src/main.c : HAL_GPIO_EXTI_Falling_Callback]
  ├─► Debounce software (250 ms)
  ├─► Toggle status global g_tamper_alarm_active
  ├─► Trigger refresh instan Display OLED (Banner Solid !ALM | SABOTASE | E01)
  └─► rtos_queue_send_tamper_event_from_isr(&msg)
                         │
                         ▼ (FreeRTOS Queue: xTamperQueue)
[DLMSTask : task_dlms_entry & rtos_dlms_process_tamper_event]
  │
  ├─► 1. dlms_tamper_log_add_event(&server.tamper_log, timestamp, code, status)
  │      └─► Ditulis ke Circular FIFO Ring Buffer Class 7 Profile Generic (Kapasitas 30 entri SPLN)
  │
  ├─► 2. dlms_obis_increment_tamper_counter(code)
  │      ├─► Total Tamper Counter (0.0.96.20.0.255) bertambah (+1)
  │      └─► Specific Event Counter (0.0.96.20.1/5/24/26/27.255) bertambah (+1)
  │
  ├─► 3. tamper_process_signal(&g_tamper_ctx, vector, is_active, timestamp)
  │      └─► Sinkronisasi status aktif aplikasi E3 (alarm_relay_trigger, alarm_led_status)
  │
  └─► 4. nvram_save_tamper_log_snapshot(&g_tamper_ctx)
         └─► Snapshot log & mask disimpan ke Flash Internal STM32U575 Bank 2 (0x08104000)
```

---

## 2. Tabel Pemetaan Kode Sabotase & OBIS SPLN D3.006:2021

| Vektor Tamper Aplikasi (E3) | DLMS Tamper Code (E2) | OBIS Code (ID Data) | Class ID | Deskripsi Kejadian Sabotase |
|---|---|---|---|---|
| `TAMPER_VECTOR_CASE_OPEN` (0x01) | `DLMS_TAMPER_METER_COVER_OPEN` (0x02) | `0.0.96.20.1.255` | 1 (Data) | Tutup Meter Utama Dibuka (Tombol PC13) |
| `TAMPER_VECTOR_TERMINAL_OPEN` (0x02) | `DLMS_TAMPER_TERMINAL_COVER_OPEN` (0x01) | `0.0.96.20.5.255` | 1 (Data) | Tutup Terminal Dibuka |
| `TAMPER_VECTOR_MAGNETIC_FIELD` (0x04) | `DLMS_TAMPER_MAGNETIC_INDUCTION` (0x03) | `0.0.96.20.26.255` | 1 (Data) | Induksi Medan Magnet Eksternal |
| `TAMPER_VECTOR_REVERSE_POWER` (0x10) | `DLMS_TAMPER_REVERSE_CURRENT` (0x04) | `0.0.96.20.27.255` | 1 (Data) | Aliran Arus Terbalik / Pembalikan Daya |
| `TAMPER_VECTOR_NEUTRAL_BYPASS` (0x08) | `DLMS_TAMPER_MISSING_NEUTRAL` (0x05) | `0.0.96.20.24.255` | 1 (Data) | Hilang Kawat Netral / Neutral Missing |
| **Semua Vektor (Akumulator)** | `DLMS_TAMPER_NONE` (0x00) | `0.0.96.20.0.255` | 1 (Data) | Akumulasi Total Seluruh Kejadian Sabotase |
| **Log Rekaman Sirkular** | **Buffer Multi-Entri** | `0.0.99.98.0.255` | 7 (Profile) | Tamper Event Log Profile Generic (FIFO 30) |

---

## 3. Rincian Modifikasi Teknis per Berkas

### A. Lapisan Protokol DLMS/COSEM (E2)

#### 1. `firmware/protocols/dlms/src/dlms_apdu.c`
* **Permasalahan Sebelumnya:** Fungsi `dlms_encode_get_response` hanya menangani data tipe `DOUBLE_LONG_UNSIGNED` (32-bit) dan `LONG_UNSIGNED` (16-bit). Ketika klien DLMS meminta buffer string biner Class 7 (`OCTET_STRING`), fungsi tidak mengemas payload data sehingga respon terpotong di header 5 byte.
* **Solusi Perbaikan:** Ditambahkan penanganan lengkap untuk tipe data `DLMS_DATA_TYPE_OCTET_STRING` (0x09). Fungsi kini mengenali buffer A-XDR terformat (dimulai dengan 0x09) dan menyalin payload utuh secara aman dengan pengecekan batas memori (`max_len`).

#### 2. `firmware/protocols/dlms/src/dlms_server.c`
* **Penanganan Atribut Class 7 Profile Generic:**
  * **Atribut 2 (`buffer`):** Memanggil `dlms_tamper_log_encode_axdr()` dan mengemas 30 entri FIFO ke dalam format A-XDR Octet String.
  * **Atribut 7 (`entries_in_use`):** Mengembalikan jumlah rekaman aktif saat ini (0..30) sebagai `DOUBLE_LONG_UNSIGNED`.
  * **Atribut 8 (`profile_entries`):** Mengembalikan batas maksimal kapasitas buffer (30 entri sesuai SPLN D3.006) sebagai `DOUBLE_LONG_UNSIGNED`.

#### 3. `firmware/protocols/dlms/inc/dlms_obis.h` & `firmware/protocols/dlms/src/dlms_obis.c`
* Mendaftarkan OBIS `0.0.96.20.1.255` (*Meter Cover Open*) ke kamus database `g_obis_db`.
* Mengimplementasikan fungsi sinkronisasi *thread-safe* (dilindungi `rtos_meter_data_lock`):
  * `dlms_obis_increment_tamper_counter(dlms_tamper_code_t code)`: Otomatis menaikkan total counter (`0.0.96.20.0.255`) dan counter spesifik kejadian.
  * `dlms_obis_set_tamper_counter(dlms_tamper_code_t code, uint32_t count)`: Menyetel nilai awal/reset.
  * `dlms_obis_get_tamper_counter(dlms_tamper_code_t code)`: Membaca nilai aktual counter.

---

### B. Lapisan Aplikasi, Tamper Manager & RTOS (E3 & Integrasi)

#### 4. `firmware/app/tamper/inc/tamper_manager.h` & `firmware/app/tamper/src/tamper_manager.c`
* Menambahkan fungsi penerjemah dua arah antara level bitmask hardware/aplikasi E3 dan kode standar DLMS:
  * `dlms_tamper_code_t tamper_vector_to_dlms_code(tamper_vector_t vector)`
  * `tamper_vector_t dlms_code_to_tamper_vector(dlms_tamper_code_t code)`

#### 5. `firmware/app/fsm/inc/rtos_tasks.h` & `firmware/app/fsm/src/rtos_tasks.c`
* Mengimplementasikan fungsi integrasi terpadu:
  ```c
  void rtos_dlms_process_tamper_event(const tamper_event_msg_t *msg);
  ```
  Fungsi ini dieksekusi oleh `task_dlms_entry` ketika menerima pesan dari `xTamperQueue`. Fungsi ini secara atomik:
  1. Menulis rekaman baru ke `server.tamper_log` (Timestamp, Event Code, Status).
  2. Menaikkan nilai akumulator OBIS Tamper Counter.
  3. Memperbarui status logika tamper E3 (`g_tamper_ctx`).
  4. Menyimpan snapshot terbaru ke Flash NVRAM internal (`nvram_save_tamper_log_snapshot`).
* Menginisialisasi `s_dlms_task_ctx` di awal boot sistem pada `rtos_system_init()`.

#### 6. `Core/Src/main.c`
* Callback interupsi hardware tombol PC13 (`HAL_GPIO_EXTI_Falling_Callback`) kini memancarkan kode standar resmi:
  ```c
  tamper_event_msg_t msg = {
      .timestamp = now / 1000,
      .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN, /* 0x02 */
      .is_active = g_tamper_alarm_active
  };
  rtos_queue_send_tamper_event_from_isr(&msg);
  ```

---

## 4. Hasil Verifikasi & Pengujian

### A. Rangkaian Pengujian Host (`CTest` Suite)
Seluruh 15 unit test suite pada lingkungan host (MinGW/GCC) lulus 100%:
* `test_dlms_association` : **PASSED**
* `test_dlms_apdu` : **PASSED**
* `test_dlms_integration` : **PASSED**
* `test_dlms_tamper_log` : **PASSED** (10 dari 10 pengujian mendalam berhasil)
  * Test 1: Inisialisasi Tamper Log Buffer
  * Test 2: Penambahan Event (Terminal Open, Magnet, Reverse, Neutral)
  * Test 3: Verifikasi Urutan FIFO (Indeks 0 = Kejadian Terbaru)
  * Test 4: Pengujian Batas Sirkular / Overflow (35 Event > 30 Maksimal)
  * Test 5: Serialisasi / Pengkodean A-XDR Buffer Class 7
  * Test 6: Integrasi OBIS Lookup Class 7
  * Test 7: Transaksi Server HDLC Membaca Buffer Profile Generic `0.0.99.98.0.255:2`
  * Test 8: GET-Request Atribut 7 (`entries_in_use`) & Atribut 8 (`profile_entries`)
  * Test 9: Sinkronisasi Akumulasi Counter OBIS (`0.0.96.20.x.255`)
  * Test 10: Konversi Dua Arah Vector Tamper E3 $\leftrightarrow$ DLMS Code
* `test_dlms_hdlc` : **PASSED**
* `test_dlms_server` : **PASSED**
* `test_dlms_task` : **PASSED**
* `test_dlms_nvm` : **PASSED**
* `test_bsp_lpuart` : **PASSED**
* `test_e3_app` : **PASSED**
* `test_fsm_statechart` : **PASSED**
* `test_load_profile` : **PASSED**
* `test_load_profile_dlms_adapter` : **PASSED**
* `test_nvram_storage` : **PASSED**
* `test_rtos_tasks` : **PASSED** (Memverifikasi Producer $\rightarrow$ FreeRTOS Queue $\rightarrow$ Consumer $\rightarrow$ DLMS Log $\rightarrow$ OBIS DB $\rightarrow$ Flash NVRAM)

### B. Pengujian Integrasi End-to-End (`test_e1_to_e2_integration.exe`)
* Inisialisasi Virtual ADE9000 SPI Mock (Tegangan 230.02 V, Arus 11.946 A, Daya 6898 W, Frekuensi 50.00 Hz).
* Pemetaan ke struktur E3 `meter_measurements_t`.
* Sinkronisasi ke kamus OBIS E2.
* Transaksi HDLC Client $\rightarrow$ Server:
  * SNRM $\rightarrow$ UA [PASS]
  * AARQ $\rightarrow$ AARE (State: `ASSOCIATED_READONLY`) [PASS]
  * GET Tegangan Fasa A (`1.0.32.7.0.255`) = 230.0 V [PASS]
  * GET Arus Fasa A (`1.0.31.7.0.255`) = 11.94 A [PASS]
  * GET Energi Aktif (`1.0.1.8.0.255`) = 125.43 kWh [PASS]
  * **Injeksi Event Sabotase & GET Tamper Log (`0.0.99.98.0.255:2`) = Berhasil dibaca via HDLC I-Frame** (Time: 1700008888, Code: 0x02, Status: 1) [PASS]
  * DISC $\rightarrow$ UA (State: `UNASSOCIATED`) [PASS]

### C. Kompilasi Target Firmware ARM (`Uji_Coba_Integrasi.elf`)
* **Toolchain:** ARM GNU Toolchain (`arm-none-eabi-gcc 14.3.1`)
* **Hasil:** **0 Errors, 0 Warnings**
* **Penggunaan Memori STM32U575VGT6:**
  * **ROM (Flash):** 84,120 Byte / 2,048 KB (**4.01%**)
  * **RAM (SRAM):** 81,264 Byte / 768 KB (**10.33%**)
