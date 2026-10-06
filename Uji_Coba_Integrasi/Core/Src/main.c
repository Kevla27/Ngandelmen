/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rtos_tasks.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "display.h"
#include "stm32u5xx.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* E1 Metrology & ADE9000 Mock Includes */
#include "RegisterMap.h"
#include "Mock.h"
#include "Metrology.h"
#include "SPI.h"

/* E2 DLMS Protocol Includes */
#include "dlms_obis.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
#define UART_RX_BUF_SIZE 256

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
DMA_HandleTypeDef handle_GPDMA1_Channel1;
static uint8_t g_uart_rx_dma_buf[UART_RX_BUF_SIZE];
volatile bool g_tamper_alarm_active = false;
static volatile bool g_tamper_trigger_instant_display = false;
static volatile uint32_t g_tamper_press_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN 0 */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00C0EAFF; // Standard 100kHz untuk 160MHz SYSCLK
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPDMA1_Init(void) {
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    handle_GPDMA1_Channel1.Init.Request = GPDMA1_REQUEST_USART2_RX;
    handle_GPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_HIGH_PRIORITY;

    HAL_DMA_Init(&handle_GPDMA1_Channel1);

    __HAL_LINKDMA(&huart2, hdmarx, handle_GPDMA1_Channel1);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void dlms_uart_tx_callback(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)len, 1000);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (Size > 0 && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            rtos_dlms_task_notify_rx(g_uart_rx_dma_buf, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
    }
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
void vApplicationTickHook(void)
{
    /* HAL_IncTick dipanggil secara andal di SysTick_Handler */
}
/* ========================================================================== */
/*                E1 METROLOGY & ADE9000 INTEGRATION ADAPTER                  */
/* ========================================================================== */

static void init_metrology_e1(void)
{
    /* 1. Inisialisasi Mock Device ADE9000 & SPI Abstraction Layer E1 */
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();

    /* 2. Set Baseline Register Metrologi 3-Fasa (Data Simulasi E1) */
    /* PHASE A: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetAIRMS(5962491);
    ADE9000_Mock_SetAVRMS(17136895);
    ADE9000_Mock_SetAIFRMS(26347436);
    ADE9000_Mock_SetAWATT(636984);
    ADE9000_Mock_SetAPERIOD(20000);
    ADE9000_Mock_SetAWATTHR_HI(0x00000001);
    ADE9000_Mock_SetAWATTHR_LO(0x00001000);

    /* PHASE B: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetBIRMS(5962491);
    ADE9000_Mock_SetBVRMS(17136895);
    ADE9000_Mock_SetBWATT(636984);
    ADE9000_Mock_SetBPERIOD(20000);
    ADE9000_Mock_SetBWATTHR_HI(0x00000001);
    ADE9000_Mock_SetBWATTHR_LO(0x00001000);

    /* PHASE C: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetCIRMS(5962491);
    ADE9000_Mock_SetCVRMS(17136895);
    ADE9000_Mock_SetCWATT(636984);
    ADE9000_Mock_SetCPERIOD(20000);
    ADE9000_Mock_SetCWATTHR_HI(0x00000001);
    ADE9000_Mock_SetCWATTHR_LO(0x00001000);

    /* NEUTRAL: I=2.39A */
    ADE9000_Mock_SetNIRMS(1192498);
}

static void update_measurements_from_e1(meter_measurements_t *out_meas)
{
    if (out_meas == NULL) return;

    /* 1. Baca register RMS Tegangan & Arus via SPI Driver E1 */
    uint32_t airms = ADE9000_SPI_ReadRegister(ADE9000_AIRMS);
    uint32_t birms = ADE9000_SPI_ReadRegister(ADE9000_BIRMS);
    uint32_t cirms = ADE9000_SPI_ReadRegister(ADE9000_CIRMS);
    uint32_t nirms = ADE9000_SPI_ReadRegister(ADE9000_NIRMS);

    uint32_t avrms = ADE9000_SPI_ReadRegister(ADE9000_AVRMS);
    uint32_t bvrms = ADE9000_SPI_ReadRegister(ADE9000_BVRMS);
    uint32_t cvrms = ADE9000_SPI_ReadRegister(ADE9000_CVRMS);

    /* 2. Baca register Daya Aktif & Frekuensi (Period) */
    int32_t awatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_AWATT);
    int32_t bwatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BWATT);
    int32_t cwatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CWATT);

    uint32_t aperiod = ADE9000_SPI_ReadRegister(ADE9000_APERIOD);

    /* 3. Baca register Akumulasi Energi */
    uint32_t awatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_HI);
    uint32_t awatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_LO);
    uint32_t bwatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_HI);
    uint32_t bwatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_LO);
    uint32_t cwatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_HI);
    uint32_t cwatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_LO);

    /* 4. Konversi nilai mentah register ke unit engineering menggunakan library Metrology E1 */
    float va = Metrology_VoltageFromAVRMS(avrms);
    float vb = Metrology_VoltageFromBVRMS(bvrms);
    float vc = Metrology_VoltageFromCVRMS(cvrms);

    float ia = Metrology_CurrentFromAIRMS(airms);
    float ib = Metrology_CurrentFromBIRMS(birms);
    float ic = Metrology_CurrentFromCIRMS(cirms);
    float in = Metrology_CurrentFromNIRMS(nirms);

    float pa = Metrology_PowerFromAWATT(awatt);
    float pb = Metrology_PowerFromBWATT(bwatt);
    float pc = Metrology_PowerFromCWATT(cwatt);
    float p_total = pa + pb + pc;

    float freq = Metrology_FrequencyFromPeriod(aperiod);

    int64_t raw_energy_a = Metrology_CombineEnergyRegister(awatthr_hi, awatthr_lo);
    int64_t raw_energy_b = Metrology_CombineEnergyRegister(bwatthr_hi, bwatthr_lo);
    int64_t raw_energy_c = Metrology_CombineEnergyRegister(cwatthr_hi, cwatthr_lo);

    float ea = Metrology_EnergyFromRaw(raw_energy_a);
    float eb = Metrology_EnergyFromRaw(raw_energy_b);
    float ec = Metrology_EnergyFromRaw(raw_energy_c);
    float e_total_wh = ea + eb + ec;

    /* 5. Petakan ke struct meter_measurements_t (E3 Display & Profiling) */
    out_meas->voltage_r_dvolts = (uint32_t)(va * 10.0f);
    out_meas->voltage_s_dvolts = (uint32_t)(vb * 10.0f);
    out_meas->voltage_t_dvolts = (uint32_t)(vc * 10.0f);

    out_meas->current_r_mamps  = (uint32_t)(ia * 1000.0f);
    out_meas->current_s_mamps  = (uint32_t)(ib * 1000.0f);
    out_meas->current_t_mamps  = (uint32_t)(ic * 1000.0f);
    out_meas->current_n_mamps  = (uint32_t)(in * 1000.0f);

    out_meas->active_power_w   = (int32_t)p_total;
    out_meas->reactive_power_var = 0;
    out_meas->apparent_power_va  = (uint32_t)((va * ia) + (vb * ib) + (vc * ic));
    out_meas->power_factor_ppm   = (out_meas->apparent_power_va > 0) ? (uint16_t)((fabsf(p_total) / out_meas->apparent_power_va) * 1000.0f) : 1000;

    out_meas->frequency_mhz    = (uint16_t)(freq * 1000.0f);

    /* Simulasi akumulasi energi aktif yang terus bertambah seiring berjalannya meter */
    static uint32_t s_simulated_energy_increment = 0;
    s_simulated_energy_increment += 4; /* ~4 Wh tiap iterasi 2 detik (~7 kW) */
    out_meas->active_energy_wh = 125430ULL + (uint64_t)e_total_wh + s_simulated_energy_increment;
}

static void task_oled128x32_carousel(void *pvParameters)
{
    (void)pvParameters;
    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 2000);

    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));

    display_spln_frame_t spln_frame;

    uint32_t elapsed_accumulator_ms = 2000; /* Langsung render saat startup */

    for (;;) {
        bool need_render = false;

        /* Tangani event tombol darurat sabotase PC13 */
        if (g_tamper_trigger_instant_display) {
            g_tamper_trigger_instant_display = false;
            elapsed_accumulator_ms = 0;
            disp_ctx.alarm_icon_active = g_tamper_alarm_active;
            if (g_tamper_alarm_active) {
                disp_ctx.current_page = DISP_PAGE_TAMPER_ALARM;
            } else {
                disp_ctx.current_page = DISP_PAGE_IDPEL;
            }
            need_render = true;
        } else if (elapsed_accumulator_ms >= 2000) {
            elapsed_accumulator_ms = 0;
            display_process_tick(&disp_ctx, 2000);
            need_render = true;
        }

        if (need_render) {
            /* 1. Ambil snapshot data pengukuran terbaru secara aman (Thread-Safe) */
            if (rtos_meter_data_get_snapshot(&meas)) {
                display_update_measurements(&disp_ctx, &meas);
            }
            disp_ctx.alarm_icon_active = g_tamper_alarm_active;
            display_render_spln_frame(&disp_ctx, &spln_frame);

            /* 2. Render ke layar OLED 128x32 sesuai Format Standar PLN (Gambar 4) */
            ssd1306_Fill(Black);

            /* BARIS 1: Simbol & Kode OBIS (Font_6x8 di Y: 0..8) */
            if (spln_frame.is_alarm_active) {
                /* INDIKATOR ALARM PROMINEN (INVERTED SOLID BANNER):
                 * Kotak putih terang dengan teks hitam "!ALM" di pojok kiri atas */
                ssd1306_FillRectangle(0, 0, 25, 8, White);
                ssd1306_SetCursor(1, 0);
                ssd1306_WriteString("!ALM", Font_6x8, Black);
            } else {
                /* Status Normal: Teks "OK" standar warna putih */
                ssd1306_SetCursor(2, 0);
                ssd1306_WriteString("OK", Font_6x8, White);
            }

            /* Simbol Baterai [B] di X = 26 */
            ssd1306_SetCursor(26, 0);
            ssd1306_WriteString("[B]", Font_6x8, White);

            /* Indikator Fasa Tegangan L123 di X = 46 */
            ssd1306_SetCursor(46, 0);
            ssd1306_WriteString("L123", Font_6x8, White);

            /* Indikator Fasa Arus I123 di X = 72 */
            ssd1306_SetCursor(72, 0);
            ssd1306_WriteString("I123", Font_6x8, White);

            /* Kode Register OBIS rata kanan di X = 98..127 */
            ssd1306_SetCursor(98, 0);
            ssd1306_WriteString(spln_frame.obis_code, Font_6x8, White);

            /* Garis Pemisah Horizontal (Y = 10)
             * - Normal: garis putus-putus halus
             * - Alarm Aktif: garis tebal padat (solid bright alert bar) */
            if (spln_frame.is_alarm_active) {
                ssd1306_Line(0, 10, 127, 10, White);
            } else {
                for (uint8_t x = 0; x < 128; x += 2) {
                    ssd1306_DrawPixel(x, 10, White);
                }
            }

            /* BARIS 2 KIRI: zz (Indeks Urutan Scrolling, Font_6x8 di Y: 22) */
            ssd1306_SetCursor(1, 22);
            ssd1306_WriteString(spln_frame.scroll_index_zz, Font_6x8, White);

            /* BARIS 2 TENGAH: Nilai Angka Utama */
            if (spln_frame.is_large_font) {
                /* Nilai angka besar (Font_11x18 di Y: 13..31) */
                int val_len = (int)strlen(spln_frame.main_value);
                int x_val = 18 + (76 - (val_len * 11)) / 2;
                if (x_val < 16) x_val = 16;
                ssd1306_SetCursor((uint8_t)x_val, 13);
                ssd1306_WriteString(spln_frame.main_value, Font_11x18, White);
            } else {
                /* Teks khusus / IDPEL panjang / SABOTASE (Font_7x10 di Y: 17) */
                int val_len = (int)strlen(spln_frame.main_value);
                int x_val = 18 + (82 - (val_len * 7)) / 2;
                if (x_val < 16) x_val = 16;
                ssd1306_SetCursor((uint8_t)x_val, 17);
                ssd1306_WriteString(spln_frame.main_value, Font_7x10, White);
            }

            /* BARIS 2 KANAN: Satuan Besaran Listrik / Kode Error (Font_6x8 di Y: 22) */
            if (spln_frame.unit[0] != '\0') {
                int unit_len = (int)strlen(spln_frame.unit);
                int x_unit = 127 - (unit_len * 6);
                ssd1306_SetCursor((uint8_t)x_unit, 22);
                ssd1306_WriteString(spln_frame.unit, Font_6x8, White);
            }

            ssd1306_UpdateScreen();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_accumulator_ms += 100;
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  printf("\n==================================================\n");
  printf("   STM32U575VGT6 HARDWARE BRING-UP INITIALIZED    \n");
  printf("   Console UART: USART2 (PA2/PA3 @ 115200 bps)    \n");
  printf("   System Clock: 160 MHz | GPDMA1 RX ACTIVE       \n");
  printf("==================================================\n\n");

  /* 1. Inisialisasi Layar OLED & Splash Screen Awal */
  /* 1. Inisialisasi Layar OLED 128x32 & Splash Screen Awal */
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_SetCursor(11, 4);
  ssd1306_WriteString("STM32U575 START", Font_7x10, White);
  ssd1306_SetCursor(0, 16);
  ssd1306_SetCursor(11, 18);
  ssd1306_WriteString("Booting RTOS...", Font_7x10, White);
  ssd1306_UpdateScreen();

  /* 2. Inisialisasi Seluruh Subsystem E3, Queue, dan NVRAM Flash */
  init_metrology_e1();
  meter_measurements_t initial_meas;
  memset(&initial_meas, 0, sizeof(initial_meas));
  update_measurements_from_e1(&initial_meas);

  rtos_system_init();
  rtos_meter_data_publish(&initial_meas);
  rtos_metrology_set_sample_cb(update_measurements_from_e1);
  rtos_dlms_task_set_tx_cb(dlms_uart_tx_callback);

  /* 3. Pembuatan Seluruh Task FreeRTOS */
  if (xTaskCreate(task_tamper_emergency_entry, "TamperTask", STACK_SIZE_TAMPER, NULL, PRIORITY_TASK_TAMPER, NULL) != pdPASS) {
      printf("[ERROR] Gagal membuat TamperTask!\r\n");
  }
  if (xTaskCreate(task_metrology_profiling_entry, "ProfileTask", STACK_SIZE_PROFILING, NULL, PRIORITY_TASK_PROFILING, NULL) != pdPASS) {
      printf("[ERROR] Gagal membuat ProfileTask!\r\n");
  }
  if (xTaskCreate(task_dlms_entry, "DLMSTask", STACK_SIZE_DLMS, NULL, PRIORITY_TASK_DLMS, NULL) != pdPASS) {
      printf("[ERROR] Gagal membuat DLMSTask!\r\n");
  }
  if (xTaskCreate(task_oled128x32_carousel, "OLEDTask", 512, NULL, PRIORITY_TASK_UI, NULL) != pdPASS) {
      printf("[ERROR] Gagal membuat OLEDTask!\r\n");
  }

  printf("[RTOS] Memulai FreeRTOS Scheduler...\r\n");

  /* 4. Mulai Scheduler FreeRTOS */
  vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Jika scheduler keluar (misal kehabisan memory heap), program akan masuk ke sini */
  while (1)
  {
  }
}

void SystemClock_Config(void)
{
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLR = 2; /* 4MHz * 80 / 2 = 160 MHz */
  
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  huart2.Init.Mode = UART_MODE_TX_RX;
  
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  /* Aktifkan interupsi USART2 untuk mendeteksi event IDLE line saat frame DLMS selesai diterima */
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();

  /* I2C1 GPIO Configuration: PB8 -> SCL, PB9 -> SDA */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* User Button / Tamper Button: PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Enable Interrupt EXTI13 pada NVIC (Prioritas 6 di bawah configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5) */
  HAL_NVIC_SetPriority(EXTI13_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI13_IRQn);
}

/* Callback interupsi hardware EXTI saat tombol PC13 ditekan */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    /* Debounce software: minimal 250ms antar penekanan tombol */
    static uint32_t s_last_btn_tick = 0;
    uint32_t now = HAL_GetTick();
    if (now - s_last_btn_tick > 250)
    {
      s_last_btn_tick = now;
      g_tamper_alarm_active = !g_tamper_alarm_active;
      g_tamper_trigger_instant_display = true;
      g_tamper_press_count++;

      tamper_event_msg_t msg = {
          .timestamp = now / 1000,
          .tamper_code = 0x01, /* Case Open / Button Tamper Event */
          .is_active = g_tamper_alarm_active
      };
      rtos_queue_send_tamper_event_from_isr(&msg);
    }
  }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  (void)GPIO_Pin;
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}