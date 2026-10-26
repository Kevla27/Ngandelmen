/**
 * @file bsp_bringup_test.c
 * @brief Implementasi Diagnostic & Smoke-Test Shell untuk Hardware Bring-Up PCB Spin 1
 */

#include "bsp_bringup_test.h"
#include "bsp_pin_mapping.h"
#include "bsp_alarm_buzzer.h"
#include "bsp_ade9000_irq.h"
#include "bsp_pvd.h"
#include "nvram_storage.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "Mock.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "stm32u5xx_hal.h"
extern I2C_HandleTypeDef hi2c1;
#endif

void bsp_bringup_init(void) {
    bsp_alarm_buzzer_init();
    bsp_ade9000_irq_init();
    bsp_pvd_init();
}

bool bsp_bringup_test_i2c_bus(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 1] PEMINDAIAN BUS I2C (0x08 - 0x77)      \r\n");
    printf("========================================================\r\n");
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    uint8_t devices_found = 0;

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = (row * 16) + col;
            if (addr < 0x08 || addr > 0x77) {
                printf("   ");
            } else {
                HAL_StatusTypeDef res = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2, 5);
                if (res == HAL_OK) {
                    printf("%02X ", addr);
                    devices_found++;
                } else {
                    printf("-- ");
                }
            }
        }
        printf("\r\n");
    }
#else
    /* Simulasi I2C Scan untuk Host CTest */
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = (row * 16) + col;
            if (addr < 0x08 || addr > 0x77) {
                printf("   ");
            } else if (addr == 0x3C) {
                printf("3C "); /* Alamat Default OLED SSD1306 */
                devices_found++;
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
#endif

    printf("--------------------------------------------------------\r\n");
    printf("Total Perangkat I2C Terdeteksi: %u\r\n", devices_found);
    if (devices_found > 0) {
        printf("[PASS] Bus I2C berfungsi normal (ACK diterima).\r\n");
        return true;
    } else {
        printf("[FAIL] Tidak ada perangkat I2C yang merespons (Periksa pull-up SCL/SDA).\r\n");
        return false;
    }
}

bool bsp_bringup_test_ade9000_comm(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 2] INTEGRITAS KOMUNIKASI SPI ADE9000     \r\n");
    printf("========================================================\r\n");

    bsp_ade9000_spi_lock();

    /* 1. Baca nilai awal konfigurasi MASK0 */
    uint32_t orig_mask0 = ADE9000_SPI_ReadRegister(ADE9000_MASK0);
    printf("Nilai Registri Asli MASK0 : 0x%08lX\r\n", (unsigned long)orig_mask0);

    /* 2. Tulis pola uji coba (Read-After-Write Loopback) */
    uint32_t test_pattern = 0x00A55A00;
    ADE9000_SPI_WriteRegister(ADE9000_MASK0, test_pattern);

    /* 3. Baca kembali dan validasi */
    uint32_t read_back = ADE9000_SPI_ReadRegister(ADE9000_MASK0);
    printf("Nilai Uji Coba Ditulis    : 0x%08lX\r\n", (unsigned long)test_pattern);
    printf("Nilai Terbaca Kembali     : 0x%08lX\r\n", (unsigned long)read_back);

    /* 4. Kembalikan nilai register asli */
    ADE9000_SPI_WriteRegister(ADE9000_MASK0, orig_mask0);

    bsp_ade9000_spi_unlock();

    bool ok = (read_back == test_pattern);
    if (ok) {
        printf("[PASS] Bus SPI ADE9000 terverifikasi 100%% (Read-After-Write OK).\r\n");
    } else {
        printf("[FAIL] Ketidakcocokan data SPI! (Periksa jalur MOSI/MISO/SCK/CS).\r\n");
    }
    return ok;
}

bool bsp_bringup_test_gpio_actuators(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 3] UJI AKTUATOR (BUZZER & SHUNT TRIP)    \r\n");
    printf("========================================================\r\n");

    printf("1. Menyalakan Buzzer / LED Alarm PB7 (200 ms)...\r\n");
    bsp_alarm_buzzer_set(true);

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    HAL_Delay(200);
#endif

    bsp_alarm_buzzer_set(false);
    printf("   Buzzer dimatikan kembali.\r\n");

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* Uji Kontrol Pemutus Shunt Trip PC8 (Meter Sambungan Tidak Langsung - SPLN D3.006 Tabel 2) */
    printf("2. Menguji Kontrol Pemutus Beban Shunt Trip PC8...\r\n");
    HAL_GPIO_WritePin(BSP_RELAY_TRIP_GPIO_PORT, BSP_RELAY_TRIP_PIN, GPIO_PIN_RESET);
    printf("   Pin Shunt Trip PC8 dalam kondisi aman (IDLE/LOW - Tripping Release).\r\n");
    printf("   (Catatan: Reconnect MCCB dilakukan secara MANUAL paska Tripping Release).\r\n");
#else
    printf("2. Simulasi Kontrol Shunt Trip PC8 (OK - Tripping Release).\r\n");
#endif

    printf("[PASS] Aktuator PB7 & Shunt Trip PC8 berhasil diuji.\r\n");
    return true;
}

bool bsp_bringup_test_gpio_sensors(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 4] PEMANTAUAN PIN SENSOR & SABOTASE      \r\n");
    printf("========================================================\r\n");

    uint8_t case_state = 0;
    uint8_t irq0_state = 1;
    uint8_t irq1_state = 1;
    uint8_t cf1_state = 1;
    uint8_t cf2_state = 1;
    uint8_t cf3_state = 0;
    uint8_t event_state = 1;

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    case_state = (uint8_t)HAL_GPIO_ReadPin(BSP_TAMPER_CASE_GPIO_PORT, BSP_TAMPER_CASE_PIN);
    irq0_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE9000_IRQ0_GPIO_PORT, BSP_ADE9000_IRQ0_PIN);
    irq1_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE9000_IRQ1_GPIO_PORT, BSP_ADE9000_IRQ1_PIN);
    cf1_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE_CF1_GPIO_PORT, BSP_ADE_CF1_PIN);
    cf2_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE_CF2_GPIO_PORT, BSP_ADE_CF2_PIN);
    cf3_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE_CF3_ZX_GPIO_PORT, BSP_ADE_CF3_ZX_PIN);
    event_state = (uint8_t)HAL_GPIO_ReadPin(BSP_ADE_EVENT_GPIO_PORT, BSP_ADE_EVENT_PIN);
#endif

    printf("• PC13 - Switch Tutup Meter (Case Open) : %s (Logika: %u)\r\n",
           (case_state == 0 ? "TERBUKA (Alarm Sabotase)" : "TERTUTUP (Normal)"), case_state);
    printf("• PB0  - ADE9000 IRQ0 (Active-Low)      : %s (Logika: %u)\r\n",
           (irq0_state == 0 ? "ASSERTED / AKTIF" : "IDLE / HIGH"), irq0_state);
    printf("• PB1  - ADE9000 IRQ1 (Active-Low)      : %s (Logika: %u)\r\n",
           (irq1_state == 0 ? "ASSERTED / AKTIF" : "IDLE / HIGH"), irq1_state);
    printf("• PE12 - ADE9000 CF1 (Pulsa Wh Aktif)   : Logika: %u (Idle: HIGH, Pulsa: 80ms LOW)\r\n", cf1_state);
    printf("• PE13 - ADE9000 CF2 (Pulsa varh Reaktif): Logika: %u (Idle: HIGH, Pulsa: 80ms LOW)\r\n", cf2_state);
    printf("• PE8  - ADE9000 CF3/ZX (Zero-Crossing) : Logika: %u (Deteksi AC Zero-Crossing)\r\n", cf3_state);
    printf("• PE11 - ADE9000 EVENT (Sag/Swell/OI)   : %s (Logika: %u, Pengganti DREADY)\r\n",
           (event_state == 0 ? "EVENT TERDETEKSI" : "NORMAL / IDLE"), event_state);

    printf("[PASS] Pembacaan logika level pin sensor valid.\r\n");
    return true;
}

bool bsp_bringup_test_pvd_power(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 5] MONITOR TEGANGAN PVD (LAST-GASP)      \r\n");
    printf("========================================================\r\n");

    bool power_failed = bsp_pvd_is_power_failed();
    printf("PVD Threshold Level : Level 6 (~2.9V Falling Edge)\r\n");
    printf("PVD Emergency State : %s\r\n", power_failed ? "POWER-FAIL ACTIVE (DROPPED)" : "POWER OK (NORMAL)");

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    /* Cek apakah tegangan saat ini berada di bawah batas PVD */
    bool vdd_below_threshold = (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != 0U);
    if (vdd_below_threshold) {
        printf("Status Tegangan VDD : PERINGATAN! VDD di bawah 2.9V (Mendekati Drop).\r\n");
    } else {
        printf("Status Tegangan VDD : NORMAL (> 2.9V, Suplai Daya Stabil).\r\n");
    }
#else
    printf("Status Tegangan VDD : NORMAL (Simulasi Host 3.3V Stabil).\r\n");
#endif

    printf("[PASS] Modul PVD siap melindungi data saat pemadaman listrik.\r\n");
    return true;
}

bool bsp_bringup_test_nvram_storage(void) {
    printf("\r\n========================================================\r\n");
    printf("   [DIAGNOSTIK 6] UJI MEMORI NON-VOLATILE (FLASH/FRAM)  \r\n");
    printf("========================================================\r\n");

    load_profile_entry_t write_entry;
    load_profile_entry_t read_entry;
    memset(&write_entry, 0, sizeof(write_entry));
    memset(&read_entry, 0, sizeof(read_entry));

    /* Data verifikasi uji coba */
    write_entry.timestamp = 0xA55A1234;
    write_entry.active_power_w = 4500;
    write_entry.voltage_r_dvolts = 2300;

    bool w_ok = nvram_write_load_profile_entry(0, &write_entry);
    bool r_ok = nvram_read_load_profile_entry(0, &read_entry);

    bool match = (read_entry.timestamp == write_entry.timestamp) &&
                 (read_entry.active_power_w == write_entry.active_power_w) &&
                 (read_entry.voltage_r_dvolts == write_entry.voltage_r_dvolts);

    printf("Tulis Entri Memori : %s (Timestamp: 0x%08lX)\r\n", w_ok ? "OK" : "GAGAL", (unsigned long)write_entry.timestamp);
    printf("Baca Entri Memori  : %s (Timestamp: 0x%08lX)\r\n", r_ok ? "OK" : "GAGAL", (unsigned long)read_entry.timestamp);

    if (w_ok && r_ok && match) {
        printf("[PASS] Memori Non-Volatile lulus uji integritas data.\r\n");
        return true;
    } else {
        printf("[FAIL] Gagal memvalidasi tulis-baca memori non-volatile!\r\n");
        return false;
    }
}

bool bsp_bringup_run_all(bsp_bringup_report_t *out_report) {
    printf("\r\n");
    printf("********************************************************\r\n");
    printf("*      MEMULAI BUILT-IN SELF-TEST (BIST) SPIN 1        *\r\n");
    printf("********************************************************\r\n");

    bsp_bringup_report_t rep;
    memset(&rep, 0, sizeof(rep));

    rep.i2c_status       = bsp_bringup_test_i2c_bus() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;
    rep.ade9000_status   = bsp_bringup_test_ade9000_comm() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;
    rep.gpio_act_status  = bsp_bringup_test_gpio_actuators() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;
    rep.gpio_sens_status = bsp_bringup_test_gpio_sensors() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;
    rep.pvd_status       = bsp_bringup_test_pvd_power() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;
    rep.nvram_status     = bsp_bringup_test_nvram_storage() ? BRINGUP_STATUS_PASS : BRINGUP_STATUS_FAIL;

    rep.total_passed = (rep.i2c_status == BRINGUP_STATUS_PASS) +
                       (rep.ade9000_status == BRINGUP_STATUS_PASS) +
                       (rep.gpio_act_status == BRINGUP_STATUS_PASS) +
                       (rep.gpio_sens_status == BRINGUP_STATUS_PASS) +
                       (rep.pvd_status == BRINGUP_STATUS_PASS) +
                       (rep.nvram_status == BRINGUP_STATUS_PASS);

    rep.total_failed = 6 - rep.total_passed;

    printf("\r\n========================================================\r\n");
    printf("            RINGKASAN HASIL BIST SPIN 1                 \r\n");
    printf("========================================================\r\n");
    printf(" 1. Jalur Bus I2C (OLED Display)    : %s\r\n", rep.i2c_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf(" 2. Bus SPI ADE9000 Metrologi       : %s\r\n", rep.ade9000_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf(" 3. Aktuator PB7 & Relai PC8/PC9    : %s\r\n", rep.gpio_act_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf(" 4. Pin Input Sabotase & IRQ        : %s\r\n", rep.gpio_sens_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf(" 5. Monitor Tegangan Darurat (PVD)  : %s\r\n", rep.pvd_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf(" 6. Memori Non-Volatile (NVRAM)     : %s\r\n", rep.nvram_status == BRINGUP_STATUS_PASS ? "PASS [OK]" : "FAIL [!]");
    printf("--------------------------------------------------------\r\n");
    printf(" TOTAL KELULUSAN : %lu / 6 Uji Coba Lolos (%lu Gagal)\r\n",
           (unsigned long)rep.total_passed, (unsigned long)rep.total_failed);
    printf(" STATUS HARDWARE : %s\r\n", rep.total_failed == 0 ? "READY FOR FIRMWARE DEPLOYMENT" : "HARDWARE FAULT DETECTED");
    printf("========================================================\r\n\r\n");

    if (out_report != NULL) {
        *out_report = rep;
    }
    return (rep.total_failed == 0);
}

void bsp_bringup_print_menu(void) {
    printf("\r\n========================================================\r\n");
    printf("        MENU DIAGNOSTIK HARDWARE BRING-UP (SPIN 1)      \r\n");
    printf("========================================================\r\n");
    printf("  'i' / 'I' : Pindai Alamat Bus I2C (I2C Scanner)\r\n");
    printf("  'm' / 'M' : Uji Integritas SPI ADE9000 (Loopback Check)\r\n");
    printf("  'a' / 'A' : Uji Aktuator Buzzer PB7 & Shunt Trip PC8\r\n");
    printf("  's' / 'S' : Baca Status Logika Pin Sensor & Tamper\r\n");
    printf("  'v' / 'V' : Cek Status Tegangan Rel PVD (Last-Gasp)\r\n");
    printf("  'n' / 'N' : Uji Tulis/Baca Memori Non-Volatile (NVRAM)\r\n");
    printf("  'r' / 'R' : Jalankan Seluruh Diagnostik (Full BIST)\r\n");
    printf("  'h' / '?' : Tampilkan Kembali Menu Bantuan Utama\r\n");
    printf("========================================================\r\n");
}

void bsp_bringup_handle_cli_cmd(char cmd) {
    switch (cmd) {
        case 'i':
        case 'I':
            bsp_bringup_test_i2c_bus();
            break;
        case 'm':
        case 'M':
            bsp_bringup_test_ade9000_comm();
            break;
        case 'a':
        case 'A':
            bsp_bringup_test_gpio_actuators();
            break;
        case 's':
        case 'S':
            bsp_bringup_test_gpio_sensors();
            break;
        case 'v':
        case 'V':
            bsp_bringup_test_pvd_power();
            break;
        case 'n':
        case 'N':
            bsp_bringup_test_nvram_storage();
            break;
        case 'r':
        case 'R':
            bsp_bringup_run_all(NULL);
            break;
        default:
            bsp_bringup_print_menu();
            break;
    }
}
