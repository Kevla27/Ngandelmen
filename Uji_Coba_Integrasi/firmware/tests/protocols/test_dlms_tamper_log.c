#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_types.h"
#include "dlms_tamper_log.h"
#include "dlms_obis.h"
#include "dlms_apdu.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_tamper_log...\n");

    /* ========================================================================= */
    /* TEST 1: INISIALISASI TAMPER LOG BUFFER                                     */
    /* ========================================================================= */
    dlms_tamper_log_t log;
    assert(dlms_tamper_log_init(&log) == DLMS_OK);
    assert(log.entries_in_use == 0);
    assert(log.total_events == 0);

    /* ========================================================================= */
    /* TEST 2: PENAMBAHAN EVENT TAMPER (COVER OPEN, MAGNET, REVERSE, NEUTRAL)   */
    /* ========================================================================= */
    assert(dlms_tamper_log_add_event(&log, 1700000000, DLMS_TAMPER_TERMINAL_COVER_OPEN, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log, 1700000100, DLMS_TAMPER_MAGNETIC_INDUCTION, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log, 1700000200, DLMS_TAMPER_REVERSE_CURRENT, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log, 1700000300, DLMS_TAMPER_MISSING_NEUTRAL, 1) == DLMS_OK);

    assert(log.entries_in_use == 4);
    assert(log.total_events == 4);

    /* ========================================================================= */
    /* TEST 3: VERIFIKASI URUTAN REKAMAN (Index 0 = Kejadian Terbaru)            */
    /* ========================================================================= */
    dlms_tamper_record_t rec;
    /* Index 0 haruslah event paling baru (Missing Neutral) */
    assert(dlms_tamper_log_get_entry(&log, 0, &rec) == DLMS_OK);
    assert(rec.event_code == DLMS_TAMPER_MISSING_NEUTRAL);
    assert(rec.timestamp_epoch == 1700000300);

    /* Index 3 haruslah event paling lama (Terminal Cover Open) */
    assert(dlms_tamper_log_get_entry(&log, 3, &rec) == DLMS_OK);
    assert(rec.event_code == DLMS_TAMPER_TERMINAL_COVER_OPEN);
    assert(rec.timestamp_epoch == 1700000000);

    /* ========================================================================= */
    /* TEST 4: PENGUJIAN BATAS SIRKULAR / OVERFLOW (Simulasi 35 Events > 30 Max) */
    /* ========================================================================= */
    for (uint32_t i = 0; i < 31; i++) {
        assert(dlms_tamper_log_add_event(&log, 1700010000 + i, DLMS_TAMPER_METER_COVER_OPEN, 1) == DLMS_OK);
    }
    /* Kapasitas aktif harus mentok di 30 entri (sesuai spesifikasi SPLN D3.006) */
    assert(log.entries_in_use == 30);
    /* Akumulator total tetap mencatat seluruh 35 kejadian */
    assert(log.total_events == 35);

    /* ========================================================================= */
    /* TEST 5: SERIALISASI / PENGKODEAN A-XDR BUFFER (Profile Generic Class 7)   */
    /* ========================================================================= */
    uint8_t axdr_buf[256];
    size_t encoded_len = dlms_tamper_log_encode_axdr(&log, axdr_buf, sizeof(axdr_buf));
    assert(encoded_len > 0);
    assert(axdr_buf[0] == DLMS_DATA_TYPE_OCTET_STRING); /* Tag A-XDR Octet String */
    assert(axdr_buf[1] == 30);                         /* Jumlah entri aktif = 30 */

    /* ========================================================================= */
    /* TEST 6: INTEGRASI OBIS LOOKUP CLASS 7 (PROFILE GENERIC TAMPER EVENT LOG)   */
    /* ========================================================================= */
    obis_code_t obis_tamper_log = {0, 0, 99, 98, 0, 255};
    uint8_t data_type = 0;
    uint32_t val_u32 = 0;
    assert(dlms_obis_lookup(7, &obis_tamper_log, 2, &data_type, &val_u32) == DLMS_OK);
    assert(data_type == DLMS_DATA_TYPE_OCTET_STRING);

    printf("[TEST SUCCESS] test_dlms_tamper_log 100%% PASSED!\n");
    return 0;
}