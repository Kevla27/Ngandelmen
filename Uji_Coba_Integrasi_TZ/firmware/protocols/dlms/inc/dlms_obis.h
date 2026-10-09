#ifndef DLMS_OBIS_H
#define DLMS_OBIS_H

#include "dlms_types.h"
#include "dlms_apdu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t class_id;
    obis_code_t obis;
    uint8_t attribute_id;
    uint8_t data_type;
    uint32_t value_u32;
} obis_entry_t;

#include "display.h"
#include "dlms_tamper_log.h"

bool dlms_obis_match(const obis_code_t *a, const obis_code_t *b);

/* Deklarasi prototipe fungsi lookup: */
dlms_result_t dlms_obis_lookup(
    uint16_t class_id, 
    const obis_code_t *obis, 
    uint8_t attribute_id, 
    uint8_t *data_type, 
    uint32_t *val_u32);

/* Fungsi sinkronisasi data metrologi ke kamus register OBIS */
void dlms_obis_update_from_meter(const meter_measurements_t *meas);

/* Fungsi sinkronisasi & manipulasi akumulasi kejadian sabotase (Tamper Counters) */
void dlms_obis_increment_tamper_counter(dlms_tamper_code_t code);
void dlms_obis_set_tamper_counter(dlms_tamper_code_t code, uint32_t count);
uint32_t dlms_obis_get_tamper_counter(dlms_tamper_code_t code);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_OBIS_H */