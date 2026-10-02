/**
* @file test_rtos_tasks.c
* @brief Pengujian Inisialisasi System Task, FreeRTOS Queue & DLMS Consumer E3
*/
#include <stdio.h>
#include <assert.h>
#include "rtos_tasks.h"
int main(void) {
 printf("=========================================\n");
 printf(" RUNNING RTOS TASKS & QUEUE TEST \n");
 printf("=========================================\n\n");
 /* 1. Inisialisasi Sistem & Queue 
 rtos_system_init();
 /* 2. Uji Kirim Pesan Event Sabotase oleh Task Tamper (Producer) */
 tamper_event_msg_t tx_msg = {
 .timestamp = 1700000000,
 .tamper_code = 0x01, /* Terminal Cover Open */
 .is_active = true
 };
 bool send_ok = rtos_queue_send_tamper_event(&tx_msg);
 assert(send_ok == true);
 printf("[TEST] Task Tamper (Producer) Send Event to Queue: SUCCESS\n");
 /* 3. Uji Terima Pesan Event Sabotase oleh Task DLMS (Consumer) */
 tamper_event_msg_t rx_msg = {0};
 bool recv_ok = rtos_queue_receive_tamper_event(&rx_msg, 100);
 assert(recv_ok == true);
 assert(rx_msg.timestamp == 1700000000);
 assert(rx_msg.tamper_code == 0x01);
 assert(rx_msg.is_active == true);
 printf("[TEST] Task DLMS (Consumer) Receive Event from Queue: SUCCESS (Code: 0x%02X)\n",
 rx_msg.tamper_code);
 /* 4. Pastikan Queue Kosong Setelah Dibaca */
 bool empty_ok = rtos_queue_receive_tamper_event(&rx_msg, 100);
 assert(empty_ok == false);
 printf("[TEST] Verify Queue Empty State: SUCCESS\n");
 printf("\n>>> ALL RTOS TASKS, QUEUE & DLMS CONSUMER TESTS PASSED 100%! <<<\n");
 printf("=========================================\n");
 return 0;
}
