/**
 * @file power_relay_manager.h
 * @brief Pengelola Asimetri Beban, Pembatas Daya (Power Limiter), dan State Relai "READY ON"
 *        Memenuhi Standar SPLN D3.006: 2021 Tabel 5 (No. 11, 12, 13)
 */

#ifndef POWER_RELAY_MANAGER_H
#define POWER_RELAY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status Kontak Relai (Bistable Latching Relay)
 */
typedef enum {
    RELAY_STATE_CONNECTED = 0,    /**< Relai tertutup / tersambung (Normal) */
    RELAY_STATE_DISCONNECTED,     /**< Relai terbuka / terputus (Trip / Remote Open) */
    RELAY_STATE_READY_ON          /**< Perintah Reconnect diterima, menunggu aksi lokal (SPLN Tabel 5 No 12) */
} relay_state_t;

/**
 * @brief Alasan Terakhir Perubahan Status Relai
 */
typedef enum {
    RELAY_REASON_NONE = 0,
    RELAY_REASON_MANUAL,          /**< Perintah manual / CLI */
    RELAY_REASON_POWER_LIMITER,   /**< Trip Pembatas Daya (SPLN Tabel 5 No 13) */
    RELAY_REASON_REMOTE_DISCONNECT,/**< Perintah Remote HES/DLMS */
    RELAY_REASON_REMOTE_RECONNECT,/**< Perintah Reconnect HES/DLMS */
    RELAY_REASON_LOCAL_CONFIRM    /**< Konfirmasi tombol lokal di meter */
} relay_reason_t;

/**
 * @brief Struktur Konfigurasi & Status Power & Relay Manager
 */
typedef struct {
    /* Konfigurasi Ambang Batas */
    uint32_t power_limit_w;            /**< Ambang batas daya aktif (Watt) - Default 6600 W */
    uint32_t over_power_duration_sec;  /**< Durasi toleransi kelebihan daya (Detik) - Default 5 s */
    uint32_t asymmetry_threshold_pct;  /**< Ambang batas asimetri arus 3-fase (%) - Default 30% */

    /* Status Operasional */
    relay_state_t  relay_state;          /**< Status relai saat ini */
    relay_reason_t last_reason;          /**< Alasan perubahan status relai */
    bool           is_asymmetry_active;  /**< Flag deteksi beban asimetris */
    uint32_t       current_deviation_pct;/**< Nilai deviasi arus 3-fase saat ini (%) */
    bool           is_limiter_tripped;   /**< Flag kelebihan daya (Power Limiter tripped) */
    uint32_t       over_power_timer_sec; /**< Akumulator durasi kelebihan daya */
    uint32_t       last_event_timestamp; /**< Waktu event terakhir (Epoch UNIX) */
} power_relay_context_t;

/**
 * @brief Inisialisasi Power & Relay Manager dengan parameter default SPLN
 */
void power_relay_init(power_relay_context_t *ctx, uint32_t power_limit_w, uint32_t trip_sec, uint32_t asym_pct);

/**
 * @brief Evaluasi Asimetri Arus 3-Fase sesuai formula SPLN Tabel 5 No 11
 *        Formula: Deviasi Maksimum / Rata-rata Arus x 100%
 * @param ctx Pointer ke konteks
 * @param i_r_ma Arus fasa R (mA)
 * @param i_s_ma Arus fasa S (mA)
 * @param i_t_ma Arus fasa T (mA)
 * @return true jika terjadi asimetri daya (> threshold 30%), false jika normal
 */
bool power_relay_check_asymmetry(power_relay_context_t *ctx, uint32_t i_r_ma, uint32_t i_s_ma, uint32_t i_t_ma);

/**
 * @brief Evaluasi Daya Aktif terhadap Pembatas Daya (Power Limiter) SPLN Tabel 5 No 13
 * @param ctx Pointer ke konteks
 * @param active_power_w Daya aktif total (Watt)
 * @param timestamp Waktu kejadian (Epoch UNIX)
 * @return true jika terjadi trip pemutusan relai, false jika normal
 */
bool power_relay_process_power(power_relay_context_t *ctx, int32_t active_power_w, uint32_t timestamp);

/**
 * @brief Pemutusan relai (Disconnect)
 */
void power_relay_disconnect(power_relay_context_t *ctx, relay_reason_t reason, uint32_t timestamp);

/**
 * @brief Permintaan penyambungan kembali relai via remote DLMS (Remote Reconnect)
 *        Transisi ke state RELAY_STATE_READY_ON (SPLN Tabel 5 No 12)
 */
void power_relay_request_reconnect(power_relay_context_t *ctx, uint32_t timestamp);

/**
 * @brief Aksi konfirmasi lokal pengguna (tekan tombol di meter) untuk menyambungkan relai
 *        Transisi dari RELAY_STATE_READY_ON ke RELAY_STATE_CONNECTED
 * @return true jika berhasil tersambung, false jika tidak dalam state READY ON
 */
bool power_relay_confirm_reconnect(power_relay_context_t *ctx, uint32_t timestamp);

/**
 * @brief Mengambil status relai saat ini
 */
relay_state_t power_relay_get_state(const power_relay_context_t *ctx);

/**
 * @brief Memeriksa apakah relai berada dalam state "READY ON"
 */
bool power_relay_is_ready_on(const power_relay_context_t *ctx);

/**
 * @brief Konversi status relai ke string untuk display / log
 */
const char* power_relay_state_to_string(relay_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* POWER_RELAY_MANAGER_H */

