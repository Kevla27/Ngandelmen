/**
 * @file bsp_alarm_buzzer.c
 * @brief Implementasi Driver Hardware Aktuasi Buzzer & LED Alarm (PB7)
 *        Memenuhi Standar SPLN D3.006: 2021 Butir 6.9 huruf c (Indikator LED Kuning & Audio Alarm)
 */

#include "bsp_alarm_buzzer.h"

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"

#define ALARM_BUZZER_GPIO_PORT   GPIOB
#define ALARM_BUZZER_PIN         GPIO_PIN_7

void bsp_alarm_buzzer_init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin(ALARM_BUZZER_GPIO_PORT, ALARM_BUZZER_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = ALARM_BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ALARM_BUZZER_GPIO_PORT, &GPIO_InitStruct);
}

void bsp_alarm_buzzer_set(bool on) {
    HAL_GPIO_WritePin(ALARM_BUZZER_GPIO_PORT, ALARM_BUZZER_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bsp_alarm_buzzer_toggle(void) {
    HAL_GPIO_TogglePin(ALARM_BUZZER_GPIO_PORT, ALARM_BUZZER_PIN);
}

bool bsp_alarm_buzzer_get(void) {
    return (HAL_GPIO_ReadPin(ALARM_BUZZER_GPIO_PORT, ALARM_BUZZER_PIN) == GPIO_PIN_SET);
}

#else
/* Implementasi Simulasi untuk PC Host Unit Test (CTest) */
static bool s_alarm_buzzer_sim_state = false;

void bsp_alarm_buzzer_init(void) {
    s_alarm_buzzer_sim_state = false;
}

void bsp_alarm_buzzer_set(bool on) {
    s_alarm_buzzer_sim_state = on;
}

void bsp_alarm_buzzer_toggle(void) {
    s_alarm_buzzer_sim_state = !s_alarm_buzzer_sim_state;
}

bool bsp_alarm_buzzer_get(void) {
    return s_alarm_buzzer_sim_state;
}
#endif

