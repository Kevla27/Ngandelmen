#ifndef BSP_LPUART_DMA_H
#define BSP_LPUART_DMA_H

#include "dlms_types.h"
#include "dlms_task.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_LPUART_BAUDRATE_DEFAULT  9600U
#define BSP_LPUART_DMA_RX_BUF_SIZE   512U
#define BSP_LPUART_DMA_TX_BUF_SIZE   512U

typedef struct {
    uint8_t rx_dma_buf[BSP_LPUART_DMA_RX_BUF_SIZE];
    uint8_t tx_dma_buf[BSP_LPUART_DMA_TX_BUF_SIZE];
    volatile size_t rx_tail;
    volatile bool tx_busy;
    dlms_task_ctx_t *target_task;
} bsp_lpuart_handle_t;

/**
 * @brief Inisialisasi driver LPUART1 DMA BSP.
 */
dlms_result_t bsp_lpuart_dma_init(bsp_lpuart_handle_t *handle, dlms_task_ctx_t *task_ctx);

/**
 * @brief Handler interupsi DMA RX / IDLE Line UART (dipanggil dari LPUART1_IRQHandler / DMA ISR).
 *        Memindahkan byte baru dari Circular DMA Buffer ke ring buffer DLMS Task secara otomatis.
 */
void bsp_lpuart_dma_rx_idle_isr(bsp_lpuart_handle_t *handle, size_t current_dma_head);

/**
 * @brief Mengirimkan frame data DLMS balasan via DMA TX secara non-blocking.
 */
dlms_result_t bsp_lpuart_dma_transmit(bsp_lpuart_handle_t *handle, const uint8_t *data, size_t len);

/**
 * @brief Callback saat pengiriman DMA TX selesai (dipanggil dari HAL_LPUART_TxCpltCallback).
 */
void bsp_lpuart_dma_tx_cplt_isr(bsp_lpuart_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LPUART_DMA_H */