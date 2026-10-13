/**
 * @file test_bsp_rtc_tamp.c
 * @brief Unit Test untuk Deteksi Sabotase Saat Meter Mati (Offline Tamper via STM32 TAMP / VBAT)
 */

#include <stdio.h>
#include <assert.h>
#include "bsp_rtc_tamp.h"
#include "rtos_tasks.h"
#include "display_task.h"

int main(void) {
    printf("=========================================\n");
    printf("    RUNNING BSP RTC TAMP UNIT TESTS      \n");
    printf("=========================================\n\n");

    /* 1. Inisialisasi Subsistem */
    rtos_system_init();
    bsp_rtc_tamp_init();

    /* 2. Verifikasi State Awal (Tidak ada sabotase) */
    bsp_offline_tamper_info_t info;
    assert(bsp_rtc_tamp_check_offline_tamper(&info) == false);
    printf("[TEST] Kondisi Awal Bersih (No Offline Tamper): SUCCESS\n");

    /* 3. Verifikasi Tulis/Baca Backup Register (BKPxR) */
    bsp_rtc_tamp_set_backup_reg(5, 0x12345678);
    assert(bsp_rtc_tamp_get_backup_reg(5) == 0x12345678);
    bsp_rtc_tamp_set_backup_reg(10, 0xDEADBEEF);
    assert(bsp_rtc_tamp_get_backup_reg(10) == 0xDEADBEEF);
    printf("[TEST] Verifikasi Akses Backup Register (BKPxR): SUCCESS\n");

    /* 4. Simulasi Terjadi Sabotase Saat Listrik Mati: Tutup Meter Terbuka (RUSAK) */
    printf("\n[TEST] Menguji Offline Tamper: Tutup Meter Terbuka (PC13 / TAMP1)...\n");
    uint32_t fake_offline_time = 1712345678;
    bsp_rtc_tamp_simulate_offline_tamper(DLMS_TAMPER_METER_COVER_OPEN, fake_offline_time);

    /* Verifikasi deteksi sebelum diproses */
    assert(bsp_rtc_tamp_check_offline_tamper(&info) == true);
    assert(info.occurred == true);
    assert(info.tamper_code == DLMS_TAMPER_METER_COVER_OPEN);
    assert(info.timestamp_epoch == fake_offline_time);
    assert(info.tamp_channel == 1);
    printf("[TEST] Verifikasi Pengecekan Event Sabotase Offline: SUCCESS\n");

    /* 5. Proses Pemulihan Event Sabotase Saat Booting (Power-On) */
    bsp_rtc_tamp_process_offline_events();

    /* Verifikasi pesan tersalurkan ke antrean FreeRTOS */
    tamper_event_msg_t rx_msg;
    bool received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_METER_COVER_OPEN);
    assert(rx_msg.timestamp == fake_offline_time);
    assert(rx_msg.is_active == true);
    assert(display_task_is_alarm_active() == true);
    assert(bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_COUNTER) == 1);
    printf("[TEST] Dispatch Event Sabotase Offline ke RTOS Queue & Display Alarm: SUCCESS\n");

    /* Verifikasi flag telah dibersihkan sehingga tidak terpicu ganda */
    assert(bsp_rtc_tamp_check_offline_tamper(&info) == false);
    printf("[TEST] Verifikasi Acknowledge & Clear Flag: SUCCESS\n");

    /* 6. Uji Coba Sabotase Offline Kedua: Tutup Terminal Terbuka (ERR20) */
    printf("\n[TEST] Menguji Offline Tamper Kedua: Tutup Terminal (ERR20)...\n");
    uint32_t fake_terminal_time = 1712349999;
    bsp_rtc_tamp_simulate_offline_tamper(DLMS_TAMPER_TERMINAL_COVER_OPEN, fake_terminal_time);

    bsp_rtc_tamp_process_offline_events();
    received = rtos_queue_receive_tamper_event(&rx_msg, 0);
    assert(received == true);
    assert(rx_msg.tamper_code == DLMS_TAMPER_TERMINAL_COVER_OPEN);
    assert(rx_msg.timestamp == fake_terminal_time);
    assert(bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_COUNTER) == 2);
    printf("[TEST] Dispatch Event Tutup Terminal Offline & Counter Increment: SUCCESS\n");

    printf("\n=========================================\n");
    printf("    SEMUA PENGUJIAN BSP RTC TAMP LULUS   \n");
    printf("=========================================\n");
    return 0;
}

