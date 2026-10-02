#include "dlms_server.h"
#include <string.h>

dlms_result_t dlms_server_init(dlms_server_t *server) {
    if (!server) return DLMS_ERR_NULL_PTR;

    memset(server, 0, sizeof(dlms_server_t));
    dlms_init(&server->ctx);
    dlms_tamper_log_init(&server->tamper_log);
    ring_buffer_init(&server->rx_rb);
    server->server_sap = DLMS_SERVER_SAP_MANAGEMENT;

    return DLMS_OK;
}

dlms_result_t dlms_server_process_bytes(dlms_server_t *server,
                                         const uint8_t *in_data,
                                         size_t in_len,
                                         uint8_t *out_tx_buf,
                                         size_t max_tx_len,
                                         size_t *out_tx_len) {
    if (!server || !out_tx_buf || !out_tx_len) return DLMS_ERR_NULL_PTR;

    *out_tx_len = 0;

    /* 1. Masukkan byte mentah yang masuk ke dalam ring buffer */
    if (in_data && in_len > 0) {
        for (size_t i = 0; i < in_len; i++) {
            ring_buffer_push(&server->rx_rb, in_data[i]);
        }
    }

    /* 2. Cek apakah ring buffer memiliki panjang minimal bingkai HDLC (7 byte) */
    if (ring_buffer_count(&server->rx_rb) < 7) {
        return DLMS_OK;
    }

    uint8_t frame_buf[512];
    size_t frame_len = 0;

    /* Cari flag pembuka HDLC 0x7E */
    uint8_t b = 0;
    while (ring_buffer_count(&server->rx_rb) > 0) {
        ring_buffer_peek(&server->rx_rb, 0, &b);
        if (b == HDLC_FLAG) break;
        ring_buffer_pop(&server->rx_rb, &b); /* Buang byte tidak valid */
    }

    if (ring_buffer_count(&server->rx_rb) < 7) return DLMS_OK;

    /* Cari flag penutup HDLC 0x7E (indeks > 0) */
    size_t end_idx = 0;
    size_t count = ring_buffer_count(&server->rx_rb);
    for (size_t i = 1; i < count; i++) {
        ring_buffer_peek(&server->rx_rb, i, &b);
        if (b == HDLC_FLAG) {
            end_idx = i;
            break;
        }
    }

    if (end_idx == 0) return DLMS_OK; /* Frame belum lengkap */

    /* Pop frame utuh dari ring buffer */
    frame_len = end_idx + 1;
    if (frame_len > sizeof(frame_buf)) frame_len = sizeof(frame_buf);

    for (size_t i = 0; i < frame_len; i++) {
        ring_buffer_pop(&server->rx_rb, &frame_buf[i]);
    }

    /* 3. Dekode Bingkai HDLC */
    uint8_t ctrl_in = 0;
    uint16_t dest_addr = 0;
    uint8_t src_addr = 0;
    uint8_t apdu_in[256];
    size_t apdu_in_len = 0;

    dlms_result_t res = dlms_hdlc_decode_frame(frame_buf, frame_len, &ctrl_in, &dest_addr, &src_addr, apdu_in, &apdu_in_len);
    if (res != DLMS_OK) return res;

    /* 4. Pemrosesan Logika Berdasarkan Byte Kontrol HDLC */
    uint8_t apdu_out[256];
    size_t apdu_out_len = 0;

    if (ctrl_in == HDLC_CTRL_SNRM) {
        /* Permintaan Koneksi HDLC (SNRM) -> Balas UA */
        dlms_init(&server->ctx);
        *out_tx_len = dlms_hdlc_encode_frame(HDLC_CTRL_UA, src_addr, (uint8_t)server->server_sap, NULL, 0, out_tx_buf, max_tx_len);
        return DLMS_OK;
    } else if (ctrl_in == HDLC_CTRL_DISC) {
        /* Permintaan Putus Koneksi HDLC (DISC) -> Balas UA */
        dlms_process_event(&server->ctx, DLMS_EVT_RX_RLRQ, NULL, 0);
        *out_tx_len = dlms_hdlc_encode_frame(HDLC_CTRL_UA, src_addr, (uint8_t)server->server_sap, NULL, 0, out_tx_buf, max_tx_len);
        return DLMS_OK;
    } else if (ctrl_in == HDLC_CTRL_I || ctrl_in == HDLC_CTRL_UI) {
        /* I-Frame Membawa Payload APDU DLMS */
        if (apdu_in_len == 0) return DLMS_ERR_INVALID_APDU;

        uint8_t tag = apdu_in[0];

        if (tag == DLMS_TAG_AARQ) {
            uint16_t client_sap = 0;
            if (dlms_parse_aarq(apdu_in, apdu_in_len, &client_sap) == DLMS_OK) {
                uint8_t aarq_payload[] = {(uint8_t)(client_sap >> 8), (uint8_t)(client_sap & 0xFF)};
                dlms_process_event(&server->ctx, DLMS_EVT_RX_AARQ, aarq_payload, sizeof(aarq_payload));
                apdu_out_len = dlms_encode_aare(apdu_out, sizeof(apdu_out), DLMS_OK);
            }
        } else if (tag == DLMS_TAG_GET_REQUEST) {
            dlms_get_req_t req;
            if (dlms_parse_get_request(apdu_in, apdu_in_len, &req) == DLMS_OK) {
                if (dlms_process_event(&server->ctx, DLMS_EVT_RX_GET_REQ, NULL, 0) == DLMS_OK) {
                    uint8_t data_type = 0;
                    uint32_t val_u32 = 0;

                    /* Pembacaan Tamper Event Log (Class 7 Profile Generic) */
                    if (req.class_id == 7 && req.obis.c == 99 && req.obis.d == 98) {
                        uint8_t log_axdr[256];
                        size_t log_axdr_len = dlms_tamper_log_encode_axdr(&server->tamper_log, log_axdr, sizeof(log_axdr));
                        apdu_out_len = dlms_encode_get_response(apdu_out, sizeof(apdu_out), req.invoke_id, DLMS_DATA_TYPE_OCTET_STRING, log_axdr, log_axdr_len);
                    } else {
                        /* Pembacaan Register OBIS Standar */
                        if (dlms_obis_lookup(req.class_id, &req.obis, req.attribute_id, &data_type, &val_u32) == DLMS_OK) {
                            apdu_out_len = dlms_encode_get_response(apdu_out, sizeof(apdu_out), req.invoke_id, data_type, &val_u32, sizeof(val_u32));
                        }
                    }
                }
            }
        } else if (tag == DLMS_TAG_SET_REQUEST) {
            /* Proteksi Read-Only Lock */
            dlms_process_event(&server->ctx, DLMS_EVT_RX_SET_REQ, NULL, 0);
        }

        if (apdu_out_len > 0) {
            *out_tx_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, src_addr, (uint8_t)server->server_sap, apdu_out, apdu_out_len, out_tx_buf, max_tx_len);
        }
        return DLMS_OK;
    }

    return DLMS_OK;
}