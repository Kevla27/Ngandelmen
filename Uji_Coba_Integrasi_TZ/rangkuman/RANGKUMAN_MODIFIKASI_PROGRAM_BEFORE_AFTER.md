# Rangkuman Modifikasi Firmware: Analisis Perubahan Sebelum dan Sesudah (Before vs. After)

Dokumen ini mendokumentasikan secara rinci seluruh penambahan, perbaikan (*bugfix*), serta penyempurnaan firmware yang telah dilakukan pada proyek **Smart Meter 3-Fasa TrustZone (STM32U575ZIT6Q)**, mencakup pemulihan kestabilan *hardware bring-up*, pembenahan interupsi keamanan, dan penerapan **Prioritas 1 Kepatuhan Standar Layar SPLN D3.006: 2021 & SPLN Gambar 4**.

---

## 1. Matriks Ringkasan Perubahan Berkas

| No | Berkas yang Dimodifikasi | Sub-Sistem / Layer | Jenis Tindakan | Ringkasan Dampak Teknis |
|:--:|---|---|:---:|---|
| **1** | [`NonSecure/STM32U575xx_FLASH_ns.ld`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/STM32U575xx_FLASH_ns.ld) | Memory Map (Linker) | **Bugfix Kritis** | Mengembalikan `ROM ORIGIN` Non-Secure ke `0x08100000` (Bank 2 MCU 2MB) sehingga OLED dan UART kembali menyala normal. |
| **2** | [`Secure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure/Core/Src/main.c) | TrustZone / Secure Core | **Bugfix Hardware** | Mengonfigurasi PC13 tombol B1 ke `GPIO_NOPULL` dan `GPIO_MODE_IT_RISING` sesuai sirkuit Active-HIGH NUCLEO-U575ZI-Q. |
| **3** | [`firmware/app/tamper/src/tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/tamper/src/tamper_manager.c) | E3 Tamper Manager | **Bugfix FreeRTOS** | Menghilangkan pemanggilan `xQueueSendFromISR` langsung dari callback interupsi Secure NVIC untuk mencegah konflik `configASSERT`. |
| **4** | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/rtos_tasks.c) | E3 RTOS Tasks | **Penyempurnaan** | Mengirimkan pesan antrean darurat sabotase secara aman dari `task_tamper_emergency_entry` dan menambahkan stub CTest untuk PC runner. |
| **5** | [`NonSecure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c) | Non-Secure Core | **Fitur Baru** | Menambahkan trigger simulasi sabotase via terminal konsol serial USART1 VCP (tombol `'t'`, `'s'`, spasi). |
| **6** | [`NonSecure/Core/Src/stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/stm32u5xx_it.c) | Non-Secure NVIC | **Penyempurnaan** | Menambahkan handler `USART1_IRQHandler(&huart1)` untuk menangkap interupsi RX ST-LINK VCP. |
| **7** | [`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/inc/display.h) | E3 Display HMI | **Prioritas 1 SPLN** | Menambahkan halaman `DISP_PAGE_TIME` (Page 15) sebelum `DISP_PAGE_TAMPER_ALARM` dan field `current_time_epoch`. |
| **8** | [`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/src/display.c) | E3 Display HMI | **Prioritas 1 SPLN** | Memperbaiki format arus menjadi 3 desimal (`X.XXX A`) dan mengimplementasikan render halaman jam/waktu resmi SPLN (`01.00`). |
| **9** | [`firmware/app/display/src/display_task.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/src/display_task.c) | E3 Display Task | **Prioritas 1 SPLN** | Menyetel interval carousel otomatis menjadi minimal 5 detik (`5000 ms`) sesuai Pasal 6.11 SPLN D3.006. |
| **10** | [`tests/test_spln_display_tamper.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/tests/test_spln_display_tamper.c) | Test Suite | **Penyempurnaan** | Memperluas pengujian unit host untuk memvalidasi presisi arus 3 desimal, halaman jam ke-15, dan wrap siklus carousel. |
| **11** | [`firmware/drivers/bsp/inc/bsp_alarm_buzzer.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/inc/bsp_alarm_buzzer.h) | BSP Driver Layer | **Prioritas 2 SPLN** | Definisi interface hardware aktuasi Buzzer dan Indikator LED Alarm SPLN Butir 6.9 c. |
| **12** | [`firmware/drivers/bsp/src/bsp_alarm_buzzer.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/drivers/bsp/src/bsp_alarm_buzzer.c) | BSP Driver Layer | **Prioritas 2 SPLN** | Implementasi GPIO PB7 (Push-Pull Output) untuk target STM32 dan mock state untuk PC runner. |
| **13** | [`Secure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/Secure/Core/Src/main.c) | TrustZone Secure | **Prioritas 2 SPLN** | Mendelegasikan atribut pin `PB7` sebagai Non-Secure (`HAL_GPIO_ConfigPinAttributes`). |
| **14** | [`NonSecure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c) | Non-Secure Core | **Prioritas 2 SPLN** | Memanggil `bsp_alarm_buzzer_init()` pada `MX_GPIO_Init()`. |
| **15** | [`firmware/app/fsm/src/rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/rtos_tasks.c) | E3 RTOS Tasks | **Prioritas 2 SPLN** | Menambahkan aktuasi pola beeping 200 ms ON / 200 ms OFF saat alarm aktif dan auto-reset saat normal. |
| **16** | [`firmware/tests/app/test_rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/app/test_rtos_tasks.c) | Test Suite | **Prioritas 2 SPLN** | Menambahkan pengujian unit aktuasi pin PB7 (init OFF, set, toggle, reset). |
| **17** | [`firmware/app/fsm/inc/power_relay_manager.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/inc/power_relay_manager.h) | E3 FSM Layer | **Prioritas 3 SPLN** | Definisi FSM Relai, Deteksi Beban Asimetris (Tabel 5 No 11), Pembatas Daya (No 13), dan State "READY ON" (No 12). |
| **18** | [`firmware/app/fsm/src/power_relay_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/power_relay_manager.c) | E3 FSM Layer | **Prioritas 3 SPLN** | Implementasi kalkulasi deviasi arus $\Delta I_{\max}/I_{\text{avg}} > 30\%$, power limiter timer 5 detik, dan transisi FSM relai. |
| **19** | [`firmware/app/display/inc/display.h`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/inc/display.h) | E3 Display HMI | **Prioritas 3 SPLN** | Penambahan halaman khusus relai `DISP_PAGE_RELAY_READY_ON` (Page "on", OBIS `00.00`, teks `"READY ON"`, unit `"RECONNECT"`). |
| **20** | [`firmware/app/display/src/display.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/src/display.c) | E3 Display HMI | **Prioritas 3 SPLN** | Rendering frame layar `"READY ON"`, propagasi flag asimetri ke status alarm (`!ALM`), dan penanganan navigasi tombol. |
| **21** | [`firmware/app/display/src/display_task.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/display/src/display_task.c) | E3 Display Task | **Prioritas 3 SPLN** | Sinkronisasi status relai `READY_ON` ke LCD dan pemicu *instant display refresh* saat memasuki kondisi menunggu konfirmasi lokal. |
| **22** | [`NonSecure/Core/Src/main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c) | Non-Secure Core | **Prioritas 3 SPLN** | Penambahan kendali interaktif serial USART1 VCP: `'p'` (trip daya), `'r'` (remote reconnect), `'c'` (konfirmasi tombol lokal). |
| **23** | [`firmware/tests/app/test_power_relay.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/tests/app/test_power_relay.c) | Test Suite | **Prioritas 3 SPLN** | Unit test komprehensif untuk deteksi asimetri, durasi trip pembatas daya 5 detik, FSM "READY ON", dan integrasi display. |

---

## 2. Rincian Teknis Sebelum vs. Sesudah per Berkas

---

### A. `NonSecure/STM32U575xx_FLASH_ns.ld`
*Penyebab OLED & UART Mati Total Setelah Flashing*

#### 🔴 Kondisi Sebelum Diubah:
```ld
/* Memories definition */
MEMORY
{
  RAM   (xrw)   : ORIGIN = 0x20040000,  LENGTH = 512K     /* Non-Secure SRAM */
  ROM   (rx)    : ORIGIN = 0x08080000,  LENGTH = 1024K    /* ALAMAT BANK 2 SALAH UNTUK 2MB */
  SRAM4 (xrw)   : ORIGIN = 0x28000000,  LENGTH = 16K
}
```
* **Masalah:** Alamat `0x08080000` adalah alamat Bank 2 untuk STM32 kapasitas 1MB (seperti STM32U575VGT6). Sedangkan MCU target adalah **STM32U575ZIT6Q (2MB Flash)** di mana Secure World melompat ke `VTOR_TABLE_NS_START_ADDR = 0x08100000`. Akibatnya, CPU melompat ke sektor kosong (`0xFFFFFFFF`), seketika mengalami HardFault, dan OLED/UART tidak sempat hidup.

#### 🟢 Kode Setelah Diubah:
```ld
/* Memories definition */
MEMORY
{
  RAM   (xrw)   : ORIGIN = 0x20040000,  LENGTH = 512K     /* Non-Secure SRAM */
  ROM   (rx)    : ORIGIN = 0x08100000,  LENGTH = 1024K    /* Bank 2 Non-Secure Flash (2MB MCU) */
  SRAM4 (xrw)   : ORIGIN = 0x28000000,  LENGTH = 16K
}
```
* **Alasan Teknis:** Menyelaraskan alamat fisik flash binary Non-Secure dengan vektor lompat Secure World di Bank 2 (`0x08100000`).

---

### B. `Secure/Core/Src/main.c`
*Perbaikan Sirkuit Tombol Sabotase PC13 (Case Open)*

#### 🔴 Kondisi Sebelum Diubah:
```c
  /* Konfigurasi lama: GPIO_PULLUP */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
```
* **Masalah:** Pada board NUCLEO-U575ZI-Q, tombol biru B1 memiliki resistor eksternal 100 kΩ pull-down ke GND dan menghubungkan ke VDD saat ditekan (Active-HIGH). Internal pull-up 40 kΩ menyebabkan tegangan pin konstan berada di ~2.36 V (> 2.31 V $V_{IH}$ threshold), sehingga MCU mengira tombol ditekan terus menerus dan tidak ada transisi edge penekanan.

#### 🟢 Kode Setelah Diubah:
```c
  /* Reconfigure PC13 for Tamper Case Open:
   * Pada board NUCLEO-U575ZI-Q, tombol B1 menghubungkan PC13 ke VDD (Active HIGH)
   * dan memiliki rangkaian pull-down eksternal ke GND. Gunakan GPIO_NOPULL. */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Enable Interrupt EXTI13 pada NVIC (Prioritas 6 di bawah FreeRTOS max syscall priority 5) */
  HAL_NVIC_SetPriority(EXTI13_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI13_IRQn);
```
* **Alasan Teknis:** Menyesuaikan konfigurasi pin internal MCU dengan skematik hardware resmi Nucleo-144 agar pulsa penekanan tombol terdeteksi akurat.

---

### C. `firmware/app/tamper/src/tamper_manager.c` & `firmware/app/fsm/src/rtos_tasks.c`
*Pencegahan FreeRTOS Crash saat Interupsi Sabotase*

#### 🔴 Kondisi Sebelum Diubah:
```c
void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
    ...
    display_task_trigger_instant_refresh(s_alarm_active);

    tamper_event_msg_t msg = {
        .timestamp = timestamp_ms / 1000,
        .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
        .is_active = s_alarm_active
    };
    /* ERROR: Memanggil queue FreeRTOS Non-Secure dari konteks Secure ISR! */
    rtos_queue_send_tamper_event_from_isr(&msg);
}
```
* **Masalah:** Ketika interupsi EXTI13 terjadi di Secure World dan memanggil callback Non-Secure, pemanggilan `xQueueSendFromISR` memeriksa register prioritas interupsi Non-Secure (`pcInterruptPriorityRegisters`). Karena interupsi dimiliki Secure NVIC, FreeRTOS menganggap prioritas tidak sah sehingga memicu `configASSERT` dan CPU berhenti dalam loop `taskDISABLE_INTERRUPTS()`.

#### 🟢 Kode Setelah Diubah:
Di [`tamper_manager.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/tamper/src/tamper_manager.c):
```c
void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
    static uint32_t s_last_btn_tick = 0;
    if (timestamp_ms - s_last_btn_tick > 250) {
        s_last_btn_tick = timestamp_ms;
        static bool s_alarm_active = false;
        s_alarm_active = !s_alarm_active;

        /* Pemicu instant refresh layar OLED ke status !ALM / RUSAK secara atomik */
        display_task_trigger_instant_refresh(s_alarm_active);
    }
}
```
Di [`rtos_tasks.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/app/fsm/src/rtos_tasks.c):
```c
void task_tamper_emergency_entry(void *pvParameters) {
    (void)pvParameters;
    printf("[TAMPER] Task started. Menunggu event sabotase...\r\n");
    bool last_alarm_state = false;
    for (;;) {
        bool current_alarm = display_task_is_alarm_active();
        if (current_alarm != last_alarm_state) {
            last_alarm_state = current_alarm;
            if (current_alarm) {
                printf("[TAMPER] *** DARURAT! SABOTASE TERDETEKSI (CASE OPEN) ***\r\n");
            } else {
                printf("[TAMPER] Sabotase dipulihkan. Sistem kembali normal.\r\n");
            }
            tamper_event_msg_t msg = {
                .timestamp = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ),
                .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
                .is_active = current_alarm
            };
            /* Pengiriman pesan antrean aman dari Task Context FreeRTOS */
            rtos_queue_send_tamper_event(&msg);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```
* **Alasan Teknis:** Menghindari eksekusi antrean FreeRTOS langsung di dalam ISR cross-domain TrustZone. Pembaruan state didelegasikan secara bersih ke FreeRTOS Task.

---

### D. `NonSecure/Core/Src/main.c` & `NonSecure/Core/Src/stm32u5xx_it.c`
*Penambahan Konsol Simulasi Sabotase via UART ST-LINK VCP*

#### 🔴 Kondisi Sebelum Diubah:
USART1 hanya menangani transmisi TX untuk konsol log, tidak menerima input RX. Simulasi sabotase hanya bergantung pada tombol fisik PC13.

#### 🟢 Kode Setelah Diubah:
Di [`stm32u5xx_it.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/stm32u5xx_it.c):
```c
extern UART_HandleTypeDef huart1;

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
```
Di [`main.c`](file:///g:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/NonSecure/Core/Src/main.c):
```c
static uint8_t s_usart1_rx_char = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        /* Bila menerima karakter dari terminal serial ST-LINK VCP ('t', 'T', 's', 'S', atau Spasi) */
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
* **Alasan Teknis:** Mempermudah pengujian otomatis dan interaktif melalui terminal serial PC tanpa harus menekan tombol fisik di board.

---

### E. `firmware/app/display/inc/display.h`
*Penerapan Prioritas 1: Halaman Waktu & Penataan Enum Carousel*

#### 🔴 Kondisi Sebelum Diubah:
```c
typedef enum {
    DISP_PAGE_IDPEL = 0,
    ...
    DISP_PAGE_FREQUENCY,
    DISP_PAGE_ACTIVE_ENERGY,
    DISP_PAGE_TAMPER_ALARM,
    DISP_PAGE_TIME,               /* Ditaruh setelah TAMPER_ALARM */
    DISP_PAGE_COUNT
} display_page_id_t;
```

#### 🟢 Kode Setelah Diubah:
```c
typedef enum {
    DISP_PAGE_IDPEL = 0,
    DISP_PAGE_VOLTAGE_R,
    DISP_PAGE_VOLTAGE_S,
    DISP_PAGE_VOLTAGE_T,
    DISP_PAGE_CURRENT_R,
    DISP_PAGE_CURRENT_S,
    DISP_PAGE_CURRENT_T,
    DISP_PAGE_CURRENT_N,
    DISP_PAGE_ACTIVE_POWER,
    DISP_PAGE_REACTIVE_POWER,
    DISP_PAGE_APPARENT_POWER,
    DISP_PAGE_POWER_FACTOR,
    DISP_PAGE_FREQUENCY,
    DISP_PAGE_ACTIVE_ENERGY,
    DISP_PAGE_TIME,               /* Halaman 15 resmi sebelum halaman sabotase */
    DISP_PAGE_TAMPER_ALARM,       /* Halaman AL (hanya muncul saat alarm) */
    DISP_PAGE_COUNT
} display_page_id_t;

typedef struct {
    ...
    uint32_t         active_tamper_mask;  /* Bitmask tamper_vector_t (SPLN D3.006 Tabel 6) */
    uint32_t         active_alarm_mask;   /* Bitmask spln_internal_alarm_t (SPLN D3.006 Tabel 4) */
    uint32_t         current_time_epoch;  /* Timestamp Epoch UNIX / waktu RTC sesaat */
} display_context_t;

void display_set_time(display_context_t *ctx, uint32_t epoch_sec);
```
* **Alasan Teknis:** Menjadikan halaman waktu (`DISP_PAGE_TIME`) bagian integral dari 15 halaman carousel reguler dan menyediakan sinkronisasi waktu clock RTC.

---

### F. `firmware/app/display/src/display.c`
*Penerapan Prioritas 1: Format Arus 3 Desimal & Rendering Jam/Waktu SPLN*

#### 🔴 Kondisi Sebelum Diubah:
```c
        case DISP_PAGE_CURRENT_R:
            obis_code = "31.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "05");
            /* BUG: modulo dibagi 10 menyisakan 2 digit, terformat cacat seperti 11.094 A */
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000),
                     (unsigned long)((ctx->meas_buffer.current_r_mamps % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        ...
        case DISP_PAGE_TIME: {
            /* Blok kode masih kosong */
        }
```

#### 🟢 Kode Setelah Diubah:
```c
        case DISP_PAGE_CURRENT_R:
            obis_code = "31.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "05");
            /* Koreksi: modulo utuh menghasilkan 3 digit desimal presisi tinggi */
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000),
                     (unsigned long)(ctx->meas_buffer.current_r_mamps % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        /* (Berlaku serupa untuk CURRENT_S, CURRENT_T, dan CURRENT_N) */

        case DISP_PAGE_TIME: {
            obis_code = "01.00";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "15");
            frame->is_large_font = false; /* Font 7x10 agar muat teks waktu */

            uint32_t t = ctx->current_time_epoch;
            uint32_t sec = t % 60;
            uint32_t min = (t / 60) % 60;
            uint32_t hr  = (t / 3600) % 24;

            /* Format Jam:Menit:Detik sesuai standar SPLN Butir 6.11 & Tabel E.2 No 33 */
            snprintf(frame->main_value, sizeof(frame->main_value), "%02lu:%02lu:%02lu",
                     (unsigned long)hr, (unsigned long)min, (unsigned long)sec);
            snprintf(frame->unit, sizeof(frame->unit), "WIB");
            break;
        }
```
* **Alasan Teknis:** Memenuhi klausul Tabel E.1 Lampiran E (Arus fasa dan netral ditampilkan dengan 3 digit desimal) dan Butir 6.11 (Format jam `00.00` s.d `23.59`).

---

### G. `firmware/app/display/src/display_task.c`
*Penerapan Prioritas 1: Interval Carousel 5 Detik*

#### 🔴 Kondisi Sebelum Diubah:
```c
    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 2000); // 2000 ms (mode demo cepat)
    ...
    else if (elapsed_accumulator_ms >= 2000) {
        elapsed_accumulator_ms = 0;
        display_process_tick(&disp_ctx, 2000);
        need_render = true;
    }
```

#### 🟢 Kode Setelah Diubah:
```c
    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 5000); // 5000 ms (resmi SPLN Butir 6.11)
    ...
    else if (elapsed_accumulator_ms >= 5000) {
        elapsed_accumulator_ms = 0;
        display_process_tick(&disp_ctx, 5000);
        need_render = true;
    }
```
* **Alasan Teknis:** Memenuhi ketentuan formal SPLN D3.006 Butir 6.11 yang mensyaratkan selang waktu scrolling tampilan otomatis minimal 5 detik.

---

### H. `tests/test_spln_display_tamper.c`
*Pembaruan Unit Test Verifikasi HMI SPLN*

#### 🔴 Kondisi Sebelum Diubah:
Hanya memverifikasi Page 01 (IDPEL) dan Page 02 (Volt R), serta langsung menguji wrap dari Page 14 (Energy) ke Page 01.

#### 🟢 Kode Setelah Diubah:
```c
    /* Cek Halaman Arus R (3 Desimal sesuai SPLN Tabel E.1) */
    ctx.current_page = DISP_PAGE_CURRENT_R;
    ctx.meas_buffer.current_r_mamps = 11946; /* 11.946 A */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "05") == 0);
    assert(strcmp(frame.main_value, "11.946") == 0);
    assert(strcmp(frame.unit, "A") == 0);
    assert(strcmp(frame.obis_code, "31.07") == 0);

    /* Cek Halaman Jam dan Waktu (DISP_PAGE_TIME) - SPLN Butir 6.11 & Tabel E.2 No 33 */
    ctx.current_page = DISP_PAGE_TIME;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "15") == 0);
    assert(strcmp(frame.obis_code, "01.00") == 0);
    assert(strcmp(frame.unit, "WIB") == 0);

    /* Cek Carousel Wrap saat Normal: Dari Page 15 (Time) harus langsung ke Page 01 (IDPEL) */
    ctx.current_page = DISP_PAGE_TIME;
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_IDPEL);
```
* **Alasan Teknis:** Memastikan seluruh spesifikasi baru teruji secara otomatis melalui regresi CTest.

---

### G. Prioritas 2: Aktuasi Buzzer & Indikator LED Alarm SPLN (Pin PB7)
*Kepatuhan terhadap Standar SPLN D3.006: 2021 Butir 6.9 huruf c (Indikator LED Kuning Alarm/Tamper) dan Aktuasi Buzzer Audio*

#### 🔴 Kondisi Sebelum Diubah:
1. Tidak ada driver aktuasi fisik untuk buzzer maupun LED alarm. Saat terjadi sabotase (misalnya tombol PC13 ditekan atau simulasi `'t'`), indikasi hanya muncul berupa teks di konsol UART dan ikon `!ALM` di layar OLED.
2. Pin `PB7` (yang terhubung ke User LED2 Blue pada board NUCLEO-U575ZI-Q dan digunakan sebagai output penggerak buzzer/LED alarm) belum dialokasikan ke Non-Secure world di Secure Core TrustZone.
3. Di dalam task `task_tamper_emergency_entry` FreeRTOS, tidak ada kendali loop periodik untuk membunyikan buzzer / mengedipkan lampu peringatan.

#### 🟢 Kode Setelah Diubah:
1. **TrustZone Secure Core (`Secure/Core/Src/main.c`)**:
   Mendelegasikan pin `PB7` sebagai Non-Secure peripheral I/O:
   ```c
   /* IO attributes management functions */
   HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_NSEC);
   ```

2. **BSP Driver Layer (`firmware/drivers/bsp/inc/bsp_alarm_buzzer.h` & `bsp_alarm_buzzer.c`)**:
   Membuat modul driver hardware yang agnostik (mendukung HAL STM32U5 target dan mock simulasi untuk PC host test runner):
   ```c
   void bsp_alarm_buzzer_init(void) {
       __HAL_RCC_GPIOB_CLK_ENABLE();
       GPIO_InitTypeDef GPIO_InitStruct = {0};
       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
       GPIO_InitStruct.Pin = GPIO_PIN_7;
       GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
       HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   }
   void bsp_alarm_buzzer_set(bool on);
   void bsp_alarm_buzzer_toggle(void);
   bool bsp_alarm_buzzer_get(void);
   ```

3. **Non-Secure System Initialization (`NonSecure/Core/Src/main.c`)**:
   Menginisialisasi GPIO PB7 saat boot di `MX_GPIO_Init()`:
   ```c
   /* Inisialisasi PB7 untuk Buzzer & Indikator Alarm SPLN (LED2 Blue pada Nucleo) */
   bsp_alarm_buzzer_init();
   ```

4. **FreeRTOS Tamper Emergency Task (`firmware/app/fsm/src/rtos_tasks.c`)**:
   Mengimplementasikan pola beeping 200 ms ON / 200 ms OFF selama kondisi alarm aktif, dan memastikan buzzer/LED langsung dipadamkan (LOW) saat alarm dipulihkan:
   ```c
   void task_tamper_emergency_entry(void *pvParameters) {
       ...
       for (;;) {
           bool current_alarm = display_task_is_alarm_active();
           if (current_alarm != last_alarm_state) {
               last_alarm_state = current_alarm;
               if (current_alarm) {
                   printf("[TAMPER] *** DARURAT! SABOTASE TERDETEKSI (CASE OPEN) ***\r\n");
               } else {
                   printf("[TAMPER] Sabotase dipulihkan. Sistem kembali normal.\r\n");
                   bsp_alarm_buzzer_set(false);
               }
               ...
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

5. **Pengujian Unit Otomatis (`firmware/tests/app/test_rtos_tasks.c`)**:
   Memvalidasi status inisialisasi default (OFF), aktuasi set HIGH/LOW, dan fungsionalitas toggle:
   ```c
   /* Uji Aktuasi Buzzer & Indikator Alarm SPLN (PB7) */
   assert(bsp_alarm_buzzer_get() == false);
   bsp_alarm_buzzer_set(true);
   assert(bsp_alarm_buzzer_get() == true);
   bsp_alarm_buzzer_toggle();
   assert(bsp_alarm_buzzer_get() == false);
   bsp_alarm_buzzer_set(false);
   assert(bsp_alarm_buzzer_get() == false);
   ```

---

### H. Prioritas 3: Pengelola Daya & FSM Relai SPLN (Asimetri Beban, Pembatas Daya, & State "READY ON")
*Kepatuhan terhadap Standar SPLN D3.006: 2021 Tabel 5 No. 11, 12, dan 13*

#### 🔴 Kondisi Sebelum Diubah:
1. **Tidak Ada Deteksi Asimetri Beban (Tabel 5 No 11):** Beban tidak seimbang antar fasa R, S, T belum dihitung, sehingga ketidakseimbangan arus melebihi 30% tidak memunculkan indikasi peringatan pada LCD.
2. **Tidak Ada Timer Durasi Pembatas Daya (Tabel 5 No 13):** Kelebihan daya aktif di atas batas langganan (misal 6600 W) belum diakumulasikan durasinya (toleransi 5 detik) dan belum memutus relai pemutus beban secara otomatis.
3. **Penyambungan Relai Berbahaya (Tanpa "READY ON" Tabel 5 No 12):** Perintah remote reconnect dari AMI/HES berisiko langsung menutup kontak relai saat beban pelanggan masih menyala, melanggar klausul keselamatan SPLN yang mewajibkan konfirmasi tindakan lokal oleh pelanggan.
4. **Tidak Ada Halaman Khusus Layar untuk Relai:** Layar LCD belum memiliki halaman dengan kode indeks `on` dan teks `"READY ON"`.

#### 🟢 Kode Setelah Diubah:
1. **Modul Pengelola Relai & Daya (`firmware/app/fsm/inc/power_relay_manager.h` & `power_relay_manager.c`)**:
   Mengimplementasikan FSM relai tiga status (`CONNECTED`, `DISCONNECTED`, `READY_ON`):
   ```c
   typedef enum {
       RELAY_STATE_CONNECTED = 0,    /**< Relai tersambung (Normal) */
       RELAY_STATE_DISCONNECTED,     /**< Relai terputus (Trip / Remote Open) */
       RELAY_STATE_READY_ON          /**< Reconnect diterima, menunggu konfirmasi lokal (Tabel 5 No 12) */
   } relay_state_t;
   ```
   Formula evaluasi asimetri arus 3-fase ($> 30\%$ deviasi terhadap arus rata-rata):
   ```c
   bool power_relay_check_asymmetry(power_relay_context_t *ctx, uint32_t i_r_ma, uint32_t i_s_ma, uint32_t i_t_ma) {
       uint32_t i_avg = (i_r_ma + i_s_ma + i_t_ma) / 3;
       if (i_avg < 500) { /* Guard arus rendah < 500mA untuk cegah false alarm standby */
           ctx->is_asymmetry_active = false;
           return false;
       }
       uint32_t dev_r = (i_r_ma > i_avg) ? (i_r_ma - i_avg) : (i_avg - i_r_ma);
       uint32_t dev_s = (i_s_ma > i_avg) ? (i_s_ma - i_avg) : (i_avg - i_s_ma);
       uint32_t dev_t = (i_t_ma > i_avg) ? (i_t_ma - i_avg) : (i_avg - i_t_ma);
       uint32_t max_dev = dev_r;
       if (dev_s > max_dev) max_dev = dev_s;
       if (dev_t > max_dev) max_dev = dev_t;
       uint32_t dev_pct = (max_dev * 100) / i_avg;
       ctx->current_deviation_pct = dev_pct;
       ctx->is_asymmetry_active = (dev_pct >= ctx->asymmetry_threshold_pct);
       return ctx->is_asymmetry_active;
   }
   ```
   Evaluasi pembatas daya kontinu 5 detik (SPLN Tabel 5 No 13):
   ```c
   bool power_relay_process_power(power_relay_context_t *ctx, int32_t active_power_w, uint32_t timestamp) {
       if (active_power_w > (int32_t)ctx->power_limit_w) {
           ctx->over_power_timer_sec++;
           if (ctx->over_power_timer_sec >= ctx->over_power_duration_sec) {
               power_relay_disconnect(ctx, RELAY_REASON_POWER_LIMITER, timestamp);
               ctx->is_limiter_tripped = true;
               return true;
           }
       } else {
           ctx->over_power_timer_sec = 0;
       }
       return false;
   }
   ```
   Transisi Reconnect Dua Langkah (Two-Step Reconnect SPLN Tabel 5 No 12):
   ```c
   void power_relay_request_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
       if (ctx->relay_state == RELAY_STATE_DISCONNECTED) {
           ctx->relay_state = RELAY_STATE_READY_ON;
           ctx->last_reason = RELAY_REASON_REMOTE_RECONNECT;
       }
   }

   bool power_relay_confirm_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
       if (ctx->relay_state == RELAY_STATE_READY_ON) {
           ctx->relay_state = RELAY_STATE_CONNECTED;
           ctx->last_reason = RELAY_REASON_LOCAL_CONFIRM;
           ctx->is_limiter_tripped = false;
           return true;
       }
       return false;
   }
   ```

2. **Integrasi Layar HMI (`firmware/app/display/src/display.c` & `display_task.c`)**:
   - Menambahkan halaman `DISP_PAGE_RELAY_READY_ON` dengan layout resmi:
     - Indeks scroll `zz = "on"`.
     - Teks nilai utama `"READY ON"`.
     - Keterangan satuan `"RECONNECT"`.
     - Kode OBIS `00.00`.
   - Propagasi otomatis flag asimetri (`asymmetry_warning`) ke banner header `!ALM` pada halaman pengukuran.
   - Pemicuan *instant refresh* ke halaman `READY ON` ketika remote reconnect diterima.

3. **Sinkronisasi Periodik FreeRTOS (`firmware/app/fsm/src/rtos_tasks.c`)**:
   - `task_metrology_profiling_entry` (1 Hz): Mengevaluasi asimetri arus 3-fase dan memeriksa batas daya 6600 W. Jika terlampaui selama 5 detik berturut-turut, relai seketika diputus (`DISCONNECTED`).
   - `task_tamper_emergency_entry`: Mengatur kedipan lambat indikator LED/Buzzer PB7 (400 ms) saat dalam status `READY ON`.

4. **Kendali Konsol Serial Interaktif (`NonSecure/Core/Src/main.c`)**:
   - Menekan `'p'` / `'P'`: Menyimulasikan kelebihan daya untuk memicu trip relai seketika.
   - Menekan `'r'` / `'R'`: Menyimulasikan perintah remote reconnect dari HES $\rightarrow$ layar beralih ke `"READY ON"`.
   - Menekan `'c'` / `'C'`: Menyimulasikan konfirmasi penekanan tombol lokal di meter $\rightarrow$ relai tersambung kembali (`CONNECTED`).

5. **Pengujian Unit Otomatis (`firmware/tests/app/test_power_relay.c`)**:
   Memverifikasi 100% fungsionalitas asimetri, tripping power limiter 5s, dan transisi FSM relai.

---

## 3. Hasil Audit Kompilasi & Pengujian

### A. Uji CTest Host (`ctest --test-dir firmware/build`)
```text
Test project G:/PPPP/Day_Sekian/Uji_Coba_Integrasi_TZ/firmware/build
      Start  1: test_dlms_association
 1/16 Test  #1: test_dlms_association ............   Passed    0.08 sec
      Start  2: test_dlms_apdu
 2/16 Test  #2: test_dlms_apdu ...................   Passed    0.08 sec
      Start  3: test_dlms_integration
 3/16 Test  #3: test_dlms_integration ............   Passed    0.08 sec
      Start  4: test_dlms_tamper_log
 4/16 Test  #4: test_dlms_tamper_log .............   Passed    0.09 sec
      Start  5: test_dlms_hdlc
 5/16 Test  #5: test_dlms_hdlc ...................   Passed    0.08 sec
      Start  6: test_dlms_server
 6/16 Test  #6: test_dlms_server .................   Passed    0.07 sec
      Start  7: test_dlms_task
 7/16 Test  #7: test_dlms_task ...................   Passed    0.08 sec
      Start  8: test_dlms_nvm
 8/16 Test  #8: test_dlms_nvm ....................   Passed    0.07 sec
      Start  9: test_bsp_lpuart
 9/16 Test  #9: test_bsp_lpuart ..................   Passed    0.08 sec
      Start 10: test_e3_app
10/16 Test #10: test_e3_app ......................   Passed    0.08 sec
      Start 11: test_fsm_statechart
11/16 Test #11: test_fsm_statechart ..............   Passed    0.08 sec
      Start 12: test_load_profile
12/16 Test #12: test_load_profile ................   Passed    0.08 sec
      Start 13: test_load_profile_dlms_adapter
13/16 Test #13: test_load_profile_dlms_adapter ...   Passed    0.08 sec
      Start 14: test_nvram_storage
14/16 Test #14: test_nvram_storage ...............   Passed    0.08 sec
      Start 15: test_rtos_tasks
15/16 Test #15: test_rtos_tasks ..................   Passed    0.08 sec
      Start 16: test_power_relay
16/16 Test #16: test_power_relay .................   Passed    0.07 sec

100% tests passed, 0 tests failed out of 16
Total Test time (real) = 1.30 sec
```

* **Hasil Eksekusi Unit Test Prioritas 3 (`test_power_relay.exe`):**
  ```text
  ====================================================================
    TEST VERIFIKASI PRIORITAS 3: POWER & RELAY FSM (SPLN D3.006)       
  ====================================================================

  [TEST] Testing Asymmetry Power Detection (SPLN Tabel 5 No 11)...
     10A/10A/10A -> Asymmetry = 0%, Active = 0 [PASS]
     300mA/100mA/200mA -> Low current guard active, Active = 0 [PASS]
     10A/9A/8A -> Asymmetry = 11% (< 30%), Active = 0 [PASS]
     10A/5A/5A -> Asymmetry = 50% (>= 30%), Active = 1 [PASS]
     Pulih normal -> Asymmetry = 0%, Active = 0 [PASS]

  [TEST] Testing Power Limiter Duration Tripping (SPLN Tabel 5 No 13)...
     5000 W selama 10s -> Tidak Trip [PASS]
     7500 W selama 4s -> Akumulasi timer 4 detik, belum trip [PASS]
     Detik ke-5 -> RELAY TRIP! State = DISCONNECTED, Reason = 2 [PASS]

  [TEST] Testing Relay FSM & READY ON State (SPLN Tabel 5 No 12)...
     Relay diputus -> State = DISCONNECTED [PASS]
     Remote Reconnect Request -> State = READY ON (Tabel 5 No 12) [PASS]
     Konfirmasi Tombol Lokal -> State = CONNECTED [PASS]
     Konfirmasi ulang saat CONNECTED ditolak dengan aman [PASS]

  [TEST] Testing Display Integration for READY ON & Asymmetry...
     Page 'on': zz='on' | Val='READY ON' | Unit='RECONNECT' | OBIS='00.00' [PASS]
     Asymmetry Warning -> Banner Alarm '!ALM' aktif di halaman pengukuran [PASS]

  ====================================================================
    ALL PRIORITAS 3 TESTS PASSED SUCCESSFULLY! (100%)                 
  ====================================================================
  ```

### B. Uji Standar Layar SPLN Gambar 4 (`tests/test_spln_display_tamper.exe`)
```text
====================================================================
  TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & KODE ERROR SPLN D3.006   
====================================================================

[UJI 1] Verifikasi Tampilan Normal (alarm_icon_active = false)... [PASS]
[UJI 2] Verifikasi Tampilan Alarm Sabotase Case Open (Tombol PC13)... [PASS]
[UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)... [PASS]
[UJI 4] Verifikasi Seluruh 9 Vektor Sabotase SPLN D3.006: 2021 Tabel 6... [PASS]
[UJI 5] Verifikasi Seluruh 8 Alarm Hardware Internal SPLN D3.006: 2021 Tabel 4... [PASS]
[UJI 6] Verifikasi Format Tampilan ASCII CLI (display_render_frame)... [PASS]

====================================================================
  HASIL: 100% SELURUH KODE ERROR SPLN D3.006 TABEL 4 & 6 LULUS!      
====================================================================
```

### C. Uji Integrasi Host E1 -> E3 -> E2 (`tests/test_e1_to_e2_integration.exe`)
```text
========================================================================
   HOST INTEGRATION TEST: E1 (METROLOGY) -> E3 (MEAS) -> E2 (DLMS/OBIS) 
========================================================================
  [4A] Mengirim Frame HDLC SNRM ... [PASS]
  [4B] Mengirim Frame DLMS AARQ ... [PASS]
  [4C] GET-Request: OBIS 1.0.32.7.0.255 (Tegangan Fasa A) ... [PASS]
  [4D] GET-Request: OBIS 1.0.31.7.0.255 (Arus Fasa A) ... [PASS]
  [4E] GET-Request: OBIS 1.0.1.8.0.255 (Total Energi Aktif) ... [PASS]
  [4F] Injeksi Event Tamper & GET-Request OBIS 0.0.99.98.0.255 ... [PASS]
  [4G] Mengirim Frame HDLC DISC (Disconnect) ... [PASS]

========================================================================
   SELURUH RANGKAIAN TEST INTEGRASI E1 -> E3 -> E2: 100% BERHASIL!      
========================================================================
```

### D. Kompilasi Target STM32U575 (ARM GCC)
* **Non-Secure Target (`Uji_Coba_Integrasi_TZ_NS.hex`):**
  * `ROM`: 86.952 Bytes / 1.024 KB (**8,29%**)
  * `RAM`: 81.104 Bytes / 512 KB (**15,47%**)
  * Status: **0 Error, 0 Warning** (Clean Build).
* **Secure Target (`Uji_Coba_Integrasi_TZ_S.hex`):**
  * `ROM`: 19.008 Bytes / 504 KB (**3,68%**)
  * `RAM`: 1.608 Bytes / 256 KB (**0,61%**)
  * Status: **0 Error, 0 Warning** (Clean Build).

---

## 4. Kesimpulan Kesiapan Sistem

Dengan diselesaikannya implementasi **Prioritas 1**, **Prioritas 2**, dan **Prioritas 3**:
1. **Perilaku Hardware Stabil:** Masalah tampilan OLED dan UART mati telah terselesaikan secara tuntas melalui pembagian memori flash Bank 2 yang tepat (`0x08100000`).
2. **Kepatuhan HMI SPLN (Prioritas 1):** Parameter arus berpresisi 3 desimal (`X.XXX A`), halaman waktu `01.00` tampil pada urutan ke-15, dan interval pergeseran layar memenuhi batas minimal 5 detik.
3. **Aktuasi Alarm SPLN (Prioritas 2):** Pin `PB7` telah didelegasikan secara aman melalui TrustZone, dikendalikan oleh modul BSP khusus, dan digerakkan dengan pola beeping 200 ms ON / 200 ms OFF di dalam task darurat FreeRTOS saat alarm sabotase aktif.
4. **FSM Relai & Pengelola Daya SPLN (Prioritas 3):** Deteksi beban asimetris $> 30\%$ telah aktif, pembatas daya 6600 W dengan toleransi 5 detik memutus relai secara otomatis, dan transisi penyambungan kembali relai memenuhi standar keselamatan "READY ON" dua langkah dengan konfirmasi tombol lokal.
5. **Firmware Siap Diuji Hardware:** Kedua citra biner (`Secure` dan `NonSecure`) terkompilasi bersih tanpa *warning*, seluruh 16 pengujian unit CTest serta uji integrasi end-to-end lulus 100%.


