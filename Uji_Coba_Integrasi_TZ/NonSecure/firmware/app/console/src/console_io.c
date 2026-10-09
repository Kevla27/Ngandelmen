/**
 * @file console_io.c
 * @brief Implementasi Modul Abstraksi Konsol Universal & Retargeting printf (E3/Application Lead)
 * 
 * Mengimplementasikan syscall Newlib _write() secara agnostik tanpa hardcode
 * ke peripheral mikrokontroler tertentu.
 */

#include "console_io.h"
#include <stdio.h>

/* Pointer callback driver pengiriman hardware */
static console_tx_fn_t s_console_tx_driver = NULL;

/* Status proteksi mute konsol (misal saat transaksi biner DLMS berlangsung) */
static volatile bool s_console_is_muted = false;

void console_io_init(console_tx_fn_t tx_func) {
    s_console_tx_driver = tx_func;
    s_console_is_muted = false;
}

void console_io_set_mute(bool mute) {
    s_console_is_muted = mute;
}

bool console_io_is_muted(void) {
    return s_console_is_muted;
}

/**
 * @brief Fungsi syscall Newlib / GCC _write() untuk meretarget printf().
 * Linker secara otomatis memetakan printf() ke fungsi ini.
 */
int _write(int file, char *ptr, int len) {
    (void)file;

    /* 1. Jika konsol sedang dibungkam (misal mode DLMS), buang teks printf */
    if (s_console_is_muted) {
        return len;
    }

    /* 2. Jika driver hardware sudah didaftarkan dan data valid, kirimkan */
    if (s_console_tx_driver != NULL && ptr != NULL && len > 0) {
        s_console_tx_driver((const uint8_t *)ptr, (size_t)len);
    }

    return len;
}

void console_io_print_boot_banner(const char *system_title, const char *uart_desc, uint32_t sysclk_mhz) {
    printf("\n==================================================\n");
    printf("   %-44s   \n", system_title ? system_title : "SYSTEM HARDWARE BRING-UP INITIALIZED");
    printf("   Console UART: %-32s\n", uart_desc ? uart_desc : "USART2 @ 115200 bps");
    printf("   System Clock: %lu MHz | GPDMA1 ACTIVE          \n", (unsigned long)sysclk_mhz);
    printf("==================================================\n\n");
}

