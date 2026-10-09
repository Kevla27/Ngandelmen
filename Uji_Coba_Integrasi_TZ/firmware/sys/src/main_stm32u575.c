/**
 * @file main_stm32u575.c
 * @brief Hardware Bring-Up & USART2 (PA2/PA3) DMA Integration (STM32U575VGT6)
 */

#include "stm32u5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rtos_tasks.h"
#include <stdio.h>
#include <string.h>

#define UART_RX_BUF_SIZE 256

/* Handle Periferal USART2 & GPDMA1 */
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* Buffer Penerimaan DMA Serial (Circular RAM Buffer) */
static uint8_t g_uart_rx_dma_buf[UART_RX_BUF_SIZE];

/**
 * @brief Inisialisasi System Clock 160 MHz (MSI + PLL)
 */
static void SystemClock_Config_160MHz(void) {
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

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

/**
 * @brief Inisialisasi GPDMA1 Channel 0 untuk USART2 RX
 */
static void MX_GPDMA1_Init(void) {
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    hdma_usart2_rx.Instance = GPDMA1_Channel0;
    hdma_usart2_rx.Init.Request = GPDMA1_REQUEST_USART2_RX;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_usart2_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_usart2_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart2_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart2_rx.Init.Priority = DMA_HIGH_PRIORITY;
    HAL_DMA_Init(&hdma_usart2_rx);

    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

/**
 * @brief Inisialisasi USART2 pada Pin PA2 (TX) dan PA3 (RX) @ 115200 bps
 */
static void MX_USART2_UART_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* PA2 = USART2_TX, PA3 = USART2_RX */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart2);

    /* Inisialisasi DMA RX Companion */
    MX_GPDMA1_Init();

    /* Aktifkan Receiver DMA dengan Deteksi Idle Line Frame */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);
}

/**
 * @brief Inisialisasi Pin EXTI Tamper (PE2 & PE3)
 */
static void MX_GPIO_Tamper_Init(void) {
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    HAL_NVIC_SetPriority(EXTI3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/* Redirect printf ke USART2 (PA2/PA3) */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/**
 * @brief Callback Interrupt UART DMA untuk USART2
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* Re-arm receiver UART DMA untuk frame berikutnya */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_dma_buf, UART_RX_BUF_SIZE);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* Callback Interupsi Hardware EXTI Tamper */
void HAL_GPIO_EXTI_Rising_Falling_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_2 || GPIO_Pin == GPIO_PIN_3) {
        tamper_event_msg_t msg = {
            .timestamp = HAL_GetTick() / 1000,
            .tamper_code = (GPIO_Pin == GPIO_PIN_2) ? 0x01 : 0x02,
            .is_active = true
        };
        rtos_queue_send_tamper_event(&msg);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config_160MHz();
    MX_USART2_UART_Init();
    MX_GPIO_Tamper_Init();

    printf("\n==================================================\n");
    printf("   STM32U575VGT6 HARDWARE BRING-UP INITIALIZED    \n");
    printf("   Console UART: USART2 (PA2/PA3 @ 115200 bps)    \n");
    printf("   System Clock: 160 MHz | GPDMA1 RX ACTIVE       \n");
    printf("==================================================\n\n");

    rtos_system_init();

    xTaskCreate(task_tamper_emergency_entry, "TamperTask", STACK_SIZE_TAMPER, NULL, PRIORITY_TASK_TAMPER, NULL);
    xTaskCreate(task_metrology_profiling_entry, "ProfileTask", STACK_SIZE_PROFILING, NULL, PRIORITY_TASK_PROFILING, NULL);
    xTaskCreate(task_dlms_entry, "DlmsTask", STACK_SIZE_DLMS, NULL, PRIORITY_TASK_DLMS, NULL);
    xTaskCreate(task_ui_display_entry, "UiTask", STACK_SIZE_UI, NULL, PRIORITY_TASK_UI, NULL);

    printf("[RTOS] Starting Task Scheduler...\n");
    vTaskStartScheduler();

    while (1) {}
    return 0;
}