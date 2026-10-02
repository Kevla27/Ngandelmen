#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_hdlc.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_hdlc...\n");

    /* ========================================================================= */
    /* TEST 1: KALKULASI CRC16-CCITT HDLC                                        */
    /* ========================================================================= */
    uint8_t test_data[] = {0xA0, 0x0A, 0x02, 0x23, 0x03, 0x93};
    uint16_t crc1 = dlms_hdlc_crc16(test_data, sizeof(test_data));
    assert(crc1 != 0);

    /* ========================================================================= */
    /* TEST 2: ENKODE & DEKODE BINGKAI KONTROL (Frame SNRM Tanpa Payload)        */
    /* ========================================================================= */
    uint8_t frame_buf[512];
    size_t frame_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, 0x0001, 0x10, NULL, 0, frame_buf, sizeof(frame_buf));
    assert(frame_len > 0);
    assert(frame_buf[0] == HDLC_FLAG);
    assert(frame_buf[frame_len - 1] == HDLC_FLAG);

    /* Dekode Bingkai SNRM */
    uint8_t ctrl_out = 0;
    uint16_t dest_out = 0;
    uint8_t src_out = 0;
    uint8_t payload_out[256];
    size_t payload_len_out = 0;

    assert(dlms_hdlc_decode_frame(frame_buf, frame_len, &ctrl_out, &dest_out, &src_out, payload_out, &payload_len_out) == DLMS_OK);
    assert(ctrl_out == HDLC_CTRL_SNRM);
    assert(dest_out == 0x0001);
    assert(src_out == 0x10);
    assert(payload_len_out == 0);

    /* ========================================================================= */
    /* TEST 3: ENKODE & DEKODE BINGKAI INFORMASI (I-Frame Dengan Payload APDU)   */
    /* ========================================================================= */
    uint8_t apdu_data[] = {0xC0, 0x01, 0x81, 0x00, 0x03, 1, 0, 32, 7, 0, 255, 0x02};
    frame_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, 0x0001, 0x10, apdu_data, sizeof(apdu_data), frame_buf, sizeof(frame_buf));
    assert(frame_len > 0);

    /* Dekode Bingkai I-Frame */
    assert(dlms_hdlc_decode_frame(frame_buf, frame_len, &ctrl_out, &dest_out, &src_out, payload_out, &payload_len_out) == DLMS_OK);
    assert(ctrl_out == HDLC_CTRL_I);
    assert(dest_out == 0x0001);
    assert(src_out == 0x10);
    assert(payload_len_out == sizeof(apdu_data));
    assert(memcmp(payload_out, apdu_data, sizeof(apdu_data)) == 0);

    /* ========================================================================= */
    /* TEST 4: PENGUJIAN BYTE STUFFING (Payload Mengandung Byte 0x7E & 0x7D)      */
    /* ========================================================================= */
    uint8_t tricky_payload[] = {0x7E, 0x7D, 0x00, 0x7E, 0x7D, 0xFF};
    frame_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, 0x0001, 0x10, tricky_payload, sizeof(tricky_payload), frame_buf, sizeof(frame_buf));
    assert(frame_len > 0);

    /* Pastikan dekode menghasilkan payload asli secara sempurna */
    assert(dlms_hdlc_decode_frame(frame_buf, frame_len, &ctrl_out, &dest_out, &src_out, payload_out, &payload_len_out) == DLMS_OK);
    assert(payload_len_out == sizeof(tricky_payload));
    assert(memcmp(payload_out, tricky_payload, sizeof(tricky_payload)) == 0);

    /* ========================================================================= */
    /* TEST 5: DETEKSI DETEKSI KORUPSI FRAME (Manipulasi Byte FCS)             */
    /* ========================================================================= */
    frame_buf[frame_len - 2] ^= 0xFF; /* Rusak byte FCS */
    assert(dlms_hdlc_decode_frame(frame_buf, frame_len, &ctrl_out, &dest_out, &src_out, payload_out, &payload_len_out) == DLMS_ERR_INVALID_APDU);

    printf("[TEST SUCCESS] test_dlms_hdlc 100%% PASSED!\n");
    return 0;
}