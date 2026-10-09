/**
 * @file load_profile_dlms_adapter.h
 * @brief Adapter Pemformatan A-XDR DLMS COSEM Class ID 7 untuk E3
 */

#ifndef LOAD_PROFILE_DLMS_ADAPTER_H
#define LOAD_PROFILE_DLMS_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "load_profile.h"
#include "tamper_manager.h"

/* Tag Tipe Data A-XDR DLMS */
#define DLMS_AXDR_TAG_STRUCTURE     0x02
#define DLMS_AXDR_TAG_OCTET_STRING  0x09
#define DLMS_AXDR_TAG_DOUBLE_LONG_U 0x06
#define DLMS_AXDR_TAG_LONG_U        0x12
#define DLMS_AXDR_TAG_LONG          0x10
#define DLMS_AXDR_TAG_LONG64_U      0x15

/**
 * @brief Mengodekan timestamp Epoch UNIX ke format DLMS Date-Time 12-byte
 */
void dlms_encode_datetime(uint32_t epoch_sec, uint8_t *out_buf);

/**
 * @brief Mengodekan 1 entri Load Profile C menjadi A-XDR Structure DLMS
 */
size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t *out_buf, size_t max_len);

/**
 * @brief Mengodekan 1 entri Tamper Log C menjadi A-XDR Structure DLMS
 */
size_t tamper_log_encode_dlms_entry(const tamper_event_entry_t *entry, uint8_t *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_PROFILE_DLMS_ADAPTER_H */