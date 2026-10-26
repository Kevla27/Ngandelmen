#ifndef DLMS_SERVER_H
#define DLMS_SERVER_H

#include "dlms_types.h"
#include "dlms_association.h"
#include "dlms_apdu.h"
#include "dlms_obis.h"
#include "dlms_tamper_log.h"
#include "dlms_hdlc.h"
#include "dlms_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    dlms_context_t      ctx;
    dlms_tamper_log_t   tamper_log;
    ring_buffer_t       rx_rb;
    uint16_t            server_sap;
} dlms_server_t;

/**
 * @brief Inisialisasi DLMS Server dispatcher.
 */
dlms_result_t dlms_server_init(dlms_server_t *server);

/**
 * @brief Memproses byte mentah dari UART, melakukan framing HDLC, dan menghasilkan balasan HDLC.
 */
dlms_result_t dlms_server_process_bytes(dlms_server_t *server,
                                         const uint8_t *in_data,
                                         size_t in_len,
                                         uint8_t *out_tx_buf,
                                         size_t max_tx_len,
                                         size_t *out_tx_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_SERVER_H */