/**
 * @file ami_server.c
 * @brief Implementasi Dispatcher Server DLMS/COSEM Modern berbasis AMI Stack
 */

#include "ami_server.h"
#include <string.h>

int ami_server_init(ami_server_t *server) {
    if (!server) return -1;
    memset(server, 0, sizeof(*server));
    ami_dlms_hdlc_init(&server->hdlc_ctx);
    ami_dlms_association_reset(&server->assoc);
    ring_buffer_init(&server->rx_rb);
    server->server_sap = 1u;      /* Management Logical Device (SAP 1) */
    server->client_sap = 0x10u;   /* Public Client SAP */
    server->is_associated = false;

    /* Inisialisasi status Clock default */
    server->snapshot.clock_date_time.year[0] = 0x07;
    server->snapshot.clock_date_time.year[1] = 0xE8; /* 2024 */
    server->snapshot.clock_date_time.month   = 10;
    server->snapshot.clock_date_time.day     = 26;
    server->snapshot.clock_date_time.hour    = 12;
    server->snapshot.clock_date_time.minute  = 0;
    server->snapshot.clock_date_time.second  = 0;
    server->snapshot.clock_date_time.day_of_week = 6;
    server->snapshot.clock_status = 0;
    server->snapshot.valid_mask = AMI_COSEM_VALID_CLOCK;

    return 0;
}

void ami_server_reset(ami_server_t *server) {
    if (!server) return;
    ami_dlms_hdlc_init(&server->hdlc_ctx);
    ami_dlms_association_reset(&server->assoc);
    ring_buffer_init(&server->rx_rb);
    server->is_associated = false;
}

void ami_server_update_from_meter(ami_server_t *server, const meter_measurements_t *meas) {
    if (!server || !meas) return;

    /* Snapshot tegangan fasa R, S, T (dalam milivolt) */
    server->snapshot.phase_voltage_millivolts[0] = meas->voltage_r_dvolts * 100u;
    server->snapshot.phase_voltage_millivolts[1] = meas->voltage_s_dvolts * 100u;
    server->snapshot.phase_voltage_millivolts[2] = meas->voltage_t_dvolts * 100u;

    /* Snapshot arus fasa R, S, T (dalam miliampere) */
    server->snapshot.phase_current_milliamps[0] = meas->current_r_mamps;
    server->snapshot.phase_current_milliamps[1] = meas->current_s_mamps;
    server->snapshot.phase_current_milliamps[2] = meas->current_t_mamps;

    /* Energi aktif impor */
    server->snapshot.active_import_wh = meas->active_energy_wh;

    server->snapshot.valid_mask |= (AMI_COSEM_VALID_VOLTAGE_A |
                                    AMI_COSEM_VALID_VOLTAGE_B |
                                    AMI_COSEM_VALID_VOLTAGE_C |
                                    AMI_COSEM_VALID_CURRENT_A |
                                    AMI_COSEM_VALID_CURRENT_B |
                                    AMI_COSEM_VALID_CURRENT_C |
                                    AMI_COSEM_VALID_ACTIVE_IMPORT);

    /* Pemetaan Extended Registers SPLN (Frekuensi, Daya, PF) */
    size_t sc = 0;

    /* 1. Frekuensi Grid (1.0.14.7.0.255) */
    static const uint8_t obis_freq[] = {1, 0, 14, 7, 0, 255};
    memcpy(server->snapshot.measurement_samples[sc].logical_name, obis_freq, 6);
    server->snapshot.measurement_samples[sc].raw_value = meas->frequency_mhz;
    server->snapshot.measurement_samples[sc].valid = 1;
    sc++;

    /* 2. Daya Aktif Ekspor / Impor */
    static const uint8_t obis_act_imp[] = {1, 0, 2, 7, 0, 255};
    memcpy(server->snapshot.measurement_samples[sc].logical_name, obis_act_imp, 6);
    server->snapshot.measurement_samples[sc].raw_value = (meas->active_power_w > 0) ? (uint64_t)meas->active_power_w : 0u;
    server->snapshot.measurement_samples[sc].valid = 1;
    sc++;

    static const uint8_t obis_act_exp[] = {1, 0, 1, 7, 0, 255};
    memcpy(server->snapshot.measurement_samples[sc].logical_name, obis_act_exp, 6);
    server->snapshot.measurement_samples[sc].raw_value = (meas->active_power_w < 0) ? (uint64_t)(-meas->active_power_w) : 0u;
    server->snapshot.measurement_samples[sc].valid = 1;
    sc++;

    /* 3. Daya Semu Total (1.0.9.7.0.255) */
    static const uint8_t obis_app_exp[] = {1, 0, 9, 7, 0, 255};
    memcpy(server->snapshot.measurement_samples[sc].logical_name, obis_app_exp, 6);
    server->snapshot.measurement_samples[sc].raw_value = meas->apparent_power_va;
    server->snapshot.measurement_samples[sc].valid = 1;
    sc++;

    /* 4. Power Factor Total (1.0.13.7.0.255) */
    static const uint8_t obis_pf[] = {1, 0, 13, 7, 0, 255};
    memcpy(server->snapshot.measurement_samples[sc].logical_name, obis_pf, 6);
    server->snapshot.measurement_samples[sc].signed_value = (int32_t)meas->power_factor_ppm;
    server->snapshot.measurement_samples[sc].valid = 1;
    sc++;

    server->snapshot.measurement_sample_count = sc;
}

void ami_server_add_tamper_event(ami_server_t *server, uint32_t timestamp_epoch, uint8_t code, uint8_t status) {
    if (!server) return;
    (void)status;
    size_t count = server->snapshot.event_log_count;
    if (count < AMI_COSEM_PROFILE_MAX_ENTRIES) {
        AmiCosemEventLogEntry *e = &server->snapshot.event_log[count];
        e->event_code = code;
        e->timestamp.year[0] = 0x07;
        e->timestamp.year[1] = 0xE8;
        e->timestamp.month   = 10;
        e->timestamp.day     = 26;
        e->timestamp.hour    = (uint8_t)((timestamp_epoch / 3600) % 24);
        e->timestamp.minute  = (uint8_t)((timestamp_epoch / 60) % 60);
        e->timestamp.second  = (uint8_t)(timestamp_epoch % 60);
        e->timestamp.day_of_week = 6;
        server->snapshot.event_log_count++;
        server->snapshot.valid_mask |= AMI_COSEM_VALID_EVENT_LOG;
    }
}

int ami_server_process_bytes(ami_server_t *server,
                             const uint8_t *in_data,
                             size_t in_len,
                             uint8_t *out_tx_buf,
                             size_t max_tx_len,
                             size_t *out_tx_len) {
    if (!server || !out_tx_buf || !out_tx_len) return -1;
    *out_tx_len = 0;

    if (in_data && in_len > 0) {
        for (size_t i = 0; i < in_len; i++) {
            ring_buffer_push(&server->rx_rb, in_data[i]);
        }
    }

    HdlcFrame frame;
    uint8_t b = 0;
    while (ring_buffer_count(&server->rx_rb) > 0) {
        if (ring_buffer_pop(&server->rx_rb, &b) != DLMS_OK) break;
        AmiHdlcStatus hs = ami_dlms_hdlc_parse_byte(&server->hdlc_ctx, b, &frame);
        if (hs == AMI_HDLC_FRAME_READY) {
            if (frame.type == AMI_HDLC_FRAME_SNRM) {
                ami_dlms_association_reset(&server->assoc);
                server->is_associated = false;
                server->client_sap = frame.source_sap;
                ami_dlms_hdlc_build_frame(&server->hdlc_ctx, &frame, NULL, 0, out_tx_buf, max_tx_len, out_tx_len);
                return 0;
            } else if (frame.type == AMI_HDLC_FRAME_DISC) {
                ami_dlms_association_reset(&server->assoc);
                server->is_associated = false;
                ami_dlms_hdlc_build_frame(&server->hdlc_ctx, &frame, NULL, 0, out_tx_buf, max_tx_len, out_tx_len);
                return 0;
            } else if (frame.type == AMI_HDLC_FRAME_RR) {
                ami_dlms_hdlc_build_frame(&server->hdlc_ctx, &frame, NULL, 0, out_tx_buf, max_tx_len, out_tx_len);
                return 0;
            } else if (frame.type == AMI_HDLC_FRAME_I) {
                if (frame.payload_length == 0) return -1;
                uint8_t tag = frame.payload[0];
                if (tag == 0x60) { /* AARQ */
                    uint8_t aare[128];
                    size_t aare_len = 0;
                    AmiDlmsAssociationStatus ast = ami_dlms_process_aarq(&server->assoc, frame.payload, frame.payload_length, aare, sizeof(aare), &aare_len);
                    if (ast == AMI_DLMS_ASSOC_OK) {
                        server->is_associated = true;
                        ami_dlms_hdlc_build_frame(&server->hdlc_ctx, &frame, aare, aare_len, out_tx_buf, max_tx_len, out_tx_len);
                        return 0;
                    }
                } else if (tag == 0xC0) { /* GET-Request */
                    uint8_t res_apdu[512];
                    size_t res_len = 0;
                    AmiDlmsApduStatus apdu_st = ami_dlms_process_get_request(&server->assoc, &server->snapshot, frame.payload, frame.payload_length, res_apdu, sizeof(res_apdu), &res_len);
                    if (apdu_st == AMI_DLMS_APDU_OK || res_len > 0) {
                        ami_dlms_hdlc_build_frame(&server->hdlc_ctx, &frame, res_apdu, res_len, out_tx_buf, max_tx_len, out_tx_len);
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

