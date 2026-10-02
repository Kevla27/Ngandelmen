#ifndef DLMS_TAMPER_LOG_H
#define DLMS_TAMPER_LOG_H

#include "dlms_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Kapasitas Minimal Log Sesuai SPLN D3.006:2021 (30 Rekaman FIFO) */
#define DLMS_TAMPER_LOG_MAX_ENTRIES 30U

/* Kode Kejadian Tamper Utama (Event Codes) */
typedef enum {
    DLMS_TAMPER_NONE                = 0x00,
    DLMS_TAMPER_TERMINAL_COVER_OPEN = 0x01,  /* OBIS 0.0.96.20.5.255 */
    DLMS_TAMPER_METER_COVER_OPEN    = 0x02,  /* OBIS 0.0.96.20.0.255 */
    DLMS_TAMPER_MAGNETIC_INDUCTION  = 0x03,  /* OBIS 0.0.96.20.26.255 */
    DLMS_TAMPER_REVERSE_CURRENT     = 0x04,  /* OBIS 0.0.96.20.27.255 */
    DLMS_TAMPER_MISSING_NEUTRAL     = 0x05   /* OBIS 0.0.96.20.24.255 */
} dlms_tamper_code_t;

/* Struktur Entri Tunggal Rekaman Tamper Log */
typedef struct {
    uint32_t timestamp_epoch;  /* Waktu kejadian (UNIX Timestamp RTC) */
    uint8_t  event_code;       /* Kode jenis tamper (dlms_tamper_code_t) */
    uint8_t  status;           /* 1 = Event Occurred (Mulai), 0 = Event Restored (Selesai) */
} dlms_tamper_record_t;

/* Struktur Buffer Sirkular Log Kejadian (Class 7 Profile Generic) */
typedef struct {
    dlms_tamper_record_t records[DLMS_TAMPER_LOG_MAX_ENTRIES];
    size_t head;            /* Indeks penulisan entri baru */
    size_t entries_in_use;  /* Jumlah rekaman aktif saat ini (0 .. 30) */
    uint32_t total_events;  /* Akumulator total seluruh kejadian tamper */
} dlms_tamper_log_t;

/**
 * @brief Inisialisasi/reset struktur Tamper Log Buffer.
 */
dlms_result_t dlms_tamper_log_init(dlms_tamper_log_t *log);

/**
 * @brief Menambahkan kejadian tamper baru ke dalam buffer FIFO (Overwrite otomatis jika penuh).
 */
dlms_result_t dlms_tamper_log_add_event(dlms_tamper_log_t *log, 
                                         uint32_t timestamp_epoch, 
                                         dlms_tamper_code_t code, 
                                         uint8_t status);

/**
 * @brief Mengambil rekaman log berdasarkan indeks (0 = Paling Baru, entries_in_use-1 = Paling Lama).
 */
dlms_result_t dlms_tamper_log_get_entry(const dlms_tamper_log_t *log, 
                                         size_t index, 
                                         dlms_tamper_record_t *out_record);

/**
 * @brief Mengodekan buffer log ke dalam format A-XDR Octet String / Compact Array untuk GET Response.
 */
size_t dlms_tamper_log_encode_axdr(const dlms_tamper_log_t *log, 
                                   uint8_t *out_buffer, 
                                   size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TAMPER_LOG_H */