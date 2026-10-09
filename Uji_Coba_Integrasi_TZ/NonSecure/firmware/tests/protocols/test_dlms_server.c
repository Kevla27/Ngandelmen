#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_server.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_server...\n");

    dlms_server_t server;
    assert(dlms_server_init(&server) == DLMS_OK);
    assert(server.ctx.current_state == DLMS_STATE_UNASSOCIATED);

    uint8_t tx_buf[512];
    size_t tx_len = 0;

    /* 1. Transaksi SNRM (Inisialisasi Lapisan HDLC) */
    uint8_t snrm_frame[128];
    size_t snrm_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, DLMS_SERVER_SAP_MANAGEMENT, 0x10, NULL, 0, snrm_frame, sizeof(snrm_frame));
    assert(snrm_len > 0);

    assert(dlms_server_process_bytes(&server, snrm_frame, snrm_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);
    assert(tx_buf[0] == HDLC_FLAG);

    /* 2. Transaksi AARQ (Asosiasi DLMS Upper Layer) */
    uint8_t aarq_apdu[] = {0x60, 0x04, 0x00, 0x10};
    uint8_t aarq_hdlc[256];
    size_t aarq_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, aarq_apdu, sizeof(aarq_apdu), aarq_hdlc, sizeof(aarq_hdlc));

    assert(dlms_server_process_bytes(&server, aarq_hdlc, aarq_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);
    assert(server.ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);

    /* 3. Transaksi GET-Request (Membaca Register OBIS Energi LWBP 1.0.1.8.1.255) */
    uint8_t get_req_apdu[] = {
        0xC0, 0x01, 0x82, 0x00, 0x03, 1, 0, 1, 8, 1, 255, 0x02
    };
    uint8_t get_req_hdlc[256];
    size_t get_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, get_req_apdu, sizeof(get_req_apdu), get_req_hdlc, sizeof(get_req_hdlc));

    assert(dlms_server_process_bytes(&server, get_req_hdlc, get_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);

    /* Verifikasi Isi Payload GET-Response HDLC Balasan */
    uint8_t res_ctrl = 0;
    uint16_t res_dest = 0;
    uint8_t res_src = 0;
    uint8_t res_apdu[256];
    size_t res_apdu_len = 0;

    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &res_ctrl, &res_dest, &res_src, res_apdu, &res_apdu_len) == DLMS_OK);
    assert(res_apdu_len >= 9);
    assert(res_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(res_apdu[4] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);

    /* 4. Transaksi DISC (Pelepasan Koneksi HDLC) */
    uint8_t disc_frame[128];
    size_t disc_len = dlms_hdlc_encode_frame(HDLC_CTRL_DISC, DLMS_SERVER_SAP_MANAGEMENT, 0x10, NULL, 0, disc_frame, sizeof(disc_frame));

    assert(dlms_server_process_bytes(&server, disc_frame, disc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);
    assert(server.ctx.current_state == DLMS_STATE_UNASSOCIATED);

    printf("[TEST SUCCESS] test_dlms_server 100%% PASSED!\n");
    return 0;
}