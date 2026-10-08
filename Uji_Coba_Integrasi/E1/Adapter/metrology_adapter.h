/**
 * @file metrology_adapter.h
 * @brief Adapter Pembacaan Sensor Metrologi ADE9000 ke Data Model Sistem (E1 Lead)
 * 
 * Modul ini menjadi jembatan resmi antara driver register mentah ADE9000 milik E1
 * dengan struktur data canonical sistem meter_measurements_t milik E3.
 */

#ifndef METROLOGY_ADAPTER_H
#define METROLOGY_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "display.h" /* Menyediakan definisi struct meter_measurements_t */

/**
 * @brief Inisialisasi Mock/Driver ADE9000 & Setup Register Baseline 3-Fasa
 */
void metrology_adapter_init(void);

/**
 * @brief Eksekusi pembacaan register SPI ADE9000 & konversi ke besaran fisik meter_measurements_t
 * @param out_meas Pointer ke struct penampung data pengukuran sistem
 */
void metrology_adapter_sample(meter_measurements_t *out_meas);

#ifdef __cplusplus
}
#endif

#endif /* METROLOGY_ADAPTER_H */

