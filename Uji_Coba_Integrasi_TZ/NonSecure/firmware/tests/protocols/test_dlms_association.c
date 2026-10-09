#include <stdio.h>
#include <assert.h>
#include "dlms_association.h"
#include "dlms_transport.h"

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_association...\n");

    dlms_context_t ctx;
    dlms_init(&ctx);

    /* Test 1: Handshake AARQ Valid */
    uint8_t aarq_payload[] = {0x00, 0x10};
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_AARQ, aarq_payload, 2) == DLMS_OK);
    assert(ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);

    /* Test 2: Penolakan Akses SET Request (Read-Only Lock) */
    assert(dlms_process_event(&ctx, DLMS_EVT_RX_SET_REQ, NULL, 0) == DLMS_ERR_UNAUTHORIZED);

    /* Test 3: Fungsi Ring Buffer FIFO */
    ring_buffer_t rb;
    ring_buffer_init(&rb);
    uint8_t byte_out = 0;
    ring_buffer_push(&rb, 0x7E);
    ring_buffer_pop(&rb, &byte_out);
    assert(byte_out == 0x7E);

    printf("[TEST SUCCESS] test_dlms_association 100%% PASSED!\n");
    return 0;
}