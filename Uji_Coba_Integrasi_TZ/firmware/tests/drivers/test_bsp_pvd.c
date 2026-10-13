/**
 * @file test_bsp_pvd.c
 * @brief Unit Test untuk Pengujian Driver PVD Last-Gasp Power-Fail (E3 Lead)
 */

#include <stdio.h>
#include <assert.h>
#include "bsp_pvd.h"
#include "bsp_alarm_buzzer.h"
#include "rtos_tasks.h"
#include "nvram_storage.h"

static bool s_custom_callback_executed = false;

static void custom_power_fail_hook(void) {
    s_custom_callback_executed = true;
}

int main(void) {
    printf("=========================================\n");
    printf("       RUNNING BSP PVD UNIT TESTS        \n");
    printf("=========================================\n\n");

    /* 1. Inisialisasi Subsistem Terkait */
    rtos_system_init();
    bsp_alarm_buzzer_init();
    bsp_pvd_init();

    /* 2. Verifikasi Kondisi Normal Awal */
    assert(bsp_pvd_is_power_failed() == false);
    printf("[TEST] PVD Initial Normal State (Vdd OK): SUCCESS\n");

    /* 3. Nyalakan Buzzer & Registrasi Callback Kustom */
    bsp_alarm_buzzer_set(true);
    assert(bsp_alarm_buzzer_get() == true);
    bsp_pvd_set_callback(custom_power_fail_hook);
    assert(s_custom_callback_executed == false);

    /* 4. Simulasikan Penurunan Tegangan Listrik (PVD Power-Fail Interrupt) */
    printf("[TEST] Memicu simulasi PVD Last-Gasp Power-Fail...\n");
    bsp_pvd_simulate_power_fail();

    /* 5. Verifikasi Efek Last-Gasp */
    assert(bsp_pvd_is_power_failed() == true);
    printf("[TEST] Verify PVD Power-Fail Flag Active: SUCCESS\n");

    /* Buzzer wajib dimatikan seketika untuk menghemat muatan kapasitor */
    assert(bsp_alarm_buzzer_get() == false);
    printf("[TEST] Verify High-Power Peripheral (Buzzer) De-energized: SUCCESS\n");

    /* Callback darurat pengguna wajib terpanggil */
    assert(s_custom_callback_executed == true);
    printf("[TEST] Verify Emergency Custom Callback Executed: SUCCESS\n");

    /* Verifikasi pencegahan eksekusi ganda saat tegangan berosilasi */
    s_custom_callback_executed = false;
    bsp_pvd_simulate_power_fail();
    assert(s_custom_callback_executed == false);
    printf("[TEST] Verify Oscillation / Re-entry Prevention: SUCCESS\n");

    /* 6. Uji Pemulihan Daya Normal */
    bsp_pvd_simulate_power_restore();
    assert(bsp_pvd_is_power_failed() == false);
    printf("[TEST] Verify Power Restore: SUCCESS\n");

    printf("\n>>> ALL BSP PVD LAST-GASP UNIT TESTS PASSED 100%! <<<\n");
    printf("=========================================\n");
    return 0;
}

