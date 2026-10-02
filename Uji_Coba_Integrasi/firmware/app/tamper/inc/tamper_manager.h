/**
 * @file tamper_manager.h
 * @brief Modul Pengelola Kejadian Sabotase / Tamper Event Manager (E3/ENG-3)
 */

#ifndef TAMPER_MANAGER_H
#define TAMPER_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TAMPER_LOG_MAX_ENTRIES  30

typedef enum {
    TAMPER_VECTOR_CASE_OPEN       = (1 << 0),
    TAMPER_VECTOR_TERMINAL_OPEN   = (1 << 1),
    TAMPER_VECTOR_MAGNETIC_FIELD  = (1 << 2),
    TAMPER_VECTOR_NEUTRAL_BYPASS  = (1 << 3),
    TAMPER_VECTOR_REVERSE_POWER   = (1 << 4),
    TAMPER_VECTOR_PHASE_LOSS      = (1 << 5),
    TAMPER_VECTOR_WRONG_SEQUENCE  = (1 << 6)
} tamper_vector_t;

typedef struct {
    uint32_t        timestamp;
    tamper_vector_t vector;
    bool            is_asserted;
    uint32_t        duration_sec;
    uint32_t        active_mask;
} tamper_event_entry_t;

typedef struct {
    uint32_t             active_tamper_mask;
    uint32_t             total_event_count;
    tamper_event_entry_t log_fifo[TAMPER_LOG_MAX_ENTRIES];
    size_t               fifo_head;
    size_t               fifo_count;
    bool                 alarm_relay_trigger;
    bool                 alarm_led_status;
} tamper_context_t;

void tamper_init(tamper_context_t *ctx);
bool tamper_process_signal(tamper_context_t *ctx, tamper_vector_t vector, bool is_asserted, uint32_t timestamp);
uint32_t tamper_get_active_mask(const tamper_context_t *ctx);
void tamper_get_log_entries(const tamper_context_t *ctx, tamper_event_entry_t *out_buffer, size_t max_records, size_t *out_count);
bool tamper_is_alarm_pending(const tamper_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* TAMPER_MANAGER_H */