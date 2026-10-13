/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_tasks.h"
#include "display_task.h"
#include "bsp_display_oled.h"
#include "console_io.h"
#include "tamper_manager.h"
#include "metrology_adapter.h"
#include "secure_nsc.h"
#include "bsp_alarm_buzzer.h"
#include "bsp_pvd.h"
#include "bsp_ade9000_irq.h"
#include "bsp_bringup_test.h"
#include "bsp_rtc_tamp.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_BUF_SIZE 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;
DMA_NodeTypeDef Node_GPDMA1_Channel1;
DMA_QListTypeDef List_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint8_t g_uart_rx_dma_buf[UART_RX_BUF_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_I2C1_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Callback Adapter Hardware Transmisi Konsol Universal E3 (Agnostik) */
static void console_uart_transmit_bsp(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)data, (uint16_t)len, 100);
    if (huart1.Instance != NULL) {
        HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, 100);
    }
}

static void dlms_uart_tx_callback(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)data, (uint16_t)len, 1000);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *hlpuart, uint16_t Size) {
    if (hlpuart->Instance == LPUART1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (Size > 0 && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            rtos_dlms_task_notify_rx(g_uart_rx_dma_buf, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

static uint8_t s_usart1_rx_char = 0;

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == LPUART1) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
    } else if (huart->Instance == USART1) {
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        HAL_UART_Receive_IT(&huart1, &s_usart1_rx_char, 1);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        /* Bila menerima karakter dari terminal serial ST-LINK VCP */
        if (s_usart1_rx_char == 't' || s_usart1_rx_char == 'T' ||
            s_usart1_rx_char == 's' || s_usart1_rx_char == 'S') {
            printf("\r\n[SIMULASI] Toggle Sabotase PC13 (Case Open RUSAK) via konsol UART!\r\n");
            tamper_handle_button_press_isr(HAL_GetTick());
        } else if (s_usart1_rx_char == 'l' || s_usart1_rx_char == 'L') {
            rtos_tamper_dump_log();
        } else if (s_usart1_rx_char == '1') {
            printf("\r\n[SIMULASI] Sabotase Tutup Terminal Terbuka (ERR20) via konsol UART!\r\n");
            rtos_tamper_record_event(DLMS_TAMPER_TERMINAL_COVER_OPEN, true);
        } else if (s_usart1_rx_char == '2') {
            printf("\r\n[SIMULASI] Sabotase Induksi Medan Magnet (ERR25) via konsol UART!\r\n");
            rtos_tamper_record_event(DLMS_TAMPER_MAGNETIC_INDUCTION, true);
        } else if (s_usart1_rx_char == '3') {
            printf("\r\n[SIMULASI] Sabotase Arus Balik / Reverse Power (ERR26) via konsol UART!\r\n");
            rtos_tamper_record_event(DLMS_TAMPER_REVERSE_CURRENT, true);
        } else if (s_usart1_rx_char == '4') {
            printf("\r\n[SIMULASI] Sabotase Kawat Netral Hilang (ERR23) via konsol UART!\r\n");
            rtos_tamper_record_event(DLMS_TAMPER_MISSING_NEUTRAL, true);
        } else if (s_usart1_rx_char == '5') {
            printf("\r\n[SIMULASI] Injeksi HW ADE9000 IRQ0: Reverse Power Tamper (ERR26)!\r\n");
            bsp_ade9000_irq_simulate_trigger(0, ADE9000_STATUS0_REV_ALL);
        } else if (s_usart1_rx_char == '6') {
            printf("\r\n[SIMULASI] Injeksi HW ADE9000 IRQ1: Missing Neutral Tamper (ERR23)!\r\n");
            bsp_ade9000_irq_simulate_trigger(1, ADE9000_STATUS1_MISNEUT);
        } else if (s_usart1_rx_char == '7') {
            printf("\r\n[SIMULASI] Injeksi HW ADE9000 IRQ1: Hilang Tegangan Fase / Sag (ERR24)!\r\n");
            bsp_ade9000_irq_simulate_trigger(1, ADE9000_STATUS1_SAG);
        } else if (s_usart1_rx_char == '8') {
            printf("\r\n[SIMULASI] Injeksi HW ADE9000 IRQ0: Urutan Fase Terbalik (ERR21)!\r\n");
            bsp_ade9000_irq_simulate_trigger(0, ADE9000_STATUS0_PWRORD);
        } else if (s_usart1_rx_char == '0') {
            printf("\r\n[SIMULASI] Pulihkan Status Sabotase ke Normal via konsol UART!\r\n");
            rtos_tamper_record_event(DLMS_TAMPER_TERMINAL_COVER_OPEN, false);
            rtos_tamper_record_event(DLMS_TAMPER_MAGNETIC_INDUCTION, false);
            rtos_tamper_record_event(DLMS_TAMPER_REVERSE_CURRENT, false);
            rtos_tamper_record_event(DLMS_TAMPER_MISSING_NEUTRAL, false);
            rtos_tamper_record_event(DLMS_TAMPER_MISSING_VOLTAGE, false);
            rtos_tamper_record_event(DLMS_TAMPER_WRONG_PHASE_SEQ, false);
        } else if (s_usart1_rx_char == 'p' || s_usart1_rx_char == 'P') {
            printf("\r\n[SIMULASI] Injeksi Beban Berlebih (Power Limiter Trip) via konsol UART!\r\n");
            rtos_power_relay_simulate_power_trip();
        } else if (s_usart1_rx_char == 'r' || s_usart1_rx_char == 'R') {
            printf("\r\n[SIMULASI] Perintah Remote Reconnect DLMS diterima via konsol UART!\r\n");
            rtos_power_relay_request_reconnect();
        } else if (s_usart1_rx_char == 'c' || s_usart1_rx_char == 'C') {
            printf("\r\n[SIMULASI] Aksi Tombol Lokal Konfirmasi Reconnect via konsol UART!\r\n");
            rtos_power_relay_confirm_reconnect();
        } else if (s_usart1_rx_char == 'v') {
            printf("\r\n[SIMULASI] Penurunan Catu Daya PVD (Last-Gasp Sequence) via konsol UART!\r\n");
            bsp_pvd_simulate_power_fail();
        } else if (s_usart1_rx_char == 'V') {
            bsp_pvd_simulate_power_restore();
        } else if (s_usart1_rx_char == 'b' || s_usart1_rx_char == 'B') {
            bsp_bringup_print_menu();
        } else if (s_usart1_rx_char == 'i' || s_usart1_rx_char == 'I') {
            bsp_bringup_test_i2c_bus();
        } else if (s_usart1_rx_char == 'm' || s_usart1_rx_char == 'M') {
            bsp_bringup_test_ade9000_comm();
        } else if (s_usart1_rx_char == 'a' || s_usart1_rx_char == 'A') {
            bsp_bringup_test_gpio_actuators();
        } else if (s_usart1_rx_char == 'd' || s_usart1_rx_char == 'D') {
            bsp_bringup_run_all(NULL);
        } else if (s_usart1_rx_char == 'n' || s_usart1_rx_char == 'N') {
            bsp_bringup_test_nvram_storage();
        } else if (s_usart1_rx_char == 'o' || s_usart1_rx_char == 'O') {
            printf("\r\n[SIMULASI] Injeksi Sabotase Tutup Meter Saat Listrik Mati (Offline Tamper SPLN)!\r\n");
            bsp_rtc_tamp_simulate_offline_tamper(DLMS_TAMPER_METER_COVER_OPEN, HAL_GetTick() / 1000);
            bsp_rtc_tamp_process_offline_events();
        } else if (s_usart1_rx_char == 'h' || s_usart1_rx_char == 'H' || s_usart1_rx_char == '?') {
            printf("\r\n======================================================\r\n");
            printf("             KONSOL PERINTAH UJI COBA UART            \r\n");
            printf("======================================================\r\n");
            printf(" [b]   : Buka Menu Diagnostik Hardware Bring-Up Spin 1\r\n");
            printf(" [d]   : Jalankan Full Built-In Self-Test (BIST All)  \r\n");
            printf(" [i]   : Pemindaian Bus I2C (I2C Address Scanner)     \r\n");
            printf(" [m]   : Uji Integritas Komunikasi SPI ADE9000        \r\n");
            printf(" [a]   : Uji Aktuator Buzzer PB7 & Relai PC8/PC9      \r\n");
            printf(" [t/s] : Toggle Sabotase Tutup Meter (RUSAK / PC13)   \r\n");
            printf(" [o]   : Simulasi Sabotase Tutup Meter Saat Padam     \r\n");
            printf(" [1]   : Trigger Tutup Terminal Dibuka (ERR20)        \r\n");
            printf(" [2]   : Trigger Induksi Medan Magnet >500mT (ERR25)  \r\n");
            printf(" [3]   : Trigger Arus Balik / Reverse Power (ERR26)   \r\n");
            printf(" [4]   : Trigger Kawat Netral Hilang / Putus (ERR23)  \r\n");
            printf(" [5]   : Simulasi HW ADE9000 IRQ0 - Reverse (ERR26)   \r\n");
            printf(" [6]   : Simulasi HW ADE9000 IRQ1 - Netral (ERR23)    \r\n");
            printf(" [7]   : Simulasi HW ADE9000 IRQ1 - Hilang V (ERR24)  \r\n");
            printf(" [8]   : Simulasi HW ADE9000 IRQ0 - Urutan F (ERR21)  \r\n");
            printf(" [0]   : Pulihkan Sabotase ke Normal (Status: Pulih)  \r\n");
            printf(" [l]   : Tampilkan Daftar Rekaman Tamper Log (Dump)   \r\n");
            printf(" [p]   : Simulasi Beban Berlebih (Power Limiter Trip) \r\n");
            printf(" [r]   : Remote Reconnect DLMS (Menuju READY ON)      \r\n");
            printf(" [c]   : Konfirmasi Tombol Lokal Reconnect Relai      \r\n");
            printf(" [v]   : Simulasi PVD Power-Fail (Last-Gasp Flash Save)\r\n");
            printf(" [V]   : Pulihkan Status Catu Daya Normal (Vdd OK)    \r\n");
            printf(" [h/?] : Tampilkan Menu Bantuan Ini                   \r\n");
            printf("======================================================\r\n\r\n");
        }
        HAL_UART_Receive_IT(&huart1, &s_usart1_rx_char, 1);
    }
}

/* Callback interupsi Secure World saat terjadi sabotase fisik PC13 (Case Open) */
static void NonSecure_Tamper_Callback(void) {
    printf("[TAMPER ISR] Interupsi PC13 Secure World diteruskan ke Non-Secure!\r\n");
    tamper_handle_button_press_isr(HAL_GetTick());
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 0. Registrasi Callback Sabotase Fisik PC13 (Case Open) ke Secure World via NSC */
  HAL_NVIC_SetPriority(EXTI13_IRQn, 6, 0);
  SECURE_RegisterCallback(SECURE_TAMPER_CB_ID, (void *)NonSecure_Tamper_Callback);

  /* 1. Inisialisasi Konsol Universal E3 & Cetak Banner Hardware Bring-Up */
  console_io_init(console_uart_transmit_bsp);
  console_io_print_boot_banner("STM32U575 TRUSTZONE SMART METER INITIALIZED", "ST-LINK VCP USART1 & LPUART1 @ 115200", 160);
  printf("Ketik 'h' atau '?' pada terminal serial untuk melihat menu uji coba interaktif (Tamper & Log).\r\n");

  /* 2. Inisialisasi Layar Display (Dependency Injection Driver OLED 128x32) */
  display_task_set_driver(bsp_display_oled_get_driver());
  display_task_show_boot_screen("STM32U575 START", "Booting RTOS...");

  /* 3. Inisialisasi Subsystem Metrologi E1, RTOS IPC, dan Callback */
  metrology_adapter_init();
  meter_measurements_t initial_meas;
  memset(&initial_meas, 0, sizeof(initial_meas));
  metrology_adapter_sample(&initial_meas);

  rtos_system_init();
  bsp_pvd_init();
  bsp_ade9000_irq_init();
  bsp_bringup_init();
  bsp_rtc_tamp_init();
  bsp_rtc_tamp_process_offline_events();
  rtos_meter_data_publish(&initial_meas);
  rtos_metrology_set_sample_cb(metrology_adapter_sample);
  rtos_dlms_task_set_tx_cb(dlms_uart_tx_callback);

  /* 4. Luncurkan Seluruh Task FreeRTOS (Tamper, Profiling, DLMS, OLED) */
  if (!rtos_start_all_tasks()) {
      printf("[ERROR] Gagal membuat sebagian FreeRTOS task!\r\n");
  }

  printf("[RTOS] Memulai FreeRTOS Scheduler...\r\n");

  /* 5. Mulai Scheduler FreeRTOS */
  vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  /* Jam sistem dan pengatur tegangan (VOS1 160MHz) sudah dikonfigurasi penuh oleh Secure world.
     Di Non-Secure cukup update SystemCoreClock untuk kalkulasi SysTick, peripheral, dan RTOS. */
  SystemCoreClockUpdate();
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
  HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */
  handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
  handle_GPDMA1_Channel1.Init.Request = GPDMA1_REQUEST_LPUART1_RX;
  handle_GPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_FIXED;
  handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_INCREMENTED;
  handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
  handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
  handle_GPDMA1_Channel1.Init.Priority = DMA_HIGH_PRIORITY;

  HAL_DMA_Init(&handle_GPDMA1_Channel1);

  __HAL_LINKDMA(&hlpuart1, hdmarx, handle_GPDMA1_Channel1);
  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */
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
  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x3042C7C7; /* True 100 kHz Standard Mode untuk 160 MHz PCLK1 */
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

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */
  HAL_NVIC_SetPriority(LPUART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(LPUART1_IRQn);

  HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART1 Initialization Function (Connected to ST-LINK VCP on PA9/PA10)
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_UART_Receive_IT(&huart1, &s_usart1_rx_char, 1);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Inisialisasi PB7 untuk Buzzer & Indikator Alarm SPLN (LED2 Blue pada Nucleo) */
  bsp_alarm_buzzer_init();
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
