#include "dlms_tamper_log.h"
#include "dlms_apdu.h"
#include <string.h>

dlms_result_t dlms_tamper_log_init(dlms_tamper_log_t *log) {
    if (!log) return DLMS_ERR_NULL_PTR;
    memset(log, 0, sizeof(dlms_tamper_log_t));
    return DLMS_OK;
}

dlms_result_t dlms_tamper_log_add_event(dlms_tamper_log_t *log, 
                                         uint32_t timestamp_epoch, 
                                         dlms_tamper_code_t code, 
                                         uint8_t status) {
    if (!log) return DLMS_ERR_NULL_PTR;

    /* Tulis rekaman pada posisi head saat ini (FIFO) */
    log->records[log->head].timestamp_epoch = timestamp_epoch;
    log->records[log->head].event_code      = (uint8_t)code;
    log->records[log->head].status          = status;

    /* Geser pointer head sirkular */
    log->head = (log->head + 1) % DLMS_TAMPER_LOG_MAX_ENTRIES;

    if (log->entries_in_use < DLMS_TAMPER_LOG_MAX_ENTRIES) {
        log->entries_in_use++;
    }
    log->total_events++;

    return DLMS_OK;
}

dlms_result_t dlms_tamper_log_get_entry(const dlms_tamper_log_t *log, 
                                         size_t index, 
                                         dlms_tamper_record_t *out_record) {
    if (!log || !out_record) return DLMS_ERR_NULL_PTR;
    if (index >= log->entries_in_use) return DLMS_ERR_UNAUTHORIZED;

    /* Hitung posisi fisik pada ring buffer (0 = Rekaman Terbaru) */
    size_t pos;
    if (log->head >= (index + 1)) {
        pos = log->head - (index + 1);
    } else {
        pos = DLMS_TAMPER_LOG_MAX_ENTRIES + log->head - (index + 1);
    }

    *out_record = log->records[pos];
    return DLMS_OK;
}

size_t dlms_tamper_log_encode_axdr(const dlms_tamper_log_t *log, 
                                   uint8_t *out_buffer, 
                                   size_t max_len) {
    if (!log || !out_buffer || max_len < 8) return 0;

    /* Tag Array A-XDR (0x01) diikuti jumlah elemen array */
    out_buffer[0] = DLMS_DATA_TYPE_OCTET_STRING; 
    size_t offset = 1;

    /* Masukkan jumlah entri yang tersedia */
    out_buffer[offset++] = (uint8_t)log->entries_in_use;

    /* Serialisasi setiap entri log ke A-XDR Structure */
    for (size_t i = 0; i < log->entries_in_use; i++) {
        dlms_tamper_record_t rec;
        if (dlms_tamper_log_get_entry(log, i, &rec) == DLMS_OK) {
            if (offset + 6 > max_len) break; /* Mencegah overflow */
            
            /* Timestamp 4-byte (uint32_t) */
            out_buffer[offset++] = (uint8_t)((rec.timestamp_epoch >> 24) & 0xFF);
            out_buffer[offset++] = (uint8_t)((rec.timestamp_epoch >> 16) & 0xFF);
            out_buffer[offset++] = (uint8_t)((rec.timestamp_epoch >> 8) & 0xFF);
            out_buffer[offset++] = (uint8_t)(rec.timestamp_epoch & 0xFF);

            /* Event Code & Status */
            out_buffer[offset++] = rec.event_code;
            out_buffer[offset++] = rec.status;
        }
    }

    return offset;
}