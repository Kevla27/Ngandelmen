/**
 * @file test_bsp_ade9000_irq.c
 * @brief Unit Test Driver Hardware Interupsi ADE9000 IRQ0 & IRQ1 (E3 Lead)
 */

#include <stdio.h>
#include <assert.h>
#include "bsp_ade9000_irq.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "Mock.h"
#include "rtos_tasks.h"
#include "display_task.h"

int main(void) {
    printf("=========================================\n");
    printf("    RUNNING BSP ADE9000 IRQ UNIT TESTS   \n");
    printf("=========================================\n\n");

    /* 1. Inisialisasi Subsistem Terkait */
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();
    rtos_system_init();
    bsp_ade9000_irq_init();

    /* 2. Verifikasi State Awal */
    assert(bsp_ade9000_irq0_get_trigger_count() == 0);
    assert(bsp_ade9000_irq1_get_trigger_count() == 0);
    assert(bsp_ade9000_irq_get_last_status0() == 0);
    assert(bsp_ade9000_irq_get_last_status1() == 0);

    /* Verifikasi konfigurasi register mask ADE9000 */
    uint32_t mask0 = ADE9000_Mock_ReadRegister(ADE9000_MASK0);
    uint32_t mask1 = ADE9000_Mock_ReadRegister(ADE9000_MASK1);
    assert((mask0 & ADE9000_STATUS0_REV_ALL) != 0);
    assert((mask0 & ADE9000_STATUS0_PWRORD) != 0);
    assert((mask1 & ADE9000_STATUS1_MISNEUT) != 0);
    assert((mask1 & ADE9000_STATUS1_SAG_ALL) != 0);
    printf("[TEST] Verify ADE9000 IRQ Masks Initialized Correctly: SUCCESS\n");

    /* 3. Uji Coba Pemicuan Hardware IRQ0: Reverse Active Power (SPLN ERR26) */
    printf("\n[TEST] Menguji IRQ0: Reverse Power Tamper (REVAP)...\n");
    bsp_ade9000_irq_simulate_trigger(0, ADE9000_STATUS0_REVAP);

    assert(bsp_ade9000_irq0_get_trigger_count() == 1);
    assert(bsp_ade9000_irq_get_last_status0() == ADE9000_STATUS0_REVAP);
    /* Verifikasi Write-1-to-Clear membersihkan status register */
    assert(ADE9000_Mock_ReadRegister(ADE9000_STATUS0) == 0);

    /* Verifikasi pesan masuk ke RTOS Queue */
    tamper_event_msg_t rx_msg;
    bool received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_REVERSE_CURRENT);
    assert(rx_msg.is_active == true);
    assert(display_task_is_alarm_active() == true);
    printf("[TEST] Verify IRQ0 Reverse Current Dispatched to RTOS Queue: SUCCESS\n");

    /* 4. Uji Coba Pemicuan Hardware IRQ0: Urutan Fase Terbalik (SPLN ERR21) */
    printf("\n[TEST] Menguji IRQ0: Urutan Fase Terbalik (PWRORD)...\n");
    bsp_ade9000_irq_simulate_trigger(0, ADE9000_STATUS0_PWRORD);

    assert(bsp_ade9000_irq0_get_trigger_count() == 2);
    assert(bsp_ade9000_irq_get_last_status0() == ADE9000_STATUS0_PWRORD);
    assert(ADE9000_Mock_ReadRegister(ADE9000_STATUS0) == 0);

    received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_WRONG_PHASE_SEQ);
    assert(rx_msg.is_active == true);
    printf("[TEST] Verify IRQ0 Reverse Phase Dispatched to RTOS Queue: SUCCESS\n");

    /* 5. Uji Coba Pemicuan Hardware IRQ1: Kawat Netral Hilang (SPLN ERR23) */
    printf("\n[TEST] Menguji IRQ1: Kawat Netral Hilang (MISNEUT)...\n");
    bsp_ade9000_irq_simulate_trigger(1, ADE9000_STATUS1_MISNEUT);

    assert(bsp_ade9000_irq1_get_trigger_count() == 1);
    assert(bsp_ade9000_irq_get_last_status1() == ADE9000_STATUS1_MISNEUT);
    assert(ADE9000_Mock_ReadRegister(ADE9000_STATUS1) == 0);

    received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_MISSING_NEUTRAL);
    assert(rx_msg.is_active == true);
    printf("[TEST] Verify IRQ1 Missing Neutral Dispatched to RTOS Queue: SUCCESS\n");

    /* 6. Uji Coba Pemicuan Hardware IRQ1: Hilang Tegangan 1/2 Fase / Sag (SPLN ERR24) */
    printf("\n[TEST] Menguji IRQ1: Hilang Tegangan Fase (SAG)...\n");
    bsp_ade9000_irq_simulate_trigger(1, ADE9000_STATUS1_SAG);

    assert(bsp_ade9000_irq1_get_trigger_count() == 2);
    assert(bsp_ade9000_irq_get_last_status1() == ADE9000_STATUS1_SAG);
    assert(ADE9000_Mock_ReadRegister(ADE9000_STATUS1) == 0);

    received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_MISSING_VOLTAGE);
    assert(rx_msg.is_active == true);
    printf("[TEST] Verify IRQ1 Voltage Sag Dispatched to RTOS Queue: SUCCESS\n");

    printf("\n=========================================\n");
    printf("   SEMUA PENGUJIAN BSP ADE9000 IRQ LULUS \n");
    printf("=========================================\n");
    return 0;
}

