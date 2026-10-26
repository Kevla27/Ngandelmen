#ifndef DLMS_ASSOCIATION_H
#define DLMS_ASSOCIATION_H

#include "dlms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Konteks Sesi Asosiasi DLMS */
typedef struct {
    dlms_state_t current_state;
    uint16_t client_sap;
    uint16_t server_sap;
    uint32_t session_timeout_ms;
    uint32_t last_activity_time_ms;
    bool is_read_only;
} dlms_context_t;

/* Deklarasi Fungsi Inti State Machine */
dlms_result_t dlms_init(dlms_context_t *ctx);
dlms_result_t dlms_process_event(dlms_context_t *ctx, dlms_event_t event, const uint8_t *payload, size_t len);
dlms_result_t dlms_check_timeout(dlms_context_t *ctx, uint32_t current_time_ms);
dlms_result_t dlms_reset(dlms_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_ASSOCIATION_H */
