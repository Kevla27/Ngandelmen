/**
 * @file load_profile.c
 * @brief Implementasi Modul Basis Data Profil Beban 15-Menitan (E3/ENG-3)
 */

#include "load_profile.h"
#include <string.h>

/**
 * @brief Menghitung CRC-16 (CCITT-FALSE) untuk integritas data entri load profile
 */
uint16_t load_profile_calc_crc16(const load_profile_entry_t *entry) {
    if (entry == NULL) return 0;

    const uint8_t *data = (const uint8_t *)entry;
    size_t length = offsetof(load_profile_entry_t, crc16); // Byte sebelum crc16
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void load_profile_init(load_profile_ctx_t *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(load_profile_ctx_t));
}

bool load_profile_add_entry(load_profile_ctx_t *ctx, load_profile_entry_t *entry) {
    if (ctx == NULL || entry == NULL) return false;

    /* Hitung dan sematkan CRC-16 sebelum disimpan */
    entry->crc16 = load_profile_calc_crc16(entry);

    /* Simpan ke ring buffer */
    ctx->entries[ctx->head] = *entry;
    ctx->head = (ctx->head + 1) % LOAD_PROFILE_MAX_ENTRIES;

    if (ctx->count < LOAD_PROFILE_MAX_ENTRIES) {
        ctx->count++;
    }
    ctx->total_written++;

    return true;
}

bool load_profile_get_entry(const load_profile_ctx_t *ctx, size_t index, load_profile_entry_t *out_entry) {
    if (ctx == NULL || out_entry == NULL || index >= ctx->count) {
        return false;
    }

    /* Hitung indeks fisik dari paling lama (oldest) ke paling baru (newest) */
    size_t start_idx;
    if (ctx->count < LOAD_PROFILE_MAX_ENTRIES) {
        start_idx = 0;
    } else {
        start_idx = ctx->head;
    }

    size_t actual_idx = (start_idx + index) % LOAD_PROFILE_MAX_ENTRIES;
    *out_entry = ctx->entries[actual_idx];

    return true;
}

size_t load_profile_get_count(const load_profile_ctx_t *ctx) {
    return (ctx != NULL) ? ctx->count : 0;
}

bool load_profile_verify_integrity(const load_profile_entry_t *entry) {
    if (entry == NULL) return false;
    uint16_t expected_crc = load_profile_calc_crc16(entry);
    return (entry->crc16 == expected_crc);
}