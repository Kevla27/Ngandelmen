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

/* --- 3. HARDWARE TAMPER CASE OPEN (SECURE WORLD - SPLN Tabel 6 No 1: RUSAK) --- */
#define BSP_TAMPER_CASE_GPIO_PORT         GPIOC
#define BSP_TAMPER_CASE_PIN               GPIO_PIN_13
#define BSP_TAMPER_CASE_IRQn              EXTI13_IRQn
#define BSP_TAMPER_CASE_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

/* --- 4. RELAI BEBAN (LATCHING RELAY PULSES - SPLN D3.006 Tabel 1 & Tabel 5) --- */
#define BSP_RELAY_TRIP_GPIO_PORT          GPIOC
#define BSP_RELAY_TRIP_PIN                GPIO_PIN_8    /* Pulsa Buka Relai 100ms */
#define BSP_RELAY_CLOSE_GPIO_PORT         GPIOC
#define BSP_RELAY_CLOSE_PIN               GPIO_PIN_9    /* Pulsa Tutup Relai 100ms */
#define BSP_RELAY_FEEDBACK_GPIO_PORT      GPIOC
#define BSP_RELAY_FEEDBACK_PIN            GPIO_PIN_7    /* Feedback Kontak Bantu Relai (ERR07) */

/* --- 5. KOMUNIKASI DUA ARAH MODEM (RS-232 DEDICATED - SPLN Butir 6.12.1 & Tabel 3) --- */
#define BSP_MODEM_UART_INSTANCE           LPUART1
#define BSP_MODEM_TX_GPIO_PORT            GPIOA
#define BSP_MODEM_TX_PIN                  GPIO_PIN_2    /* PA2 (LPUART1_TX AF8) -> MAX3232 -> Pin 5 RJ-45: TX (out) */
#define BSP_MODEM_RX_GPIO_PORT            GPIOA
#define BSP_MODEM_RX_PIN                  GPIO_PIN_3    /* PA3 (LPUART1_RX AF8) <- MAX3232 <- Pin 4 RJ-45: RX (in) */
#define BSP_MODEM_PWR_EN_GPIO_PORT        GPIOB
#define BSP_MODEM_PWR_EN_PIN              GPIO_PIN_10   /* PB10 Switch Catu Daya DC 12V 5W (Butir 6.13) & Hard Reset via Power-Cycle */
#define BSP_MODEM_DEFAULT_BAUDRATE        19200         /* Setelan Awal SPLN: 19.200 bps */

/* --- 6. KOMUNIKASI LOKAL OPTICAL PROBE (SPLN Butir 6.12.2 @ 9600 BPS) --- */
#define BSP_OPTICAL_UART_INSTANCE         USART3
#define BSP_OPTICAL_TX_GPIO_PORT          GPIOC
#define BSP_OPTICAL_TX_PIN                GPIO_PIN_4    /* PC4 (USART3_TX AF7) */
#define BSP_OPTICAL_RX_GPIO_PORT          GPIOC
#define BSP_OPTICAL_RX_PIN                GPIO_PIN_5    /* PC5 (USART3_RX AF7) */
#define BSP_OPTICAL_DEFAULT_BAUDRATE      9600          /* 9.600 bps */

/* --- 7. TOMBOL NAVIGASI DISPLAY (SPLN Butir 6.10 & Tabel 5 No 12) --- */
#define BSP_BTN_UP_GPIO_PORT              GPIOB
#define BSP_BTN_UP_PIN                    GPIO_PIN_2    /* PB2 Tombol Naik (▲) */
#define BSP_BTN_DOWN_GPIO_PORT            GPIOB
#define BSP_BTN_DOWN_PIN                  GPIO_PIN_6    /* PB6 Tombol Turun (▼) */

/* --- 8. DETEKSI SABOTASE TAMBAHAN (SPLN Tabel 6 No 2 & 7) --- */
#define BSP_TAMPER_TERMINAL_GPIO_PORT     GPIOE
#define BSP_TAMPER_TERMINAL_PIN           GPIO_PIN_6    /* PE6 Tutup Terminal (TAMP_IN3 - SECURE / VSW VBAT Domain) */
#define BSP_SENSOR_MAGNET_GPIO_PORT       GPIOC
#define BSP_SENSOR_MAGNET_PIN             GPIO_PIN_0    /* PC0 Sensor Hall >500mT (ERR25) */

/* --- 9. INDIKATOR LED PULSA KALIBRASI (SPLN Butir 6.9 a & b) --- */
#define BSP_LED_PULSE_KWH_GPIO_PORT       GPIOB
#define BSP_LED_PULSE_KWH_PIN             GPIO_PIN_4    /* PB4 LED Merah 1 (imp/kWh) */
#define BSP_LED_PULSE_KVARH_GPIO_PORT     GPIOB
#define BSP_LED_PULSE_KVARH_PIN           GPIO_PIN_5    /* PB5 LED Merah 2 (imp/kVARh) */

/* --- 10. MONITORING TEGANGAN BATERAI (SPLN Butir 6.11 & Tabel 4 No 3) --- */
#define BSP_BATTERY_SENSE_GPIO_PORT       GPIOA
#define BSP_BATTERY_SENSE_PIN             GPIO_PIN_1    /* PA1 ADC1_IN6 Baterai 2.7V (ERR03) */

#endif /* EMBEDDED_HARDWARE_TARGET || USE_FREERTOS */

#endif /* BSP_PIN_MAPPING_H */

