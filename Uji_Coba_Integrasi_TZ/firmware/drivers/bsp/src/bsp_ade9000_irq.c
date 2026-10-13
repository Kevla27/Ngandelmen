/**
 * @file bsp_ade9000_irq.c
 * @brief Implementasi Driver Hardware Interupsi ADE9000 IRQ0 (PB0) & IRQ1 (PB1) via EXTI
 */

#include "bsp_ade9000_irq.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "Mock.h"
#include "rtos_tasks.h"
#include "display_task.h"
#include <stdio.h>
#include <time.h>

static volatile uint32_t s_irq0_count = 0;
static volatile uint32_t s_irq1_count = 0;
static volatile uint32_t s_last_status0 = 0;
static volatile uint32_t s_last_status1 = 0;

uint32_t bsp_ade9000_irq0_get_trigger_count(void) {
    return s_irq0_count;
}

uint32_t bsp_ade9000_irq1_get_trigger_count(void) {
    return s_irq1_count;
}

uint32_t bsp_ade9000_irq_get_last_status0(void) {
    return s_last_status0;
}

uint32_t bsp_ade9000_irq_get_last_status1(void) {
    return s_last_status1;
}

void bsp_ade9000_irq_reset_counters(void) {
    s_irq0_count = 0;
    s_irq1_count = 0;
    s_last_status0 = 0;
    s_last_status1 = 0;
}

void bsp_ade9000_irq0_handler(void) {
    s_irq0_count++;
    uint32_t status0 = ADE9000_SPI_ReadRegister(ADE9000_STATUS0);
    s_last_status0 = status0;

    uint32_t now_sec;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    now_sec = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
#else
    now_sec = (uint32_t)time(NULL);
#endif

    /* 1. Evaluasi Reverse Active Power (SPLN Tabel 6 No 8: REVERSE ERR26) */
    if ((status0 & ADE9000_STATUS0_REV_ALL) != 0U) {
        tamper_event_msg_t msg = {
            .timestamp = now_sec,
            .tamper_code = (uint8_t)DLMS_TAMPER_REVERSE_CURRENT,
            .is_active = true
        };
        rtos_queue_send_tamper_event_from_isr(&msg);
        display_task_trigger_instant_refresh(true);
        printf("\r\n[ADE9000 IRQ0] *** HARDWARE TAMPER: ARUS BALIK / REVERSE POWER (ERR26) ***\r\n");
    }

    /* 2. Evaluasi Urutan Fase Terbalik (SPLN Tabel 6 No 3: PERIKSA ERR21) */
    if ((status0 & ADE9000_STATUS0_PWRORD) != 0U) {
        tamper_event_msg_t msg = {
            .timestamp = now_sec,
            .tamper_code = (uint8_t)DLMS_TAMPER_WRONG_PHASE_SEQ,
            .is_active = true
        };
        rtos_queue_send_tamper_event_from_isr(&msg);
        display_task_trigger_instant_refresh(true);
        printf("\r\n[ADE9000 IRQ0] *** HARDWARE TAMPER: URUTAN FASE TERBALIK (ERR21) ***\r\n");
    }

    /* 3. Acknowledge interupsi (Write-1-to-Clear) pada register STATUS0 ADE9000 */
    ADE9000_SPI_WriteRegister(ADE9000_STATUS0, status0);
}

void bsp_ade9000_irq1_handler(void) {
    s_irq1_count++;
    uint32_t status1 = ADE9000_SPI_ReadRegister(ADE9000_STATUS1);
    s_last_status1 = status1;

    uint32_t now_sec;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    now_sec = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
#else
    now_sec = (uint32_t)time(NULL);
#endif

    /* 1. Evaluasi Kawat Netral Hilang / Pembumian Arus (SPLN Tabel 6 No 5: PERIKSA ERR23) */
    if ((status1 & (ADE9000_STATUS1_MISNEUT | ADE9000_STATUS1_ISUM)) != 0U) {
        tamper_event_msg_t msg = {
            .timestamp = now_sec,
            .tamper_code = (uint8_t)DLMS_TAMPER_MISSING_NEUTRAL,
            .is_active = true
        };
        rtos_queue_send_tamper_event_from_isr(&msg);
        display_task_trigger_instant_refresh(true);
        printf("\r\n[ADE9000 IRQ1] *** HARDWARE TAMPER: KAWAT NETRAL HILANG / PUTUS (ERR23) ***\r\n");
    }

    /* 2. Evaluasi Hilang Tegangan 1/2 Fase / Voltage Sag (SPLN Tabel 6 No 6: PERIKSA ERR24) */
    if ((status1 & ADE9000_STATUS1_SAG_ALL) != 0U) {
        tamper_event_msg_t msg = {
            .timestamp = now_sec,
            .tamper_code = (uint8_t)DLMS_TAMPER_MISSING_VOLTAGE,
            .is_active = true
        };
        rtos_queue_send_tamper_event_from_isr(&msg);
        display_task_trigger_instant_refresh(true);
        printf("\r\n[ADE9000 IRQ1] *** HARDWARE TAMPER: HILANG TEGANGAN FASE (ERR24) ***\r\n");
    }

    /* 3. Acknowledge interupsi (Write-1-to-Clear) pada register STATUS1 ADE9000 */
    ADE9000_SPI_WriteRegister(ADE9000_STATUS1, status1);
}

void bsp_ade9000_irq_simulate_trigger(uint8_t irq_line, uint32_t status_val) {
    if (irq_line == 0) {
        ADE9000_Mock_SetSTATUS0(status_val);
        bsp_ade9000_irq0_handler();
    } else {
        ADE9000_Mock_SetSTATUS1(status_val);
        bsp_ade9000_irq1_handler();
    }
}

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "bsp_pin_mapping.h"

void bsp_ade9000_irq_init(void) {
    bsp_ade9000_irq_reset_counters();

    /* 1. Aktifkan Clock Port GPIO IRQ */
    BSP_ADE9000_IRQ0_CLK_ENABLE();
    BSP_ADE9000_IRQ1_CLK_ENABLE();

    /* 2. Konfigurasi IRQ0 & IRQ1 sebagai EXTI Falling Edge dengan Pull-Up */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BSP_ADE9000_IRQ0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_ADE9000_IRQ0_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_ADE9000_IRQ1_PIN;
    HAL_GPIO_Init(BSP_ADE9000_IRQ1_GPIO_PORT, &GPIO_InitStruct);

    /* 3. Atur Prioritas NVIC ke 5 (Syscall Safe untuk FreeRTOS) & Aktifkan Interupsi */
    HAL_NVIC_SetPriority(BSP_ADE9000_IRQ0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(BSP_ADE9000_IRQ0_IRQn);

    HAL_NVIC_SetPriority(BSP_ADE9000_IRQ1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(BSP_ADE9000_IRQ1_IRQn);

    /* 4. Konfigurasi Enable Mask pada Register ADE9000 via SPI */
    ADE9000_SPI_WriteRegister(ADE9000_MASK0, ADE9000_STATUS0_REV_ALL | ADE9000_STATUS0_PWRORD);
    ADE9000_SPI_WriteRegister(ADE9000_MASK1, ADE9000_STATUS1_SAG_ALL | ADE9000_STATUS1_MISNEUT | ADE9000_STATUS1_ISUM);

    printf("[ADE9000 IRQ] Driver Hardware IRQ0 & IRQ1 diinisialisasi.\r\n");
}

/**
 * @brief Implementasi callback HAL EXTI Falling Edge saat pin berstatus Active-Low
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == BSP_ADE9000_IRQ0_PIN) {
        bsp_ade9000_irq0_handler();
    } else if (GPIO_Pin == BSP_ADE9000_IRQ1_PIN) {
        bsp_ade9000_irq1_handler();
    }
}

#else

void bsp_ade9000_irq_init(void) {
    bsp_ade9000_irq_reset_counters();
    ADE9000_SPI_WriteRegister(ADE9000_MASK0, ADE9000_STATUS0_REV_ALL | ADE9000_STATUS0_PWRORD);
    ADE9000_SPI_WriteRegister(ADE9000_MASK1, ADE9000_STATUS1_SAG_ALL | ADE9000_STATUS1_MISNEUT | ADE9000_STATUS1_ISUM);
}

#endif

