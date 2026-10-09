# Dokumentasi Kondisi Program Sebelum Dilakukan Perubahan (Initial State)

Dokumen ini mendokumentasikan secara lengkap kondisi program asli (**kode mula-mula**) sebelum dilakukan modifikasi atau perbaikan apapun.

---

## 1. Ringkasan Eksekutif Kondisi Awal

Sebelum dilakukan perbaikan, firmware berada dalam kondisi **tidak berfungsi sebagaimana mestinya**:
1. **FreeRTOS Tidak Pernah Berjalan:** Task-task firmware tidak pernah dibuat dan scheduler kernel (`vTaskStartScheduler`) tidak pernah dipanggil. Program berakhir di loop `while (1) {}` kosong.
2. **Potensi Deadlock / Hang di Booting:** Pemanggilan `HAL_Delay()` di awal boot bergantung pada `HAL_IncTick()`, padahal tick HAL hanya diletakkan di `vApplicationTickHook()` FreeRTOS yang belum aktif.
3. **Hilangnya Macro `USE_FREERTOS`:** Tanpa macro ini, file `rtos_tasks.c` masuk ke mode simulasi PC (CTest) di mana fungsi penundaan waktu `vTaskDelay()` merupakan fungsi kosong dummy (`(void)ticks;`).
4. **Driver I2C Tidak Lengkap:** Callback inisialisasi hardware I2C (`HAL_I2C_MspInit`) belum didefinisikan di `stm32u5xx_hal_msp.c`.
5. **Konfigurasi Layar Belum Sesuai Hardware:** Ukuran layar OLED belum disetel ke spesifikasi 128x32.
6. **Tidak Ada Deteksi Tombol:** Pin PC13 (User Button) belum dikonfigurasi untuk mendeteksi event sabotase.

---

## 2. Analisis Kode Sumber Asli per File

### A. File `Core/Src/main.c` (Asli)
Pada file `main.c` mula-mula, fungsi `main()` hanya menginisialisasi periferal, menampilkan pesan splash, dan langsung masuk ke *infinite loop*:

```c
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  ssd1306_Fill(Black);

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("STM32U575 START", Font_7x10, White);
  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString("Booting RTOS...", Font_7x10, White);
  ssd1306_UpdateScreen();

  printf("\n==================================================\n");
  printf("   STM32U575VGT6 HARDWARE BRING-UP INITIALIZED    \n");
  printf("   Console UART: USART2 (PA2/PA3 @ 115200 bps)    \n");
  printf("   System Clock: 160 MHz | GPDMA1 RX ACTIVE       \n");
  printf("==================================================\n\n");
  /* USER CODE END 2 */

  while (1)
  {
      /* LOOP KOSONG - Tidak ada task yang berjalan */
  }
}
```

#### Masalah pada `main.c` Asli:
- **`rtos_system_init()`** tidak pernah dipanggil $\rightarrow$ Seluruh subsistem E3 (Tamper, Load Profile, Display, antrean FreeRTOS) tidak terinisialisasi.
- **`xTaskCreate()`** tidak ada $\rightarrow$ Keempat task FreeRTOS (`TamperTask`, `ProfileTask`, `DLMSTask`, `UITask`) tidak pernah didaftarkan ke kernel.
- **`vTaskStartScheduler()`** tidak ada $\rightarrow$ Kernel FreeRTOS tidak pernah mengambil alih kontrol CPU.
- **`vApplicationTickHook()`** berisi `HAL_IncTick()`, tetapi hook ini tidak pernah terpanggil sebelum scheduler aktif.

---

### B. File `Core/Src/stm32u5xx_hal_msp.c` (Asli)
File MSP (MCU Support Package) asli hanya mengonfigurasi USART2:
- Terdapat fungsi `HAL_MspInit()`, `HAL_UART_MspInit()`, dan `HAL_UART_MspDeInit()`.
- **Fungsi `HAL_I2C_MspInit()` dan `HAL_I2C_MspDeInit()` sama sekali tidak ada**.
- Akibatnya, clock source I2C1 di RCC (`RCC_PERIPHCLK_I2C1`) tidak dikonfigurasi secara resmi oleh HAL.

---

### C. File `CMakeLists.txt` (Asli)
Pada bagian pendefinisian simbol kompilasi (baris 100-103):
```cmake
# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)
```
- **Masalah Fatal:** Macro `USE_FREERTOS` dan `EMBEDDED_HARDWARE_TARGET` tidak disertakan.
- Hal ini menyebabkan baris 15 pada `firmware/app/fsm/src/rtos_tasks.c`:
  ```c
  #if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
  #include "FreeRTOS.h"
  #include "task.h"
  #include "queue.h"
  static QueueHandle_t xTamperQueue = NULL;
  #else
  #ifndef pdMS_TO_TICKS
  #define pdMS_TO_TICKS(ms) (ms)
  #endif
  static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }
  ...
  ```
  masuk ke blok `#else`. Fungsi `vTaskDelay()` menjadi fungsi kosong dummy `(void)ticks;`, sehingga task prioritas 4 (`TamperTask`) akan memonopoli 100% CPU secara permanen.

---

### D. File `firmware/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c` (Asli)
Handler interrupt SysTick bawaan port FreeRTOS Cortex-M33:
```c
void SysTick_Handler(void) /* PRIVILEGED_FUNCTION */
{
  uint32_t ulPreviousMask;

  ulPreviousMask = portSET_INTERRUPT_MASK_FROM_ISR();
#if (configNUMBER_OF_CORES > 1)
  UBaseType_t uxSavedInterruptStatus = portENTER_CRITICAL_FROM_ISR();
#endif

  traceISR_ENTER();
  {
    /* Increment the RTOS tick. */
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
```
#### Masalah pada `port.c` Asli:
1. `SysTick_Handler` **tidak memanggil `HAL_IncTick()`**. Akibatnya variabel tick HAL (`uwTick`) tidak pernah bertambah sebelum scheduler aktif.
2. Tidak ada proteksi `xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED`. Ketika SysTick mulai berdetik setelah `HAL_Init()`, `xTaskIncrementTick()` langsung dipanggil padahal tabel task FreeRTOS belum diinisialisasi, memicu dereferensi pointer liar (*HardFault*).

---

### E. File `Core/Inc/ssd1306_conf.h` (Asli)
Konfigurasi dimensi layar pada file asli:
```c
// The width of the screen can be set using this
// define. The default value is 128.
// #define SSD1306_WIDTH           64

// The height can be changed as well if necessary.
// It can be 32, 64 or 128. The default value is 64.
// #define SSD1306_HEIGHT          64
```
- Nilai `SSD1306_WIDTH` dan `SSD1306_HEIGHT` masih di-*comment* (nonaktif), sehingga bergantung pada nilai fallback yang tidak eksplisit.

---

### F. File `Core/Src/ssd1306.c` (Asli)
Fungsi inisialisasi layar asli:
```c
void ssd1306_Init(void) {
    // Reset OLED
    ssd1306_Reset();

    // Wait for the screen to boot
    HAL_Delay(100);

    // Init OLED
    ssd1306_SetDisplayOn(0); //display off
    ...
    // Clear screen
    ssd1306_Fill(Black);
    
    // Flush buffer to screen
    ssd1306_UpdateScreen();
    ...
    SSD1306.Initialized = 1;
}
```
- Tidak memiliki proteksi terhadap pemanggilan berulang (*re-initialization*).
- Setiap kali `ssd1306_Init()` dipanggil (misal di awal task UI), layar akan mengalami *blocking delay* 100 ms dan seluruh isi layar dihapus kembali menjadi hitam (*blank*).

---

## 3. Matriks Perbandingan: Sebelum vs Sesudah

| Aspek / Fitur | Kondisi Sebelum Diubah (Original) | Kondisi Sesudah Diubah (Final) |
|---|---|---|
| **Eksekusi FreeRTOS** | Tidak ada task yang dibuat, scheduler tidak pernah dijalankan | 5 Task FreeRTOS aktif secara multitasking preemptive |
| **Scheduler Kernel** | Tidak aktif (CPU idle di `while(1)`) | Aktif via `vTaskStartScheduler()` |
| **Inisialisasi E3** | `rtos_system_init()` tidak dipanggil | `rtos_system_init()` dipanggil sebelum scheduler |
| **Fungsi `vTaskDelay`** | Dummy `(void)ticks;` (CPU starvation) | Menggunakan kernel `vTaskDelay` asli (via macro `USE_FREERTOS`) |
| **Integrasi SysTick & HAL** | Konflik; berpotensi HardFault & freeze di `HAL_Delay` | Terproteksi `xTaskGetSchedulerState()` & memanggil `HAL_IncTick()` |
| **Konfigurasi I2C1 MSP** | Tidak ada `HAL_I2C_MspInit` di `hal_msp.c` | Didefinisikan lengkap dengan routing clock PCLK1 (160 MHz) |
| **Spesifikasi Layar OLED** | Tidak eksplisit (default 64 baris) | Eksplisit 128x32 piksel (4 RAM pages, 512 byte buffer) |
| **Tampilan OLED Runtime** | Mati/stuck di splash screen awal | Carousel dinamis 3-baris berganti data setiap 2 detik |
| **Proteksi Re-Init OLED** | Layar terhapus jika `Init` dipanggil ulang | Diproteksi dengan *guard flag* `SSD1306.Initialized` |
| **Simulasi Sabotase (Tamper)** | Tidak ada tombol hardware yang dimonitor | Tombol PC13 dimonitor secara debounced; memicu tamper alert |
| **Folder `/firmware/app`** | Asli | **Tetap 100% Asli (0 perubahan)** |
