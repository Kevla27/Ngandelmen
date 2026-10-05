/**
 * @file tamper_manager.c
 * @brief Implementasi Modul Pengelola Kejadian Sabotase / Tamper Event Manager (E3/ENG-3)
 */

#include "tamper_manager.h"
#include <string.h>

void tamper_init(tamper_context_t *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(tamper_context_t));
}

bool tamper_process_signal(tamper_context_t *ctx, tamper_vector_t vector, bool is_asserted, uint32_t timestamp) {
    if (ctx == NULL) return false;

    uint32_t prev_mask = ctx->active_tamper_mask;

    if (is_asserted) {
        ctx->active_tamper_mask |= (uint32_t)vector;
    } else {
        ctx->active_tamper_mask &= ~((uint32_t)vector);
    }

    // Jika terjadi perubahan status kecurangan
    if (prev_mask != ctx->active_tamper_mask) {
        tamper_event_entry_t *entry = &ctx->log_fifo[ctx->fifo_head];
        entry->timestamp = timestamp;
        entry->vector = vector;
        entry->is_asserted = is_asserted;
        entry->active_mask = ctx->active_tamper_mask;

        ctx->fifo_head = (ctx->fifo_head + 1) % TAMPER_LOG_MAX_ENTRIES;
        if (ctx->fifo_count < TAMPER_LOG_MAX_ENTRIES) ctx->fifo_count++;
        ctx->total_event_count++;

        ctx->alarm_led_status = (ctx->active_tamper_mask != 0);
        if (is_asserted && (vector & (TAMPER_VECTOR_CASE_OPEN | TAMPER_VECTOR_MAGNETIC_FIELD))) {
            ctx->alarm_relay_trigger = true;
        }

        return true;
    }
    return false;
}

uint32_t tamper_get_active_mask(const tamper_context_t *ctx) {
    return (ctx != NULL) ? ctx->active_tamper_mask : 0;
}

void tamper_get_log_entries(const tamper_context_t *ctx, tamper_event_entry_t *out_buffer, size_t max_records, size_t *out_count) {
    if (ctx == NULL || out_buffer == NULL || out_count == NULL || max_records == 0) {
        if (out_count) *out_count = 0;
        return;
    }

    size_t count_to_copy = (ctx->fifo_count < max_records) ? ctx->fifo_count : max_records;
    size_t start_index = (ctx->fifo_count < TAMPER_LOG_MAX_ENTRIES) ? 0 : ctx->fifo_head;

    for (size_t i = 0; i < count_to_copy; i++) {
        size_t idx = (start_index + i) % TAMPER_LOG_MAX_ENTRIES;
        out_buffer[i] = ctx->log_fifo[idx];
    }
    *out_count = count_to_copy;
}

bool tamper_is_alarm_pending(const tamper_context_t *ctx) {
    return (ctx != NULL) && ((ctx->active_tamper_mask != 0) || ctx->alarm_relay_trigger);
}