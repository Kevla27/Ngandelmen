/**
 * @file ami_task.c
 * @brief Implementasi FreeRTOS Task Wrapper untuk AMI DLMS Server Engine
 */

#include "ami_task.h"
#include <string.h>

#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#endif

int ami_task_init(ami_task_ctx_t *task_ctx, ami_uart_tx_fn_t tx_cb) {
    if (!task_ctx) return -1;
    memset(task_ctx, 0, sizeof(*task_ctx));
    ami_server_init(&task_ctx->server);
    task_ctx->tx_cb = tx_cb;
    task_ctx->is_running = true;
    return 0;
}

int ami_task_notify_rx(ami_task_ctx_t *task_ctx, const uint8_t *data, size_t len) {
    if (!task_ctx || !data || len == 0) return -1;
    for (size_t i = 0; i < len; i++) {
        ring_buffer_push(&task_ctx->server.rx_rb, data[i]);
    }
    return 0;
}

int ami_task_step(ami_task_ctx_t *task_ctx) {
    if (!task_ctx || !task_ctx->is_running) return -1;

    uint8_t tx_buf[512];
    size_t  tx_len = 0;

    int res = ami_server_process_bytes(&task_ctx->server, NULL, 0, tx_buf, sizeof(tx_buf), &tx_len);
    if (res == 0 && tx_len > 0) {
        if (task_ctx->tx_cb) {
            task_ctx->tx_cb(tx_buf, tx_len);
        }
    }
    return 0;
}

void ami_task_entry(void *pvParameters) {
    ami_task_ctx_t *task_ctx = (ami_task_ctx_t *)pvParameters;
    if (!task_ctx) {
#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelete(NULL);
#endif
        return;
    }

    while (1) {
        ami_task_step(task_ctx);
#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
        vTaskDelay(pdMS_TO_TICKS(10));
#else
        break;
#endif
    }
}

