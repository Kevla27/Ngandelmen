/**
 * @file ami_server.h
 * @brief Dispatcher Server DLMS/COSEM Modern berbasis AMI Stack (SPLN D3.002:2021)
 */

#ifndef AMI_SERVER_H
#define AMI_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ami_cosem_model.h"
#include "ami_dlms_association.h"
#include "ami_dlms_apdu.h"
#include "ami_dlms_hdlc.h"
#include "ami_security.h"
#include "dlms_transport.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    HdlcContext              hdlc_ctx;
    AmiDlmsAssociation       assoc;
    AmiCosemRegisterSnapshot snapshot;
    ring_buffer_t            rx_rb;
    uint32_t                 server_sap;
    uint32_t                 client_sap;
    bool                     is_associated;
} ami_server_t;

int  ami_server_init(ami_server_t *server);
void ami_server_reset(ami_server_t *server);

/* Sinkronisasi data pengukuran listrik E3 ke Model Snapshot COSEM */
void ami_server_update_from_meter(ami_server_t *server, const meter_measurements_t *meas);

/* Pencatatan kejadian sabotase (Tamper) ke Log COSEM Class 7 */
void ami_server_add_tamper_event(ami_server_t *server, uint32_t timestamp_epoch, uint8_t code, uint8_t status);

/* Pemrosesan stream byte masuk UART dan pembentukan respons HDLC/APDU */
int  ami_server_process_bytes(ami_server_t *server,
                              const uint8_t *in_data,
                              size_t in_len,
                              uint8_t *out_tx_buf,
                              size_t max_tx_len,
                              size_t *out_tx_len);

#ifdef __cplusplus
}
#endif

#endif /* AMI_SERVER_H */
