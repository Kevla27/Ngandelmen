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
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "task.h"
#include "queue.h"
#include "rtos_tasks.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "display.h"
#include "stm32u5xx.h"
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

int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
static void task_oled128x32_carousel(void *pvParameters)
{
    (void)pvParameters;
    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 2000);

    /* Simulasi pengukuran meter untuk visualisasi OLED 128x32 */
    meter_measurements_t meas = {
        .voltage_r_dvolts = 2205,    /* 220.5 V */
        .voltage_s_dvolts = 2212,    /* 221.2 V */
        .voltage_t_dvolts = 2198,    /* 219.8 V */
        .current_r_mamps  = 5420,    /* 5.420 A */
        .active_power_w   = 1195,    /* 1.195 kW */
        .frequency_mhz    = 50000    /* 50.00 Hz */
    };
    display_update_measurements(&disp_ctx, &meas);

    char l1[32], l2[32], l3[32];

    for (;;) {
        display_process_tick(&disp_ctx, 2000);
        display_render_frame(&disp_ctx, l1, l2, l3, sizeof(l1));

        ssd1306_Fill(Black);
        /* Format 3 Baris Proporsional untuk Layar OLED 128x32 */
        ssd1306_SetCursor(0, 1);
        ssd1306_WriteString(l1, Font_6x8, White);
        ssd1306_SetCursor(0, 11);
        ssd1306_WriteString(l2, Font_6x8, White);
        ssd1306_SetCursor(0, 21);
        ssd1306_WriteString(l3, Font_7x10, White);
        ssd1306_UpdateScreen();

        vTaskDelay(pdMS_TO_TICKS(2000));
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
  rtos_system_init();

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
  huart2.Init.Mode = UART_MODE_TX_RX;
  
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

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
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Enable Interrupt EXTI13 pada NVIC (Prioritas 6 di bawah configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5) */
  HAL_NVIC_SetPriority(EXTI13_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI13_IRQn);
}

/* Callback interupsi hardware EXTI saat tombol PC13 ditekan/dilepas */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    tamper_event_msg_t msg = {
        .timestamp = HAL_GetTick() / 1000,
        .tamper_code = 0x01, /* Case Open / Button Tamper Event */
        .is_active = true
    };
    rtos_queue_send_tamper_event(&msg);
  }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
    tamper_event_msg_t msg = {
        .timestamp = HAL_GetTick() / 1000,
        .tamper_code = 0x01, /* Case Open / Button Tamper Event */
        .is_active = true
    };
    rtos_queue_send_tamper_event(&msg);
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}