/**
* @file rtos_tasks.h
* @brief Definisi Task & Inter-Task Communication FreeRTOS E3
*/
#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "display.h"
#include "tamper_manager.h"
#include "dlms_task.h"

/* Prioritas Task FreeRTOS */
#define PRIORITY_TASK_TAMPER 4
#define PRIORITY_TASK_PROFILING 3
#define PRIORITY_TASK_DLMS 2
#define PRIORITY_TASK_UI 1

/* Ukuran Stack Task (Words) */
#define STACK_SIZE_TAMPER 256
#define STACK_SIZE_PROFILING 512
#define STACK_SIZE_DLMS 1024
#define STACK_SIZE_UI 512

/* Kapasitas Queue */
#define TAMPER_QUEUE_MAX_ITEMS 10

/**
* @brief Struktur Pesan Queue Event Sabotase / Tamper
*/
typedef struct {
 uint32_t timestamp; /**< Waktu kejadian (Epoch UNIX) */
 uint8_t tamper_code; /**< Kode jenis sabotase */
 bool is_active; /**< Status: true = Terdeteksi, false = Pulih */
} tamper_event_msg_t;

/**
* @brief Inisialisasi seluruh Subsystem E3, Queue, Mutex, dan Snapshot NVRAM
*/
void rtos_system_init(void);

/**
* @brief Mengirim pesan event tamper ke Queue (Producer)
*/
bool rtos_queue_send_tamper_event(const tamper_event_msg_t *msg);

/**
* @brief Mengirim pesan event tamper ke Queue dari ISR (Interrupt Context)
*/
bool rtos_queue_send_tamper_event_from_isr(const tamper_event_msg_t *msg);

/**
* @brief Menerima pesan event tamper dari Queue (Consumer)
*/
bool rtos_queue_receive_tamper_event(tamper_event_msg_t *out_msg, uint32_t timeout_ms);

/* ========================================================================== */
/*           THREAD-SAFE MEASUREMENT & METROLOGY SERVICE (IPC)                */
/* ========================================================================== */

/**
* @brief Tipe callback fungsi sampling metrologi E1
*/
typedef void (*meter_sample_fn_t)(meter_measurements_t *out_meas);

/**
* @brief Mendaftarkan callback sampling metrologi untuk dieksekusi oleh ProfileTask
*/
void rtos_metrology_set_sample_cb(meter_sample_fn_t cb);

/**
* @brief Mempublikasikan snapshot pengukuran terbaru secara aman (Thread-Safe)
* Mengunci Mutex, mengupdate buffer global, dan menyinkronkan nilai ke kamus OBIS DLMS.
*/
void rtos_meter_data_publish(const meter_measurements_t *meas);

/**
* @brief Mengambil snapshot data pengukuran terbaru untuk UI Display (Thread-Safe)
*/
bool rtos_meter_data_get_snapshot(meter_measurements_t *out_meas);

/**
* @brief Mengunci akses ke kamus register OBIS dan data pengukuran (Mutex Lock)
*/
void rtos_meter_data_lock(void);

/**
* @brief Membuka kunci akses ke kamus register OBIS dan data pengukuran (Mutex Unlock)
*/
void rtos_meter_data_unlock(void);

/* Task Entry Points */
void task_tamper_emergency_entry(void *pvParameters);
void task_metrology_profiling_entry(void *pvParameters);
void task_dlms_entry(void *pvParameters);
void task_ui_display_entry(void *pvParameters);

/**
 * @brief Membuat dan meluncurkan seluruh 4 Task FreeRTOS aplikasi
 * @return true jika semua task berhasil dibuat, false jika gagal
 */
bool rtos_start_all_tasks(void);

/**
* @brief Callback pengiriman byte UART dari DLMS Task
*/
typedef void (*dlms_uart_tx_fn_t)(const uint8_t *data, size_t len);

/**
* @brief Konfigurasi callback transmisi UART untuk DLMS Task
*/
void rtos_dlms_task_set_tx_cb(dlms_uart_tx_fn_t tx_cb);

/**
* @brief Mengirim byte data masuk dari UART RX ISR ke ring buffer DLMS Task
*/
void rtos_dlms_task_notify_rx(const uint8_t *data, size_t len);

/**
* @brief Memproses pesan event sabotase secara langsung ke DLMS Log dan tabel OBIS
*/
void rtos_dlms_process_tamper_event(const tamper_event_msg_t *msg);

/**
* @brief Mengambil pointer konteks DLMS Task aktif
*/
dlms_task_ctx_t* rtos_dlms_get_task_ctx(void);

/**
* @brief Mengambil pointer konteks Tamper aktif
*/
tamper_context_t* rtos_tamper_get_ctx(void);

#ifdef __cplusplus
}
#endif
#endif /* RTOS_TASKS_H */