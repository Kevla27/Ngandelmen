/**
 * @file sim_main.c
 * @brief Runner Simulasi Konsol Real-Time untuk Firmware E3
 */

#include <stdio.h>
#include <stdbool.h>
#include "display.h"
#include "tamper_manager.h"
#include "fsm_statechart.h"

int main(void) {
    fsm_context_t fsm;
    display_context_t disp;
    tamper_context_t tamper;
    meter_measurements_t meas = { .voltage_r_dvolts = 2205, .current_r_mamps = 5000 };

    // 1. Inisialisasi Modul
    fsm_init(&fsm);
    display_init(&disp, "54321678901", 3000); // Carousel 3 detik
    tamper_init(&tamper);

    printf("=====================================================\n");
    printf("   STARTING REAL-TIME FIRMWARE SIMULATION (E3)       \n");
    printf("=====================================================\n\n");

    // Booting awal
    fsm_process_event(&fsm, FSM_EVT_POR_COMPLETE);
    fsm_process_event(&fsm, FSM_EVT_SELF_TEST_PASS);

    // Loop Simulasi (Setiap iterasi mewakili 1 detik)
    for (uint32_t t_sec = 0; t_sec <= 15; t_sec++) {
        // Simulasi tick timer display 1000 ms
        display_process_tick(&disp, 1000);

        // Simulasi Event pada Detik Ke-5: Tamper Case Open
        if (t_sec == 5) {
            printf("\n[EVENT t=5s] >>> Sensor Case Open Terpemicu! <<<\n");
            tamper_process_signal(&tamper, TAMPER_VECTOR_CASE_OPEN, true, t_sec);
            fsm_process_event(&fsm, FSM_EVT_IRQ0_TAMPER_TRIGGER);
            disp.alarm_icon_active = true;
        }

        // Simulasi Event pada Detik Ke-10: Power Fail (Last Gasp)
        if (t_sec == 10) {
            printf("\n[EVENT t=10s] >>> Pemadaman Listrik AC Terdeteksi! <<<\n");
            fsm_process_event(&fsm, FSM_EVT_PWR_GOOD_LOW);
        }

        // Render tampilan LCD ke string
        char l1[32], l2[32], l3[32];
        display_render_frame(&disp, l1, l2, l3, sizeof(l1));

        // Cetak status ke terminal konsol
        printf("[%02us] State: %-23s | LCD L1: %-12s | LCD L3: %s\n", 
               t_sec, fsm_state_to_string(fsm.current_state), l1, l3);
    }

    printf("\n=====================================================\n");
    printf("   SIMULATION FINISHED SUCCESSFULLY                 \n");
    printf("=====================================================\n");

    return 0;
}