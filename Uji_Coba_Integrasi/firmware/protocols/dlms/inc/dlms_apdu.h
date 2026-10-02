#ifndef DLMS_APDU_H
#define DLMS_APDU_H

#include "dlms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tag APDU Utama DLMS COSEM */
#define DLMS_TAG_AARQ          0x60U
#define DLMS_TAG_AARE          0x61U
#define DLMS_TAG_GET_REQUEST   0xC0U
#define DLMS_TAG_GET_RESPONSE  0xC4U
#define DLMS_TAG_RLRQ          0x62U

#define DLMS_GET_REQ_NORMAL    0x01U
#define DLMS_GET_RES_NORMAL    0x01U

#define DLMS_TAG_SET_REQUEST    0xC1

/* Tipe Data A-XDR Standard */
#define DLMS_DATA_TYPE_OCTET_STRING         0x09U
#define DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED 0x06U  /* uint32_t (4 bytes) */
#define DLMS_DATA_TYPE_LONG_UNSIGNED        0x12U  /* uint16_t (2 bytes) */

/* Struktur GET-Request */
typedef struct {
    uint8_t invoke_id;
    uint16_t class_id;
    obis_code_t obis;
    uint8_t attribute_id;
} dlms_get_req_t;

/* Parser & Encoder Function Declarations */
dlms_result_t dlms_parse_aarq(const uint8_t *buffer, size_t len, uint16_t *client_sap);
size_t dlms_encode_aare(uint8_t *buffer, size_t max_len, dlms_result_t status);

dlms_result_t dlms_parse_get_request(const uint8_t *buffer, size_t len, dlms_get_req_t *req);
size_t dlms_encode_get_response(uint8_t *buffer, size_t max_len, uint8_t invoke_id, uint8_t data_type, const void *val_ptr, size_t val_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_APDU_H */