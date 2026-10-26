#ifndef DLMS_TYPES_H
#define DLMS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parameter Terkunci Gate G0 Baseline */
#define DLMS_SERVER_SAP_MANAGEMENT  0x0001U  /* Management Logical Device */
#define DLMS_CLIENT_SAP_PUBLIC      0x0010U  /* Public Client SAP (Read-Only) */
#define DLMS_MAX_APDU_SIZE          256U     /* Optimalisasi RAM Cortex-M33 */
#define DLMS_INACTIVITY_TIMEOUT_MS  30000U   /* Timeout 30 detik */

/* Enumerasi Status Sesi Asosiasi */
typedef enum {
    DLMS_STATE_UNASSOCIATED = 0,
    DLMS_STATE_ASSOCIATING,
    DLMS_STATE_ASSOCIATED_READONLY,
    DLMS_STATE_DISCONNECTING
} dlms_state_t;

/* Enumerasi Pemicu Kejadian (Event Code) */
typedef enum {
    DLMS_EVT_NONE = 0,
    DLMS_EVT_RX_AARQ,
    DLMS_EVT_RX_GET_REQ,
    DLMS_EVT_RX_SET_REQ,
    DLMS_EVT_RX_RLRQ,
    DLMS_EVT_TIMEOUT,
    DLMS_EVT_COMM_ERROR
} dlms_event_t;

/* Enumerasi Kode Kembalian Error (Return Code) */
typedef enum {
    DLMS_OK = 0,
    DLMS_ERR_NULL_PTR,
    DLMS_ERR_INVALID_PARAM,
    DLMS_ERR_INVALID_STATE,
    DLMS_ERR_UNAUTHORIZED,
    DLMS_ERR_INVALID_APDU,
    DLMS_ERR_BUFFER_OVERFLOW,
    DLMS_ERR_BUFFER_EMPTY,
    DLMS_ERR_TIMEOUT
} dlms_result_t;

/* Struktur Objek Identifikasi OBIS (6 Byte) */
typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
} obis_code_t;

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TYPES_H */