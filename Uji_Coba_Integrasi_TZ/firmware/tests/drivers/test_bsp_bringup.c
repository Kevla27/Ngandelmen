/**
 * @file test_bsp_bringup.c
 * @brief Unit Test untuk Hardware Bring-Up & Diagnostic BIST Suite (E3 Lead)
 */

#include <stdio.h>
#include <assert.h>
#include "bsp_bringup_test.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "Mock.h"
#include "rtos_tasks.h"

int main(void) {
    printf("=========================================\n");
    printf("   RUNNING BSP BRINGUP BIST UNIT TESTS   \n");
    printf("=========================================\n\n");

    /* 1. Inisialisasi Mock & Sistem */
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();
    rtos_system_init();
    bsp_bringup_init();

    /* 2. Pengujian Diagnostik 1: I2C Scanner */
    printf("\n[TEST] Menguji I2C Bus Scanner...\n");
    bool i2c_ok = bsp_bringup_test_i2c_bus();
    assert(i2c_ok == true);
    printf("[TEST] I2C Bus Scanner: SUCCESS\n");

    /* 3. Pengujian Diagnostik 2: ADE9000 SPI Integrity */
    printf("\n[TEST] Menguji ADE9000 SPI Loopback Integrity...\n");
    bool spi_ok = bsp_bringup_test_ade9000_comm();
    assert(spi_ok == true);
    printf("[TEST] ADE9000 SPI Integrity: SUCCESS\n");

    /* 4. Pengujian Diagnostik 3: GPIO Actuators */
    printf("\n[TEST] Menguji GPIO Actuators (Buzzer & Relay)...\n");
    bool act_ok = bsp_bringup_test_gpio_actuators();
    assert(act_ok == true);
    printf("[TEST] GPIO Actuators: SUCCESS\n");

    /* 5. Pengujian Diagnostik 4: GPIO Sensors & Tamper Readout */
    printf("\n[TEST] Menguji GPIO Sensors Readout...\n");
    bool sens_ok = bsp_bringup_test_gpio_sensors();
    assert(sens_ok == true);
    printf("[TEST] GPIO Sensors Readout: SUCCESS\n");

    /* 6. Pengujian Diagnostik 5: PVD Power Monitor */
    printf("\n[TEST] Menguji PVD Power Monitor Check...\n");
    bool pvd_ok = bsp_bringup_test_pvd_power();
    assert(pvd_ok == true);
    printf("[TEST] PVD Power Monitor: SUCCESS\n");

    /* 7. Pengujian Diagnostik 6: NVRAM Storage Integrity */
    printf("\n[TEST] Menguji NVRAM Read/Write Integrity...\n");
    bool nvm_ok = bsp_bringup_test_nvram_storage();
    assert(nvm_ok == true);
    printf("[TEST] NVRAM Storage Integrity: SUCCESS\n");

    /* 8. Pengujian Eksekusi Lengkap BIST (Built-In Self-Test) */
    printf("\n[TEST] Menjalankan Full BIST Suite...\n");
    bsp_bringup_report_t report;
    bool bist_all_ok = bsp_bringup_run_all(&report);
    assert(bist_all_ok == true);
    assert(report.total_passed == 6);
    assert(report.total_failed == 0);
    assert(report.i2c_status == BRINGUP_STATUS_PASS);
    assert(report.ade9000_status == BRINGUP_STATUS_PASS);
    assert(report.gpio_act_status == BRINGUP_STATUS_PASS);
    assert(report.gpio_sens_status == BRINGUP_STATUS_PASS);
    assert(report.pvd_status == BRINGUP_STATUS_PASS);
    assert(report.nvram_status == BRINGUP_STATUS_PASS);
    printf("[TEST] Full BIST Suite Verification: SUCCESS\n");

    /* 9. Pengujian CLI Command Handler */
    printf("\n[TEST] Menguji CLI Shell Command Handler...\n");
    bsp_bringup_handle_cli_cmd('i');
    bsp_bringup_handle_cli_cmd('m');
    bsp_bringup_handle_cli_cmd('a');
    bsp_bringup_handle_cli_cmd('s');
    bsp_bringup_handle_cli_cmd('v');
    bsp_bringup_handle_cli_cmd('n');
    bsp_bringup_handle_cli_cmd('r');
    bsp_bringup_handle_cli_cmd('h');
    printf("[TEST] CLI Shell Command Handler: SUCCESS\n");

    printf("\n=========================================\n");
    printf("   SEMUA PENGUJIAN BSP BRINGUP LULUS     \n");
    printf("=========================================\n");
    return 0;
}

