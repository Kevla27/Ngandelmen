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

dlms_tamper_code_t tamper_vector_to_dlms_code(tamper_vector_t vector) {
    if (vector & TAMPER_VECTOR_CASE_OPEN) return DLMS_TAMPER_METER_COVER_OPEN;
    if (vector & TAMPER_VECTOR_TERMINAL_OPEN) return DLMS_TAMPER_TERMINAL_COVER_OPEN;
    if (vector & TAMPER_VECTOR_MAGNETIC_FIELD) return DLMS_TAMPER_MAGNETIC_INDUCTION;
    if (vector & TAMPER_VECTOR_REVERSE_POWER) return DLMS_TAMPER_REVERSE_CURRENT;
    if (vector & TAMPER_VECTOR_NEUTRAL_BYPASS) return DLMS_TAMPER_MISSING_NEUTRAL;
    return DLMS_TAMPER_NONE;
}

tamper_vector_t dlms_code_to_tamper_vector(dlms_tamper_code_t code) {
    switch (code) {
        case DLMS_TAMPER_METER_COVER_OPEN: return TAMPER_VECTOR_CASE_OPEN;
        case DLMS_TAMPER_TERMINAL_COVER_OPEN: return TAMPER_VECTOR_TERMINAL_OPEN;
        case DLMS_TAMPER_MAGNETIC_INDUCTION: return TAMPER_VECTOR_MAGNETIC_FIELD;
        case DLMS_TAMPER_REVERSE_CURRENT: return TAMPER_VECTOR_REVERSE_POWER;
        case DLMS_TAMPER_MISSING_NEUTRAL: return TAMPER_VECTOR_NEUTRAL_BYPASS;
        default: return (tamper_vector_t)0;
    }
}

#include "rtos_tasks.h"
#include "display_task.h"

void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
    static uint32_t s_last_btn_tick = 0;
    if (timestamp_ms - s_last_btn_tick > 250) {
        s_last_btn_tick = timestamp_ms;
        static bool s_alarm_active = false;
        s_alarm_active = !s_alarm_active;

        display_task_trigger_instant_refresh(s_alarm_active);

        tamper_event_msg_t msg = {
            .timestamp = timestamp_ms / 1000,
            .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
            .is_active = s_alarm_active
        };
        rtos_queue_send_tamper_event_from_isr(&msg);
    }
}