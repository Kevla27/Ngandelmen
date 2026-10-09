# Laporan Teknis: Pembersihan `main.c`, Modularisasi Arsitektur, dan Bukti Universalitas Sistem

**Peran:** System Integrator & Lead Engineer (E3 Lead)  
**Target Hardware:** STM32U575VGT6 (Arm Cortex-M33)  
**Status Verifikasi:** 100% LULUS (Build Target ARM GCC 0 Error, 15/15 CTest Suites Passed, End-to-End Test Passed)  
**Tanggal:** 8 Oktober 2026  

---

## 1. Latar Belakang & Tujuan Refaktorisasi

Sebelumnya, file `Core/Src/main.c` menampung lebih dari 569 baris kode yang mencampurkan:
1. Kode inisialisasi CubeMX HAL.
2. Register mock dan konversi unit ADE9000 (bagian dari E1 Metrologi).
3. Kode render layar OLED 128x32 carousel SPLN Gambar 4 (~130 baris).
4. Implementasi hook memori FreeRTOS statis (`vApplicationGetIdleTaskMemory`, dll).
5. Logika pembuatan seluruh task FreeRTOS.
6. Penanganan debounce dan pembuatan pesan event sabotase di dalam handler interupsi EXTI.

Kondisi tersebut menimbulkan risiko tinggi:
- **Resiko Overwrite CubeMX:** Setiap kali konfigurasi pin/clock di STM32CubeMX di-*generate* ulang, blok kode non-CubeMX rentan terhapus atau tertimpa.
- **Keterikatan Keras (Tight Coupling):** Logika sistem terkunci pada USART2, tombol PC13, dan pin STM32U575 tertentu, melanggar prinsip *Hardware Abstraction Layer* (HAL) dan *Clean Architecture*.

---

## 2. Struktur Modul Baru & Pembagian Tanggung Jawab

Seluruh logika non-inisialisasi diekstraksi ke modul terpisah dengan batas kepemilikan yang tegas:

```
Uji_Coba_Integrasi/
├── Core/Src/main.c                      <-- Murni boilerplate CubeMX & wiring callback (~278 baris)
├── E1/
│   └── Adapter/
│       ├── metrology_adapter.h          <-- Kontrak sampling metrologi E1
│       └── metrology_adapter.c          <-- Integrasi register ADE9000 -> meter_measurements_t
└── firmware/
    ├── app/
    │   ├── console/
    │   │   ├── inc/console_io.h         <-- Universal I/O retargeting printf
    │   │   └── src/console_io.c         <-- Implementasi _write() agnostik hardware
    │   ├── display/
    │   │   ├── inc/display_task.h       <-- Header task display carousel OLED
    │   │   └── src/display_task.c       <-- Task FreeRTOS render layout SPLN Gambar 4
    │   ├── fsm/
    │   │   ├── inc/rtos_tasks.h         <-- Deklarasi IPC & rtos_start_all_tasks()
    │   │   └── src/rtos_tasks.c         <-- Task spawner & sinkronisasi data antar-task
    │   └── tamper/
    │       ├── inc/tamper_manager.h     <-- Kontrak tamper & handler interupsi
    │       └── src/tamper_manager.c     <-- Debounce software & pengiriman queue dari ISR
    └── sys/
        └── src/freertos_hooks.c         <-- FreeRTOS memory & tick hooks
```

---

## 3. Analisis & Bukti Sifat Universalitas Sistem

Sifat universalitas sistem **tetap dipertahankan 100% dan bahkan jauh lebih kuat** dibandingkan sebelumnya berkat pemisahan antarmuka (Dependency Injection & Callback Pattern):

### A. Universalitas Konsol I/O (`printf`)
- **Implementasi:** `console_io_init(console_uart_transmit_bsp);`
- **Mengapa Universal:** Fungsi libc `_write()` di [`console_io.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/console/src/console_io.c) tidak memanggil `HAL_UART_Transmit` ataupun register STM32. Modul ini hanya memanggil *function pointer* `s_console_tx_cb(data, len)`.
- **Fleksibilitas Porting:**
  - Jika port dipindah dari USART2 ke USART1 / UART3: Cukup ubah handle UART di callback `main.c`.
  - Jika memakai RS485: Pasang pin toggle DIR/DE sebelum dan sesudah transmit di callback `main.c`. Modul konsol tidak perlu disentuh sama sekali.
  - Jika porting ke ESP32, NXP, AVR, atau PC Simulator: Cukup arahkan callback ke driver UART target atau `fwrite()`.

### B. Universalitas Metrologi (Adapter E1 $\rightarrow$ E3)
- **Implementasi:** [`metrology_adapter_sample()`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/E1/Adapter/metrology_adapter.c)
- **Mengapa Universal:** Seluruh task FreeRTOS, display OLED, load profile, dan DLMS berkomunikasi menggunakan struktur data murni standar ANSI C (`meter_measurements_t`).
- **Fleksibilitas Porting:**
  - Jika IC metrologi diganti dari ADE9000 ke ADE7880, CS5463, BL0942, atau shunt ADC internal STM32, yang diubah **hanya** isi file `metrology_adapter.c`.
  - Task profiling, queue, DLMS server, kamus OBIS, dan OLED tidak berubah 1 baris pun.

### C. Universalitas Komunikasi DLMS/COSEM
- **Implementasi:** `rtos_dlms_task_set_tx_cb(dlms_uart_tx_callback);`
- **Mengapa Universal:** Stack DLMS (HDLC, APDU, Association, Server) bersifat murni protocol engine independen.
- **Fleksibilitas Porting:** Transmisi respon DLMS dapat diarahkan ke RS485, optical probe IEC 62056-21, modem GSM/GPRS, atau TCP/IP socket hanya dengan mengubah callback transmisi.

### D. Universalitas Penanganan Sabotase (Tamper Manager)
- **Implementasi:** `tamper_handle_button_press_isr(HAL_GetTick());`
- **Mengapa Universal:** Fungsi ini menerima nilai *timestamp* generik dalam milidetik.
- **Fleksibilitas Porting:** Sumber pemicu dapat berasal dari pin EXTI manapun (misal PC13, PA0), interupsi komparator tegangan rendah, switch magnetik (reed switch), atau pesan interupsi eksternal tanpa mengubah logika penanganan alarm dan pengiriman queue FreeRTOS.

### E. Abstraksi Display Hardware Abstraction Layer (Display HAL)
- **Sebelumnya:** `display_task.c` memanggil langsung fungsi `ssd1306_*`. Jika layar diganti ke LCD 16x2 atau TFT, developer terpaksa mengedit `display_task.c`.
- **Sesudah Refaktorisasi (Display HAL):**
  - Dibuatkan antarmuka agnostik `display_driver_interface_t`:
    ```c
    typedef struct {
        void (*init)(void);
        void (*render_frame)(const display_spln_frame_t *frame);
        void (*show_boot_screen)(const char *title, const char *subtitle);
    } display_driver_interface_t;
    ```
  - `display_task.c` **100% bebas dari driver SSD1306 dan font matriks**.
  - Driver konkret OLED 128x32 dipindahkan ke BSP: [`bsp_display_oled.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/inc/bsp_display_oled.h) & [`bsp_display_oled.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/src/bsp_display_oled.c).
  - Di `main.c`, driver diinjeksikan secara modular (*Dependency Injection*):
    ```c
    display_task_set_driver(bsp_display_oled_get_driver());
    ```
  - **Dampak Universalitas:** Jika besok Anda mengganti layar ke **LCD Karakter 16x2 / 20x4, Glass Segment LCD, TFT ST7735, atau Headless (tanpa layar)**, file `display_task.c` dan `display.c` **TIDAK AKAN BERUBAH SATU HURUF PUN**. Anda cukup membuat file BSP driver baru dan menginjeksikannya saat startup.

---

## 4. Perbandingan Sebelum dan Sesudah Pembersihan

| Parameter Evaluasi | Sebelum Pembersihan | Sesudah Pembersihan Tahap 2 (Display HAL) |
| :--- | :--- | :--- |
| **Total Baris `main.c`** | 569 baris | **265 baris** (-53%) |
| **Logika Bisnis di `main.c`** | Campur aduk (Metrologi, OLED, RTOS Hooks, Tamper) | **Nol (0)** — Murni konfigurasi hardware & startup spawner |
| **Ketergantungan Layar di `display_task.c`** | Terkunci keras pada SSD1306 OLED | **Agnostik 100%** via `display_driver_interface_t` |
| **Keamanan Regenerasi CubeMX** | Rentan terhapus jika di-generate ulang | **Sangat Aman** (semua logika di dalam blok `USER CODE` ringkas) |
| **Handler EXTI Tombol** | 24 baris (debounce, toggle, struct create, queue send) | **6 baris** (delegasi langsung ke `tamper_handle_button_press_isr`) |
| **Pembuatan Task RTOS** | 25 baris manual `xTaskCreate` berulang | **3 baris** (`rtos_start_all_tasks()`) |
| **Ketergantungan UART `printf`** | Terkunci pada `huart2` di `syscalls.c` / `main.c` | **Agnostik** via callback `console_io_init` |

---

## 5. Ringkasan Verifikasi & Validasi Pengujian

1. **Kompilasi Target STM32 ARM Cortex-M33:**
   ```
   [2/2] Linking C executable Uji_Coba_Integrasi.elf
   RAM:   81280 B / 768 KB (10.34%)
   ROM:   83816 B / 2 MB   (4.00%)
   Hasil: 0 Error, 0 Warning.
   ```

2. **Pengujian Suite CTest Host (15 Suite Unit Test):**
   ```
   100% tests passed, 0 tests failed out of 15
   Total Test time (real) = 0.44 sec
   ```

3. **Uji Integrasi End-to-End (E1 Metrologi $\rightarrow$ E3 Measure $\rightarrow$ E2 DLMS/OBIS):**
   - Inisialisasi SPI Mock ADE9000: **LULUS**
   - Konversi unit engineering: **LULUS**
   - Transaksi HDLC SNRM, AARQ, GET-Request OBIS Tegangan, Arus, Energi: **LULUS (230.0V, 11.94A, 125.43kWh)**
   - Pembacaan Tamper Event Log Profile Generic (0.0.99.98.0.255): **LULUS**

4. **Uji Layar OLED Standar SPLN Gambar 4 & Alarm Sabotase:**
   - Halaman normal (OK, [B], L123, I123, zz, Nilai, Satuan, OBIS): **LULUS**
   - Pemicu alarm seketika (Solid banner `!ALM`, Halaman darurat `SABOTASE` `E01` `96.50`): **LULUS**
   - Pemulihan status alarm: **LULUS**

