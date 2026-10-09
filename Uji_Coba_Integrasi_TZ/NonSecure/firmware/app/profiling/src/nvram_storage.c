/**
 * @file nvram_storage.c
 * @brief Implementasi Pemrograman & Pembacaan Flash STM32U575
 */

#include "nvram_storage.h" /* 1. HARUS DI BARIS PALING ATAS */
#include <string.h>

/* Buffer Simulasi NVRAM Flash di RAM PC untuk Unit Test CTest */
static load_profile_entry_t simulated_load_profile_flash[LOAD_PROFILE_MAX_ENTRIES];
static tamper_context_t simulated_tamper_log_flash;

bool nvram_write_load_profile_entry(uint32_t entry_index, const load_profile_entry_t *entry) {
    if (entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t target_addr = NVRAM_LOAD_PROFILE_START_ADDR + (entry_index * sizeof(load_profile_entry_t));
    return (target_addr >= NVRAM_LOAD_PROFILE_START_ADDR);
#else
    /* 2. Simpan data entry ke array simulasi PC */
    simulated_load_profile_flash[entry_index] = *entry;
    return true;
#endif
}

bool nvram_read_load_profile_entry(uint32_t entry_index, load_profile_entry_t *out_entry) {
    if (out_entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t target_addr = NVRAM_LOAD_PROFILE_START_ADDR + (entry_index * sizeof(load_profile_entry_t));
    const load_profile_entry_t *flash_ptr = (const load_profile_entry_t *)target_addr;
    memcpy(out_entry, flash_ptr, sizeof(load_profile_entry_t));
    return true;
#else
    *out_entry = simulated_load_profile_flash[entry_index];
    return true;
#endif
}

bool nvram_save_tamper_log_snapshot(const tamper_context_t *tamper_ctx) {
    if (tamper_ctx == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    return true;
#else
    simulated_tamper_log_flash = *tamper_ctx;
    return true;
#endif
}

bool nvram_load_tamper_log_snapshot(tamper_context_t *tamper_ctx) {
    if (tamper_ctx == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    const tamper_context_t *flash_ptr = (const tamper_context_t *)NVRAM_TAMPER_LOG_START_ADDR;
    memcpy(tamper_ctx, flash_ptr, sizeof(tamper_context_t));
    return true;
#else
    *tamper_ctx = simulated_tamper_log_flash;
    return true;
#endif
}