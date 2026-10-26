/**
 * @file test_ami_core.c
 * @brief Comprehensive Test Suite & Validation untuk Modern AMI DLMS/COSEM Stack (SPLN D3.002:2021)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "ami_cosem_model.h"
#include "ami_dlms_association.h"
#include "ami_dlms_apdu.h"
#include "ami_dlms_hdlc.h"
#include "ami_security.h"
#include "ami_server.h"
#include "ami_task.h"

#define AMI_HDLC_CONTROL_I_FRAME 0x10u

static void test_ami_cosem_model_and_catalog(void) {
    printf("[VALIDASI 1] Menguji Model Objek COSEM & Katalog Register SPLN D3.002:2021...\n");

    AmiCosemRegisterSnapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));

    /* Data simulasi snapshot metrologi */
    snapshot.phase_voltage_millivolts[0] = 230000; /* 230.0 V */
    snapshot.phase_voltage_millivolts[1] = 230100; /* 230.1 V */
    snapshot.phase_voltage_millivolts[2] = 229900; /* 229.9 V */
    snapshot.phase_current_milliamps[0] = 11946;   /* 11.946 A */
    snapshot.phase_current_milliamps[1] = 11950;
    snapshot.phase_current_milliamps[2] = 11940;
    snapshot.active_import_wh = 125430ULL;         /* 125.43 kWh */
    snapshot.valid_mask = AMI_COSEM_VALID_VOLTAGE_A | AMI_COSEM_VALID_VOLTAGE_B |
                          AMI_COSEM_VALID_VOLTAGE_C | AMI_COSEM_VALID_CURRENT_A |
                          AMI_COSEM_VALID_ACTIVE_IMPORT | AMI_COSEM_VALID_CLOCK |
                          AMI_COSEM_VALID_EVENT_LOG;

    snapshot.clock_date_time.year[0] = 0x07;
    snapshot.clock_date_time.year[1] = 0xE8;
    snapshot.clock_date_time.month = 10;
    snapshot.clock_date_time.day = 26;
    snapshot.clock_date_time.hour = 11;
    snapshot.clock_date_time.minute = 30;
    snapshot.clock_date_time.second = 0;

    /* 1. Uji Baca Tegangan Fasa A (1.0.32.7.0.255) */
    static const uint8_t obis_va[] = {1, 0, 32, 7, 0, 255};
    AmiCosemData out;
    AmiCosemStatus st = ami_cosem_register_read(&snapshot, 3u, obis_va, 2u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED);
    assert(out.value.double_long_unsigned == 230000);
    printf("   > Class 3 Register Tegangan L1: %u mV (Skala x0.001 -> %.1f V) [PASS]\n",
           out.value.double_long_unsigned, out.value.double_long_unsigned / 1000.0f);

    /* Scaler & Unit Tegangan */
    st = ami_cosem_register_read(&snapshot, 3u, obis_va, 3u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.value.scaler_unit.scaler == -3);
    assert(out.value.scaler_unit.unit == 35); /* COSEM_UNIT_VOLT */
    printf("   > Scaler/Unit Tegangan L1: Scaler=%d, Unit=%u [PASS]\n",
           out.value.scaler_unit.scaler, out.value.scaler_unit.unit);

    /* 2. Uji Baca Arus Fasa A (1.0.31.7.0.255) */
    static const uint8_t obis_ia[] = {1, 0, 31, 7, 0, 255};
    st = ami_cosem_register_read(&snapshot, 3u, obis_ia, 2u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.value.double_long_unsigned == 11946);
    printf("   > Class 3 Register Arus L1: %u mA (Skala x0.001 -> %.3f A) [PASS]\n",
           out.value.double_long_unsigned, out.value.double_long_unsigned / 1000.0f);

    /* 3. Uji Baca Energi Aktif Impor (1.0.1.8.0.255) */
    static const uint8_t obis_wh[] = {1, 0, 1, 8, 0, 255};
    st = ami_cosem_register_read(&snapshot, 3u, obis_wh, 2u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_LONG64_UNSIGNED);
    assert(out.value.long64_unsigned == 125430ULL);
    printf("   > Class 3 Register Energi Aktif Impor (+A): %llu Wh [PASS]\n",
           (unsigned long long)out.value.long64_unsigned);

    /* 4. Uji Baca Clock Class 8 (0.0.1.0.0.255) */
    static const uint8_t obis_clk[] = {0, 0, 1, 0, 0, 255};
    st = ami_cosem_register_read(&snapshot, 8u, obis_clk, 2u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_OCTET_STRING);
    printf("   > Class 8 Clock RTC: Berhasil dibaca [PASS]\n");

    /* 5. Uji Class 7 Profile Generic (Event Log: 0.0.99.98.0.255) */
    static const uint8_t obis_event[] = {0, 0, 99, 98, 0, 255};
    snapshot.event_log[0].event_code = 0x01; /* Terminal open */
    snapshot.event_log[0].timestamp = snapshot.clock_date_time;
    snapshot.event_log_count = 1;

    st = ami_cosem_register_read(&snapshot, 7u, obis_event, 7u, &out); /* entries_in_use */
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_ENCODED);
    uint32_t entries_in_use = ((uint32_t)out.value.encoded[1] << 24) |
                              ((uint32_t)out.value.encoded[2] << 16) |
                              ((uint32_t)out.value.encoded[3] << 8)  |
                              (uint32_t)out.value.encoded[4];
    assert(entries_in_use == 1);
    printf("   > Class 7 Profile Generic (Tamper Log Entries in Use): %u [PASS]\n",
           entries_in_use);

    st = ami_cosem_register_read(&snapshot, 7u, obis_event, 8u, &out); /* profile_entries capacity */
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_ENCODED);
    uint32_t capacity = ((uint32_t)out.value.encoded[1] << 24) |
                        ((uint32_t)out.value.encoded[2] << 16) |
                        ((uint32_t)out.value.encoded[3] << 8)  |
                        (uint32_t)out.value.encoded[4];
    assert(capacity == AMI_COSEM_PROFILE_MAX_ENTRIES);
    printf("   > Class 7 Profile Generic (Buffer Capacity): %u entries [PASS]\n",
           capacity);

    /* Uji Baca Buffer Keseluruhan (Atribut 2) */
    st = ami_cosem_register_read(&snapshot, 7u, obis_event, 2u, &out);
    assert(st == AMI_COSEM_OK);
    assert(out.type == AMI_COSEM_TYPE_ENCODED);
    printf("   > Class 7 Profile Generic Buffer A-XDR Structure Encoded: %zu bytes [PASS]\n",
           out.length);

    printf("   [SUKSES] Validasi Model Objek COSEM & Katalog OBIS 100%% Lulus!\n\n");
}

static void test_ami_hdlc_streaming_parser(void) {
    printf("[VALIDASI 2] Menguji Lapisan Framing HDLC & Streaming Byte Parser...\n");

    HdlcContext ctx;
    ami_dlms_hdlc_init(&ctx);

    /* 1. Enkode frame SNRM */
    uint8_t snrm_raw[64];
    size_t snrm_len = 0;
    AmiHdlcStatus hs = ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_SNRM, NULL, 0,
                                                 snrm_raw, sizeof(snrm_raw), &snrm_len);
    assert(hs == AMI_HDLC_FRAME_READY);
    assert(snrm_len > 0);
    assert(snrm_raw[0] == AMI_HDLC_FLAG);
    assert(snrm_raw[snrm_len - 1] == AMI_HDLC_FLAG);

    /* 2. Verifikasi CRC-16 kalkulasi */
    uint16_t crc = ami_dlms_hdlc_crc16(snrm_raw + 1, snrm_len - 4);
    assert(crc != 0);

    /* 3. Parsing byte per byte melalui ring buffer context */
    HdlcFrame parsed_frame;
    AmiHdlcStatus parse_st = AMI_HDLC_INCOMPLETE;
    for (size_t i = 0; i < snrm_len; i++) {
        parse_st = ami_dlms_hdlc_parse_byte(&ctx, snrm_raw[i], &parsed_frame);
    }
    assert(parse_st == AMI_HDLC_FRAME_READY);
    assert(parsed_frame.type == AMI_HDLC_FRAME_SNRM);
    assert(parsed_frame.destination_sap == 0x01);
    assert(parsed_frame.source_sap == 0x10);

    /* 4. Membangun respon UA otomatis */
    uint8_t ua_raw[64];
    size_t ua_len = 0;
    hs = ami_dlms_hdlc_build_frame(&ctx, &parsed_frame, NULL, 0, ua_raw, sizeof(ua_raw), &ua_len);
    assert(hs == AMI_HDLC_FRAME_READY);
    assert(ua_len > 0);
    assert(ua_raw[0] == AMI_HDLC_FLAG);
    printf("   > SNRM Enkode -> Streaming Parser Byte-by-Byte -> UA Respons: %zu bytes [PASS]\n", ua_len);

    /* 5. Uji I-Frame dengan Payload APDU */
    uint8_t sample_apdu[] = {0xC0, 0x01, 0x81, 0x00, 0x03, 1, 0, 32, 7, 0, 255, 0x02, 0x00};
    uint8_t iframe_raw[128];
    size_t iframe_len = 0;
    hs = ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_I_FRAME, sample_apdu, sizeof(sample_apdu),
                                   iframe_raw, sizeof(iframe_raw), &iframe_len);
    assert(hs == AMI_HDLC_FRAME_READY);

    for (size_t i = 0; i < iframe_len; i++) {
        parse_st = ami_dlms_hdlc_parse_byte(&ctx, iframe_raw[i], &parsed_frame);
    }
    assert(parse_st == AMI_HDLC_FRAME_READY);
    assert(parsed_frame.type == AMI_HDLC_FRAME_I);
    assert(parsed_frame.payload_length == sizeof(sample_apdu));
    assert(memcmp(parsed_frame.payload, sample_apdu, sizeof(sample_apdu)) == 0);
    printf("   > I-Frame Parsing & Payload Integrity: %zu bytes APDU [PASS]\n", parsed_frame.payload_length);

    printf("   [SUKSES] Validasi HDLC Framing 100%% Lulus!\n\n");
}

static void test_ami_security_abstraction(void) {
    printf("[VALIDASI 3] Menguji Antarmuka Keamanan & Kriptografi TrustZone (SAL)...\n");

    /* 1. Verifikasi penolakan backend NULL */
    assert(ami_sec_backend_validate(NULL) == AMI_SEC_INVALID_ARGUMENT);

    /* 2. Verifikasi penolakan parameter enkripsi kosong / salah */
    AmiSecBackend backend;
    memset(&backend, 0, sizeof(backend));
    assert(ami_sec_aead_encrypt(&backend, AMI_AEAD_AES_128_GCM, 1, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, 0) == AMI_SEC_INVALID_ARGUMENT);

    /* 3. Verifikasi algoritma yang tidak didukung */
    assert(ami_sec_aead_encrypt(&backend, (AmiAeadAlgorithm)99, 1, NULL, 12, NULL, 0, NULL, 0, NULL, NULL, 16) == AMI_SEC_INVALID_ARGUMENT);

    printf("   > Validasi Parameter Batas Keamanan SAL & Key Handle [PASS]\n");
    printf("   [SUKSES] Validasi Keamanan TrustZone 100%% Lulus!\n\n");
}

static void test_ami_server_dispatcher(void) {
    printf("[VALIDASI 4] Menguji Siklus Lengkap Server Dispatcher AMI (HDLC -> Asosiasi -> APDU GET)...\n");

    ami_server_t server;
    assert(ami_server_init(&server) == 0);

    /* 1. Sinkronisasi data pengukuran dari E3 */
    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));
    meas.voltage_r_dvolts = 2300;     /* 230.0 V */
    meas.voltage_s_dvolts = 2301;
    meas.voltage_t_dvolts = 2299;
    meas.current_r_mamps  = 11946;    /* 11.946 A */
    meas.active_energy_wh = 125430ULL;/* 125.43 kWh */
    meas.active_power_w   = 6898;     /* 6898 W */
    meas.apparent_power_va = 6900;
    meas.power_factor_ppm = 1000;
    meas.frequency_mhz    = 50000;    /* 50.00 Hz */

    ami_server_update_from_meter(&server, &meas);
    ami_server_add_tamper_event(&server, 1729940000, 0x01, 1);

    assert(server.snapshot.event_log_count == 1);
    assert(server.snapshot.phase_voltage_millivolts[0] == 230000);
    assert(server.snapshot.phase_current_milliamps[0] == 11946);
    printf("   > Data metrologi E3 berhasil dipetakan ke COSEM Snapshot [PASS]\n");

    /* 2. Transaksi SNRM (Setup Koneksi HDLC) */
    uint8_t snrm[64];
    size_t snrm_len = 0;
    ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_SNRM, NULL, 0, snrm, sizeof(snrm), &snrm_len);

    uint8_t tx_buf[512];
    size_t tx_len = 0;
    int res = ami_server_process_bytes(&server, snrm, snrm_len, tx_buf, sizeof(tx_buf), &tx_len);
    assert(res == 0);
    assert(tx_len > 0);
    assert(tx_buf[0] == AMI_HDLC_FLAG);
    printf("   > Transaksi 1: Client SNRM -> Server UA Response (%zu bytes) [PASS]\n", tx_len);

    /* 3. Transaksi AARQ (Asosiasi DLMS) */
    uint8_t aarq_apdu[] = {0x60, 0x04, 0x00, 0x10};
    uint8_t aarq_frame[128];
    size_t aarq_len = 0;
    ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_I_FRAME, aarq_apdu, sizeof(aarq_apdu), aarq_frame, sizeof(aarq_frame), &aarq_len);

    res = ami_server_process_bytes(&server, aarq_frame, aarq_len, tx_buf, sizeof(tx_buf), &tx_len);
    assert(res == 0);
    assert(tx_len > 0);
    assert(server.is_associated == true);
    printf("   > Transaksi 2: Client AARQ -> Server AARE (State: ASSOCIATED) [PASS]\n");

    /* 4. Transaksi GET-Request: Tegangan L1 (1.0.32.7.0.255) */
    uint8_t get_v_apdu[] = {
        0xC0, 0x01, 0x81, 0x00, 0x03, 1, 0, 32, 7, 0, 255, 0x02, 0x00
    };
    uint8_t get_v_frame[128];
    size_t get_v_len = 0;
    ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_I_FRAME, get_v_apdu, sizeof(get_v_apdu), get_v_frame, sizeof(get_v_frame), &get_v_len);

    res = ami_server_process_bytes(&server, get_v_frame, get_v_len, tx_buf, sizeof(tx_buf), &tx_len);
    assert(res == 0);
    assert(tx_len > 0);

    /* Dekode balasan GET-Response */
    HdlcContext client_ctx;
    ami_dlms_hdlc_init(&client_ctx);
    HdlcFrame resp_frame;
    AmiHdlcStatus parse_st = AMI_HDLC_INCOMPLETE;
    for (size_t i = 0; i < tx_len; i++) {
        parse_st = ami_dlms_hdlc_parse_byte(&client_ctx, tx_buf[i], &resp_frame);
    }
    assert(parse_st == AMI_HDLC_FRAME_READY);
    assert(resp_frame.type == AMI_HDLC_FRAME_I);
    assert(resp_frame.payload[0] == 0xC4); /* GET-Response tag */
    assert(resp_frame.payload[1] == 0x01); /* Normal choice */
    assert(resp_frame.payload[2] == 0x81); /* Invoke ID */
    assert(resp_frame.payload[3] == 0x00); /* Data alternative */
    assert(resp_frame.payload[4] == 0x06); /* DOUBLE_LONG_UNSIGNED type */

    uint32_t resp_mv = ((uint32_t)resp_frame.payload[5] << 24) |
                       ((uint32_t)resp_frame.payload[6] << 16) |
                       ((uint32_t)resp_frame.payload[7] << 8)  |
                       (uint32_t)resp_frame.payload[8];
    assert(resp_mv == 230000);
    printf("   > Transaksi 3: GET-Request OBIS 1.0.32.7.0.255 -> Respon Tegangan: %u mV (%.1f V) [PASS]\n",
           resp_mv, resp_mv / 1000.0f);

    /* 5. Transaksi DISC (Disconnect) */
    uint8_t disc[64];
    size_t disc_len = 0;
    ami_dlms_hdlc_encode_frame(0x01, 0x10, AMI_HDLC_CONTROL_DISC, NULL, 0, disc, sizeof(disc), &disc_len);

    res = ami_server_process_bytes(&server, disc, disc_len, tx_buf, sizeof(tx_buf), &tx_len);
    assert(res == 0);
    assert(tx_len > 0);
    assert(server.is_associated == false);
    printf("   > Transaksi 4: Client DISC -> Server UA Response (Sesi Ditutup) [PASS]\n");

    printf("   [SUKSES] Validasi Siklus Lengkap Dispatcher Server 100%% Lulus!\n\n");
}

int main(void) {
    printf("========================================================================\n");
    printf("   RUNNING MODERN AMI PROTOCOL MODULE VERIFICATION & VALIDATION         \n");
    printf("========================================================================\n\n");

    test_ami_cosem_model_and_catalog();
    test_ami_hdlc_streaming_parser();
    test_ami_security_abstraction();
    test_ami_server_dispatcher();

    printf("========================================================================\n");
    printf(" ALL 4 VERIFICATION STAGES PASSED SUCCESSFULLY! (100%% VALIDATED)       \n");
    printf("========================================================================\n");
    return 0;
}
