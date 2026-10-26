/**
 * @file load_profile.h
 * @brief Modul Basis Data Profil Beban 15-Menitan (Load Profile - ER-02) (E3/ENG-3)
 * @details Mengelola ring buffer pencatatan profil beban transaksi kelistrikan 3-fasa.
 */

#ifndef LOAD_PROFILE_H
#define LOAD_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define LOAD_PROFILE_MAX_ENTRIES   100  /* Kapasitas ring buffer simulasi (dapat diekspansi) */
#define LOAD_PROFILE_INTERVAL_SEC  900  /* Interval 15 menit = 900 detik (ER-02 / ER-04) */

/**
 * @brief Struktur Data Entri Profil Beban 15-Menitan (72-Byte Compact Record)
 */
typedef struct {
    uint32_t timestamp;                     /**< Timestamp RTC (Epoch UNIX) */
    
    /* Vektor Tegangan (dVolts = x0.1 V) */
    uint16_t voltage_r_dvolts;
    uint16_t voltage_s_dvolts;
    uint16_t voltage_t_dvolts;
    
    /* Vektor Arus (mAmps = x0.001 A) */
    uint32_t current_r_mamps;
    uint32_t current_s_mamps;
    uint32_t current_t_mamps;
    uint32_t current_n_mamps;
    
    /* Daya & Faktor Daya */
    int32_t  active_power_w;
    int32_t  reactive_power_var;
    uint16_t power_factor_permille;         /**< Power Factor permille (1000 = 1.000) */
    
    /* Akumulasi Energi (Wh / varh) */
    uint64_t active_energy_import_wh;
    uint64_t active_energy_export_wh;
    uint64_t reactive_energy_import_varh;
    uint64_t reactive_energy_export_varh;
    
    uint32_t status_mask;                  /**< Bitmask status alarm/tamper saat snapshot */
    uint16_t crc16;                        /**< Checksum CRC-16 untuk integritas data */
} load_profile_entry_t;

/**
 * @brief Konteks Ring Buffer Basis Data Load Profile
 */
typedef struct {
    load_profile_entry_t entries[LOAD_PROFILE_MAX_ENTRIES];
    size_t head;                            /**< Indeks penulisan berikutnya */
    size_t count;                           /**< Jumlah entri tersimpan saat ini */
    uint32_t total_written;                 /**< Total entri yang pernah ditulis (kumulatif) */
} load_profile_ctx_t;

/* --- API Functions --- */
void load_profile_init(load_profile_ctx_t *ctx);
uint16_t load_profile_calc_crc16(const load_profile_entry_t *entry);
bool load_profile_add_entry(load_profile_ctx_t *ctx, load_profile_entry_t *entry);
bool load_profile_get_entry(const load_profile_ctx_t *ctx, size_t index, load_profile_entry_t *out_entry);
size_t load_profile_get_count(const load_profile_ctx_t *ctx);
bool load_profile_verify_integrity(const load_profile_entry_t *entry);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_PROFILE_H */