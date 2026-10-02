#ifndef DLMS_TASK_H
#define DLMS_TASK_H

#include "dlms_types.h"
#include "dlms_server.h"

#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Tipe Callback untuk Driver Pengiriman UART HAL */
typedef void (*dlms_uart_tx_fn_t)(const uint8_t *data, size_t len);

/* Konfigurasi Task DLMS */
#define DLMS_TASK_STACK_SIZE   1024U /* 1024 words (4KB Stack) */
#define DLMS_TASK_PRIORITY     2U    /* Priority Normal / Medium */

typedef struct {
    dlms_server_t     server;
    dlms_uart_tx_fn_t tx_cb;
    bool              is_running;
} dlms_task_ctx_t;

/**
 * @brief Inisialisasi konteks Task DLMS dan Server Dispatcher.
 */
dlms_result_t dlms_task_init(dlms_task_ctx_t *task_ctx, dlms_uart_tx_fn_t tx_cb);

/**
 * @brief Mengirimkan byte baru dari ISR/DMA UART ke dalam ring buffer Task DLMS.
 */
dlms_result_t dlms_task_notify_rx(dlms_task_ctx_t *task_ctx, const uint8_t *data, size_t len);

/**
 * @brief Iterasi tunggal pemrosesan DLMS Task (non-blocking).
 */
dlms_result_t dlms_task_step(dlms_task_ctx_t *task_ctx);

/**
 * @brief Entry point utama untuk FreeRTOS Task (loop while(1)).
 */
void dlms_task_entry(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TASK_H */