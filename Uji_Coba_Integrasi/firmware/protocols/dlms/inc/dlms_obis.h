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

bool dlms_obis_match(const obis_code_t *a, const obis_code_t *b);

/* Deklarasi prototipe fungsi lookup: */
dlms_result_t dlms_obis_lookup(
    uint16_t class_id, 
    const obis_code_t *obis, 
    uint8_t attribute_id, 
    uint8_t *data_type, 
    uint32_t *val_u32);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_OBIS_H */