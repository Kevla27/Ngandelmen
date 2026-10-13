/**
 * @file bsp_rtc_tamp.c
 * @brief Implementasi Deteksi Sabotase Saat Meter Mati (Offline Tamper via STM32 TAMP / VBAT)
 */

#include "bsp_rtc_tamp.h"
#include "bsp_pin_mapping.h"
#include "rtos_tasks.h"
#include "display_task.h"
#include "tamper_manager.h"
#include "dlms_tamper_log.h"
#include "nvram_storage.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"
#endif

#if !defined(EMBEDDED_HARDWARE_TARGET) && !defined(USE_FREERTOS)
/* Buffer Simulasi Backup Register untuk Pengujian Host CTest */
static uint32_t s_simulated_bkp[32] = {0};
static volatile bool s_simulated_hw_flag = false;
#endif

void bsp_rtc_tamp_init(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* 1. Aktifkan Clock Power Controller (PWR) */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* 2. Berikan izin akses tulis ke Backup Domain (RTC & TAMP) */
    HAL_PWR_EnableBkUpAccess();

    /* 3. Aktifkan Clock RTC & TAMP APB */
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    /* 4. Konfigurasi TAMP1 (PC13) */
    /* Pastikan TAMP1 aktif untuk mendeteksi pembukaan switch fisik saat meter mati */
    if ((TAMP->CR1 & TAMP_CR1_TAMP1E) == 0U) {
        TAMP->CR1 |= TAMP_CR1_TAMP1E;
    }

    printf("[TAMP] Driver Hardware RTC/TAMP Backup Domain diinisialisasi (VBAT Active).\r\n");
#else
    printf("[TAMP] Inisialisasi Mock RTC/TAMP Backup Domain (Host CTest).\r\n");
#endif
}

uint32_t bsp_rtc_tamp_get_backup_reg(uint32_t reg_index) {
    if (reg_index >= 32) return 0;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    volatile uint32_t *bkp_ptr = &TAMP->BKP0R;
    return bkp_ptr[reg_index];
#else
    return s_simulated_bkp[reg_index];
#endif
}

void bsp_rtc_tamp_set_backup_reg(uint32_t reg_index, uint32_t value) {
    if (reg_index >= 32) return;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    volatile uint32_t *bkp_ptr = &TAMP->BKP0R;
    bkp_ptr[reg_index] = value;
#else
    s_simulated_bkp[reg_index] = value;
#endif
}

bool bsp_rtc_tamp_check_offline_tamper(bsp_offline_tamper_info_t *out_info) {
    if (out_info == NULL) return false;
    memset(out_info, 0, sizeof(bsp_offline_tamper_info_t));

    uint32_t bkp0 = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_TAMPER_FLAG);
    bool bkp_marker_set = ((bkp0 & 0xFFFF0000U) == TAMP_OFFLINE_MAGIC_MARKER);

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    bool hw_flag_set = ((TAMP->SR & TAMP_SR_TAMP1F) != 0U);
#else
    bool hw_flag_set = s_simulated_hw_flag;
#endif

    if (hw_flag_set || bkp_marker_set) {
        uint8_t code = (uint8_t)(bkp0 & 0x000000FFU);
        if (code == 0) {
            code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN; /* Default: PC13 Case Open */
        }

        uint32_t ts = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_TIMESTAMP);
        if (ts == 0) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
            ts = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
#else
            ts = (uint32_t)time(NULL);
#endif
        }

        out_info->occurred = true;
        out_info->tamper_code = code;
        out_info->timestamp_epoch = ts;
        out_info->tamp_channel = 1; /* PC13 / TAMP_IN1 */
        return true;
    }

    return false;
}

void bsp_rtc_tamp_clear_offline_flags(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* Acknowledge flag TAMP1F di register TAMP_SCR */
    TAMP->SCR = TAMP_SCR_CTAMP1F;
#else
    s_simulated_hw_flag = false;
#endif
    /* Bersihkan marker register backup */
    bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_TAMPER_FLAG, 0);
}

void bsp_rtc_tamp_process_offline_events(void) {
    bsp_offline_tamper_info_t info;
    if (bsp_rtc_tamp_check_offline_tamper(&info)) {
        const char *desc = (info.tamper_code == DLMS_TAMPER_METER_COVER_OPEN) ?
                           "TUTUP METER TERBUKA (RUSAK)" : "TUTUP TERMINAL DIBUKA (ERR20)";

        printf("\r\n======================================================================\r\n");
        printf(" [OFFLINE TAMPER] *** PERINGATAN: SABOTASE TERDETEKSI SAAT LISTRIK MATI! ***\r\n");
        printf("                 Status: %s\r\n", desc);
        printf("                 Waktu Kejadian (Epoch): %lu detik\r\n", (unsigned long)info.timestamp_epoch);
        printf("                 Kanal Fisik: TAMP1 (PC13 Microswitch)\r\n");
        printf("                 Kepatuhan: Standar SPLN D3.006: 2021 Bagian 5\r\n");
        printf("======================================================================\r\n");

        /* 1. Kirim event sabotase ke antrean FreeRTOS */
        tamper_event_msg_t msg = {
            .timestamp = info.timestamp_epoch,
            .tamper_code = info.tamper_code,
            .is_active = true
        };
        rtos_queue_send_tamper_event(&msg);

        /* 2. Pemicu alarm visual seketika pada display */
        display_task_trigger_instant_refresh(true);

        /* 3. Tambah counter kejadian offline di Backup Register */
        uint32_t count = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_COUNTER);
        bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_COUNTER, count + 1);

        /* 4. Bersihkan status register agar tidak terdeteksi ganda */
        bsp_rtc_tamp_clear_offline_flags();
    }
}

void bsp_rtc_tamp_simulate_offline_tamper(uint8_t tamper_code, uint32_t simulated_timestamp) {
    /* Set marker dan kode di Backup Register */
    uint32_t flag_val = TAMP_OFFLINE_MAGIC_MARKER | (uint32_t)tamper_code;
    bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_TAMPER_FLAG, flag_val);
    bsp_rtc_tamp_set_backup_reg(TAMP_BKP_REG_TIMESTAMP, simulated_timestamp);
#if !defined(EMBEDDED_HARDWARE_TARGET) && !defined(USE_FREERTOS)
    s_simulated_hw_flag = true;
#endif
    printf("[SIMULASI OFFLINE] Sabotase disimulasikan terjadi saat listrik mati (Code: 0x%02X, Time: %lu s).\r\n",
           tamper_code, (unsigned long)simulated_timestamp);
}

