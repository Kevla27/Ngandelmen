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
#include "rtos_tasks.h"
#include "display_task.h"
#include "bsp_display_oled.h"
#include "console_io.h"
#include "tamper_manager.h"
#include "metrology_adapter.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
#define UART_RX_BUF_SIZE 256

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
DMA_HandleTypeDef handle_GPDMA1_Channel1;
static uint8_t g_uart_rx_dma_buf[UART_RX_BUF_SIZE];
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

/* Callback Adapter Hardware Transmisi Konsol Universal E3 (Agnostik) */
static void console_uart_transmit_bsp(const uint8_t *data, size_t len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)data, (uint16_t)len, 100);
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
  /* 1. Inisialisasi Konsol Universal E3 & Cetak Banner Hardware Bring-Up */
  console_io_init(console_uart_transmit_bsp);
  console_io_print_boot_banner("STM32U575VGT6 HARDWARE BRING-UP INITIALIZED", "USART2 (PA2/PA3 @ 115200 bps)", 160);

  /* 2. Inisialisasi Layar Display (Dependency Injection Driver OLED 128x32) */
  display_task_set_driver(bsp_display_oled_get_driver());
  display_task_show_boot_screen("STM32U575 START", "Booting RTOS...");

  /* 3. Inisialisasi Subsystem Metrologi E1, RTOS IPC, dan Callback */
  metrology_adapter_init();
  meter_measurements_t initial_meas;
  memset(&initial_meas, 0, sizeof(initial_meas));
  metrology_adapter_sample(&initial_meas);

  rtos_system_init();
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
    tamper_handle_button_press_isr(HAL_GetTick());
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