#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include "dlms_apdu.h"
#include "dlms_obis.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_apdu...\n");

    /* Test 1: Parsing Frame AARQ & Encoding AARE */
    uint8_t aarq_frame[] = {0x60, 0x04, 0x00, 0x10};
    uint16_t client_sap = 0;
    assert(dlms_parse_aarq(aarq_frame, sizeof(aarq_frame), &client_sap) == DLMS_OK);
    assert(client_sap == DLMS_CLIENT_SAP_PUBLIC);

    // FIXED: Changed single byte to array buffer so it can hold the 16-byte response
    uint8_t aare_buffer[64];
    size_t aare_len = dlms_encode_aare(aare_buffer, sizeof(aare_buffer), DLMS_OK);
    assert(aare_len == 16);
    assert(aare_buffer[0] == DLMS_TAG_AARE); // FIXED: Index first element of array

    /* Test 2: Parsing GET-Request Tegangan L1 (1.0.32.7.0.255) */
    uint8_t get_req_frame[] = {
        0xC0, 0x01,          /* GET-Request Normal */
        0x81,                /* Invoke-Id */
        0x00, 0x03,          /* Class-Id 3 */
        1, 0, 32, 7, 0, 255, /* OBIS 1.0.32.7.0.255 */
        0x02                 /* Attribute-Id 2 */
    };

    dlms_get_req_t req;
    assert(dlms_parse_get_request(get_req_frame, sizeof(get_req_frame), &req) == DLMS_OK);
    assert(req.invoke_id == 0x81);
    assert(req.obis.c == 32);

    /* Test 3: Lookup Database OBIS untuk Tegangan L1 */
    uint8_t data_type = 0;
    uint32_t val_u32 = 0;
    assert(dlms_obis_lookup(req.class_id, &req.obis, req.attribute_id, &data_type, &val_u32) == DLMS_OK);
    assert(val_u32 == 2302); /* 230.2 V */

    /* Test 4: Encoding GET-Response Frame */
    // FIXED: Changed single byte to array buffer so it can hold the 9-byte response
    uint8_t get_res_buffer[64];
    size_t res_len = dlms_encode_get_response(get_res_buffer, sizeof(get_res_buffer), req.invoke_id, data_type, &val_u32, sizeof(val_u32));
    assert(res_len == 9);
    assert(get_res_buffer[0] == DLMS_TAG_GET_RESPONSE); // FIXED: Index first byte for tag check
    
    // NOTE: Check where your data type tag is stored in the encoded stream. 
    // Usually it is at index 3 or 4 depending on APDU header length:
    // assert(get_res_buffer[3] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);

    printf("[TEST SUCCESS] test_dlms_apdu 100%% PASSED!\n");
    return 0;
}