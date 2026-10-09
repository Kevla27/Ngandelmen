#include "dlms_hdlc.h"
#include <string.h>

/* Lookup Table Fast CRC16-CCITT (Polinomial 0x8408 - Standar PPP/HDLC) */
static const uint16_t fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbe0, 0xea69, 0xd8f2, 0xc97b,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3ad1, 0x4e64, 0x5fed, 0x6d76, 0x7cfd,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bc3, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ed3, 0x2f5a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t dlms_hdlc_crc16(const uint8_t *data, size_t len) {
    if (!data) return 0;
    uint16_t fcs = 0xFFFFU;
    for (size_t i = 0; i < len; i++) {
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ data[i]) & 0xFFU];
    }
    return fcs ^ 0xFFFFU;
}

size_t dlms_hdlc_encode_frame(uint8_t control, 
                             uint16_t dest_addr, 
                             uint8_t src_addr, 
                             const uint8_t *payload, 
                             size_t payload_len, 
                             uint8_t *out_frame, 
                             size_t max_out_len) {
    if (!out_frame || max_out_len < 16) return 0;

    uint8_t raw_buf[512];
    size_t raw_len = 0;

    /* 1. Alokasi 2 byte untuk Frame Format */
    size_t format_idx = raw_len;
    raw_len += 2;

    /* 2. Format Alamat Tujuan (Destination Address) */
    if (dest_addr <= 0x7FU) {
        raw_buf[raw_len++] = (uint8_t)((dest_addr << 1) | 0x01U);
    } else {
        raw_buf[raw_len++] = (uint8_t)((dest_addr >> 7) << 1);
        raw_buf[raw_len++] = (uint8_t)(((dest_addr & 0x7FU) << 1) | 0x01U);
    }

    /* 3. Format Alamat Asal (Source Address) */
    raw_buf[raw_len++] = (uint8_t)((src_addr << 1) | 0x01U);

    /* 4. Byte Kontrol HDLC */
    raw_buf[raw_len++] = control;

    bool has_info = (payload && payload_len > 0);

    if (has_info) {
        size_t header_len = raw_len;
        size_t expected_frame_len = header_len + 2 + 3 + payload_len + 2;
        uint16_t frame_format = (uint16_t)(0xA000U | (expected_frame_len & 0x07FFU));
        
        raw_buf[format_idx]     = (uint8_t)(frame_format >> 8);
        raw_buf[format_idx + 1] = (uint8_t)(frame_format & 0xFFU);

        /* Kalkulasi HCS pada Header */
        uint16_t hcs = dlms_hdlc_crc16(raw_buf, header_len);
        raw_buf[raw_len++] = (uint8_t)(hcs & 0xFFU);
        raw_buf[raw_len++] = (uint8_t)((hcs >> 8) & 0xFFU);

        /* Header LLC-SNAP */
        raw_buf[raw_len++] = DLMS_LLC_DSAP;
        raw_buf[raw_len++] = DLMS_LLC_SSAP;
        raw_buf[raw_len++] = DLMS_LLC_CTRL;

        /* Payload APDU */
        if (raw_len + payload_len + 2 > sizeof(raw_buf)) return 0;
        memcpy(&raw_buf[raw_len], payload, payload_len);
        raw_len += payload_len;
    } else {
        /* Frame Kontrol Tanpa Payload (SNRM, UA, DISC) */
        size_t expected_frame_len = raw_len + 2;
        uint16_t frame_format = (uint16_t)(0xA000U | (expected_frame_len & 0x07FFU));
        raw_buf[format_idx]     = (uint8_t)(frame_format >> 8);
        raw_buf[format_idx + 1] = (uint8_t)(frame_format & 0xFFU);
    }

    /* Kalkulasi FCS pada seluruh bingkai */
    uint16_t fcs = dlms_hdlc_crc16(raw_buf, raw_len);
    raw_buf[raw_len++] = (uint8_t)(fcs & 0xFFU);
    raw_buf[raw_len++] = (uint8_t)((fcs >> 8) & 0xFFU);

    /* 5. Byte Stuffing & Framing ke out_frame */
    size_t out_idx = 0;
    out_frame[out_idx++] = HDLC_FLAG;

    for (size_t i = 0; i < raw_len; i++) {
        uint8_t b = raw_buf[i];
        if (b == HDLC_FLAG) {
            if (out_idx + 2 >= max_out_len) return 0;
            out_frame[out_idx++] = HDLC_ESCAPE;
            out_frame[out_idx++] = (uint8_t)(HDLC_FLAG ^ HDLC_ESCAPE_BIT);
        } else if (b == HDLC_ESCAPE) {
            if (out_idx + 2 >= max_out_len) return 0;
            out_frame[out_idx++] = HDLC_ESCAPE;
            out_frame[out_idx++] = (uint8_t)(HDLC_ESCAPE ^ HDLC_ESCAPE_BIT);
        } else {
            if (out_idx + 1 >= max_out_len) return 0;
            out_frame[out_idx++] = b;
        }
    }

    out_frame[out_idx++] = HDLC_FLAG;
    return out_idx;
}

dlms_result_t dlms_hdlc_decode_frame(const uint8_t *in_frame, 
                                     size_t in_len, 
                                     uint8_t *out_control, 
                                     uint16_t *out_dest_addr, 
                                     uint8_t *out_src_addr, 
                                     uint8_t *out_payload, 
                                     size_t *out_payload_len) {
    if (!in_frame || in_len < 7 || !out_control) return DLMS_ERR_NULL_PTR;

    /* 1. Verifikasi Flag Pembuka dan Penutup */
    if (in_frame[0] != HDLC_FLAG || in_frame[in_len - 1] != HDLC_FLAG) {
        return DLMS_ERR_INVALID_APDU;
    }

    /* 2. Proses Destuffing */
    uint8_t raw_buf[512];
    size_t raw_len = 0;

    for (size_t i = 1; i < in_len - 1; i++) {
        if (in_frame[i] == HDLC_ESCAPE) {
            if (i + 1 >= in_len - 1) return DLMS_ERR_INVALID_APDU;
            i++;
            raw_buf[raw_len++] = (uint8_t)(in_frame[i] ^ HDLC_ESCAPE_BIT);
        } else {
            raw_buf[raw_len++] = in_frame[i];
        }
        if (raw_len >= sizeof(raw_buf)) return DLMS_ERR_INVALID_APDU;
    }

    if (raw_len < 5) return DLMS_ERR_INVALID_APDU;

    /* 3. Verifikasi FCS Frame */
    size_t data_len_for_fcs = raw_len - 2;
    uint16_t expected_fcs = (uint16_t)(raw_buf[data_len_for_fcs] | (raw_buf[data_len_for_fcs + 1] << 8));
    uint16_t calc_fcs = dlms_hdlc_crc16(raw_buf, data_len_for_fcs);
    if (expected_fcs != calc_fcs) {
        return DLMS_ERR_INVALID_APDU;
    }

    /* 4. Parse Header */
    size_t idx = 0;
    uint16_t frame_format = (uint16_t)((raw_buf[idx] << 8) | raw_buf[idx + 1]);
    idx += 2;

    if ((frame_format & 0xF000U) != 0xA000U) {
        return DLMS_ERR_INVALID_APDU;
    }

    /* Alamat Tujuan */
    uint16_t dest = 0;
    if ((raw_buf[idx] & 0x01U) == 0x01U) {
        dest = (uint16_t)(raw_buf[idx] >> 1);
        idx += 1;
    } else {
        dest = (uint16_t)(((raw_buf[idx] >> 1) << 7) | (raw_buf[idx + 1] >> 1));
        idx += 2;
    }
    if (out_dest_addr) *out_dest_addr = dest;

    /* Alamat Asal */
    uint8_t src = (uint8_t)(raw_buf[idx] >> 1);
    idx += 1;
    if (out_src_addr) *out_src_addr = src;

    /* Byte Kontrol */
    *out_control = raw_buf[idx++];

    /* Verifikasi apakah memiliki payload informasi & HCS */
    if (data_len_for_fcs > idx + 2) {
        size_t header_len = idx;
        uint16_t expected_hcs = (uint16_t)(raw_buf[idx] | (raw_buf[idx + 1] << 8));
        uint16_t calc_hcs = dlms_hdlc_crc16(raw_buf, header_len);
        if (expected_hcs != calc_hcs) {
            return DLMS_ERR_INVALID_APDU;
        }
        idx += 2;

        /* Melewati Header LLC-SNAP (3 byte: 0xE6 0xE6 0x00) */
        if (idx + 3 <= data_len_for_fcs && 
            raw_buf[idx] == DLMS_LLC_DSAP && 
            raw_buf[idx + 1] == DLMS_LLC_SSAP) {
            idx += 3;
        }

        /* Ekstraksi Payload APDU */
        size_t payload_len = data_len_for_fcs - idx;
        if (out_payload && out_payload_len) {
            memcpy(out_payload, &raw_buf[idx], payload_len);
            *out_payload_len = payload_len;
        }
    } else {
        if (out_payload_len) *out_payload_len = 0;
    }

    return DLMS_OK;
}