#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "bsp_lpuart_dma.h"
#include "dlms_task.h"

static uint8_t g_tx_out_buf[512];
static size_t  g_tx_out_len = 0;

static void mock_hw_tx_cb(const uint8_t *data, size_t len) {
    if (data && len > 0) {
        memcpy(g_tx_out_buf, data, len);
        g_tx_out_len = len;
    }
}

int main(void) {
    printf("[TEST RUNNER] Running test_bsp_lpuart...\n");

    dlms_task_ctx_t task_ctx;
    assert(dlms_task_init(&task_ctx, mock_hw_tx_cb) == DLMS_OK);

    bsp_lpuart_handle_t bsp;
    assert(bsp_lpuart_dma_init(&bsp, &task_ctx) == DLMS_OK);

    /* 1. Simulasi Penerimaan Frame SNRM via Circular DMA LPUART1 */
    uint8_t snrm_frame[16];
    size_t snrm_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, DLMS_SERVER_SAP_MANAGEMENT, 0x10, NULL, 0, snrm_frame, sizeof(snrm_frame));
    assert(snrm_len > 0);

    /* Tulis frame ke DMA RX Buffer */
    memcpy(&bsp.rx_dma_buf[0], snrm_frame, snrm_len);

    /* Simulasi pemicuan LPUART1 IDLE Line Interrupt (Head berpindah ke posisi snrm_len) */
    bsp_lpuart_dma_rx_idle_isr(&bsp, snrm_len);

    /* Eksekusi 1 iterasi FreeRTOS Task */
    g_tx_out_len = 0;
    assert(dlms_task_step(&task_ctx) == DLMS_OK);
    assert(g_tx_out_len > 0);
    assert(g_tx_out_buf[0] == HDLC_FLAG); /* Memastikan balasan UA HDLC dikirim */

    /* 2. Simulasi Transmit DMA TX */
    assert(bsp_lpuart_dma_transmit(&bsp, g_tx_out_buf, g_tx_out_len) == DLMS_OK);
    assert(bsp.tx_busy == true);

    /* Pemicuan TX Complete Callback */
    bsp_lpuart_dma_tx_cplt_isr(&bsp);
    assert(bsp.tx_busy == false);

    printf("[TEST SUCCESS] test_bsp_lpuart 100%% PASSED!\n");
    return 0;
}