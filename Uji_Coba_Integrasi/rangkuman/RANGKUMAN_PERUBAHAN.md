# Rangkuman Perubahan Integrasi Firmware STM32U575 & OLED 128x32

Dokumen ini merangkum seluruh analisis masalah, perbaikan teknis, serta adaptasi perangkat keras yang telah diterapkan pada proyek **Uji Coba Integrasi Firmware E3 (Host MCU STM32U575VGT6)** dengan spesifikasi layar **OLED SSD1306 128x32 I2C**.

> **Catatan Kepatuhan:**
> Sesuai batasan yang diberikan, seluruh file di dalam direktori **/firmware/app** **tidak disentuh sama sekali (100% original / 0 modifikasi)**.

---

## 1. Masalah Utama yang Ditemukan pada Program Awal

1. **FreeRTOS Tasks dan Scheduler Belum Dijalankan:**
   - Di `main.c`, pustaka FreeRTOS sudah di-*include*, namun fungsi `rtos_system_init()`, pembuatan task (`xTaskCreate`), dan pemanggilan scheduler (`vTaskStartScheduler()`) belum pernah dipanggil. Program langsung tertahan di loop `while(1) {}` kosong.
2. **Potensi Freeze / HardFault pada SysTick & HAL_Delay:**
   - `SysTick_Handler` FreeRTOS bawaan pada arsitektur Cortex-M33 langsung memanggil `xTaskIncrementTick()` tanpa memeriksa apakah scheduler sudah aktif. Ketika `HAL_Init()` mengaktifkan SysTick di awal `main()`, pointer list kernel FreeRTOS yang belum diinisialisasi memicu dereferensi pointer liar / HardFault.
   - Selain itu, `HAL_IncTick()` ditaruh di tick hook yang hanya berdetik saat scheduler berjalan, sehingga `HAL_Delay()` di `ssd1306_Init()` sebelum scheduler aktif mengalami *hang/freeze*.
3. **Inisialisasi MSP I2C1:**
   - Fungsi `HAL_I2C_MspInit` belum didefinisikan secara resmi di `stm32u5xx_hal_msp.c`, sehingga clock source I2C1 belum dikonfigurasi ke PCLK1 secara eksplisit.
5. **Penyebab Layar Stuck di "Booting RTOS..." (CPU Starvation):**
   - Di dalam `firmware/app/fsm/src/rtos_tasks.c`, pemanggilan header FreeRTOS dibungkus dalam `#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)`.
   - Karena macro `USE_FREERTOS` belum didefinisikan pada `CMakeLists.txt`, `rtos_tasks.c` masuk ke blok `#else` (mode simulasi PC CTest) di mana `vTaskDelay` didefinisikan sebagai fungsi kosong dummy: `static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }`.
   - Akibatnya, `task_tamper_emergency_entry` yang memiliki **Prioritas 4 (tertinggi)** mengeksekusi loop `for(;;)` tanpa pernah *delay/yield*. Task ini memonopoli 100% CPU time, menyebabkan task berprioritas lebih rendah (termasuk `OLEDTask` dan `UITask` berprioritas 1) **sama sekali tidak pernah mendapatkan giliran eksekusi**. Layar pun macet (*stuck*) selamanya di pesan awal "Booting RTOS...".

---

## 2. Daftar File yang Diubah

| No | File Path | Kategori | Keterangan Perubahan |
|---|---|---|---|
| 1 | `CMakeLists.txt` | Build System | Menambahkan macro `USE_FREERTOS` agar task RTOS menggunakan `vTaskDelay` & queue asli |
| 2 | `Core/Inc/ssd1306_conf.h` | Driver Config | Mengatur `SSD1306_WIDTH = 128` dan `SSD1306_HEIGHT = 32` |
| 3 | `Core/Src/ssd1306.c` | Driver Source | Menambahkan proteksi *guard* pada `ssd1306_Init` agar tidak menghapus layar saat re-init |
| 4 | `Core/Src/main.c` | Aplikasi Utama | Inisialisasi periferal, banner UART, splash screen 128x32, inisialisasi RTOS, task creation, dan task carousel OLED |
| 5 | `Core/Src/stm32u5xx_hal_msp.c` | HAL MSP | Menambahkan implementasi resmi `HAL_I2C_MspInit` dan `HAL_I2C_MspDeInit` |
| 6 | `firmware/FreeRTOS/.../port.c` | FreeRTOS Port | Memanggil `HAL_IncTick()` di `SysTick_Handler` dan memproteksi `xTaskIncrementTick()` dengan `taskSCHEDULER_NOT_STARTED` |
| 7 | `/firmware/app/` *(semua file)* | App Subsystem | **TIDAK DIUBAH SAMA SEKALI (0 Perubahan)** |

---

## 3. Rincian Teknis Perubahan per File

### A. `CMakeLists.txt`
Menambahkan macro `USE_FREERTOS` pada `target_compile_definitions`:
```cmake
# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    USE_FREERTOS
)
```
**Dampak Teknis:**
- Mengaktifkan implementasi asli FreeRTOS di `rtos_tasks.c` (`xQueueCreate` dan `vTaskDelay` asli).
- Menghentikan monopoli CPU oleh `TamperTask` sehingga `OLEDTask` dan `UITask` dapat berjalan normal.

---

### B. `Core/Inc/ssd1306_conf.h`
Mengaktifkan konfigurasi layar OLED 128x32 secara eksplisit:
```c
// The width of the screen can be set using this define. Default 128.
#define SSD1306_WIDTH           128

// The height can be changed as well if necessary (32, 64, or 128).
#define SSD1306_HEIGHT          32
```
**Dampak Teknis:**
- Buffer grafis `SSD1306_Buffer` berkurang menjadi **512 Byte** ($128 \times 32 / 8$).
- Jumlah halaman pemindaian RAM menjadi **4 page** (Page 0 – Page 3).
- Perintah *Multiplex Ratio* otomatis disetel ke `0x1F` (32 baris scan).
- Perintah *COM Pins Hardware Configuration* otomatis disetel ke `0x02` (Sequential COM pin configuration standar 128x32).

---

### C. `Core/Src/ssd1306.c`
Menambahkan pengecekan *guard* status inisialisasi pada `ssd1306_Init()`:
```c
/* Initialize the oled screen */
void ssd1306_Init(void) {
    if (SSD1306.Initialized) {
        return; /* Menghindari reset ulang layar dan delay 100ms di dalam task */
    }

    // Reset OLED
    ssd1306_Reset();
    ...
```
**Dampak Teknis:**
- Karena task bawaan dari `/firmware/app/fsm/src/rtos_tasks.c` memanggil `ssd1306_Init()` saat pertama kali berjalan, guard ini memastikan buffer layar tidak dibersihkan/dihapus kembali saat task tersebut mulai berjalan.

---

### D. `firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c`
Memperbaiki integrasi interrupt SysTick antara FreeRTOS dan STM32 HAL:
```c
extern void HAL_IncTick(void);

void SysTick_Handler(void) /* PRIVILEGED_FUNCTION */
{
  HAL_IncTick();

  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    uint32_t ulPreviousMask;

    ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();
#if (configNUMBER_OF_CORES > 1)
    UBaseType_t uxSavedInterruptStatus = portENTER_CRITICAL_FROM_ISR();
#endif

    traceISR_ENTER();
    {
      if (xTaskIncrementTick() != pdFALSE) {
        traceISR_EXIT_TO_SCHEDULER();
        portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
      } else {
        traceISR_EXIT();
      }
    }
#if (configNUMBER_OF_CORES > 1)
    portEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
#endif

    portCLEAR_INTERRUPT_MASK_FROM_ISR(ulPreviousMask);
  }
}
```
**Dampak Teknis:**
1. `HAL_GetTick()` dan `HAL_Delay()` berfungsi dengan akurat di setiap fase (sebelum maupun sesudah scheduler FreeRTOS aktif).
2. Mencegah pemanggilan `xTaskIncrementTick()` pada tabel task yang belum dialokasikan saat bootloader/HAL init, mengeliminasi risiko HardFault di awal eksekusi.

---

### E. `Core/Src/stm32u5xx_hal_msp.c`
Menambahkan inisialisasi hardware periferal I2C1:
```c
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hi2c->Instance==I2C1)
  {
    /* Clock Selection: I2C1 menggunakan PCLK1 (160 MHz) */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* PB8 -> I2C1_SCL, PB9 -> I2C1_SDA */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);
  }
}
```

---

### F. `Core/Src/main.c`
Memperbarui alur utama dan menambahkan task khusus OLED 128x32:
1. **Splash Screen Awal (Tinggi 32 px):**
   - Baris 1: `(X=11, Y=4)` `"STM32U575 START"` (Font 7x10)
   - Baris 2: `(X=11, Y=18)` `"Booting RTOS..."` (Font 7x10)
2. **Inisialisasi Subsistem:**
   - Memanggil `rtos_system_init()` untuk inisialisasi modul E3, antrean FreeRTOS, dan Flash NVRAM.
3. **Task OLED Carousel 128x32 (`task_oled128x32_carousel`):**
   - Menghubungkan modul `display.c` dengan driver grafis SSD1306.
   - Tata letak 3 baris yang proporsional untuk tinggi 32 piksel:
     - **Baris 1 ($Y=1$):** Status meter `[AUTO]  OK ` (Font 6x8)
     - **Baris 2 ($Y=11$):** Judul halaman, misal `VOLTAGE PHASE R` (Font 6x8)
     - **Baris 3 ($Y=21$):** Nilai parameter, misal `220.5 V` (Font 7x10)
4. **Task Creation & Scheduler:**
   - Mendaftarkan kelima task FreeRTOS:
     - `task_tamper_emergency_entry` (Priority 4)
     - `task_metrology_profiling_entry` (Priority 3)
     - `task_dlms_entry` (Priority 2)
     - `task_ui_display_entry` (Priority 1)
     - `task_oled128x32_carousel` (Priority 1)
   - Menjalankan scheduler via `vTaskStartScheduler()`.

---

## 4. Hasil Kompilasi & Verifikasi Akhir

Kompilasi proyek diuji menggunakan toolchain `arm-none-eabi-gcc` 14.3.1 dan build system `ninja`:

```text
[1/5] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/firmware/app/fsm/src/rtos_tasks.c.obj
[2/5] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/ssd1306_fonts.c.obj
[3/5] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/main.c.obj
[4/5] Building C object CMakeFiles/Uji_Coba_Integrasi.dir/Core/Src/ssd1306.c.obj
[5/5] Linking C executable Uji_Coba_Integrasi.elf

Memory region         Used Size  Region Size  %age Used
             RAM:       79792 B       768 KB     10.15%
             ROM:       56112 B         2 MB      2.68%
           SRAM4:           0 B        16 KB      0.00%
```

- **Status Kompilasi:** 100% Berhasil, 0 error, 0 warning.
- **Integritas `/firmware/app`:** Diverifikasi melalui `git diff firmware/app` dengan hasil **0 perubahan**.

