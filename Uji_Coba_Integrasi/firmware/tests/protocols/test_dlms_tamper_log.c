#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_types.h"
#include "dlms_tamper_log.h"
#include "dlms_obis.h"
#include "dlms_apdu.h"
#include "dlms_server.h"
#include "tamper_manager.h"

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

    /* ========================================================================= */
    /* TEST 7: TRANSAKSI HDLC SERVER MEMBACA BUFFER PROFILE GENERIC 0.0.99.98.0.255 */
    /* ========================================================================= */
    dlms_server_t server;
    assert(dlms_server_init(&server) == DLMS_OK);

    /* A. Setup koneksi HDLC (SNRM -> UA) */
    uint8_t tx_buf[512];
    size_t tx_len = 0;
    uint8_t snrm[64];
    size_t snrm_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, DLMS_SERVER_SAP_MANAGEMENT, 0x10, NULL, 0, snrm, sizeof(snrm));
    assert(dlms_server_process_bytes(&server, snrm, snrm_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);

    /* B. Asosiasi DLMS (AARQ -> AARE) */
    uint8_t aarq_apdu[] = {0x60, 0x04, 0x00, 0x10};
    uint8_t aarq_hdlc[64];
    size_t aarq_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, aarq_apdu, sizeof(aarq_apdu), aarq_hdlc, sizeof(aarq_hdlc));
    assert(dlms_server_process_bytes(&server, aarq_hdlc, aarq_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(server.ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);

    /* C. Tambahkan event simulasi ke server tamper log */
    assert(dlms_tamper_log_add_event(&server.tamper_log, 1700005000, DLMS_TAMPER_METER_COVER_OPEN, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&server.tamper_log, 1700005500, DLMS_TAMPER_MAGNETIC_INDUCTION, 1) == DLMS_OK);

    /* D. Kirim GET-Request untuk Profile Generic Buffer: Class 7, OBIS 0.0.99.98.0.255, Attribute 2 */
    uint8_t get_buf_apdu[] = {
        0xC0, 0x01,          /* GET-Request Normal */
        0x90,                /* Invoke-ID 0x90 */
        0x00, 0x07,          /* Class-ID 7 (Profile Generic) */
        0, 0, 99, 98, 0, 255,/* OBIS 0.0.99.98.0.255 */
        0x02                 /* Attribute-ID 2 (Buffer) */
    };
    uint8_t get_buf_hdlc[128];
    size_t get_buf_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, get_buf_apdu, sizeof(get_buf_apdu), get_buf_hdlc, sizeof(get_buf_hdlc));
    assert(dlms_server_process_bytes(&server, get_buf_hdlc, get_buf_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);

    /* E. Dekode response HDLC & verifikasi isi APDU */
    uint8_t r_ctrl, r_src;
    uint16_t r_dest;
    uint8_t r_apdu[512];
    size_t r_apdu_len = 0;
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &r_ctrl, &r_dest, &r_src, r_apdu, &r_apdu_len) == DLMS_OK);
    assert(r_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(r_apdu[1] == DLMS_GET_RES_NORMAL);
    assert(r_apdu[2] == 0x90); /* Invoke ID */
    assert(r_apdu[3] == 0x00); /* Data Success */
    assert(r_apdu[4] == DLMS_DATA_TYPE_OCTET_STRING); /* Tag A-XDR */
    assert(r_apdu[5] == 2);    /* 2 Rekaman Tamper Aktif */

    /* Verifikasi record pertama dalam buffer (indeks 0 = kejadian terbaru: Magnetic) */
    uint32_t rec0_ts = ((uint32_t)r_apdu[6] << 24) | ((uint32_t)r_apdu[7] << 16) | ((uint32_t)r_apdu[8] << 8) | (uint32_t)r_apdu[9];
    assert(rec0_ts == 1700005500);
    assert(r_apdu[10] == DLMS_TAMPER_MAGNETIC_INDUCTION);
    assert(r_apdu[11] == 1);

    /* Verifikasi record kedua dalam buffer (indeks 1 = Cover Open) */
    uint32_t rec1_ts = ((uint32_t)r_apdu[12] << 24) | ((uint32_t)r_apdu[13] << 16) | ((uint32_t)r_apdu[14] << 8) | (uint32_t)r_apdu[15];
    assert(rec1_ts == 1700005000);
    assert(r_apdu[16] == DLMS_TAMPER_METER_COVER_OPEN);
    assert(r_apdu[17] == 1);

    /* ========================================================================= */
    /* TEST 8: GET-REQUEST ATTRIBUTE 7 (entries_in_use) & ATTRIBUTE 8 (profile_entries) */
    /* ========================================================================= */
    /* Attribute 7: entries_in_use */
    uint8_t get_cnt_apdu[] = {
        0xC0, 0x01, 0x91, 0x00, 0x07, 0, 0, 99, 98, 0, 255, 0x07
    };
    size_t get_cnt_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, get_cnt_apdu, sizeof(get_cnt_apdu), get_buf_hdlc, sizeof(get_buf_hdlc));
    assert(dlms_server_process_bytes(&server, get_buf_hdlc, get_cnt_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &r_ctrl, &r_dest, &r_src, r_apdu, &r_apdu_len) == DLMS_OK);
    assert(r_apdu[4] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);
    uint32_t in_use = ((uint32_t)r_apdu[5] << 24) | ((uint32_t)r_apdu[6] << 16) | ((uint32_t)r_apdu[7] << 8) | (uint32_t)r_apdu[8];
    assert(in_use == 2);

    /* Attribute 8: profile_entries */
    uint8_t get_max_apdu[] = {
        0xC0, 0x01, 0x92, 0x00, 0x07, 0, 0, 99, 98, 0, 255, 0x08
    };
    size_t get_max_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, get_max_apdu, sizeof(get_max_apdu), get_buf_hdlc, sizeof(get_buf_hdlc));
    assert(dlms_server_process_bytes(&server, get_buf_hdlc, get_max_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &r_ctrl, &r_dest, &r_src, r_apdu, &r_apdu_len) == DLMS_OK);
    uint32_t max_ent = ((uint32_t)r_apdu[5] << 24) | ((uint32_t)r_apdu[6] << 16) | ((uint32_t)r_apdu[7] << 8) | (uint32_t)r_apdu[8];
    assert(max_ent == DLMS_TAMPER_LOG_MAX_ENTRIES);

    /* ========================================================================= */
    /* TEST 9: UJI SINKRONISASI PENGHITUNG KEJADIAN OBIS (Tamper Counters)       */
    /* ========================================================================= */
    uint32_t init_total = dlms_obis_get_tamper_counter(DLMS_TAMPER_NONE);
    uint32_t init_cover = dlms_obis_get_tamper_counter(DLMS_TAMPER_METER_COVER_OPEN);

    dlms_obis_increment_tamper_counter(DLMS_TAMPER_METER_COVER_OPEN);
    assert(dlms_obis_get_tamper_counter(DLMS_TAMPER_NONE) == init_total + 1);
    assert(dlms_obis_get_tamper_counter(DLMS_TAMPER_METER_COVER_OPEN) == init_cover + 1);

    /* ========================================================================= */
    /* TEST 10: UJI KONVERSI VECTOR TAMPER E3 <-> DLMS CODE                     */
    /* ========================================================================= */
    assert(tamper_vector_to_dlms_code(TAMPER_VECTOR_CASE_OPEN) == DLMS_TAMPER_METER_COVER_OPEN);
    assert(tamper_vector_to_dlms_code(TAMPER_VECTOR_TERMINAL_OPEN) == DLMS_TAMPER_TERMINAL_COVER_OPEN);
    assert(tamper_vector_to_dlms_code(TAMPER_VECTOR_MAGNETIC_FIELD) == DLMS_TAMPER_MAGNETIC_INDUCTION);
    assert(tamper_vector_to_dlms_code(TAMPER_VECTOR_REVERSE_POWER) == DLMS_TAMPER_REVERSE_CURRENT);
    assert(tamper_vector_to_dlms_code(TAMPER_VECTOR_NEUTRAL_BYPASS) == DLMS_TAMPER_MISSING_NEUTRAL);

    assert(dlms_code_to_tamper_vector(DLMS_TAMPER_METER_COVER_OPEN) == TAMPER_VECTOR_CASE_OPEN);
    assert(dlms_code_to_tamper_vector(DLMS_TAMPER_MAGNETIC_INDUCTION) == TAMPER_VECTOR_MAGNETIC_FIELD);

    printf("[TEST SUCCESS] test_dlms_tamper_log 100%% PASSED (Tests 1-10)!\n");
    return 0;
}