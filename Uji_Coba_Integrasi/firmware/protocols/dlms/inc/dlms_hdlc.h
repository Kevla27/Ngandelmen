#ifndef DLMS_HDLC_H
#define DLMS_HDLC_H

#include "dlms_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HDLC_FLAG       0x7EU
#define HDLC_ESCAPE     0x7DU
#define HDLC_ESCAPE_BIT 0x20U

/* Byte Kontrol HDLC Standar */
#define HDLC_CTRL_SNRM 0x93U
#define HDLC_CTRL_UA   0x73U
#define HDLC_CTRL_DISC 0x53U
#define HDLC_CTRL_DM   0x1FU
#define HDLC_CTRL_UI   0x03U
#define HDLC_CTRL_I    0x10U

/* Header LLC-SNAP Standar DLMS COSEM */
#define DLMS_LLC_DSAP 0xE6U
#define DLMS_LLC_SSAP 0xE6U
#define DLMS_LLC_CTRL 0x00U

/**
 * @brief Menghitung nilai CRC16-CCITT (FCS / HCS) standar HDLC/PPP.
 */
uint16_t dlms_hdlc_crc16(const uint8_t *data, size_t len);

/**
 * @brief Mengodekan payload APDU/Frame Kontrol ke dalam bingkai HDLC lengkap
 *        beserta LLC-SNAP header, HCS, FCS, dan Byte Stuffing.
 */
size_t dlms_hdlc_encode_frame(uint8_t control, 
                             uint16_t dest_addr, 
                             uint8_t src_addr, 
                             const uint8_t *payload, 
                             size_t payload_len, 
                             uint8_t *out_frame, 
                             size_t max_out_len);

/**
 * @brief Membongkar bingkai HDLC masuk, memverifikasi HCS & FCS, melakukan byte destuffing,
 *        serta mengespesifikasi byte kontrol, alamat, dan payload APDU.
 */
dlms_result_t dlms_hdlc_decode_frame(const uint8_t *in_frame, 
                                     size_t in_len, 
                                     uint8_t *out_control, 
                                     uint16_t *out_dest_addr, 
                                     uint8_t *out_src_addr, 
                                     uint8_t *out_payload, 
                                     size_t *out_payload_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_HDLC_H */