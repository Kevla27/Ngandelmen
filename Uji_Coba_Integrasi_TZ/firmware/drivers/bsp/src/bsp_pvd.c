/**
 * @file bsp_pvd.c
 * @brief Implementasi Driver Hardware PVD (Programmable Voltage Detector / Last-Gasp)
 */

#include "bsp_pvd.h"
#include "bsp_alarm_buzzer.h"
#include "nvram_storage.h"
#include "rtos_tasks.h"
#include <stdio.h>

static bsp_pvd_callback_t s_user_pvd_cb = NULL;
static volatile bool s_pvd_power_failed = false;

void bsp_pvd_set_callback(bsp_pvd_callback_t cb) {
    s_user_pvd_cb = cb;
}

bool bsp_pvd_is_power_failed(void) {
    return s_pvd_power_failed;
}

void bsp_pvd_execute_emergency_power_fail(void) {
    if (s_pvd_power_failed) return; /* Cegah eksekusi ganda jika tegangan berosilasi */
    s_pvd_power_failed = true;

    /* 1. De-energisasi beban periferal boros daya seketika */
    bsp_alarm_buzzer_set(false);

    /* 2. Simpan snapshot Tamper Log & State E3 ke Flash Non-Volatile */
    tamper_context_t *t_ctx = rtos_tamper_get_ctx();
    if (t_ctx != NULL) {
        nvram_save_tamper_log_snapshot(t_ctx);
    }

    /* 3. Panggil callback tambahan pengguna jika terdaftar */
    if (s_user_pvd_cb != NULL) {
        s_user_pvd_cb();
    }

    printf("\r\n[PVD LAST-GASP] *** DARURAT! TEGANGAN VDD ANJLOK (<2.9V) ***\r\n");
    printf("[PVD LAST-GASP] Data Tamper Log & State berhasil diamankan ke Flash NVRAM!\r\n");
}

void bsp_pvd_simulate_power_fail(void) {
    bsp_pvd_execute_emergency_power_fail();
}

void bsp_pvd_simulate_power_restore(void) {
    s_pvd_power_failed = false;
    printf("[PVD] Tegangan catu daya normal kembali (Vdd > 2.9V).\r\n");
}

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"

void bsp_pvd_init(void) {
    s_pvd_power_failed = false;

    PWR_PVDTypeDef sConfigPVD = {0};
    /* Level 6: Ambang batas ~2.9 V untuk memberikan early warning terpanjang sebelum BOR (~1.9V) */
    sConfigPVD.PVDLevel = PWR_PVDLEVEL_6;
    sConfigPVD.Mode = PWR_PVD_MODE_IT_RISING; /* EXTI Rising edge saat Vdd turun melewati ambang */
    
    if (HAL_PWR_ConfigPVD(&sConfigPVD) == HAL_OK) {
        HAL_PWR_EnablePVD();

        /* Set prioritas tertinggi (0) agar interupsi PVD mendahului task dan ISR lainnya */
        HAL_NVIC_SetPriority(PVD_PVM_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(PVD_PVM_IRQn);
    }
}

/**
 * @brief Implementasi callback HAL PVD yang dipanggil dari HAL_PWR_PVD_IRQHandler()
 */
void HAL_PWR_PVDCallback(void) {
    bsp_pvd_execute_emergency_power_fail();
}

#else

void bsp_pvd_init(void) {
    s_pvd_power_failed = false;
}

#endif

