#include "bsp_lpuart_dma.h"
#include <string.h>

dlms_result_t bsp_lpuart_dma_init(bsp_lpuart_handle_t *handle, dlms_task_ctx_t *task_ctx) {
    if (!handle || !task_ctx) return DLMS_ERR_NULL_PTR;

    memset(handle, 0, sizeof(bsp_lpuart_handle_t));
    handle->target_task = task_ctx;
    handle->rx_tail = 0;
    handle->tx_busy = false;

    return DLMS_OK;
}

void bsp_lpuart_dma_rx_idle_isr(bsp_lpuart_handle_t *handle, size_t current_dma_head) {
    if (!handle || !handle->target_task) return;

    if (current_dma_head >= BSP_LPUART_DMA_RX_BUF_SIZE) {
        current_dma_head %= BSP_LPUART_DMA_RX_BUF_SIZE;
    }

    size_t tail = handle->rx_tail;
    if (current_dma_head == tail) return; /* Tidak ada byte baru */

    uint8_t temp_buf[BSP_LPUART_DMA_RX_BUF_SIZE];
    size_t new_bytes_count = 0;

    if (current_dma_head > tail) {
        /* Data kontigu dari tail ke current_dma_head */
        new_bytes_count = current_dma_head - tail;
        memcpy(temp_buf, &handle->rx_dma_buf[tail], new_bytes_count);
    } else {
        /* Data melingkar (wrap around circular buffer) */
        size_t chunk1 = BSP_LPUART_DMA_RX_BUF_SIZE - tail;
        size_t chunk2 = current_dma_head;
        memcpy(temp_buf, &handle->rx_dma_buf[tail], chunk1);
        if (chunk2 > 0) {
            memcpy(&temp_buf[chunk1], &handle->rx_dma_buf, chunk2);
        }
        new_bytes_count = chunk1 + chunk2;
    }

    handle->rx_tail = current_dma_head;

    /* Masukkan byte dari ISR DMA langsung ke ring buffer DLMS Task */
    dlms_task_notify_rx(handle->target_task, temp_buf, new_bytes_count);
}

dlms_result_t bsp_lpuart_dma_transmit(bsp_lpuart_handle_t *handle, const uint8_t *data, size_t len) {
    if (!handle || !data || len == 0) return DLMS_ERR_NULL_PTR;
    if (len > BSP_LPUART_DMA_TX_BUF_SIZE) return DLMS_ERR_BUFFER_OVERFLOW;
    if (handle->tx_busy) return DLMS_ERR_INVALID_APDU; /* Port sedang sibuk */

    handle->tx_busy = true;
    memcpy(handle->tx_dma_buf, data, len);

    /* Pada hardware STM32U575 sungguhan:
     * HAL_LPUART_Transmit_DMA(&hlpuart1, handle->tx_dma_buf, (uint16_t)len);
     */

    return DLMS_OK;
}

void bsp_lpuart_dma_tx_cplt_isr(bsp_lpuart_handle_t *handle) {
    if (!handle) return;
    handle->tx_busy = false;
}