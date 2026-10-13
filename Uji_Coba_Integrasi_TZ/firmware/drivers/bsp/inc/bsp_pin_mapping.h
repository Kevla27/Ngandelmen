/**
 * @file bsp_pin_mapping.h
 * @brief Pemetaan Pinout Hardware Terpusat (Hardware Abstraction Pinout Config)
 * 
 * Memusatkan seluruh konfigurasi port, pin, dan jalur interupsi mikrokontroler.
 * Jika terdapat revisi skematik PCB (misal: transisi NUCLEO -> Spin 1 Board),
 * perubahan hanya dilakukan pada berkas ini tanpa menyentuh logika bisnis driver.
 */

#ifndef BSP_PIN_MAPPING_H
#define BSP_PIN_MAPPING_H

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"

/* ========================================================================== */
/*               BSP PINOUT CONFIGURATION (PCB REVISION MAPPING)              */
/* ========================================================================== */

/* --- 1. BUZZER & LED ALARM (SPLN D3.006: 2021 Butir 6.9) --- */
#define BSP_BUZZER_GPIO_PORT              GPIOB
#define BSP_BUZZER_PIN                    GPIO_PIN_7
#define BSP_BUZZER_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()

/* --- 2. ADE9000 HARDWARE INTERRUPTS (Pin 31 IRQ0 & Pin 32 IRQ1) --- */
#define BSP_ADE9000_IRQ0_GPIO_PORT        GPIOB
#define BSP_ADE9000_IRQ0_PIN              GPIO_PIN_0
#define BSP_ADE9000_IRQ0_IRQn             EXTI0_IRQn
#define BSP_ADE9000_IRQ0_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()

#define BSP_ADE9000_IRQ1_GPIO_PORT        GPIOB
#define BSP_ADE9000_IRQ1_PIN              GPIO_PIN_1
#define BSP_ADE9000_IRQ1_IRQn             EXTI1_IRQn
#define BSP_ADE9000_IRQ1_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()

/* --- 3. HARDWARE TAMPER CASE OPEN (SECURE WORLD) --- */
#define BSP_TAMPER_CASE_GPIO_PORT         GPIOC
#define BSP_TAMPER_CASE_PIN               GPIO_PIN_13
#define BSP_TAMPER_CASE_IRQn              EXTI13_IRQn
#define BSP_TAMPER_CASE_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

/* --- 4. RELAI BEBAN (LATCHING RELAY PULSES - SPLN D3.006 Tabel 5) --- */
/* Disiapkan untuk Spin 1: Pulsa Trip (Open) & Close */
#define BSP_RELAY_TRIP_GPIO_PORT          GPIOC
#define BSP_RELAY_TRIP_PIN                GPIO_PIN_8
#define BSP_RELAY_CLOSE_GPIO_PORT         GPIOC
#define BSP_RELAY_CLOSE_PIN               GPIO_PIN_9

#endif /* EMBEDDED_HARDWARE_TARGET || USE_FREERTOS */

#endif /* BSP_PIN_MAPPING_H */

