/**
 * @file bsp_watchdog.c
 * @brief Implementasi Driver Pengawas Hardware & Supervisor Multi-Task (STM32 IWDG & FreeRTOS)
 */

#include "bsp_watchdog.h"
#include "bsp_rtc_tamp.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#endif

static bsp_watchdog_status_t s_status = {
    .is_initialized = false,
    .last_reset_was_watchdog = false,
    .reset_counter = 0,
    .monitored_mask = WATCHDOG_DEFAULT_MONITORED_MASK,
    .current_heartbeat_mask = 0,
    .missing_tasks_mask = 0,
    .timeout_ms = WATCHDOG_DEFAULT_TIMEOUT_MS,
    .hw_countdown_ms = WATCHDOG_DEFAULT_TIMEOUT_MS
};

static uint32_t s_frozen_task_mask = 0;

void bsp_watchdog_init(uint32_t timeout_ms) {
    if (timeout_ms == 0) {
        timeout_ms = WATCHDOG_DEFAULT_TIMEOUT_MS;
    }
    s_status.timeout_ms = timeout_ms;
    s_status.hw_countdown_ms = timeout_ms;
    s_status.current_heartbeat_mask = 0;
    s_status.missing_tasks_mask = 0;
    s_frozen_task_mask = 0;

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* Pastikan LSI Clock Aktif */
    __HAL_RCC_LSI_ENABLE();
    uint32_t lsi_wait = 50000U;
    while ((__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET) && (--lsi_wait > 0U)) {
        /* Wait for LSI ready with safety guard */
    }

    /* 1. Periksa Apakah Boot Ini Disebabkan Oleh Timeout IWDG */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        s_status.last_reset_was_watchdog = true;

        /* Ambil counter reset sebelumnya dari Backup Register TAMP_BKP3R */
        uint32_t prev = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_WATCHDOG_COUNT);
        s_status.reset_counter = prev + 1;
        bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_WATCHDOG_COUNT, s_status.reset_counter);

        printf("\r\n======================================================================\r\n");
        printf(" [WATCHDOG] *** PERINGATAN: SISTEM BOOT KARENA IWDG RESET (ERR04)! ***\r\n");
        printf("            Total IWDG Reset Terakumulasi: %lu kali\r\n", (unsigned long)s_status.reset_counter);
        printf("            Kepatuhan: Standar SPLN D3.006: 2021 Tabel 4 (MCU Failure)\r\n");
        printf("======================================================================\r\n\r\n");

        /* Bersihkan flag reset di register RCC_RSR */
        __HAL_RCC_CLEAR_RESET_FLAGS();
    } else {
        s_status.reset_counter = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_WATCHDOG_COUNT);
    }

    /* 2. Konfigurasi Register Hardware IWDG STM32U575 */
    /* Prescaler /64: Dari LSI 32 kHz -> Clock IWDG = 500 Hz (1 tick = 2 ms) */
    IWDG->KR = 0x5555U; /* Buka write protection register PR & RLR */
    IWDG->PR = IWDG_PR_PR_2; /* Divider /64 */

    uint32_t reload = (timeout_ms / 2U);
    if (reload > 0x0FFFU) reload = 0x0FFFU; /* Max 12-bit: 4095 (~8,19 detik) */
    IWDG->RLR = reload;

    /* Tunggu hingga register PR dan RLR selesai di-update dengan safety guard */
    uint32_t wait_timeout = 100000U;
    while (((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) && (--wait_timeout > 0U)) {
        /* Wait idle with guard */
    }

    /* Catatan: Pencacah hardware IWDG (0xCCCC) akan di-arm saat supervisor task berjalan */
    printf("[WATCHDOG] Konfigurasi Hardware IWDG Siap (Timeout: %lu ms, Prescaler /64).\r\n",
           (unsigned long)timeout_ms);
#else
    printf("[WATCHDOG] Mock Hardware IWDG & Supervisor Diinisialisasi (Host CTest).\r\n");
#endif

    s_status.is_initialized = true;
}

bool bsp_watchdog_was_reset_by_watchdog(void) {
    return s_status.last_reset_was_watchdog;
}

uint32_t bsp_watchdog_get_reset_count(void) {
    return s_status.reset_counter;
}

void bsp_watchdog_kick_task(uint32_t task_bit) {
    /* Jika task sedang disimulasikan macet/freeze, abaikan heartbeat-nya */
    if ((s_frozen_task_mask & task_bit) != 0U) {
        return;
    }

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    taskENTER_CRITICAL();
    s_status.current_heartbeat_mask |= task_bit;
    taskEXIT_CRITICAL();
#else
    s_status.current_heartbeat_mask |= task_bit;
#endif
}

bool bsp_watchdog_supervisor_step(void) {
    /* Periksa apakah SELURUH task yang dimonitor telah memberikan heartbeat */
    bool all_healthy = ((s_status.current_heartbeat_mask & s_status.monitored_mask) == s_status.monitored_mask);

    if (all_healthy) {
        /* Semua task sehat! Refresh hardware IWDG dan reset heartbeat untuk siklus berikutnya */
        s_status.current_heartbeat_mask = 0;
        s_status.missing_tasks_mask = 0;
        s_status.hw_countdown_ms = s_status.timeout_ms;

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        IWDG->KR = 0xAAAAU; /* Refresh hardware IWDG */
#endif
        return true;
    } else {
        /* Ada task yang macet! Jangan refresh hardware IWDG! */
        s_status.missing_tasks_mask = s_status.monitored_mask & (~s_status.current_heartbeat_mask);
        printf("[WATCHDOG SUPERVISOR] PERINGATAN: Task macet terdeteksi (Missing Mask: 0x%02lX)! Menolak refresh IWDG.\r\n",
               (unsigned long)s_status.missing_tasks_mask);
        return false;
    }
}

void bsp_watchdog_get_status(bsp_watchdog_status_t *out_status) {
    if (out_status != NULL) {
        *out_status = s_status;
    }
}

void bsp_watchdog_set_monitored_mask(uint32_t mask) {
    s_status.monitored_mask = mask;
}

void bsp_watchdog_supervisor_task_entry(void *pvParameters) {
    (void)pvParameters;
    printf("[WATCHDOG] FreeRTOS Watchdog Supervisor Task Aktif (Periode: %u ms).\r\n",
           WATCHDOG_SUPERVISOR_PERIOD_MS);

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* Arm hardware IWDG saat kernel FreeRTOS sudah aktif menjadwalkan task */
    IWDG->KR = 0xCCCCU;
    IWDG->KR = 0xAAAAU;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_SUPERVISOR_PERIOD_MS));
        bsp_watchdog_supervisor_step();
    }
#endif
}

bool bsp_watchdog_start_supervisor_task(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    BaseType_t res = xTaskCreate(
        bsp_watchdog_supervisor_task_entry,
        "WdgSupervisor",
        256,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    return (res == pdPASS);
#else
    return true;
#endif
}

void bsp_watchdog_simulate_hw_countdown(uint32_t elapsed_ms) {
    if (s_status.hw_countdown_ms > elapsed_ms) {
        s_status.hw_countdown_ms -= elapsed_ms;
    } else {
        s_status.hw_countdown_ms = 0;
    }
}

bool bsp_watchdog_simulate_is_hw_expired(void) {
    return (s_status.hw_countdown_ms == 0);
}

void bsp_watchdog_simulate_set_reset_flag(bool was_reset) {
    s_status.last_reset_was_watchdog = was_reset;
    if (was_reset) {
        s_status.reset_counter++;
        bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_WATCHDOG_COUNT, s_status.reset_counter);
    }
}

void bsp_watchdog_simulate_freeze_task(uint32_t task_bit) {
    s_frozen_task_mask |= task_bit;
    printf("[SIMULASI WATCHDOG] Task bit 0x%02lX dibekukan/simulasi hang!\r\n",
           (unsigned long)task_bit);
}

