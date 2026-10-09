#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_association.h"
#include "dlms_apdu.h"
#include "dlms_obis.h"
#include "dlms_transport.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_integration...\n");

    /* ========================================================================= */
    /* STEP 1: INISIALISASI CONTEXT & TRANSPORT RING BUFFER                      */
    /* ========================================================================= */
    dlms_context_t ctx;
    assert(dlms_init(&ctx) == DLMS_OK);
    assert(ctx.current_state == DLMS_STATE_UNASSOCIATED);

    ring_buffer_t rx_rb;
    assert(ring_buffer_init(&rx_rb) == DLMS_OK);

    /* ========================================================================= */
    /* STEP 2: PROSES ASOSIASI DLMS (AARQ -> STATE ASSOCIATED -> AARE)           */
    /* ========================================================================= */
    uint8_t aarq_frame[] = {0x60, 0x04, 0x00, 0x10}; /* Frame AARQ SAP Public 0x0010 */
    uint16_t client_sap = 0;
    
    /* Parse Frame AARQ */
    assert(dlms_parse_aarq(aarq_frame, sizeof(aarq_frame), &client_sap) == DLMS_OK);
    assert(client_sap == DLMS_CLIENT_SAP_PUBLIC);

    /* Eksekusi Transisi State Asosiasi */
    uint8_t aarq_payload[] = {0x00, 0x10};
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_AARQ, aarq_payload, sizeof(aarq_payload)) == DLMS_OK);
    assert(ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);

    /* Encode Response AARE */
    /* FIXED: Changed single byte to array buffer to hold the 16-byte response */
    uint8_t aare_out[32];
    size_t aare_len = dlms_encode_aare(aare_out, sizeof(aare_out), DLMS_OK);
    assert(aare_len == 16);
    assert(aare_out[0] == DLMS_TAG_AARE); /* FIXED: Check first byte of the array */

    /* ========================================================================= */
    /* STEP 3: GET-REQUEST OBIS BARU (Energi Aktif LWBP Tariff 1: 1.0.1.8.1.255) */
    /* ========================================================================= */
    uint8_t get_req_lwbp[] = {
        0xC0, 0x01,          /* GET-Request Normal */
        0x82,                /* Invoke-Id 0x82 */
        0x00, 0x03,          /* Class-Id 3 (Register) */
        1, 0, 1, 8, 1, 255,  /* OBIS 1.0.1.8.1.255 */
        0x02                 /* Attribute-Id 2 (Value) */
    };

    dlms_get_req_t req;
    assert(dlms_parse_get_request(get_req_lwbp, sizeof(get_req_lwbp), &req) == DLMS_OK);
    assert(req.invoke_id == 0x82);
    assert(req.class_id == 3);
    assert(req.obis.c == 1 && req.obis.d == 8 && req.obis.e == 1);

    /* Cek izin State Machine untuk menerima GET Request */
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_GET_REQ, NULL, 0) == DLMS_OK);

    /* Lookup Objek di Database OBIS */
    uint8_t data_type = 0;
    uint32_t val_u32 = 0;
    assert(dlms_obis_lookup(req.class_id, &req.obis, req.attribute_id, &data_type, &val_u32) == DLMS_OK);
    assert(data_type == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);
    assert(val_u32 == 80000); /* Nilai 800.00 kWh (skala x0.01) */

    /* Encode Response GET-Response */
    /* FIXED: Changed single byte to array buffer to hold the 9-byte response */
    uint8_t get_res_out[32];
    size_t res_len = dlms_encode_get_response(get_res_out, sizeof(get_res_out), req.invoke_id, data_type, &val_u32, sizeof(val_u32));
    assert(res_len == 9);
    assert(get_res_out[0] == DLMS_TAG_GET_RESPONSE); 
    assert(get_res_out[4] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);/* FIXED: Check first byte of the array */

    /* ========================================================================= */
    /* STEP 4: GET-REQUEST OBIS TAMPER COUNTER (Cover Open: 0.0.96.20.5.255)     */
    /* ========================================================================= */
    uint8_t get_req_tamper[] = {
        0xC0, 0x01,          /* GET-Request Normal */
        0x83,                /* Invoke-Id 0x83 */
        0x00, 0x01,          /* Class-Id 1 (Data) */
        0, 0, 96, 20, 5, 255,/* OBIS 0.0.96.20.5.255 */
        0x02                 /* Attribute-Id 2 (Value) */
    };

    assert(dlms_parse_get_request(get_req_tamper, sizeof(get_req_tamper), &req) == DLMS_OK);
    assert(dlms_obis_lookup(req.class_id, &req.obis, req.attribute_id, &data_type, &val_u32) == DLMS_OK);
    assert(data_type == DLMS_DATA_TYPE_LONG_UNSIGNED);
    assert(val_u32 == 1); /* 1 Kali Kejadian Tamper */

    /* ========================================================================= */
    /* STEP 5: UJI PROTEKSI READ-ONLY LOCK (PENOLAKAN SET-REQUEST)               */
    /* ========================================================================= */
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_SET_REQ, NULL, 0) == DLMS_ERR_UNAUTHORIZED);
    assert(ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY); /* State tetap dikunci */

    /* ========================================================================= */
    /* STEP 6: PELEPASAN KONEKSI / DISCONNECT (RLRQ -> UNASSOCIATED)            */
    /* ========================================================================= */
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_RLRQ, NULL, 0) == DLMS_OK);
    assert(ctx.current_state == DLMS_STATE_UNASSOCIATED);

    printf("[TEST SUCCESS] test_dlms_integration 100%% PASSED!\n");
    return 0;
}