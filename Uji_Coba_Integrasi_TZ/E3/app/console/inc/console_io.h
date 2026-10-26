/**
 * @file console_io.h
 * @brief Modul Abstraksi Konsol Universal & Retargeting printf (E3/Application Lead)
 * 
 * Modul ini menyediakan antarmuka I/O konsol yang 100% agnostik terhadap hardware.
 * Dapat dihubungkan ke UART port mana pun (USART1, USART2, LPUART1, RS-485, dsb.),
 * mikrokontroler mana pun (STM32, ESP32, NXP), atau simulator host PC.
 */

#ifndef CONSOLE_IO_H
#define CONSOLE_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Prototipe fungsi callback pengiriman data ke antarmuka fisik (UART/RS485/USB)
 * @param data Pointer ke buffer data yang akan dikirim
 * @param len Panjang data dalam byte
 */
typedef void (*console_tx_fn_t)(const uint8_t *data, size_t len);

/**
 * @brief Inisialisasi modul konsol dengan mendaftarkan driver pengiriman fisik.
 * @param tx_func Fungsi callback untuk mentransmisikan data ke hardware.
 */
void console_io_init(console_tx_fn_t tx_func);

/**
 * @brief Mengatur status mute pada konsol (Fitur Pengaman DLMS E2).
 *        Saat true, output printf akan ditahan/diabaikan agar tidak merusak paket biner DLMS.
 * @param mute true untuk mematikan output konsol, false untuk mengaktifkan kembali.
 */
void console_io_set_mute(bool mute);

/**
 * @brief Memeriksa apakah konsol saat ini dalam status dibungkam (muted).
 * @return true jika konsol dibungkam, false jika aktif.
 */
bool console_io_is_muted(void);

/**
 * @brief Mencetak banner hardware bring-up ke konsol
 * @param system_title Judul sistem / MCU (NULL untuk default)
 * @param uart_desc Deskripsi port UART (NULL untuk default)
 * @param sysclk_mhz Frekuensi core clock sistem dalam MHz (misal 160)
 */
void console_io_print_boot_banner(const char *system_title, const char *uart_desc, uint32_t sysclk_mhz);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_IO_H */

