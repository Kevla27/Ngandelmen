/**
 * @file bsp_watchdog.h
 * @brief Driver Pengawas Hardware & Supervisor Multi-Task (STM32 IWDG & FreeRTOS Heartbeat)
 * 
 * Memenuhi Standar Keamanan & Reliabilitas:
 * - Standar SPLN D3.006: 2021 Tabel 4 (Alarm Internal Meter - ERR04 MCU Failure / Watchdog Reset).
 * - Multi-Task Heartbeat Supervisor Pattern: Menjamin SELURUH task FreeRTOS penting
 *   (Metrologi, Sabotase, DLMS, UI Display, ADE9000 IRQ) aktif normal sebelum IWDG di-refresh.
 * - Mencegah "False Refresh" di mana Watchdog Task tetap me-reset IWDG padahal Task Metrologi / DLMS hang.
 * - Pencatatan reset counter dan flag penyebab boot di domain Backup Register TAMP_BKP3R.
 */

#ifndef BSP_WATCHDOG_H
#define BSP_WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Alokasi Bitmask Task FreeRTOS yang Diawasi oleh Watchdog Supervisor */
#define WATCHDOG_TASK_TAMPER_BIT      (1U << 0)  /**< Bit 0: Task Darurat Sabotase (TamperTask) */
#define WATCHDOG_TASK_METROLOGY_BIT   (1U << 1)  /**< Bit 1: Task Sampling & Profil Beban (ProfileTask) */
#define WATCHDOG_TASK_DLMS_BIT        (1U << 2)  /**< Bit 2: Task Server Protokol DLMS/COSEM (DLMSTask) */
#define WATCHDOG_TASK_UI_BIT          (1U << 3)  /**< Bit 3: Task Tampilan Display Layar (OLED/LCD Task) */
#define WATCHDOG_TASK_ADE9000_BIT     (1U << 4)  /**< Bit 4: Task Penanganan Interupsi ADE9000 (IRQ Worker) */

#define WATCHDOG_DEFAULT_MONITORED_MASK (WATCHDOG_TASK_TAMPER_BIT | \
                                         WATCHDOG_TASK_METROLOGY_BIT | \
                                         WATCHDOG_TASK_DLMS_BIT | \
                                         WATCHDOG_TASK_UI_BIT | \
                                         WATCHDOG_TASK_ADE9000_BIT)

#define WATCHDOG_DEFAULT_TIMEOUT_MS   3000U      /**< Timeout IWDG Hardware: 3,0 detik */
#define WATCHDOG_SUPERVISOR_PERIOD_MS 1000U      /**< Periode Evaluasi Supervisor: 1,0 detik */
#define TAMP_BKP_REG_WATCHDOG_COUNT   3U         /**< TAMP_BKP3R: Register Akumulator Watchdog Reset */

/**
 * @brief Status dan Telemetri Pengawas Watchdog
 */
typedef struct {
    bool is_initialized;             /**< true jika periferal IWDG telah aktif */
    bool last_reset_was_watchdog;    /**< true jika reboot terakhir dipicu oleh IWDG reset */
    uint32_t reset_counter;          /**< Akumulator total IWDG reset seumur hidup meter */
    uint32_t monitored_mask;         /**< Bitmask task yang wajib memberikan heartbeat */
    uint32_t current_heartbeat_mask; /**< Bitmask task yang telah melapor pada siklus aktif */
    uint32_t missing_tasks_mask;     /**< Bitmask task yang macet/gagal melapor */
    uint32_t timeout_ms;             /**< Batas waktu timeout hardware IWDG (milidetik) */
    uint32_t hw_countdown_ms;        /**< Sisa waktu sebelum hardware reset (untuk simulasi) */
} bsp_watchdog_status_t;

/**
 * @brief Inisialisasi driver IWDG hardware, pemeriksaan reset cause, dan setup supervisor
 * @param timeout_ms Waktu timeout hardware IWDG (rekomendasi: 3000 ms)
 */
void bsp_watchdog_init(uint32_t timeout_ms);

/**
 * @brief Memeriksa apakah boot sistem terakhir disebabkan oleh IWDG hardware timeout
 * @return true jika MCU me-reboot akibat watchdog timeout
 */
bool bsp_watchdog_was_reset_by_watchdog(void);

/**
 * @brief Mendapatkan akumulator jumlah IWDG reset dari domain Backup Register
 */
uint32_t bsp_watchdog_get_reset_count(void);

/**
 * @brief Mengirimkan heartbeat tanda hidup dari task FreeRTOS tertentu
 * @param task_bit Bit penanda task (misal WATCHDOG_TASK_METROLOGY_BIT)
 */
void bsp_watchdog_kick_task(uint32_t task_bit);

/**
 * @brief Mengevaluasi seluruh heartbeat task dan me-refresh hardware IWDG jika sehat
 * @return true jika semua task sehat dan hardware IWDG berhasil di-kick,
 *         false jika ada task yang macet (IWDG sengaja TIDAK di-refresh)
 */
bool bsp_watchdog_supervisor_step(void);

/**
 * @brief Mengambil salinan status dan telemetri pengawas watchdog saat ini
 */
void bsp_watchdog_get_status(bsp_watchdog_status_t *out_status);

/**
 * @brief Mengubah daftar task yang dimonitor oleh supervisor
 */
void bsp_watchdog_set_monitored_mask(uint32_t mask);

/**
 * @brief Meluncurkan task background FreeRTOS khusus untuk Supervisor Watchdog
 * @return true jika task berhasil dibuat
 */
bool bsp_watchdog_start_supervisor_task(void);

/**
 * @brief Task entry point untuk FreeRTOS Watchdog Supervisor
 */
void bsp_watchdog_supervisor_task_entry(void *pvParameters);

/* ========================================================================== */
/*                SIMULATION APIS (HOST CTEST & DIAGNOSTIK UART)              */
/* ========================================================================== */

/**
 * @brief Mensimulasikan pengurangan counter hardware IWDG (Host CTest)
 */
void bsp_watchdog_simulate_hw_countdown(uint32_t elapsed_ms);

/**
 * @brief Memeriksa apakah countdown hardware IWDG simulasi telah kedaluwarsa (expired)
 */
bool bsp_watchdog_simulate_is_hw_expired(void);

/**
 * @brief Mengatur status reset flag secara manual untuk keperluan pengujian
 */
void bsp_watchdog_simulate_set_reset_flag(bool was_reset);

/**
 * @brief Memicu simulasi freeze/hang pada task tertentu untuk menguji ketegasan supervisor
 */
void bsp_watchdog_simulate_freeze_task(uint32_t task_bit);

#ifdef __cplusplus
}
#endif

#endif /* BSP_WATCHDOG_H */

