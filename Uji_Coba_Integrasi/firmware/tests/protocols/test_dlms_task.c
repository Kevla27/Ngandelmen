#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_task.h"

static uint8_t g_last_tx_buf[512];
static size_t  g_last_tx_len = 0;

static void mock_uart_tx(const uint8_t *data, size_t len) {
    if (data && len > 0 && len <= sizeof(g_last_tx_buf)) {
        memcpy(g_last_tx_buf, data, len);
        g_last_tx_len = len;
    }
}

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_task...\n");

    dlms_task_ctx_t task_ctx;
    assert(dlms_task_init(&task_ctx, mock_uart_tx) == DLMS_OK);
    assert(task_ctx.is_running == true);

    /* 1. Simulasi SNRM Frame masuk dari ISR UART */
    uint8_t snrm_frame[64];
    size_t snrm_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, DLMS_SERVER_SAP_MANAGEMENT, 0x10, NULL, 0, snrm_frame, sizeof(snrm_frame));
    assert(snrm_len > 0);

    /* Push data ke Task */
    assert(dlms_task_notify_rx(&task_ctx, snrm_frame, snrm_len) == DLMS_OK);

    /* Eksekusi 1 step Task */
    g_last_tx_len = 0;
    assert(dlms_task_step(&task_ctx) == DLMS_OK);
    assert(g_last_tx_len > 0);
    assert(g_last_tx_buf[0] == HDLC_FLAG);

    /* 2. Simulasi AARQ Frame masuk */
    uint8_t aarq_apdu[] = {0x60, 0x04, 0x00, 0x10};
    uint8_t aarq_hdlc[64];
    size_t aarq_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, 0x10, aarq_apdu, sizeof(aarq_apdu), aarq_hdlc, sizeof(aarq_hdlc));

    assert(dlms_task_notify_rx(&task_ctx, aarq_hdlc, aarq_hdlc_len) == DLMS_OK);

    g_last_tx_len = 0;
    assert(dlms_task_step(&task_ctx) == DLMS_OK);
    assert(g_last_tx_len > 0);
    assert(task_ctx.server.ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);

    printf("[TEST SUCCESS] test_dlms_task 100%% PASSED!\n");
    return 0;
}