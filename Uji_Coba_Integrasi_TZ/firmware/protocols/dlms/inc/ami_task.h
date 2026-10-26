/**
 * @file ami_task.h
 * @brief FreeRTOS Task Wrapper untuk AMI DLMS Server Engine
 */

#ifndef AMI_TASK_H
#define AMI_TASK_H

#include "ami_server.h"

#if defined(USE_FREERTOS) || defined(INC_FREERTOS_H)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ami_uart_tx_fn_t)(const uint8_t *data, size_t len);

#define AMI_TASK_STACK_SIZE   1024U
#define AMI_TASK_PRIORITY     2U

typedef struct {
    ami_server_t      server;
    ami_uart_tx_fn_t  tx_cb;
    bool              is_running;
} ami_task_ctx_t;

int  ami_task_init(ami_task_ctx_t *task_ctx, ami_uart_tx_fn_t tx_cb);
int  ami_task_notify_rx(ami_task_ctx_t *task_ctx, const uint8_t *data, size_t len);
int  ami_task_step(ami_task_ctx_t *task_ctx);
void ami_task_entry(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* AMI_TASK_H */
