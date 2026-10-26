#ifndef DLMS_NVM_H
#define DLMS_NVM_H

#include "dlms_types.h"
#include "dlms_tamper_log.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_NVM_MAGIC_TAMP   0x54414D50U /* "TAMP" ASCII Header */
#define DLMS_NVM_VERSION_1    0x0001U

/* Callback Abstraksi Read/Write Driver Hardware (EEPROM / Flash HAL) */
typedef dlms_result_t (*dlms_nvm_write_fn_t)(uint32_t addr, const uint8_t *data, size_t len);
typedef dlms_result_t (*dlms_nvm_read_fn_t)(uint32_t addr, uint8_t *data, size_t len);

/* Header Struktur Data NVM dengan Proteksi Checksum */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t payload_len;
    uint16_t crc16;
} dlms_nvm_header_t;

/**
 * @brief Menyimpan struktur Tamper Log dari RAM ke NVM beserta Header Magic & CRC16.
 */
dlms_result_t dlms_nvm_save_tamper_log(const dlms_tamper_log_t *log, 
                                        dlms_nvm_write_fn_t write_fn, 
                                        uint32_t start_addr);

/**
 * @brief Membaca dan memverifikasi integritas Tamper Log dari NVM ke RAM saat startup/power-up.
 */
dlms_result_t dlms_nvm_load_tamper_log(dlms_tamper_log_t *log, 
                                        dlms_nvm_read_fn_t read_fn, 
                                        uint32_t start_addr);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_NVM_H */