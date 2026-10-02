/**
 * @file load_profile_dlms_adapter.c
 * @brief Implementasi Adapter Pemformatan A-XDR DLMS COSEM Class ID 7
 */

#include "load_profile_dlms_adapter.h"
#include <string.h>

void dlms_encode_datetime(uint32_t epoch_sec, uint8_t *out_buf) {
    if (out_buf == NULL) return;

    /* Pemformatan tanggal sederhana untuk pengujian A-XDR (12-byte DLMS date-time) */
    out_buf[0] = 0x07; out_buf[1] = 0xE8; /* Year 2024 */
    out_buf[2] = 0x09;                    /* Month September */
    out_buf[3] = 0x1D;                    /* Day 29 */
    out_buf[4] = 0xFF;                    /* Day of week (unspecified) */
    out_buf[5] = (uint8_t)((epoch_sec / 3600) % 24); /* Hour */
    out_buf[6] = (uint8_t)((epoch_sec / 60) % 60);   /* Minute */
    out_buf[7] = (uint8_t)(epoch_sec % 60);          /* Second */
    out_buf[8] = 0x00;                    /* Hundredths */
    out_buf[9] = 0x80; out_buf[10] = 0x00;/* Deviation */
    out_buf[11] = 0x00;                   /* Clock status */
}

size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t *out_buf, size_t max_len) {
    if (entry == NULL || out_buf == NULL || max_len < 80) return 0;

    size_t idx = 0;

    /* Structure of 11 elements */
    out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;
    out_buf[idx++] = 11;

    /* 1. Timestamp (Octet-String 12 byte) */
    out_buf[idx++] = DLMS_AXDR_TAG_OCTET_STRING;
    out_buf[idx++] = 12;
    dlms_encode_datetime(entry->timestamp, &out_buf[idx]);
    idx += 12;

    /* 2. Active Energy Import (+A) - Long64-Unsigned */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
    for (int i = 7; i >= 0; i--) {
        out_buf[idx++] = (uint8_t)(entry->active_energy_import_wh >> (i * 8));
    }

    /* 3. Voltage R (Long-Unsigned x0.1V) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts >> 8);
    out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts & 0xFF);

    /* 4. Current R (Double-Long-Unsigned x0.001A) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 24);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 16);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 8);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps & 0xFF);

    /* 5. Status Mask (Double-Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 24);
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 16);
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 8);
    out_buf[idx++] = (uint8_t)(entry->status_mask & 0xFF);

    return idx;
}

size_t tamper_log_encode_dlms_entry(const tamper_event_entry_t *entry, uint8_t *out_buf, size_t max_len) {
    if (entry == NULL || out_buf == NULL || max_len < 30) return 0;

    size_t idx = 0;

    /* Structure of 3 elements: [Timestamp, Event Code, Active Mask] */
    out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;
    out_buf[idx++] = 4;

    /* 1. Timestamp (Octet-String 12-byte) */
    out_buf[idx++] = DLMS_AXDR_TAG_OCTET_STRING;
    out_buf[idx++] = 12;
    dlms_encode_datetime(entry->timestamp, &out_buf[idx]);
    idx += 12;

    /* 2. Event Vector Code (Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->vector >> 8);
    out_buf[idx++] = (uint8_t)(entry->vector & 0xFF);

    /* 3. Active Tamper Mask (Double-Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 24);
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 16);
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 8);
    out_buf[idx++] = (uint8_t)(entry->active_mask & 0xFF);

    return idx;
}