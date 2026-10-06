#include "dlms_obis.h"

#if defined(USE_FREERTOS)
#include "rtos_tasks.h"
#else
static inline void rtos_meter_data_lock(void) {}
static inline void rtos_meter_data_unlock(void) {}
#endif

/* 
 * Pangkalan Data Register OBIS (Dataset Read-Only Gate G0 Baseline)
 * Mengacu pada ICD & SRS Spesifikasi Antarmuka DLMS Smart Meter 3-Fasa
 */
static obis_entry_t g_obis_db[] = {
    /* ----------------------------------------------------------------------------------- */
    /* 1. BESARAN TEGANGAN SESAAT (INSTANTANEOUS VOLTAGE) - CLASS ID 3                      */
    /* ----------------------------------------------------------------------------------- */
    /* Tegangan Fasa A / L1 (1.0.32.7.0.255) -> 230.2 V (skala x0.1 V) */
    {3, {1, 0, 32, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 2302},
    /* Tegangan Fasa B / L2 (1.0.52.7.0.255) -> 229.8 V (skala x0.1 V) */
    {3, {1, 0, 52, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 2298},
    /* Tegangan Fasa C / L3 (1.0.72.7.0.255) -> 230.5 V (skala x0.1 V) */
    {3, {1, 0, 72, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 2305},

    /* ----------------------------------------------------------------------------------- */
    /* 2. BESARAN ARUS SESAAT (INSTANTANEOUS CURRENT) - CLASS ID 3                          */
    /* ----------------------------------------------------------------------------------- */
    /* Arus Fasa A / L1 (1.0.31.7.0.255) -> 5.00 A (skala x0.01 A) */
    {3, {1, 0, 31, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 500},
    /* Arus Fasa B / L2 (1.0.51.7.0.255) -> 4.98 A (skala x0.01 A) */
    {3, {1, 0, 51, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 498},
    /* Arus Fasa C / L3 (1.0.71.7.0.255) -> 5.02 A (skala x0.01 A) */
    {3, {1, 0, 71, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 502},
    /* Arus Netral N (1.0.91.7.0.255) -> 0.12 A (skala x0.01 A) */
    {3, {1, 0, 91, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 12},

    /* ----------------------------------------------------------------------------------- */
    /* 3. DAYA AKTIF, REAKTIF, SEMU & KUALITAS DAYA - CLASS ID 3                            */
    /* ----------------------------------------------------------------------------------- */
    /* Daya Aktif Total +P (1.0.1.7.0.255) -> 3450 Watt */
    {3, {1, 0, 1, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 3450},
    /* Daya Aktif L1 (+P1) (1.0.21.7.0.255) -> 1151 Watt */
    {3, {1, 0, 21, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 1151},
    /* Daya Aktif L2 (+P2) (1.0.41.7.0.255) -> 1144 Watt */
    {3, {1, 0, 41, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 1144},
    /* Daya Aktif L3 (+P3) (1.0.61.7.0.255) -> 1155 Watt */
    {3, {1, 0, 61, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 1155},

    /* Daya Reaktif Total +Q (1.0.3.7.0.255) -> 250 var */
    {3, {1, 0, 3, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 250},
    /* Daya Semu Total S (1.0.9.7.0.255) -> 3459 VA */
    {3, {1, 0, 9, 7, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 3459},

    /* Faktor Daya Total / Power Factor (1.0.13.7.0.255) -> 0.997 (skala x0.001) */
    {3, {1, 0, 13, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 997},
    /* Frekuensi Jaringan (1.0.14.7.0.255) -> 50.00 Hz (skala x0.01 Hz) */
    {3, {1, 0, 14, 7, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 5000},

    /* ----------------------------------------------------------------------------------- */
    /* 4. REGISTER AKUMULASI ENERGI PENAGIHAN (BILLING REGISTERS) - CLASS ID 3             */
    /* ----------------------------------------------------------------------------------- */
    /* Energi Aktif Impor Total +A (1.0.1.8.0.255) -> 1234.56 kWh (skala x0.01 kWh) */
    {3, {1, 0, 1, 8, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 123456},
    /* Energi Aktif Impor Tarif 1 / LWBP (1.0.1.8.1.255) -> 800.00 kWh (skala x0.01 kWh) */
    {3, {1, 0, 1, 8, 1, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 80000},
    /* Energi Aktif Impor Tarif 2 / WBP (1.0.1.8.2.255) -> 434.56 kWh (skala x0.01 kWh) */
    {3, {1, 0, 1, 8, 2, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 43456},

    /* Energi Aktif Ekspor Total -A (1.0.2.8.0.255) -> 0.00 kWh (skala x0.01 kWh) */
    {3, {1, 0, 2, 8, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 0},
    /* Energi Reaktif Impor Q1 +R (1.0.5.8.0.255) -> 43.21 kvarh (skala x0.01 kvarh) */
    {3, {1, 0, 5, 8, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 4321},
    /* Energi Reaktif Ekspor Q4 -R (1.0.6.8.0.255) -> 0.00 kvarh (skala x0.01 kvarh) */
    {3, {1, 0, 6, 8, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 0},
    /* Energi Semu Total S (1.0.9.8.0.255) -> 1235.32 kVAh (skala x0.01 kVAh) */
    {3, {1, 0, 9, 8, 0, 255}, 2, DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED, 123532},

    /* ----------------------------------------------------------------------------------- */
    /* 5. INFORMASI PERANGKAT & AKUMULASI PENGHITUNG TAMPER (CLASS ID 1 & CLASS ID 3)      */
    /* ----------------------------------------------------------------------------------- */
    /* Penghitung Kejadian Tamper Total (0.0.96.20.0.255) -> 3 Kali */
    {1, {0, 0, 96, 20, 0, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 3},
    /* Akumulasi Tutup Terminal Dibuka / Terminal Cover Open (0.0.96.20.5.255) -> 1 Kali */
    {1, {0, 0, 96, 20, 5, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 1},
    /* Akumulasi Hilang Netral / Missing Neutral (0.0.96.20.24.255) -> 0 Kali */
    {1, {0, 0, 96, 20, 24, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 0},
    /* Akumulasi Induksi Magnet / Magnetic Field (0.0.96.20.26.255) -> 1 Kali */
    {1, {0, 0, 96, 20, 26, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 1},
    /* Akumulasi Arus Terbalik / Reverse Current (0.0.96.20.27.255) -> 1 Kali */
    {1, {0, 0, 96, 20, 27, 255}, 2, DLMS_DATA_TYPE_LONG_UNSIGNED, 1},

    /* ----------------------------------------------------------------------------------- */
    /* 6. PROFIL LOG KEJADIAN & PROFIL BEBAN (CLASS ID 7 - PROFILE GENERIC)               */
    /* ----------------------------------------------------------------------------------- */
    /* Tamper Event Log Profile Generic (0.0.99.98.0.255) */
    {7, {0, 0, 99, 98, 0, 255}, 2, DLMS_DATA_TYPE_OCTET_STRING, 0},
    /* Standard Event Log Profile Generic (0.0.99.98.1.255) */
    {7, {0, 0, 99, 98, 1, 255}, 2, DLMS_DATA_TYPE_OCTET_STRING, 0},
    /* Load Profile 15-menitan (1.0.99.1.0.255) */
    {7, {1, 0, 99, 1, 0, 255}, 2, DLMS_DATA_TYPE_OCTET_STRING, 0}
};

static const size_t g_obis_db_count = sizeof(g_obis_db) / sizeof(g_obis_db[0]);

bool dlms_obis_match(const obis_code_t *a, const obis_code_t *b) {
    if (!a || !b) return false;
    return (a->a == b->a && 
            a->b == b->b && 
            a->c == b->c &&
            a->d == b->d && 
            a->e == b->e && 
            a->f == b->f);
}

dlms_result_t dlms_obis_lookup(uint16_t class_id, 
                               const obis_code_t *obis, 
                               uint8_t attribute_id, 
                               uint8_t *data_type, 
                               uint32_t *val_u32) {
    if (!obis || !data_type || !val_u32) return DLMS_ERR_NULL_PTR;

    rtos_meter_data_lock();
    for (size_t i = 0; i < g_obis_db_count; i++) {
        if (g_obis_db[i].class_id == class_id &&
            g_obis_db[i].attribute_id == attribute_id &&
            dlms_obis_match(&g_obis_db[i].obis, obis)) {
            
            *data_type = g_obis_db[i].data_type;
            *val_u32 = g_obis_db[i].value_u32;
            rtos_meter_data_unlock();
            return DLMS_OK;
        }
    }
    rtos_meter_data_unlock();

    /* Jika OBIS Code atau Attribute tidak terdaftar di Gate G0 */
    return DLMS_ERR_UNAUTHORIZED;
}

void dlms_obis_update_from_meter(const meter_measurements_t *meas) {
    if (!meas) return;

    for (size_t i = 0; i < g_obis_db_count; i++) {
        obis_entry_t *entry = &g_obis_db[i];
        
        if (entry->class_id == 3 && entry->attribute_id == 2) {
            /* 1. Tegangan Sesaat (skala x0.1 V) */
            if (entry->obis.c == 32 && entry->obis.d == 7) {
                entry->value_u32 = meas->voltage_r_dvolts;
            } else if (entry->obis.c == 52 && entry->obis.d == 7) {
                entry->value_u32 = meas->voltage_s_dvolts;
            } else if (entry->obis.c == 72 && entry->obis.d == 7) {
                entry->value_u32 = meas->voltage_t_dvolts;
            }
            /* 2. Arus Sesaat (skala x0.01 A / 10 mA) */
            else if (entry->obis.c == 31 && entry->obis.d == 7) {
                entry->value_u32 = meas->current_r_mamps / 10;
            } else if (entry->obis.c == 51 && entry->obis.d == 7) {
                entry->value_u32 = meas->current_s_mamps / 10;
            } else if (entry->obis.c == 71 && entry->obis.d == 7) {
                entry->value_u32 = meas->current_t_mamps / 10;
            } else if (entry->obis.c == 91 && entry->obis.d == 7) {
                entry->value_u32 = meas->current_n_mamps / 10;
            }
            /* 3. Daya Aktif Total (Watt) */
            else if (entry->obis.c == 1 && entry->obis.d == 7) {
                entry->value_u32 = (uint32_t)(meas->active_power_w >= 0 ? meas->active_power_w : -meas->active_power_w);
            }
            /* 4. Daya Reaktif & Daya Semu Total (var & VA) */
            else if (entry->obis.c == 3 && entry->obis.d == 7) {
                entry->value_u32 = (uint32_t)(meas->reactive_power_var >= 0 ? meas->reactive_power_var : -meas->reactive_power_var);
            } else if (entry->obis.c == 9 && entry->obis.d == 7) {
                entry->value_u32 = meas->apparent_power_va;
            }
            /* 5. Power Factor (x0.001) & Frekuensi Grid (x0.01 Hz) */
            else if (entry->obis.c == 13 && entry->obis.d == 7) {
                entry->value_u32 = meas->power_factor_ppm;
            } else if (entry->obis.c == 14 && entry->obis.d == 7) {
                entry->value_u32 = meas->frequency_mhz / 10;
            }
            /* 6. Energi Aktif Impor Total (skala x0.01 kWh = 10 Wh per LSB) */
            else if (entry->obis.c == 1 && entry->obis.d == 8 && entry->obis.e == 0) {
                entry->value_u32 = (uint32_t)(meas->active_energy_wh / 10);
            }
        }
    }
}