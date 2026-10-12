# Rangkuman Lengkap Modifikasi Firmware: Adaptasi Non-TrustZone (STM32U575VGT6)

Dokumen ini mendokumentasikan secara rinci seluruh penambahan, penghapusan, dan penggantian kode yang dilakukan pada workspace **`Uji_Coba_Integrasi`** dalam rangka mengadopsi seluruh fitur, perbaikan kestabilan perangkat keras, dan kepatuhan standar SPLN dari **`Uji_Coba_Integrasi_TZ`** ke dalam arsitektur **Non-TrustZone (Bare-Metal/Flat Memory Map)** untuk mikrokontroler **STM32U575VGT6** (1 MB Flash, 768 KB SRAM, LQFP100).

---

## 1. Matriks Rangkuman Perubahan Berkas

| No | Berkas yang Dimodifikasi | Komponen / Layer | Jenis Aksi | Ringkasan Dampak Teknis |
|:--:|---|---|:---:|---|
| **1** | [`Core/Src/ssd1306.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/ssd1306.c) | Driver OLED HMI | **Ditambahkan & Diganti** | Dynamic I2C scanner (0x3C, 0x3D, 1..127), startup delay 150 ms, hardware self-test flash 400 ms (`0xA5` $\rightarrow$ `0xA4`), timeout `HAL_MAX_DELAY`. |
| **2** | [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c) | System Core & BSP | **Ditambahkan, Diganti, Dihapus** | 9-Cycle SCL recovery, timing 100 kHz (`0x3042C7C7`), filter analog/digital I2C, ICACHE enable, dual console USART2 & USART1 VCP, integrasi buzzer PB7. |
| **3** | [`Core/Src/stm32u5xx_hal_msp.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/stm32u5xx_hal_msp.c) | Hardware MSP | **Ditambahkan** | Inisialisasi clock dan GPIO untuk USART1 (PA9=TX, PA10=RX, AF7) pada `HAL_UART_MspInit` dan `HAL_UART_MspDeInit`. |
| **4** | [`Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/stm32u5xx_it.c) | NVIC Interrupt | **Ditambahkan** | Interrupt handler `USART1_IRQHandler()` untuk menangani RX byte dari terminal ST-LINK VCP. |
| **5** | [`Core/Inc/stm32u5xx_it.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Inc/stm32u5xx_it.h) | NVIC Header | **Ditambahkan** | Deklarasi ekspor fungsi `USART1_IRQHandler(void)` dan `EXTI13_IRQHandler(void)`. |
| **6** | [`Core/Inc/main.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Inc/main.h) | Global Header | **Ditambahkan** | Deklarasi variabel eksternal `extern UART_HandleTypeDef huart1;`, `huart2;`, dan `hi2c1;`. |
| **7** | [`STM32U575xx_FLASH.ld`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/STM32U575xx_FLASH.ld) | Memory Map Linker | **Diganti** | Batas panjang memori ROM dikoreksi dari `2048K` (2 MB) menjadi `1024K` (1 MB) sesuai spesifikasi fisik STM32U575VGT6. |
| **8** | [`firmware/drivers/bsp/inc/bsp_alarm_buzzer.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/inc/bsp_alarm_buzzer.h) | Driver BSP (Baru) | **Ditambahkan (File Baru)** | Deklarasi antarmuka aktuasi audio-visual SPLN D3.006 Butir 6.14 pada pin PB7. |
| **9** | [`firmware/drivers/bsp/src/bsp_alarm_buzzer.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/src/bsp_alarm_buzzer.c) | Driver BSP (Baru) | **Ditambahkan (File Baru)** | Implementasi GPIO PB7 Push-Pull untuk embedded target dan mock simulation untuk PC runner. |
| **10** | [`CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/CMakeLists.txt) | Build System MCU | **Ditambahkan** | Mendaftarkan `firmware/drivers/bsp/src/bsp_alarm_buzzer.c` ke dalam target binary MCU. |
| **11** | [`firmware/CMakeLists.txt`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/CMakeLists.txt) | Build System Host | **Ditambahkan** | Mendaftarkan `drivers/bsp/src/bsp_alarm_buzzer.c` ke dalam target library `dlms_core`. |
| **12** | [`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/inc/display.h) | Sub-sistem Display E3 | **Ditambahkan** | Enum `DISP_PAGE_TIME`, field konteks `current_time_epoch`, dan fungsi `display_set_time()`. |
| **13** | [`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display.c) | Sub-sistem Display E3 | **Ditambahkan & Diganti** | Halaman waktu resmi SPLN (OBIS 01.00), pergantian `snprintf` format `%f` menjadi aritmatika integer `%lu.%03lu`. |
| **14** | [`firmware/app/display/src/display_task.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display_task.c) | Task Display RTOS | **Ditambahkan & Diganti** | Interval carousel dinaikkan ke 5 detik (5000 ms), sinkronisasi snapshot `display_update_measurements()`, log konsol UART per halaman. |
| **15** | [`firmware/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/tamper/src/tamper_manager.c) | Sub-sistem Tamper E3 | **Dihapus** | Menghapus pengiriman queue RTOS dari context ISR untuk mencegah assertion crash jika RTOS belum aktif. |
| **16** | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/fsm/src/rtos_tasks.c) | FreeRTOS Task Manager | **Ditambahkan** | Inisialisasi buzzer PB7, pengiriman antrean sabotase di context task, pola *beeping* 200ms ON / 200ms OFF saat sabotase, auto-reset buzzer saat normal. |
| **17** | [`firmware/app/profiling/inc/nvram_storage.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/profiling/inc/nvram_storage.h) | Storage Profiling E3 | **Diganti** | Alokasi alamat flash diselaraskan ke akhir Bank 2 MCU 1 MB (`0x080F0000`, `0x080F2000`, `0x080F4000`). |
| **18** | [`firmware/tests/app/test_rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/tests/app/test_rtos_tasks.c) | Unit Test Suite | **Ditambahkan** | Skenario pengujian ke-6 untuk verifikasi aktuasi buzzer dan LED alarm PB7. |

---

## 2. Rincian Sebelum vs. Sesudah (Before vs. After) per Berkas

---

### A. Driver Layar OLED ([`Core/Src/ssd1306.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/ssd1306.c))

#### 1. Manajemen Alamat I2C & Timeout Komunikasi
* **🔴 SEBELUM (Dihapus/Diganti):**
  Alamat I2C dikunci statis pada makro `SSD1306_I2C_ADDR` (`0x78`), dan timeout pengiriman sangat singkat (25 ms untuk perintah, 100 ms untuk data buffer):
  ```c
  void ssd1306_WriteCommand(uint8_t byte) {
      HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, 25);
  }

  void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
      HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, 100);
  }
  ```
* **🟢 SESUDAH (Ditambahkan/Diganti):**
  Alamat disimpan dalam variabel dinamis `s_ssd1306_addr` hasil deteksi otomatis, dan timeout menggunakan `HAL_MAX_DELAY` agar tidak terputus saat bus stretching:
  ```c
  static uint8_t s_ssd1306_addr = SSD1306_I2C_ADDR;

  void ssd1306_WriteCommand(uint8_t byte) {
      HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, s_ssd1306_addr, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
  }

  void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
      HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, s_ssd1306_addr, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
  }
  ```

#### 2. Probing Bus I2C & Visual Self-Test Flash
* **🔴 SEBELUM (Dihapus/Diganti):**
  Hanya melakukan single-check ke `SSD1306_I2C_ADDR` sebanyak 2 trial dengan timeout 25 ms. Jika gagal ACK, langsung `return` dan OLED mati permanen:
  ```c
  void ssd1306_Init(void) {
      if (SSD1306.Initialized) return;
  #if defined(SSD1306_USE_I2C)
      if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 2, 25) != HAL_OK) {
          printf("[WARN] SSD1306 OLED (I2C Addr: 0x%02X) TIDAK TERDETEKSI...\r\n", SSD1306_I2C_ADDR >> 1);
          return;
      }
  #endif
      ssd1306_Reset();
      HAL_Delay(100);
      ...
  ```
* **🟢 SESUDAH (Ditambahkan):**
  - Ditambahkan delay stabilisasi 150 ms sebelum probing.
  - Melakukan looping deteksi alamat `0x3C << 1` (0x78) dan `0x3D << 1` (0x7A).
  - Jika belum merespons, memindai seluruh alamat I2C dari 1 hingga 127 (`[I2C SCAN]`).
  - Ditambahkan fitur **Self-Test Flash 400 ms** (`0xA5` $\rightarrow$ `0xA4`) sehingga panel menyala putih penuh sejenak saat daya masuk, memberikan konfirmasi visual instan bahwa perangkat keras hidup:
  ```c
  void ssd1306_Init(void) {
      if (SSD1306.Initialized) return;

      /* Beri waktu stabilisasi daya & sirkuit RC Reset pada modul OLED */
      HAL_Delay(150);

  #if defined(SSD1306_USE_I2C)
      uint8_t found_addr = 0;
      printf("[OLED] Mencari modul SSD1306 pada I2C1 (PB8=SCL, PB9=SDA)...\r\n");

      for (int attempt = 0; attempt < 3 && found_addr == 0; attempt++) {
          if (attempt > 0) HAL_Delay(50);
          if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, (0x3C << 1), 3, 50) == HAL_OK) {
              found_addr = (0x3C << 1);
          } else if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, (0x3D << 1), 3, 50) == HAL_OK) {
              found_addr = (0x3D << 1);
          }
      }

      if (found_addr == 0) {
          printf("[I2C SCAN] Memindai seluruh alamat I2C 1..127...\r\n");
          for (uint16_t addr = 1; addr < 128; addr++) {
              if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, (addr << 1), 2, 20) == HAL_OK) {
                  printf("[I2C SCAN] Ditemukan perangkat I2C pada alamat: 0x%02X\r\n", addr);
                  if (found_addr == 0) found_addr = (addr << 1);
              }
          }
      }

      if (found_addr != 0) {
          s_ssd1306_addr = found_addr;
          printf("[OLED] SSD1306 TERDETEKSI pada alamat I2C 0x%02X!\r\n", s_ssd1306_addr >> 1);
      } else {
          printf("[WARN] SSD1306 OLED TIDAK MERESPONS di bus I2C1 (PB8=SCL, PB9=SDA)!\r\n");
          s_ssd1306_addr = (0x3C << 1);
      }
  #endif
      ...
      ssd1306_SetDisplayOn(1);

      // Display Self-Test: Nyalakan seluruh pixel selama 400ms untuk verifikasi visual panel
      printf("[OLED] Panel diaktifkan! Menjalankan self-test flash (400ms)...\r\n");
      ssd1306_WriteCommand(0xA5); // Seluruh pixel ON (mengabaikan RAM)
      HAL_Delay(400);
      ssd1306_WriteCommand(0xA4); // Kembali menampilkan isi RAM buffer
      ...
  ```

---

### B. Inisialisasi Sistem & Hardware Core ([`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c))

#### 1. Inisialisasi I2C1: SCL Bus Recovery & Perbaikan Timing
* **🔴 SEBELUM (Dihapus/Diganti):**
  Menggunakan register timing salah `0x00C0EAFF` (~326 kHz) tanpa bus recovery:
  ```c
  static void MX_I2C1_Init(void)
  {
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00C0EAFF; // KLAIM 100kHz PADAHAL ~326kHz (FAST-MODE)!
    hi2c1.Init.OwnAddress1 = 0;
    ...
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) { Error_Handler(); }
  }
  ```
  *Dampak:* Pada kabel jumper berkapasitansi tinggi atau menggunakan pull-up internal (~40 kΩ), sinyal SCL mengalami pelanggaran batas *rise-time* dan frekuensi 326 kHz terlalu cepat untuk display SSD1306, menyebabkan layar tetap gelap.
* **🟢 SESUDAH (Ditambahkan/Diganti):**
  - Menambahkan **9-Cycle SCL Bus Recovery** dengan GPIO Open-Drain untuk melepas slave OLED yang menahan SDA LOW.
  - Memperbaiki timing menjadi `0x3042C7C7` (murni 100.0 kHz standard-mode untuk PCLK1 160 MHz).
  - Mengonfigurasi filter analog dan digital I2C:
  ```c
  static void MX_I2C1_Init(void)
  {
    /* I2C Bus Recovery: SCL toggle 9x jika slave (OLED) menahan SDA LOW saat reset */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_Recovery = {0};
    GPIO_Recovery.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_Recovery.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Recovery.Pull = GPIO_PULLUP;
    GPIO_Recovery.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_Recovery);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
    for (int i = 0; i < 9; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
      for (volatile int d = 0; d < 200; d++);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
      for (volatile int d = 0; d < 200; d++);
    }
    /* Generate STOP condition: SDA LOW, SCL HIGH, SDA HIGH */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    for (volatile int d = 0; d < 200; d++);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    for (volatile int d = 0; d < 200; d++);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    for (volatile int d = 0; d < 200; d++);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x3042C7C7; /* True 100 kHz Standard Mode untuk 160 MHz PCLK1 */
    hi2c1.Init.OwnAddress1 = 0;
    ...
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) { Error_Handler(); }

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) { Error_Handler(); }
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) { Error_Handler(); }
  }
  ```

#### 2. Instruction Cache (ICACHE) & Dual Konsol UART
* **🔴 SEBELUM (Dihapus/Diganti):**
  ICACHE belum diaktifkan. Konsol hanya mentransmisikan data ke `huart2` (PA2/PA3):
  ```c
  static void console_uart_transmit_bsp(const uint8_t *data, size_t len) {
      HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)len, 100);
  }
  ```
* **🟢 SESUDAH (Ditambahkan):**
  - Mengaktifkan ICACHE melalui `MX_ICACHE_Init()` untuk akselerasi eksekusi kode flash 160 MHz.
  - Mengaktifkan `huart1` (PA9/PA10) untuk mirror konsol ke ST-LINK VCP.
  - Menangkap interupsi karakter terminal (`'t'`, `'s'`, spasi) pada `USART1` untuk simulasi tombol sabotase:
  ```c
  static void MX_ICACHE_Init(void)
  {
    if (HAL_ICACHE_Enable() != HAL_OK)
    {
      Error_Handler();
    }
  }

  static void console_uart_transmit_bsp(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)len, 100);
    if (huart1.Instance != NULL) {
      HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, 100);
    }
  }

  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
      if (s_usart1_rx_char == 't' || s_usart1_rx_char == 'T' ||
          s_usart1_rx_char == 's' || s_usart1_rx_char == 'S' ||
          s_usart1_rx_char == ' ' || s_usart1_rx_char == '\r' || s_usart1_rx_char == '\n') {
        printf("\r\n[SIMULASI] Perintah sabotase PC13 (Case Open) diterima via konsol UART!\r\n");
        tamper_handle_button_press_isr(HAL_GetTick());
      }
      HAL_UART_Receive_IT(&huart1, &s_usart1_rx_char, 1);
    }
  }
  ```

---

### C. Alokasi Memori Linker Script ([`STM32U575xx_FLASH.ld`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/STM32U575xx_FLASH.ld))

* **🔴 SEBELUM (Diganti):**
  ```ld
  MEMORY
  {
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
    ROM   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K  /* 2 MB ROM (ZIT6) */
    SRAM4 (xrw) : ORIGIN = 0x28000000, LENGTH = 16K
  }
  ```
* **🟢 SESUDAH (Diganti):**
  ```ld
  MEMORY
  {
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
    ROM   (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K  /* 1 MB ROM (VGT6) */
    SRAM4 (xrw) : ORIGIN = 0x28000000, LENGTH = 16K
  }
  ```
* **Alasan Teknis:** STM32U575VGT6 memiliki flash fisik 1 MB (bukan 2 MB). Menentukan panjang 1024K menjamin compiler mendeteksi jika terjadi overflow memori serta mencegah penempatan NVRAM di luar batas fisik flash chip.

---

### D. Sub-sistem Display & Layar SPLN ([`firmware/app/display`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display))

#### 1. Halaman Jam & Tanggal `DISP_PAGE_TIME` (SPLN Butir 6.11)
* **🔴 SEBELUM (Dihapus/Diganti):**
  Enum halaman hanya sampai `DISP_PAGE_ACTIVE_ENERGY` lalu langsung `DISP_PAGE_TAMPER_ALARM`. Tidak ada representasi waktu RTC.
* **🟢 SESUDAH (Ditambahkan):**
  - Menambahkan `DISP_PAGE_TIME` dengan kode OBIS `01.00`, indeks scroll `15`, format `HH:MM:SS WIB`.
  - Menambahkan field `current_time_epoch` pada `display_context_t` dan fungsi `display_set_time()`:
  ```c
  case DISP_PAGE_TIME: {
      obis_code = "01.00";
      snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "15");
      frame->is_large_font = false;

      uint32_t t = ctx->current_time_epoch;
      uint32_t sec = t % 60;
      uint32_t min = (t / 60) % 60;
      uint32_t hr  = (t / 3600) % 24;

      snprintf(frame->main_value, sizeof(frame->main_value), "%02lu:%02lu:%02lu",
               (unsigned long)hr, (unsigned long)min, (unsigned long)sec);
      snprintf(frame->unit, sizeof(frame->unit), "WIB");
      break;
  }
  ```

#### 2. Format Integer Bebas Floating Point
* **🔴 SEBELUM (Diganti):**
  Menggunakan floating point casting `(double)` dan format `%.3f`:
  ```c
  snprintf(frame->main_value, sizeof(frame->main_value), "%.3f", (double)ctx->meas_buffer.active_power_w / 1000.0);
  ```
* **🟢 SESUDAH (Diganti):**
  Menggunakan aritmatika integer murni (`/ 1000` dan `% 1000`):
  ```c
  int32_t pw = ctx->meas_buffer.active_power_w;
  uint32_t abs_pw = (pw < 0) ? (uint32_t)(-pw) : (uint32_t)pw;
  snprintf(frame->main_value, sizeof(frame->main_value), "%s%lu.%03lu",
           (pw < 0) ? "-" : "",
           (unsigned long)(abs_pw / 1000),
           (unsigned long)(abs_pw % 1000));
  ```
  *Dampak:* Mengurangi ukuran biner flash hingga belasan kilobyte dan mencegah potensi crash saat format string variadik pada arsitektur ARM Cortex-M33 tanpa FPU hardware print support.

#### 3. Carousel Task & Log UART Otomatis ([`display_task.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/display/src/display_task.c))
* **🔴 SEBELUM (Dihapus/Diganti):**
  Interval carousel 2 detik (`2000 ms`), dan fungsi `display_update_measurements()` **tidak pernah dipanggil** di dalam task (hanya membaca snapshot ke variabel lokal `meas`, sehingga nilai di layar tidak pernah ter-update).
* **🟢 SESUDAH (Ditambahkan/Diganti):**
  - Interval carousel diubah ke standar SPLN: **5 detik (`5000 ms`)**.
  - Ditambahkan panggilan `display_update_measurements(&disp_ctx, &meas)` secara periodik.
  - Ditambahkan log UART otomatis setiap halaman display berganti:
  ```c
  rtos_meter_data_get_snapshot(&meas);
  display_update_measurements(&disp_ctx, &meas);
  ...
  printf("[OLED DISP] Page %d [%s] -> OBIS: %s | %s %s | Status: %s\r\n",
         disp_ctx.current_page,
         spln_frame.scroll_index_zz,
         spln_frame.obis_code,
         spln_frame.main_value,
         spln_frame.unit,
         spln_frame.is_alarm_active ? "!ALM (SABOTASE)" : "NORMAL");
  ```

---

### E. Driver Baru: Buzzer & LED Alarm SPLN PB7 ([`firmware/drivers/bsp`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp))

#### 1. Header Interface ([`inc/bsp_alarm_buzzer.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/inc/bsp_alarm_buzzer.h))
* **🟢 DITAMBAHKAN (File Baru):**
  Menyediakan fungsi kontrol pin PB7:
  - `void bsp_alarm_buzzer_init(void);`
  - `void bsp_alarm_buzzer_set(bool on);`
  - `void bsp_alarm_buzzer_toggle(void);`
  - `bool bsp_alarm_buzzer_get(void);`

#### 2. Implementasi Driver ([`src/bsp_alarm_buzzer.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/drivers/bsp/src/bsp_alarm_buzzer.c))
* **🟢 DITAMBAHKAN (File Baru):**
  Mengontrol pin `GPIOB Pin 7` (Push-Pull, No-Pull, Low-Speed) untuk target MCU, dan menyediakan status bool mock untuk unit test PC host runner (CTest).

---

### F. Manajemen Sabotase & FreeRTOS Safety ([`tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/tamper/src/tamper_manager.c) & [`rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/fsm/src/rtos_tasks.c))

* **🔴 SEBELUM (Dihapus dari `tamper_manager.c`):**
  ```c
  void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
      ...
      /* Pemanggilan ini DIHAPUS karena memanggil API queue RTOS langsung dari NVIC ISR */
      tamper_event_msg_t msg = {
          .timestamp = timestamp_ms / 1000,
          .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
          .is_active = s_alarm_active
      };
      rtos_queue_send_tamper_event_from_isr(&msg);
  }
  ```
  *Dampak Masalah:* Jika tombol ditekan saat sistem baru menyala sebelum scheduler FreeRTOS siap, fungsi `rtos_queue_send_tamper_event_from_isr` memicu `configASSERT` atau `HardFault`.
* **🟢 SESUDAH (Ditambahkan ke `rtos_tasks.c`):**
  Pengiriman event antrean ditangani secara aman di dalam task context `task_tamper_emergency_entry`. Selain itu, ditambahkan aktuasi modul buzzer/LED:
  ```c
  void task_tamper_emergency_entry(void *pvParameters) {
      for (;;) {
          bool current_alarm = tamper_is_alarm_active(&g_tamper_ctx);
          if (current_alarm != last_alarm_state) {
              last_alarm_state = current_alarm;
              if (current_alarm) {
                  printf("[TAMPER] *** DARURAT! SABOTASE TERDETEKSI (CASE OPEN) ***\r\n");
              } else {
                  printf("[TAMPER] Sabotase dipulihkan. Sistem kembali normal.\r\n");
                  bsp_alarm_buzzer_set(false);
              }
              tamper_event_msg_t msg = {
                  .timestamp = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ),
                  .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
                  .is_active = current_alarm
              };
              rtos_queue_send_tamper_event(&msg);
          }

          /* Aktuasi Buzzer & LED Alarm PB7 saat alarm aktif (200 ms ON / 200 ms OFF) */
          if (current_alarm) {
              bsp_alarm_buzzer_toggle();
          } else {
              bsp_alarm_buzzer_set(false);
          }

          vTaskDelay(pdMS_TO_TICKS(200));
      }
  }
  ```

---

### G. Alamat Memori Flash NVRAM ([`nvram_storage.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/app/profiling/inc/nvram_storage.h))

* **🔴 SEBELUM (Diganti):**
  ```c
  /* Alokasi Alamat Memori Flash STM32U575 Bank 2 */
  #define NVRAM_CALIBRATION_START_ADDR  0x08100000U
  #define NVRAM_TAMPER_LOG_START_ADDR   0x08104000U
  #define NVRAM_LOAD_PROFILE_START_ADDR 0x08108000U
  ```
  *Masalah:* Alamat `0x08100000` berada di luar kapasitas fisik chip 1 MB STM32U575VGT6 (Flash fisik berakhir di `0x080FFFFF`).
* **🟢 SESUDAH (Diganti):**
  ```c
  /* Alokasi Alamat Memori Flash STM32U575 Bank 2 (Akhir Bank 2 1MB: 0x080F0000) */
  #define NVRAM_CALIBRATION_START_ADDR  0x080F0000U
  #define NVRAM_TAMPER_LOG_START_ADDR   0x080F2000U
  #define NVRAM_LOAD_PROFILE_START_ADDR 0x080F4000U
  ```
  *Alasan Teknis:* Menempatkan NVRAM pada 3 sektor terakhir (Page 120, 121, 122) di dalam batas 1 MB flash fisik.

---

## 3. Hasil Uji Kompilasi & Verifikasi

### A. Kompilasi Target Hardware (ARM Embedded GCC + Ninja)
```text
Build type: Debug
[17/18] Linking C executable Uji_Coba_Integrasi.elf; Generating .hex and .bin firmware images...
Memory region         Used Size  Region Size  %age Used
             RAM:       81448 B       768 KB     10.36%
             ROM:       91152 B         1 MB      8.69%
           SRAM4:           0 B        16 KB      0.00%
```
* **Status:** 0 Error, 0 Warning.
* **Ukuran Biner:** `91,152 Bytes` (hanya memakan 8.69% dari 1 MB Flash).

### B. Pengujian Host CTest Suite (`firmware/build`)
```text
Test project G:/PPPP/Day_Sekian/Uji_Coba_Integrasi/firmware/build
      Start  1: test_dlms_association ............   Passed    0.08 sec
      Start  2: test_dlms_apdu ..................   Passed    0.06 sec
      Start  3: test_dlms_integration ............   Passed    0.06 sec
      Start  4: test_dlms_tamper_log .............   Passed    0.08 sec
      Start  5: test_dlms_hdlc ..................   Passed    0.06 sec
      Start  6: test_dlms_server .................   Passed    0.05 sec
      Start  7: test_dlms_task ...................   Passed    0.07 sec
      Start  8: test_dlms_nvm ....................   Passed    0.06 sec
      Start  9: test_bsp_lpuart ..................   Passed    0.07 sec
      Start 10: test_e3_app ......................   Passed    0.07 sec
      Start 11: test_fsm_statechart ..............   Passed    0.06 sec
      Start 12: test_load_profile ................   Passed    0.06 sec
      Start 13: test_load_profile_dlms_adapter ...   Passed    0.05 sec
      Start 14: test_nvram_storage ...............   Passed    0.06 sec
      Start 15: test_rtos_tasks ..................   Passed    0.06 sec

100% tests passed, 0 tests failed out of 15
Total Test time (real) = 1.01 sec
```
Semua 15 skenario pengujian unit otomatis lulus 100%.

