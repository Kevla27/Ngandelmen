# Rangkuman Diagnosa & Perbaikan Bug Freeze Hardware (Docklight & OLED Blank)

Dokumen ini merangkum secara teknis dan mendalam investigasi akar masalah (*root-cause analysis*), perbaikan kode (*code remedies*), dan penambahan mekanisme proteksi keandalan (*fail-safe & fault tolerance*) yang diterapkan pada proyek **Smart Meter 3-Fasa STM32U575VGT6 (Uji_Coba_Integrasi)** terkait kendala **Docklight Serial Monitor dan Layar OLED SSD1306 yang tidak merespons (blank/freeze)** saat mikrokontroler dinyalakan.

---

## 1. Gejala Permasalahan (*Symptoms*)

Saat firmware di-flash ke mikrokontroler STM32U575VGT6:
1. **Docklight Serial Monitor:**
   - Port USART2 (PA2/PA3 @ 115200 bps) tidak memunculkan teks apapun sama sekali (termasuk banner awal `printf`).
2. **Layar OLED SSD1306 (128x32 I2C1):**
   - Layar tetap gelap/mati, tidak memunculkan teks *splash screen* `"STM32U575 START"` ataupun carousel data PLN.
3. **Status Sistem:**
   - LED indikator tidak berkedip, tidak ada tanda-tanda FreeRTOS scheduler berjalan. Mikrokontroler seolah-olah mengalami *bricked* atau macet total sesaat setelah reset.

---

## 2. Investigasi & Analisis Akar Masalah (*Root Cause Analysis*)

### A. Akar Masalah Utama: `USART2_IRQHandler` Tidak Terdefinisi
Pada [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c), inisialisasi peripheral UART dilakukan pada fungsi `MX_USART2_UART_Init()`:
```c
/* Mengaktifkan interupsi USART2 di NVIC */
HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
HAL_NVIC_EnableIRQ(USART2_IRQn);

/* Memulai penerimaan DMA dengan deteksi Idle Line */
HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
```

1. **Pemicu Interupsi Instan:**
   Fungsi HAL `HAL_UARTEx_ReceiveToIdle_DMA` langsung menyalakan bit interupsi *IDLE Line Detection* (`USART_CR1_IDLEIE`).
   Karena pin RX UART (PA3) dalam kondisi normal bernilai logika HIGH (*idle*), hardware USART STM32U5 langsung memicu interupsi hardware `USART2_IRQn` sesaat setelah peripheral aktif.

2. **Hilangnya Interrupt Handler:**
   Pada file [`Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/stm32u5xx_it.c), fungsi **`USART2_IRQHandler` belum diimplementasikan sama sekali**.

3. **Perangkap Loop Tak Hingga (*Trap* di Startup):**
   Dalam tabel vektor interupsi assembly [`startup_stm32u575xx.s`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/startup_stm32u575xx.s):
   ```assembly
   .weak   USART2_IRQHandler
   .thumb_set USART2_IRQHandler,Default_Handler
   ...
   Default_Handler:
   Infinite_Loop:
       b   Infinite_Loop   /* CPU terjebak di sini selamanya */
   ```
   Karena tidak ada implementasi fungsi C bernama `USART2_IRQHandler`, linker mengarahkan vektor tersebut ke `Default_Handler`.
   
4. **Dampak Fatal:**
   CPU ARM Cortex-M33 langsung terjebak di `Infinite_Loop` **di dalam panggilan `MX_USART2_UART_Init()` pada fungsi `main()`**.
   Semua instruksi berikutnya **tidak pernah dieksekusi**:
   * `MX_I2C1_Init()` tidak pernah dipanggil.
   * Banner `printf(...)` pembuka tidak pernah dikirim ke Docklight.
   * `ssd1306_Init()` tidak pernah dijalankan.
   * `vTaskStartScheduler()` tidak pernah dicapai.

---

### B. Akar Masalah Sekunder: Kerentanan `HAL_MAX_DELAY` pada Driver I2C OLED
Pada file driver [`Core/Src/ssd1306.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/ssd1306.c) versi sebelumnya:
```c
void ssd1306_WriteCommand(uint8_t byte) {
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}
```
* **Kelemahan:** Menggunakan `HAL_MAX_DELAY` berarti jika modul OLED belum terpasang, kabel SCL/SDA kendor, salah jalur, atau modul mengalami NACK, fungsi `HAL_I2C_Mem_Write` akan memblokir CPU tanpa batas waktu (*infinite hang*).

---

## 3. Rincian Perubahan Kode (Sebelum vs Sesudah)

### A. [`Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/stm32u5xx_it.c)
Menambahkan variabel eksternal UART handle dan implementasi fungsi interupsi `USART2_IRQHandler`.

```c
/* ==================== SEBELUM ==================== */
/* External variables */
extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
/* USART2_IRQHandler TIDAK ADA */
```

```c
/* ==================== SESUDAH ==================== */
/* External variables */
extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
extern UART_HandleTypeDef huart2;

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}
```

---

### B. [`Core/Inc/stm32u5xx_it.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Inc/stm32u5xx_it.h)
Mendeklarasikan prototipe fungsi handler ke header agar dikenali oleh modul lain.

```c
/* ==================== SEBELUM ==================== */
void GPDMA1_Channel1_IRQHandler(void);
```

```c
/* ==================== SESUDAH ==================== */
void GPDMA1_Channel1_IRQHandler(void);
void USART2_IRQHandler(void);
```

---

### C. [`Core/Src/ssd1306.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/ssd1306.c)
1. Menghilangkan `HAL_MAX_DELAY` dan menggantinya dengan timeout terukur.
2. Menambahkan fitur auto-probing hardware OLED pada bus I2C.
3. Menambahkan pengecekan status inisialisasi pada proses render buffer.

```c
/* ==================== SEBELUM ==================== */
void ssd1306_WriteCommand(uint8_t byte) {
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

void ssd1306_Init(void) {
    if (SSD1306.Initialized) return;
    ssd1306_Reset();
    ...
}

void ssd1306_UpdateScreen(void) {
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++) {
        ...
    }
}
```

```c
/* ==================== SESUDAH ==================== */
void ssd1306_WriteCommand(uint8_t byte) {
    /* Timeout 25 ms aman untuk transmisi 1 byte pada 100 kHz */
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, 25);
}

void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    /* Timeout 100 ms cukup untuk burst frame buffer 128 byte */
    HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, 100);
}

void ssd1306_Init(void) {
    if (SSD1306.Initialized) return;

#if defined(SSD1306_USE_I2C)
    /* Verifikasi apakah modul OLED merespons pada bus I2C */
    if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 2, 25) != HAL_OK) {
        printf("[WARN] SSD1306 OLED (I2C Addr: 0x%02X) TIDAK TERDETEKSI di I2C1 (PB8=SCL, PB9=SDA)!\r\n", SSD1306_I2C_ADDR >> 1);
        printf("       Pastikan kabel OLED: VCC->3.3V, GND->GND, SCL->PB8, SDA->PB9.\r\n");
        return;
    }
#endif

    ssd1306_Reset();
    ...
}

void ssd1306_UpdateScreen(void) {
    /* Jangan kirim transaksi I2C jika modul layar tidak terdeteksi */
    if (!SSD1306.Initialized) {
        return;
    }
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++) {
        ...
    }
}
```

---

### D. [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c)
1. Menambahkan guard scheduler pada callback RX event UART.
2. Menambahkan `HAL_UART_ErrorCallback` untuk pemulihan otomatis jika terjadi *Overrun/Framing Error*.

```c
/* ==================== SEBELUM ==================== */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (Size > 0) {
            rtos_dlms_task_notify_rx(g_uart_rx_dma_buf, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
/* Tidak ada HAL_UART_ErrorCallback */
```

```c
/* ==================== SESUDAH ==================== */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        /* Cegah pemanggilan notifikasi RTOS jika scheduler belum aktif */
        if (Size > 0 && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            rtos_dlms_task_notify_rx(g_uart_rx_dma_buf, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* Pemulihan otomatis jika terjadi Overrun / Framing / Noise Error pada UART */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
    }
}
```

---

## 4. Rangkuman File yang Dimodifikasi

| No | File Path | Fungsi Utama yang Diubah | Tujuan Utama |
| :---: | :--- | :--- | :--- |
| 1 | [`Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/stm32u5xx_it.c) | `USART2_IRQHandler()` | Mencegah CPU terperangkap di `Default_Handler` saat interupsi UART terpicu. |
| 2 | [`Core/Inc/stm32u5xx_it.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Inc/stm32u5xx_it.h) | Prototipe `USART2_IRQHandler()` | Ekspos deklarasi fungsi interupsi ke project. |
| 3 | [`Core/Src/ssd1306.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/ssd1306.c) | `ssd1306_WriteCommand()`, `ssd1306_WriteData()`, `ssd1306_Init()`, `ssd1306_UpdateScreen()` | Eliminasi `HAL_MAX_DELAY`, auto-detect modul OLED di bus I2C, pencegahan freeze I2C. |
| 4 | [`Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi/Core/Src/main.c) | `HAL_UARTEx_RxEventCallback()`, `HAL_UART_ErrorCallback()` | Proteksi pemanggilan RTOS pra-scheduler dan auto-recovery komunikasi UART DMA. |

---

## 5. Hasil Verifikasi & Kompilasi

Proyek dikompilasi menggunakan toolchain `arm-none-eabi-gcc` melalui CMake:
```text
[1/4] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/stm32u5xx_it.c.obj
[2/4] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/ssd1306.c.obj
[3/4] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/main.c.obj
[4/4] Linking C executable Uji_Coba_Integrasi.elf

Memory region         Used Size  Region Size  %age Used
             RAM:       81248 B       768 KB     10.33%
             ROM:       82920 B         2 MB      3.95%
           SRAM4:           0 B        16 KB      0.00%

BUILD SUCCESSFUL: 0 Errors, 0 Warnings
```

### Panduan Koneksi Fisik (*Wiring Checklist*)
Saat melakukan pengujian hardware pada board STM32U575VGT6:
1. **USB-UART (USART2 - Docklight):**
   * `PA2` (MCU TX) $\rightarrow$ `RXD` USB-UART Adapter
   * `PA3` (MCU RX) $\rightarrow$ `TXD` USB-UART Adapter
   * `GND` $\rightarrow$ `GND`
   * Baudrate: **115200 bps**, 8 Data bits, No Parity, 1 Stop bit.
2. **Layar OLED 128x32 SSD1306 (I2C1):**
   * `VCC` $\rightarrow$ `3.3V`
   * `GND` $\rightarrow$ `GND`
   * `SCL` $\rightarrow$ `PB8`
   * `SDA` $\rightarrow$ `PB9`
   * Alamat I2C: `0x3C` (default 7-bit).
3. **Tombol Tamper / Sabotase (EXTI13):**
   * `PC13` (User Button onboard Nucleo / Eksternal Pushbutton terhubung ke GND).
