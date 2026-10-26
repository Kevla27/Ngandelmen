#include "dlms_association.h"

dlms_result_t dlms_init(dlms_context_t *ctx) {
    if (ctx == NULL) return DLMS_ERR_NULL_PTR;

    ctx->current_state = DLMS_STATE_UNASSOCIATED;
    ctx->client_sap = 0;
    ctx->server_sap = DLMS_SERVER_SAP_MANAGEMENT;
    ctx->session_timeout_ms = DLMS_INACTIVITY_TIMEOUT_MS;
    ctx->last_activity_time_ms = 0;
    ctx->is_read_only = true;

    return DLMS_OK;
}

dlms_result_t dlms_reset(dlms_context_t *ctx) {
    return dlms_init(ctx);
}

dlms_result_t dlms_process_event(dlms_context_t *ctx, dlms_event_t event, const uint8_t *payload, size_t len) {
    if (ctx == NULL) return DLMS_ERR_NULL_PTR;

    switch (ctx->current_state) {
        case DLMS_STATE_UNASSOCIATED:
            if (event == DLMS_EVT_RX_AARQ) {
                uint16_t requested_client_sap = DLMS_CLIENT_SAP_PUBLIC;
                if (payload != NULL && len >= 2) {
                    /* PERBAIKAN: Gunakan payload dan payload */
                    requested_client_sap = (uint16_t)((payload[0] << 8) | payload[1]);
                }

                if (requested_client_sap == DLMS_CLIENT_SAP_PUBLIC) {
                    ctx->current_state = DLMS_STATE_ASSOCIATED_READONLY;
                    ctx->client_sap = requested_client_sap;
                    ctx->is_read_only = true;
                    return DLMS_OK;
                } else {
                    ctx->current_state = DLMS_STATE_UNASSOCIATED;
                    return DLMS_ERR_UNAUTHORIZED;
                }
            } else if (event == DLMS_EVT_RX_GET_REQ || event == DLMS_EVT_RX_SET_REQ) {
                return DLMS_ERR_INVALID_STATE;
            }
            break;

        case DLMS_STATE_ASSOCIATED_READONLY:
            if (event == DLMS_EVT_RX_GET_REQ) {
                return DLMS_OK;
            } else if (event == DLMS_EVT_RX_SET_REQ) {
                return DLMS_ERR_UNAUTHORIZED; /* Penolakan Read-Only Lock */
            } else if (event == DLMS_EVT_RX_RLRQ || event == DLMS_EVT_TIMEOUT || event == DLMS_EVT_COMM_ERROR) {
                ctx->current_state = DLMS_STATE_UNASSOCIATED;
                return DLMS_OK;
            }
            break;

        default:
            ctx->current_state = DLMS_STATE_UNASSOCIATED;
            break;
    }

    return DLMS_ERR_INVALID_STATE;
}

dlms_result_t dlms_check_timeout(dlms_context_t *ctx, uint32_t current_time_ms) {
    if (ctx == NULL) return DLMS_ERR_NULL_PTR;

    if (ctx->current_state == DLMS_STATE_ASSOCIATED_READONLY) {
        if ((current_time_ms - ctx->last_activity_time_ms) >= ctx->session_timeout_ms) {
            dlms_process_event(ctx, DLMS_EVT_TIMEOUT, NULL, 0);
            return DLMS_ERR_TIMEOUT;
        }
    }
    return DLMS_OK;
}