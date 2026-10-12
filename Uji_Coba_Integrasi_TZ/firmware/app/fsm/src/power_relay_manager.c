/**
 * @file power_relay_manager.c
 * @brief Implementasi Pengelola Asimetri Beban, Pembatas Daya, dan State Relai "READY ON"
 *        Sesuai Standar SPLN D3.006: 2021 Tabel 5 (No. 11, 12, 13)
 */

#include "power_relay_manager.h"
#include <stdio.h>

void power_relay_init(power_relay_context_t *ctx, uint32_t power_limit_w, uint32_t trip_sec, uint32_t asym_pct) {
    if (ctx == NULL) return;

    ctx->power_limit_w = (power_limit_w > 0) ? power_limit_w : 6600;
    ctx->over_power_duration_sec = (trip_sec > 0) ? trip_sec : 5;
    ctx->asymmetry_threshold_pct = (asym_pct > 0) ? asym_pct : 30;

    ctx->relay_state = RELAY_STATE_CONNECTED;
    ctx->last_reason = RELAY_REASON_NONE;
    ctx->is_asymmetry_active = false;
    ctx->current_deviation_pct = 0;
    ctx->is_limiter_tripped = false;
    ctx->over_power_timer_sec = 0;
    ctx->last_event_timestamp = 0;
}

bool power_relay_check_asymmetry(power_relay_context_t *ctx, uint32_t i_r_ma, uint32_t i_s_ma, uint32_t i_t_ma) {
    if (ctx == NULL) return false;

    /* Rata-rata arus ketiga fasa */
    uint32_t i_avg = (i_r_ma + i_s_ma + i_t_ma) / 3;

    /* Hindari false alarm saat kondisi tanpa beban (arus < 500 mA) */
    if (i_avg < 500) {
        ctx->current_deviation_pct = 0;
        ctx->is_asymmetry_active = false;
        return false;
    }

    /* Hitung selisih absolut tiap fasa terhadap rata-rata */
    uint32_t diff_r = (i_r_ma >= i_avg) ? (i_r_ma - i_avg) : (i_avg - i_r_ma);
    uint32_t diff_s = (i_s_ma >= i_avg) ? (i_s_ma - i_avg) : (i_avg - i_s_ma);
    uint32_t diff_t = (i_t_ma >= i_avg) ? (i_t_ma - i_avg) : (i_avg - i_t_ma);

    /* Cari deviasi maksimum */
    uint32_t max_diff = diff_r;
    if (diff_s > max_diff) max_diff = diff_s;
    if (diff_t > max_diff) max_diff = diff_t;

    /* Deviasi = (Deviasi Maksimum / Rata-rata Arus) x 100% per SPLN Tabel 5 No 11 */
    uint32_t dev_pct = (uint32_t)(((uint64_t)max_diff * 100) / i_avg);
    ctx->current_deviation_pct = dev_pct;

    if (dev_pct > ctx->asymmetry_threshold_pct) {
        ctx->is_asymmetry_active = true;
        return true;
    } else {
        ctx->is_asymmetry_active = false;
        return false;
    }
}

bool power_relay_process_power(power_relay_context_t *ctx, int32_t active_power_w, uint32_t timestamp) {
    if (ctx == NULL) return false;

    /* Jika daya aktif melebihi ambang batas kontrak */
    if (active_power_w > (int32_t)ctx->power_limit_w) {
        ctx->over_power_timer_sec++;
        if (ctx->over_power_timer_sec >= ctx->over_power_duration_sec) {
            ctx->is_limiter_tripped = true;
            if (ctx->relay_state == RELAY_STATE_CONNECTED) {
                power_relay_disconnect(ctx, RELAY_REASON_POWER_LIMITER, timestamp);
                return true; /* Trip terjadi */
            }
        }
    } else {
        ctx->over_power_timer_sec = 0;
        ctx->is_limiter_tripped = false;
    }

    return false;
}

void power_relay_disconnect(power_relay_context_t *ctx, relay_reason_t reason, uint32_t timestamp) {
    if (ctx == NULL) return;

    ctx->relay_state = RELAY_STATE_DISCONNECTED;
    ctx->last_reason = reason;
    ctx->last_event_timestamp = timestamp;
}

void power_relay_request_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
    if (ctx == NULL) return;

    /* Sesuai SPLN Tabel 5 No 12: Remote reconnect masuk ke state READY ON */
    if (ctx->relay_state == RELAY_STATE_DISCONNECTED) {
        ctx->relay_state = RELAY_STATE_READY_ON;
        ctx->last_reason = RELAY_REASON_REMOTE_RECONNECT;
        ctx->last_event_timestamp = timestamp;
    }
}

bool power_relay_confirm_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
    if (ctx == NULL) return false;

    /* Aksi konfirmasi lokal pengguna menyambungkan relai kembali ke CONNECTED */
    if (ctx->relay_state == RELAY_STATE_READY_ON) {
        ctx->relay_state = RELAY_STATE_CONNECTED;
        ctx->last_reason = RELAY_REASON_LOCAL_CONFIRM;
        ctx->last_event_timestamp = timestamp;
        ctx->over_power_timer_sec = 0;
        ctx->is_limiter_tripped = false;
        return true;
    }

    return false;
}

relay_state_t power_relay_get_state(const power_relay_context_t *ctx) {
    if (ctx == NULL) return RELAY_STATE_CONNECTED;
    return ctx->relay_state;
}

bool power_relay_is_ready_on(const power_relay_context_t *ctx) {
    if (ctx == NULL) return false;
    return (ctx->relay_state == RELAY_STATE_READY_ON);
}

const char* power_relay_state_to_string(relay_state_t state) {
    switch (state) {
        case RELAY_STATE_CONNECTED:    return "CONNECTED";
        case RELAY_STATE_DISCONNECTED: return "DISCONNECTED";
        case RELAY_STATE_READY_ON:     return "READY ON";
        default:                       return "UNKNOWN";
    }
}

