/**
 * @file bsp_bringup_test.h
 * @brief Diagnostic & Smoke-Test Shell untuk Hardware Bring-Up PCB Spin 1
 * 
 * Modul ini menyediakan suite pengujian mandiri (Built-In Self-Test / BIST)
 * untuk memverifikasi fungsionalitas elektrikal pada hari pertama PCB Spin 1 tiba:
 * 1. Pemindaian Jalur I2C (OLED SSD1306, EEPROM, RTC)
 * 2. Integritas Bus SPI Metrologi ADE9000 (Read/Write Loopback Test)
 * 3. Aktuasi Aktuator GPIO (Buzzer Alarm PB7 & Relai Latching PC8/PC9)
 * 4. Pemantauan Pin Sensor & Sabotase (Microswitch PC13, ADE9000 IRQ0/1)
 * 5. Verifikasi Detektor Tegangan Darurat PVD (Power-Fail Last-Gasp 2.9V)
 * 6. Validasi Penyimpanan Permanen Non-Volatile (Flash / FRAM)
 */

#ifndef BSP_BRINGUP_TEST_H
#define BSP_BRINGUP_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Hasil pengujian individual subsystem
 */
typedef enum {
    BRINGUP_STATUS_PASS = 0,    /**< Subsistem berfungsi normal */
    BRINGUP_STATUS_FAIL = 1,    /**< Subsistem gagal merespons atau error */
    BRINGUP_STATUS_WARN = 2,    /**< Subsistem merespons dengan anomali */
    BRINGUP_STATUS_NOT_TESTED = 3 /**< Belum diuji */
} bsp_bringup_status_t;

/**
 * @brief Laporan ringkasan pengujian Built-In Self-Test (BIST)
 */
typedef struct {
    bsp_bringup_status_t i2c_status;      /**< Status I2C Bus & OLED */
    bsp_bringup_status_t ade9000_status;  /**< Status SPI ADE9000 */
    bsp_bringup_status_t gpio_act_status; /**< Status Buzzer & Relai */
    bsp_bringup_status_t gpio_sens_status;/**< Status Pin Tamper & IRQ */
    bsp_bringup_status_t pvd_status;      /**< Status PVD Power Monitor */
    bsp_bringup_status_t nvram_status;    /**< Status Memori Non-Volatile */
    uint32_t total_passed;                /**< Total sub-pengujian yang lulus */
    uint32_t total_failed;                /**< Total sub-pengujian yang gagal */
} bsp_bringup_report_t;

/**
 * @brief Inisialisasi modul diagnostic bring-up
 */
void bsp_bringup_init(void);

/**
 * @brief Pengujian 1: Pindai seluruh alamat I2C 7-bit (0x08 s.d. 0x77)
 * Menampilkan peta alamat perangkat yang merespons ACK.
 * @return true jika ditemukan minimal 1 perangkat I2C (misal OLED SSD1306 di 0x3C)
 */
bool bsp_bringup_test_i2c_bus(void);

/**
 * @brief Pengujian 2: Uji integritas bus SPI ADE9000
 * Melakukan Read-After-Write pada register konfigurasi MASK0 untuk memastikan SPI bekerja.
 * @return true jika komunikasi SPI ADE9000 valid tanpa CRC error
 */
bool bsp_bringup_test_ade9000_comm(void);

/**
 * @brief Pengujian 3: Uji aktuator GPIO (Buzzer Alarm PB7 & Relai PC8/PC9)
 * Menghasilkan pulsa bunyi buzzer dan menguji aktivasi pin relai.
 * @return true jika aktuator berhasil dipicu
 */
bool bsp_bringup_test_gpio_actuators(void);

/**
 * @brief Pengujian 4: Membaca status logika real-time dari pin input sensor
 * (PC13 Case Open, PB0 ADE9000 IRQ0, PB1 ADE9000 IRQ1).
 * @return true jika semua pin input terbaca dalam rentang logika valid
 */
bool bsp_bringup_test_gpio_sensors(void);

/**
 * @brief Pengujian 5: Memeriksa kesiapan Programmable Voltage Detector (PVD)
 * Memverifikasi threshold deteksi pemadaman listrik (2.9V).
 * @return true jika PVD aktif dan siap memicu Last-Gasp
 */
bool bsp_bringup_test_pvd_power(void);

/**
 * @brief Pengujian 6: Uji baca-tulis memori non-volatile (Flash / FRAM)
 * Menulis data verifikasi magic word ke partisi log dan membacanya kembali.
 * @return true jika data yang dibaca 100% identik dengan yang ditulis
 */
bool bsp_bringup_test_nvram_storage(void);

/**
 * @brief Menjalankan seluruh paket pengujian (BIST) dan mencetak laporan ke UART
 * @param out_report Pointer untuk menampung hasil struktur laporan
 * @return true jika semua subsistem PASS, false jika ada kegagalan
 */
bool bsp_bringup_run_all(bsp_bringup_report_t *out_report);

/**
 * @brief Menampilkan menu interaktif Bring-Up Shell ke konsol UART
 */
void bsp_bringup_print_menu(void);

/**
 * @brief Memproses karakter input dari terminal serial ST-LINK VCP
 * @param cmd Karakter perintah (misal: 'i', 'm', 'a', 's', 'v', 'n', 'r')
 */
void bsp_bringup_handle_cli_cmd(char cmd);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BRINGUP_TEST_H */

