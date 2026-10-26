
#include <stdint.h>     // For uint8_t
#include <stddef.h>     // For size_t and NULL
#include <stdbool.h>    // For bool (if used in your code)

// Replace "dlms_transport.h" with the actual header file name where
// dlms_result_t and ring_buffer_t are declared in your project.
#include "dlms_transport.h"

dlms_result_t ring_buffer_init(ring_buffer_t *rb) {
    if (rb == NULL) return DLMS_ERR_NULL_PTR;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return DLMS_OK;
}

dlms_result_t ring_buffer_push(ring_buffer_t *rb, uint8_t byte) {
    if (rb == NULL) return DLMS_ERR_NULL_PTR;
    if (rb->count >= DLMS_RING_BUFFER_SIZE) return DLMS_ERR_BUFFER_OVERFLOW;

    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1U) % DLMS_RING_BUFFER_SIZE;
    rb->count++;
    return DLMS_OK;
}

dlms_result_t ring_buffer_pop(ring_buffer_t *rb, uint8_t *byte) {
    if (rb == NULL || byte == NULL) return DLMS_ERR_NULL_PTR;
    if (rb->count == 0) return DLMS_ERR_BUFFER_EMPTY;

    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1U) % DLMS_RING_BUFFER_SIZE;
    rb->count--;
    return DLMS_OK;
}

dlms_result_t ring_buffer_peek(const ring_buffer_t *rb, size_t offset, uint8_t *byte) {
    if (rb == NULL || byte == NULL) return DLMS_ERR_NULL_PTR;
    if (offset >= rb->count) return DLMS_ERR_BUFFER_EMPTY;

    size_t index = (rb->tail + offset) % DLMS_RING_BUFFER_SIZE;
    *byte = rb->buffer[index];
    return DLMS_OK;
}

size_t ring_buffer_available(const ring_buffer_t *rb) {
    return (rb == NULL) ? 0 : rb->count;
}
size_t ring_buffer_count(const ring_buffer_t *rb) { 
    return rb ? rb->count : 0; }



