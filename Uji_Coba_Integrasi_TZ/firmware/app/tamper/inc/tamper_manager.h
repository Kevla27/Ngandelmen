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
#include "dlms_tamper_log.h"

#define TAMPER_LOG_MAX_ENTRIES  30

typedef enum {
    TAMPER_VECTOR_CASE_OPEN       = (1 << 0), /* SPLN Tabel 6 No 1: Tutup Meter Dibuka -> Teks: "RUSAK", Kode: "-" */
    TAMPER_VECTOR_TERMINAL_OPEN   = (1 << 1), /* SPLN Tabel 6 No 2: Tutup Terminal Dibuka -> Teks: "PERIKSA", Kode: "ERR20" */
    TAMPER_VECTOR_MAGNETIC_FIELD  = (1 << 2), /* SPLN Tabel 6 No 7: Induksi Medan Magnet -> Teks: "PERIKSA", Kode: "ERR25" */
    TAMPER_VECTOR_NEUTRAL_BYPASS  = (1 << 3), /* SPLN Tabel 6 No 5: Kawat Netral Hilang/Putus -> Teks: "PERIKSA", Kode: "ERR23" */
    TAMPER_VECTOR_REVERSE_POWER   = (1 << 4), /* SPLN Tabel 6 No 8: Reverse Power -> Teks: "REVERSE", Kode: "ERR26" */
    TAMPER_VECTOR_PHASE_LOSS      = (1 << 5), /* SPLN Tabel 6 No 6: Hilang Tegangan 1/2 Fase -> Teks: "PERIKSA", Kode: "ERR24" */
    TAMPER_VECTOR_WRONG_SEQUENCE  = (1 << 6), /* SPLN Tabel 6 No 3: Urutan Fase Terbalik -> Teks: "PERIKSA", Kode: "ERR21" */
    TAMPER_VECTOR_CROSS_PHASE     = (1 << 7), /* SPLN Tabel 6 No 4: Pengawatan Arus/Tegangan Silang -> Teks: "PERIKSA", Kode: "ERR22" */
    TAMPER_VECTOR_LOW_PF_QUAD4    = (1 << 8)  /* SPLN Tabel 6 No 9: Kuadran 4 / PF < 0.85 -> Teks: "PERIKSA", Kode: "ERR27" */
} tamper_vector_t;

/**
 * @brief Kode Alarm Internal Meter / Kerusakan Komponen Sesuai SPLN D3.006:2021 Tabel 4
 */
typedef enum {
    SPLN_ALARM_NONE          = 0,
    SPLN_ALARM_FLASH_ERROR   = (1 << 0), /* ERR00: Flash memory rusak/error */
    SPLN_ALARM_RAM_ERROR     = (1 << 1), /* ERR01: RAM rusak/error */
    SPLN_ALARM_RTC_ERROR     = (1 << 2), /* ERR02: Clock loss / RTC error */
    SPLN_ALARM_LOW_BATTERY   = (1 << 3), /* ERR03: Low battery (threshold 2.7 V) */
    SPLN_ALARM_MCU_ERROR     = (1 << 4), /* ERR04: Mikroprosesor tidak berfungsi normal */
    SPLN_ALARM_ADC_ERROR     = (1 << 5), /* ERR05: Kegagalan sampling data ADC */
    SPLN_ALARM_SUPERCAP_FAIL = (1 << 6), /* ERR06: Superkapasitor rusak/lepas */
    SPLN_ALARM_RELAY_FAIL    = (1 << 7)  /* ERR07: Relai/shunt trip gagal membuka/menutup */
} spln_internal_alarm_t;


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

/**
 * @brief Konversi antara Tamper Vector E3 dan DLMS Tamper Code SPLN D3.006
 */
dlms_tamper_code_t tamper_vector_to_dlms_code(tamper_vector_t vector);
tamper_vector_t dlms_code_to_tamper_vector(dlms_tamper_code_t code);

/**
 * @brief Handler interupsi hardware penekanan tombol sabotase PC13 (Debounced & RTOS Queue)
 * @param timestamp_ms Waktu tick saat interupsi terjadi (HAL_GetTick)
 */
void tamper_handle_button_press_isr(uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif /* TAMPER_MANAGER_H */