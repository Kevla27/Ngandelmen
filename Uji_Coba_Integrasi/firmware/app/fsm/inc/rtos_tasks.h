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
/* Prioritas Task FreeRTOS */
#define PRIORITY_TASK_TAMPER 4
#define PRIORITY_TASK_PROFILING 3
#define PRIORITY_TASK_DLMS 2
#define PRIORITY_TASK_UI 1

/* Ukuran Stack Task (Words) */
#define STACK_SIZE_TAMPER 256
#define STACK_SIZE_PROFILING 512
#define STACK_SIZE_DLMS 1024
#define STACK_SIZE_UI 256
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
* @brief Inisialisasi seluruh Subsystem E3, Queue, dan Snapshot NVRAM
*/
void rtos_system_init(void);
/**
* @brief Mengirim pesan event tamper ke Queue (Producer)
*/
bool rtos_queue_send_tamper_event(const tamper_event_msg_t *msg);
/**
* @brief Menerima pesan event tamper dari Queue (Consumer)
*/
bool rtos_queue_receive_tamper_event(tamper_event_msg_t *out_msg, uint32_t timeout_ms);
/* Task Entry Points */
void task_tamper_emergency_entry(void *pvParameters);
void task_metrology_profiling_entry(void *pvParameters);
void task_dlms_entry(void *pvParameters);
void task_ui_display_entry(void *pvParameters);
#ifdef __cplusplus
}
#endif
#endif /* RTOS_TASKS_H */