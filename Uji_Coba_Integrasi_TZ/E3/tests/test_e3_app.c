/**
 * @file test_e3_app.c
 * @brief Unit Test untuk Modul Display & Tamper Manager E3
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "display.h"
#include "tamper_manager.h"

void test_display_carousel(void) {
    printf("[TEST] Testing Display Auto Carousel & Button Nav...\n");

    display_context_t disp;
    bool ok = display_init(&disp, "54321678901", 3000);
    assert(ok);
    assert(disp.current_page == DISP_PAGE_IDPEL);

    meter_measurements_t meas = { .voltage_r_dvolts = 2205 }; // 220.5 V
    display_update_measurements(&disp, &meas);

    // Simulasi 3 detik -> Carousel bergulir ke VOLTAGE_R
    display_process_tick(&disp, 3000);
    assert(disp.current_page == DISP_PAGE_VOLTAGE_R);

    // FIX 1: Ubah dari char tunggal menjadi array string (buffer)
    char l1[64], l2[64], l3[64];
    display_render_frame(&disp, l1, l2, l3, sizeof(l1));
    
    printf("   Render Line 1: %s\n", l1);
    printf("   Render Line 2: %s\n", l2);
    printf("   Render Line 3: %s\n", l3);

    assert(strstr(l3, "220.5 V") != NULL);

    printf("   [PASS] Display Carousel & Rendering OK!\n\n");
}

void test_tamper_manager_fifo(void) {
    printf("[TEST] Testing Tamper Event Manager & FIFO Log...\n");

    tamper_context_t tamper;
    tamper_init(&tamper);

    // Event 1: Case Open
    tamper_process_signal(&tamper, TAMPER_VECTOR_CASE_OPEN, true, 1000);
    assert(tamper_get_active_mask(&tamper) & TAMPER_VECTOR_CASE_OPEN);

    // Event 2: Medan Magnet
    tamper_process_signal(&tamper, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1005);

    // FIX 2: Ubah dari struct tunggal menjadi array struct
    tamper_event_entry_t logs[10];
    size_t count = 0;
    tamper_get_log_entries(&tamper, logs, 10, &count);
    assert(count == 2);

    printf("   Active Mask: 0x%02X, Log Count: %zu\n", tamper_get_active_mask(&tamper), count);
    printf("   [PASS] Tamper Event Manager OK!\n\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING E3 FIRMWARE APPLICATION TESTS   \n");
    printf("=========================================\n\n");

    test_display_carousel();
    test_tamper_manager_fifo();

    printf("=========================================\n");
    printf(" ALL E3 TESTS PASSED SUCCESSFULLY! (100%)\n");
    printf("=========================================\n");
    return 0;
}