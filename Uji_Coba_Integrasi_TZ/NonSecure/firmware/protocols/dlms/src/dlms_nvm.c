#include "dlms_nvm.h"
#include "dlms_hdlc.h"
#include <string.h>

dlms_result_t dlms_nvm_save_tamper_log(const dlms_tamper_log_t *log, 
                                        dlms_nvm_write_fn_t write_fn, 
                                        uint32_t start_addr) {
    if (!log || !write_fn) return DLMS_ERR_NULL_PTR;

    dlms_nvm_header_t hdr;
    hdr.magic       = DLMS_NVM_MAGIC_TAMP;
    hdr.version     = DLMS_NVM_VERSION_1;
    hdr.payload_len = (uint16_t)sizeof(dlms_tamper_log_t);
    hdr.crc16       = dlms_hdlc_crc16((const uint8_t *)log, sizeof(dlms_tamper_log_t));

    /* 1. Tulis Header ke Memori Non-Volatile */
    dlms_result_t res = write_fn(start_addr, (const uint8_t *)&hdr, sizeof(dlms_nvm_header_t));
    if (res != DLMS_OK) return res;

    /* 2. Tulis Payload Structure Tamper Log ke Memori Non-Volatile */
    return write_fn(start_addr + sizeof(dlms_nvm_header_t), (const uint8_t *)log, sizeof(dlms_tamper_log_t));
}

dlms_result_t dlms_nvm_load_tamper_log(dlms_tamper_log_t *log, 
                                        dlms_nvm_read_fn_t read_fn, 
                                        uint32_t start_addr) {
    if (!log || !read_fn) return DLMS_ERR_NULL_PTR;

    dlms_nvm_header_t hdr;

    /* 1. Baca Header dari NVM */
    dlms_result_t res = read_fn(start_addr, (uint8_t *)&hdr, sizeof(dlms_nvm_header_t));
    if (res != DLMS_OK) return res;

    /* 2. Verifikasi Magic Number dan Versi */
    if (hdr.magic != DLMS_NVM_MAGIC_TAMP || hdr.version != DLMS_NVM_VERSION_1) {
        return DLMS_ERR_INVALID_APDU; /* Corrupted / Uninitialized NVM */
    }

    if (hdr.payload_len != sizeof(dlms_tamper_log_t)) {
        return DLMS_ERR_INVALID_APDU;
    }

    /* 3. Baca Payload Tamper Log dari NVM */
    res = read_fn(start_addr + sizeof(dlms_nvm_header_t), (uint8_t *)log, sizeof(dlms_tamper_log_t));
    if (res != DLMS_OK) return res;

    /* 4. Verifikasi Integrity Checksum CRC16 */
    uint16_t calc_crc = dlms_hdlc_crc16((const uint8_t *)log, sizeof(dlms_tamper_log_t));
    if (hdr.crc16 != calc_crc) {
        memset(log, 0, sizeof(dlms_tamper_log_t)); /* Reset buffer jika korup */
        return DLMS_ERR_INVALID_APDU;
    }

    return DLMS_OK;
}