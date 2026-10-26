#ifndef DLMS_TRANSPORT_H
#define DLMS_TRANSPORT_H

#include "dlms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_RING_BUFFER_SIZE 512U

/* Struktur Generic Ring Buffer Non-blocking */
typedef struct {
    uint8_t buffer[DLMS_RING_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
} ring_buffer_t;

/*Deklarasi Ring Buffer Count*/
size_t ring_buffer_count(const ring_buffer_t *rb);

/* Deklarasi Operasi Ring Buffer */
dlms_result_t ring_buffer_init(ring_buffer_t *rb);
dlms_result_t ring_buffer_push(ring_buffer_t *rb, uint8_t byte);
dlms_result_t ring_buffer_pop(ring_buffer_t *rb, uint8_t *byte);
dlms_result_t ring_buffer_peek(const ring_buffer_t *rb, size_t offset, uint8_t *byte);
size_t ring_buffer_available(const ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TRANSPORT_H */