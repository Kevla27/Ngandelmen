#include "dlms_task.h"
#include <string.h>

dlms_result_t dlms_task_init(dlms_task_ctx_t *task_ctx, dlms_uart_tx_fn_t tx_cb) {
    if (!task_ctx) return DLMS_ERR_NULL_PTR;

    memset(task_ctx, 0, sizeof(dlms_task_ctx_t));
    dlms_result_t res = dlms_server_init(&task_ctx->server);
    if (res != DLMS_OK) return res;

    task_ctx->tx_cb = tx_cb;
    task_ctx->is_running = true;

    return DLMS_OK;
}

dlms_result_t dlms_task_notify_rx(dlms_task_ctx_t *task_ctx, const uint8_t *data, size_t len) {
    if (!task_ctx || !data || len == 0) return DLMS_ERR_NULL_PTR;

    /* Memasukkan byte dari ISR/DMA ke ring buffer server */
    for (size_t i = 0; i < len; i++) {
        ring_buffer_push(&task_ctx->server.rx_rb, data[i]);
    }

    return DLMS_OK;
}

dlms_result_t dlms_task_step(dlms_task_ctx_t *task_ctx) {
    if (!task_ctx) return DLMS_ERR_NULL_PTR;

    uint8_t tx_buf[512];
    size_t tx_len = 0;

    /* Memproses stream byte dalam ring buffer secara non-blocking */
    dlms_result_t res = dlms_server_process_bytes(&task_ctx->server,
                                                  NULL, 0,
                                                  tx_buf, sizeof(tx_buf),
                                                  &tx_len);

    if (res == DLMS_OK && tx_len > 0) {
        if (task_ctx->tx_cb) {
            task_ctx->tx_cb(tx_buf, tx_len);
        }
    }

    return res;
}

void dlms_task_entry(void *pvParameters) {
    dlms_task_ctx_t *task_ctx = (dlms_task_ctx_t *)pvParameters;
    if (!task_ctx) return;

    while (task_ctx->is_running) {
        dlms_task_step(task_ctx);

#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
        /* Berikan kesempatan eksekusi ke task lain jika tidak ada data */
        vTaskDelay(pdMS_TO_TICKS(10));
#endif
    }
}