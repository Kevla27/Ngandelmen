/**
 * @file test_bsp_watchdog.c
 * @brief Unit Test Driver Watchdog Supervisor & Hardware IWDG STM32
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "bsp_watchdog.h"
#include "bsp_rtc_tamp.h"

void test_watchdog_init_and_default_state(void) {
    printf("[TEST] Testing Watchdog Init & Default Parameters...\n");

    bsp_rtc_tamp_init();
    bsp_watchdog_init(3000);

    bsp_watchdog_status_t status;
    bsp_watchdog_get_status(&status);

    assert(status.is_initialized == true);
    assert(status.timeout_ms == 3000);
    assert(status.hw_countdown_ms == 3000);
    assert(status.monitored_mask == WATCHDOG_DEFAULT_MONITORED_MASK);
    assert(status.current_heartbeat_mask == 0);
    assert(status.missing_tasks_mask == 0);

    printf("   [PASS] Init & State Default Verified OK!\n\n");
}

void test_watchdog_normal_heartbeat_and_refresh(void) {
    printf("[TEST] Testing Multi-Task Heartbeat & Successful IWDG Refresh...\n");

    bsp_watchdog_init(3000);

    /* Seluruh 5 task memberikan heartbeat tanda hidup */
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_ADE9000_BIT);

    bsp_watchdog_status_t status;
    bsp_watchdog_get_status(&status);
    assert(status.current_heartbeat_mask == WATCHDOG_DEFAULT_MONITORED_MASK);

    /* Supervisor mengevaluasi siklus */
    bool ok = bsp_watchdog_supervisor_step();
    assert(ok == true);

    /* Setelah step sukses: heartbeat di-reset ke 0, countdown di-reload ke 3000 ms */
    bsp_watchdog_get_status(&status);
    assert(status.current_heartbeat_mask == 0);
    assert(status.missing_tasks_mask == 0);
    assert(status.hw_countdown_ms == 3000);
    assert(bsp_watchdog_simulate_is_hw_expired() == false);

    printf("   [PASS] All-Tasks Heartbeat & Hardware Reload Verified OK!\n\n");
}

void test_watchdog_task_hang_detection_and_refusal(void) {
    printf("[TEST] Testing Task Freeze Detection & Watchdog Timeout Trigger...\n");

    bsp_watchdog_init(3000);

    /* Simulasi: Hanya Tamper & Metrology yang berjalan normal, DLMS & UI macet/hang */
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);

    /* Supervisor mengevaluasi */
    bool ok = bsp_watchdog_supervisor_step();
    assert(ok == false); /* Harus MENOLAK refresh! */

    bsp_watchdog_status_t status;
    bsp_watchdog_get_status(&status);
    uint32_t expected_missing = WATCHDOG_TASK_DLMS_BIT | WATCHDOG_TASK_UI_BIT | WATCHDOG_TASK_ADE9000_BIT;
    assert((status.missing_tasks_mask & expected_missing) == expected_missing);

    /* Waktu berjalan: IWDG hardware menghitung mundur */
    bsp_watchdog_simulate_hw_countdown(1500);
    assert(bsp_watchdog_simulate_is_hw_expired() == false);

    bsp_watchdog_simulate_hw_countdown(1500);
    assert(bsp_watchdog_simulate_is_hw_expired() == true); /* Hard Reset Triggered! */

    printf("   [PASS] Task Hang Detected, Refresh Refused, & Hard Reset Triggered OK!\n\n");
}

void test_watchdog_boot_cause_and_backup_register_counter(void) {
    printf("[TEST] Testing Boot Reset Cause Detection & TAMP_BKP3R Accumulator...\n");

    bsp_rtc_tamp_init();

    /* Simulasi MCU me-reboot akibat IWDG timeout */
    bsp_watchdog_simulate_set_reset_flag(true);

    assert(bsp_watchdog_was_reset_by_watchdog() == true);
    uint32_t count = bsp_watchdog_get_reset_count();
    assert(count >= 1);

    /* Pastikan tersimpan permanen di Backup Register TAMP_BKP3R */
    uint32_t bkp_val = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_WATCHDOG_COUNT);
    assert(bkp_val == count);

    printf("   Reset Count di Backup Register: %lu\n", (unsigned long)bkp_val);
    printf("   [PASS] Boot Reset Cause & Backup Register Retention Verified OK!\n\n");
}

void test_watchdog_custom_monitored_mask(void) {
    printf("[TEST] Testing Custom Task Monitored Mask...\n");

    bsp_watchdog_init(3000);

    /* Ubah mask monitor hanya untuk Tamper & Metrology */
    uint32_t custom_mask = WATCHDOG_TASK_TAMPER_BIT | WATCHDOG_TASK_METROLOGY_BIT;
    bsp_watchdog_set_monitored_mask(custom_mask);

    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);

    bool ok = bsp_watchdog_supervisor_step();
    assert(ok == true);

    printf("   [PASS] Custom Task Mask Evaluation Verified OK!\n\n");
}

void test_watchdog_null_safety(void) {
    printf("[TEST] Testing Watchdog Null Pointer Safety...\n");

    bsp_watchdog_get_status(NULL); /* Harus aman dan tidak crash */

    printf("   [PASS] Null Pointer Safety Verified OK!\n\n");
}

int main(void) {
    printf("======================================================================\n");
    printf("     RUNNING SUITE: BSP HARDWARE WATCHDOG & SUPERVISOR (IWDG)         \n");
    printf("     Standar Kepatuhan: SPLN D3.006: 2021 Tabel 4 (ERR04 MCU Alarm)   \n");
    printf("======================================================================\n\n");

    test_watchdog_init_and_default_state();
    test_watchdog_normal_heartbeat_and_refresh();
    test_watchdog_task_hang_detection_and_refusal();
    test_watchdog_boot_cause_and_backup_register_counter();
    test_watchdog_custom_monitored_mask();
    test_watchdog_null_safety();

    printf("======================================================================\n");
    printf(" ALL WATCHDOG & SUPERVISOR TESTS PASSED 100%!                         \n");
    printf("======================================================================\n");
    return 0;
}

