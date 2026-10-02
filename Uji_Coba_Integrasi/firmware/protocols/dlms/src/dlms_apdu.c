#include "dlms_apdu.h"

#include <string.h>

dlms_result_t dlms_parse_aarq(const uint8_t *buffer, size_t len, uint16_t *client_sap) {
    if (!buffer || !client_sap) return DLMS_ERR_NULL_PTR;
    if (len < 4 || buffer[0]!= DLMS_TAG_AARQ) return DLMS_ERR_INVALID_APDU;

    if (len >= 6) {
        *client_sap = (uint16_t)((buffer[len - 2] << 8) | buffer[len - 1]);
    } else {
        *client_sap = DLMS_CLIENT_SAP_PUBLIC;
    }
    return DLMS_OK;
}

size_t dlms_encode_aare(uint8_t *buffer, size_t max_len, dlms_result_t status) {
    if (!buffer || max_len < 16) return 0;

    uint8_t result_code = (status == DLMS_OK) ? 0x00 : 0x01;

    buffer[0] = DLMS_TAG_AARE;
    buffer[1]  = 0x0E;
    buffer[2]  = 0x00;
    buffer[3]  = 0x07;
    buffer[4]  = 0x60; buffer[5]  = 0x85; buffer[6]  = 0x74; buffer[7]  = 0x05;
    buffer[8]  = 0x08; buffer[9]  = 0x01; buffer[10] = 0x01; /* LN without ciphering */
    buffer[11] = 0x02;
    buffer[12] = 0x01;
    buffer[13] = result_code;
    buffer[14] = (uint8_t)(DLMS_SERVER_SAP_MANAGEMENT >> 8);
    buffer[15] = (uint8_t)(DLMS_SERVER_SAP_MANAGEMENT & 0xFF);

    return 16;
}

dlms_result_t dlms_parse_get_request(const uint8_t *buffer, size_t len, dlms_get_req_t *req) {
    if (!buffer || !req) return DLMS_ERR_NULL_PTR;
    if (len < 12 || buffer[0] != DLMS_TAG_GET_REQUEST || buffer[1] != DLMS_GET_REQ_NORMAL) {
        return DLMS_ERR_INVALID_APDU;
    }

    req->invoke_id = buffer[2];
    req->class_id = (uint16_t)((buffer[3] << 8) | buffer[4]);
    req->obis.a = buffer[5]; req->obis.b = buffer[6]; req->obis.c = buffer[7];
    req->obis.d = buffer[8]; req->obis.e = buffer[9]; req->obis.f = buffer[10];
    req->attribute_id = buffer[11];

    return DLMS_OK;
}

size_t dlms_encode_get_response(uint8_t *buffer, size_t max_len, uint8_t invoke_id, uint8_t data_type, const void *val_ptr, size_t val_len) {
    if (!buffer || max_len < 8) return 0;

    buffer[0] = DLMS_TAG_GET_RESPONSE;
    buffer[1] = DLMS_GET_RES_NORMAL;
    buffer[2] = invoke_id;
    buffer[3] = 0x00; /* Data Success */
    buffer[4] = data_type;

    size_t offset = 5;

    if (data_type == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED) {
        if (max_len < offset + 4 || !val_ptr) return 0;
        uint32_t val = *(const uint32_t *)val_ptr;
        buffer[offset++] = (uint8_t)((val >> 24) & 0xFF);
        buffer[offset++] = (uint8_t)((val >> 16) & 0xFF);
        buffer[offset++] = (uint8_t)((val >> 8) & 0xFF);
        buffer[offset++] = (uint8_t)(val & 0xFF);
    } else if (data_type == DLMS_DATA_TYPE_LONG_UNSIGNED) {
        if (max_len < offset + 2 || !val_ptr) return 0;
        uint16_t val = (uint16_t)(*(const uint32_t *)val_ptr);
        buffer[offset++] = (uint8_t)((val >> 8) & 0xFF);
        buffer[offset++] = (uint8_t)(val & 0xFF);
    }

    return offset;
}