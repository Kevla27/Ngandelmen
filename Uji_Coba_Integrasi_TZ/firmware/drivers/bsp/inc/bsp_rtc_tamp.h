/**
 * @file bsp_rtc_tamp.h
 * @brief Driver Deteksi Sabotase Saat Meter Mati (Offline Tamper via STM32 TAMP / VBAT)
 * 
 * Memenuhi Standar SPLN D3.006: 2021 Bagian 5 & Tabel 6:
 * - Mendeteksi pembukaan tutup meter (PC13 / TAMP_IN1) saat suplai listrik padam total (unpowered).
 * - Beroperasi menggunakan daya cadangan baterai litium 3.6V ER14250 pada rel VBAT (<500 nA).
 * - Mengamankan timestamp dan jenis sabotase di dalam Backup Register TAMP_BKPxR.
 * - Memulihkan rekaman sabotase secara otomatis saat listrik kembali menyala (Power-On Boot).
 */

#ifndef BSP_RTC_TAMP_H
#define BSP_RTC_TAMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dlms_tamper_log.h"

/* Magic Marker Penanda Kejadian Sabotase Offline di Backup Register */
#define TAMP_OFFLINE_MAGIC_MARKER   0xA55A0000U
#define TAMP_BKP_REG_TAMPER_FLAG    0U  /**< TAMP_BKP0R: Marker & Kode Tamper */
#define TAMP_BKP_REG_TIMESTAMP      1U  /**< TAMP_BKP1R: Epoch Waktu Kejadian */
#define TAMP_BKP_REG_COUNTER        2U  /**< TAMP_BKP2R: Akumulator Offline Tamper */

/**
 * @brief Informasi Kejadian Sabotase Offline (Saat Listrik Mati)
 */
typedef struct {
    bool occurred;              /**< true jika terjadi sabotase saat listrik padam */
    uint8_t tamper_code;        /**< Kode jenis sabotase (dlms_tamper_code_t) */
    uint32_t timestamp_epoch;   /**< Waktu kejadian (Epoch UNIX) */
    uint8_t tamp_channel;       /**< Kanal TAMP fisik (1 = PC13 Case Open) */
} bsp_offline_tamper_info_t;

/**
 * @brief Inisialisasi periferal STM32 TAMP, domain Backup Battery (VBAT), dan proteksi write access
 */
void bsp_rtc_tamp_init(void);

/**
 * @brief Memeriksa apakah terjadi sabotase saat listrik padam (Offline Tamper)
 * @param out_info Pointer struktur untuk menampung data sabotase jika ada
 * @return true jika terdeteksi sabotase offline, false jika aman
 */
bool bsp_rtc_tamp_check_offline_tamper(bsp_offline_tamper_info_t *out_info);

/**
 * @brief Memproses seluruh rekaman sabotase offline saat booting (Power-On Reset)
 * Memindahkan log dari Backup Register ke RTOS Queue & DLMS Class 7 Profile Generic.
 */
void bsp_rtc_tamp_process_offline_events(void);

/**
 * @brief Menghapus flag sabotase dan status register TAMP di domain Backup
 */
void bsp_rtc_tamp_clear_offline_flags(void);

/**
 * @brief Membaca nilai 32-bit dari Backup Register (BKP0R .. BKP31R)
 */
uint32_t bsp_rtc_tamp_get_backup_reg(uint32_t reg_index);

/**
 * @brief Menulis nilai 32-bit ke Backup Register (BKP0R .. BKP31R)
 */
void bsp_rtc_tamp_set_backup_reg(uint32_t reg_index, uint32_t value);

/**
 * @brief Fungsi simulasi injeksi sabotase offline (untuk Unit Test CTest dan pengujian terminal UART)
 * @param tamper_code Kode sabotase DLMS (misal DLMS_TAMPER_METER_COVER_OPEN)
 * @param simulated_timestamp Waktu simulasi saat listrik padam
 */
void bsp_rtc_tamp_simulate_offline_tamper(uint8_t tamper_code, uint32_t simulated_timestamp);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RTC_TAMP_H */

