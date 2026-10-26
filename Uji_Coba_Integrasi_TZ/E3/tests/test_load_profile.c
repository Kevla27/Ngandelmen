/**
 * @file test_load_profile.c
 * @brief Unit Test untuk Modul Profil Beban 15-Menit (Load Profile - ER-02)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "load_profile.h"

void test_load_profile_basic(void) {
    printf("[TEST] Testing Load Profile Init, Insert, & CRC-16 Checksum...\n");

    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);
    assert(load_profile_get_count(&lp_ctx) == 0);

    load_profile_entry_t entry1 = {
        .timestamp = 1727570400, // Sample epoch
        .voltage_r_dvolts = 2200,
        .voltage_s_dvolts = 2205,
        .voltage_t_dvolts = 2198,
        .current_r_mamps = 5000,
        .current_s_mamps = 5020,
        .current_t_mamps = 4980,
        .current_n_mamps = 20,
        .active_power_w = 3300,
        .reactive_power_var = 150,
        .power_factor_permille = 998,
        .active_energy_import_wh = 125000,
        .status_mask = 0
    };

    bool ok = load_profile_add_entry(&lp_ctx, &entry1);
    assert(ok);
    assert(load_profile_get_count(&lp_ctx) == 1);

    load_profile_entry_t read_back;
    ok = load_profile_get_entry(&lp_ctx, 0, &read_back);
    assert(ok);
    assert(read_back.voltage_r_dvolts == 2200);
    assert(load_profile_verify_integrity(&read_back) == true);

    printf("   [PASS] Insert & Integrity Verification OK!\n\n");
}

void test_load_profile_ring_buffer_wrap(void) {
    printf("[TEST] Testing Load Profile Ring Buffer Wrap-Around...\n");

    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);

    /* Isi melampaui kapasitas LOAD_PROFILE_MAX_ENTRIES (100 entri) */
    for (size_t i = 0; i < 120; i++) {
        load_profile_entry_t e = {
            .timestamp = 1727570400 + (uint32_t)(i * 900),
            .voltage_r_dvolts = (uint16_t)(2200 + (i % 10))
        };
        load_profile_add_entry(&lp_ctx, &e);
    }

    assert(load_profile_get_count(&lp_ctx) == LOAD_PROFILE_MAX_ENTRIES);

    /* Pastikan entri tertua adalah entri ke-20 (karena 0-19 tertimpa) */
    load_profile_entry_t oldest;
    bool ok = load_profile_get_entry(&lp_ctx, 0, &oldest);
    assert(ok);
    assert(oldest.timestamp == 1727570400 + (20 * 900));

    printf("   Oldest Timestamp: %u\n", oldest.timestamp);
    printf("   [PASS] Ring Buffer Wrap-Around Verified OK!\n\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING LOAD PROFILE UNIT TESTS (ER-02) \n");
    printf("=========================================\n\n");

    test_load_profile_basic();
    test_load_profile_ring_buffer_wrap();

    printf("=========================================\n");
    printf(" ALL LOAD PROFILE TESTS PASSED 100%!     \n");
    printf("=========================================\n");
    return 0;
}