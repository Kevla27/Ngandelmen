/**
 * @file bsp_alarm_buzzer.c
 * @brief Implementasi Driver Hardware Aktuasi Buzzer & LED Alarm (PB7)
 *        Memenuhi Standar SPLN D3.006: 2021 Butir 6.9 huruf c (Indikator LED Kuning & Audio Alarm)
 */

#include "bsp_alarm_buzzer.h"

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "bsp_pin_mapping.h"

void bsp_alarm_buzzer_init(void) {
    BSP_BUZZER_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin(BSP_BUZZER_GPIO_PORT, BSP_BUZZER_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = BSP_BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_BUZZER_GPIO_PORT, &GPIO_InitStruct);
}

void bsp_alarm_buzzer_set(bool on) {
    HAL_GPIO_WritePin(BSP_BUZZER_GPIO_PORT, BSP_BUZZER_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bsp_alarm_buzzer_toggle(void) {
    HAL_GPIO_TogglePin(BSP_BUZZER_GPIO_PORT, BSP_BUZZER_PIN);
}

bool bsp_alarm_buzzer_get(void) {
    return (HAL_GPIO_ReadPin(BSP_BUZZER_GPIO_PORT, BSP_BUZZER_PIN) == GPIO_PIN_SET);
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

