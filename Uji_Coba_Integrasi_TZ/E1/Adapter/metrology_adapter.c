/**
 * @file metrology_adapter.c
 * @brief Implementasi Adapter Pembacaan Sensor Metrologi ADE9000 (E1 Lead)
 */

#include "metrology_adapter.h"
#include "RegisterMap.h"
#include "Mock.h"
#include "Metrology.h"
#include "SPI.h"
#include <string.h>
#include <math.h>

void metrology_adapter_init(void)
{
    /* 1. Inisialisasi Mock Device ADE9000 & SPI Abstraction Layer E1 */
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();

    /* 2. Set Baseline Register Metrologi 3-Fasa (Data Simulasi E1) */
    /* PHASE A: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetAIRMS(5962491);
    ADE9000_Mock_SetAVRMS(17136895);
    ADE9000_Mock_SetAIFRMS(26347436);
    ADE9000_Mock_SetAWATT(636984);
    ADE9000_Mock_SetAPERIOD(20000);
    ADE9000_Mock_SetAWATTHR_HI(0x00000001);
    ADE9000_Mock_SetAWATTHR_LO(0x00001000);

    /* PHASE B: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetBIRMS(5962491);
    ADE9000_Mock_SetBVRMS(17136895);
    ADE9000_Mock_SetBWATT(636984);
    ADE9000_Mock_SetBPERIOD(20000);
    ADE9000_Mock_SetBWATTHR_HI(0x00000001);
    ADE9000_Mock_SetBWATTHR_LO(0x00001000);

    /* PHASE C: V=230V, I=11.95A, P=2.3kW, f=50Hz */
    ADE9000_Mock_SetCIRMS(5962491);
    ADE9000_Mock_SetCVRMS(17136895);
    ADE9000_Mock_SetCWATT(636984);
    ADE9000_Mock_SetCPERIOD(20000);
    ADE9000_Mock_SetCWATTHR_HI(0x00000001);
    ADE9000_Mock_SetCWATTHR_LO(0x00001000);

    /* NEUTRAL: I=2.39A */
    ADE9000_Mock_SetNIRMS(1192498);
}

void metrology_adapter_sample(meter_measurements_t *out_meas)
{
    if (out_meas == NULL) return;

    /* 1. Baca register RMS Tegangan & Arus via SPI Driver E1 */
    uint32_t airms = ADE9000_SPI_ReadRegister(ADE9000_AIRMS);
    uint32_t birms = ADE9000_SPI_ReadRegister(ADE9000_BIRMS);
    uint32_t cirms = ADE9000_SPI_ReadRegister(ADE9000_CIRMS);
    uint32_t nirms = ADE9000_SPI_ReadRegister(ADE9000_NIRMS);

    uint32_t avrms = ADE9000_SPI_ReadRegister(ADE9000_AVRMS);
    uint32_t bvrms = ADE9000_SPI_ReadRegister(ADE9000_BVRMS);
    uint32_t cvrms = ADE9000_SPI_ReadRegister(ADE9000_CVRMS);

    /* 2. Baca register Daya Aktif & Frekuensi (Period) */
    int32_t awatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_AWATT);
    int32_t bwatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BWATT);
    int32_t cwatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CWATT);

    uint32_t aperiod = ADE9000_SPI_ReadRegister(ADE9000_APERIOD);

    /* 3. Baca register Akumulasi Energi */
    uint32_t awatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_HI);
    uint32_t awatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_LO);
    uint32_t bwatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_HI);
    uint32_t bwatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_LO);
    uint32_t cwatthr_hi = ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_HI);
    uint32_t cwatthr_lo = ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_LO);

    /* 4. Konversi nilai mentah register ke unit engineering menggunakan library Metrology E1 */
    float va = Metrology_VoltageFromAVRMS(avrms);
    float vb = Metrology_VoltageFromBVRMS(bvrms);
    float vc = Metrology_VoltageFromCVRMS(cvrms);

    float ia = Metrology_CurrentFromAIRMS(airms);
    float ib = Metrology_CurrentFromBIRMS(birms);
    float ic = Metrology_CurrentFromCIRMS(cirms);
    float in = Metrology_CurrentFromNIRMS(nirms);

    float pa = Metrology_PowerFromAWATT(awatt);
    float pb = Metrology_PowerFromBWATT(bwatt);
    float pc = Metrology_PowerFromCWATT(cwatt);
    float p_total = pa + pb + pc;

    float freq = Metrology_FrequencyFromPeriod(aperiod);

    int64_t raw_energy_a = Metrology_CombineEnergyRegister(awatthr_hi, awatthr_lo);
    int64_t raw_energy_b = Metrology_CombineEnergyRegister(bwatthr_hi, bwatthr_lo);
    int64_t raw_energy_c = Metrology_CombineEnergyRegister(cwatthr_hi, cwatthr_lo);

    float ea = Metrology_EnergyFromRaw(raw_energy_a);
    float eb = Metrology_EnergyFromRaw(raw_energy_b);
    float ec = Metrology_EnergyFromRaw(raw_energy_c);
    float e_total_wh = ea + eb + ec;

    /* 5. Petakan ke struct meter_measurements_t (E3 Display & Profiling) */
    out_meas->voltage_r_dvolts = (uint32_t)(va * 10.0f);
    out_meas->voltage_s_dvolts = (uint32_t)(vb * 10.0f);
    out_meas->voltage_t_dvolts = (uint32_t)(vc * 10.0f);

    out_meas->current_r_mamps  = (uint32_t)(ia * 1000.0f);
    out_meas->current_s_mamps  = (uint32_t)(ib * 1000.0f);
    out_meas->current_t_mamps  = (uint32_t)(ic * 1000.0f);
    out_meas->current_n_mamps  = (uint32_t)(in * 1000.0f);

    out_meas->active_power_w   = (int32_t)p_total;
    out_meas->reactive_power_var = 0;
    out_meas->apparent_power_va  = (uint32_t)((va * ia) + (vb * ib) + (vc * ic));
    out_meas->power_factor_ppm   = (out_meas->apparent_power_va > 0) ? (uint16_t)((fabsf(p_total) / out_meas->apparent_power_va) * 1000.0f) : 1000;

    out_meas->frequency_mhz    = (uint16_t)(freq * 1000.0f);

    /* Simulasi akumulasi energi aktif yang terus bertambah seiring berjalannya meter */
    static uint32_t s_simulated_energy_increment = 0;
    s_simulated_energy_increment += 4; /* ~4 Wh tiap iterasi 2 detik (~7 kW) */
    out_meas->active_energy_wh = 125430ULL + (uint64_t)e_total_wh + s_simulated_energy_increment;
}

