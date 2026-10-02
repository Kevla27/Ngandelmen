#include "Metrology.h"

/* =========================================================
 * CURRENT
 * ========================================================= */

float Metrology_CurrentFromAIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromBIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromCIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromNIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

/* =========================================================
 * VOLTAGE
 * ========================================================= */

float Metrology_VoltageFromAVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromBVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromCVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

/* =========================================================
 * POWER
 * ========================================================= */

float Metrology_PowerFromAWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromBWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromCWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

/* =========================================================
 * ENERGY
 * ========================================================= */

int64_t Metrology_CombineEnergyRegister(uint32_t hi, uint32_t lo)
{
    /* ADE9000 energy registers are signed 45-bit values: HI[31:0]
     * contains bits [44:13], and LO[12:0] contains bits [12:0]. */
    uint64_t raw45 = ((uint64_t)hi << 13) | ((uint64_t)lo & 0x1FFFULL);

    /* Sign-extend bit 44 through the upper bits. */
    if ((raw45 & (1ULL << 44)) != 0)
    {
        raw45 |= 0xFFFFE00000000000ULL;
    }

    return (int64_t)raw45;
}

float Metrology_EnergyFromRaw(int64_t raw)
{
    return (float)raw * ENERGY_CONVERSION_CONSTANT;
}

float Metrology_EnergyKWhFromRaw(int64_t raw)
{
    return Metrology_EnergyFromRaw(raw) / 1000.0f;
}

/* =========================================================
 * FREQUENCY
 * ========================================================= */

float Metrology_FrequencyFromPeriod(uint32_t raw)
{
    if (raw == 0U)
    {
        return 0.0f;
    }

    /* Assumption: one raw count equals one microsecond. */
    const float period_seconds = (float)raw * 1.0e-6f;
    return 1.0f / period_seconds;
}

/* =========================================================
 * THD
 * ========================================================= */

float Metrology_THDFromRaw(uint32_t raw)
{
    /* Apply ADE9000 register scaling/calibration when established. */
    return (float)raw;
}