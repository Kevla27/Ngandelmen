/**
 * @file bsp_ade9000_irq.h
 * @brief Driver Hardware Interupsi ADE9000 IRQ0 (PB0) & IRQ1 (PB1) via EXTI
 * 
 * Menangani deteksi perangkat keras berkecepatan tinggi (<20 ms) dari IC Metrologi ADE9000:
 * - IRQ0 (Pin 31 ADE9000 -> PB0 STM32 / EXTI0): Reverse Power (STATUS0 REVAP/BP/CP) & Urutan Fase Terbalik (STATUS0 PWRORD)
 * - IRQ1 (Pin 32 ADE9000 -> PB1 STM32 / EXTI1): Kawat Netral Hilang (STATUS1 MISNEUT/ISUM) & Hilang Tegangan Fase (STATUS1 SAG)
 * 
 * Sesuai SPLN D3.006: 2021 Tabel 6 & 26-Week Development Plan Rev D.
 */

#ifndef BSP_ADE9000_IRQ_H
#define BSP_ADE9000_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dlms_tamper_log.h"

/* ========================================================================== */
/*                ADE9000 STATUS0 REGISTER BITS (IRQ0)                        */
/* ========================================================================== */
#define ADE9000_STATUS0_AEHF        (1U << 0)   /**< Active Energy Half Full */
#define ADE9000_STATUS0_REHF        (1U << 1)   /**< Reactive Energy Half Full */
#define ADE9000_STATUS0_SEHF        (1U << 2)   /**< Apparent Energy Half Full */
#define ADE9000_STATUS0_PAGE_FULL   (1U << 3)   /**< Waveform Buffer Page Full */
#define ADE9000_STATUS0_REVAP       (1U << 17)  /**< Phase A Reverse Active Power */
#define ADE9000_STATUS0_REVBP       (1U << 18)  /**< Phase B Reverse Active Power */
#define ADE9000_STATUS0_REVCP       (1U << 19)  /**< Phase C Reverse Active Power */
#define ADE9000_STATUS0_REV_ALL     (ADE9000_STATUS0_REVAP | ADE9000_STATUS0_REVBP | ADE9000_STATUS0_REVCP)
#define ADE9000_STATUS0_PWRORD      (1U << 20)  /**< Phase Sequence Error (Reverse Phase) */

/* ========================================================================== */
/*                ADE9000 STATUS1 REGISTER BITS (IRQ1)                        */
/* ========================================================================== */
#define ADE9000_STATUS1_SAG         (1U << 16)  /**< Combined Voltage Sag Detection */
#define ADE9000_STATUS1_SAGA        (1U << 17)  /**< Phase A Voltage Sag */
#define ADE9000_STATUS1_SAGB        (1U << 18)  /**< Phase B Voltage Sag */
#define ADE9000_STATUS1_SAGC        (1U << 19)  /**< Phase C Voltage Sag */
#define ADE9000_STATUS1_SAG_ALL     (ADE9000_STATUS1_SAG | ADE9000_STATUS1_SAGA | ADE9000_STATUS1_SAGB | ADE9000_STATUS1_SAGC)
#define ADE9000_STATUS1_MISNEUT     (1U << 24)  /**< Missing Neutral Event */
#define ADE9000_STATUS1_ISUM        (1U << 25)  /**< Current Sum Anomaly */

/* ========================================================================== */
/*                           PUBLIC API DEFINITION                            */
/* ========================================================================== */

/**
 * @brief Inisialisasi GPIO PB0 (IRQ0) & PB1 (IRQ1), interrupt EXTI, dan konfigurasi mask register ADE9000
 */
void bsp_ade9000_irq_init(void);

/**
 * @brief Handler interupsi hardware ADE9000 IRQ0 (Dipanggil saat PB0 Falling Edge / EXTI0)
 */
void bsp_ade9000_irq0_handler(void);

/**
 * @brief Handler interupsi hardware ADE9000 IRQ1 (Dipanggil saat PB1 Falling Edge / EXTI1)
 */
void bsp_ade9000_irq1_handler(void);

/**
 * @brief Mengambil total counter pemicuan interupsi IRQ0
 */
uint32_t bsp_ade9000_irq0_get_trigger_count(void);

/**
 * @brief Mengambil total counter pemicuan interupsi IRQ1
 */
uint32_t bsp_ade9000_irq1_get_trigger_count(void);

/**
 * @brief Mengambil nilai register STATUS0 terakhir yang dibaca saat IRQ0
 */
uint32_t bsp_ade9000_irq_get_last_status0(void);

/**
 * @brief Mengambil nilai register STATUS1 terakhir yang dibaca saat IRQ1
 */
uint32_t bsp_ade9000_irq_get_last_status1(void);

/**
 * @brief Reset internal trigger counters dan status cache (terutama untuk unit test)
 */
void bsp_ade9000_irq_reset_counters(void);

/**
 * @brief Fungsi simulasi injeksi interupsi ADE9000 untuk pengujian host CTest atau UART CLI
 * @param irq_line 0 untuk IRQ0, 1 untuk IRQ1
 * @param status_val Nilai bit register status yang disimulasikan
 */
void bsp_ade9000_irq_simulate_trigger(uint8_t irq_line, uint32_t status_val);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADE9000_IRQ_H */

