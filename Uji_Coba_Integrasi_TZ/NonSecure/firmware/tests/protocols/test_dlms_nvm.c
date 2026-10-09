#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dlms_nvm.h"

/* Mock Buffer Simulasi EEPROM (1024 Byte) */
static uint8_t g_mock_eeprom[1024];

static dlms_result_t mock_eeprom_write(uint32_t addr, const uint8_t *data, size_t len) {
    if (!data || (addr + len) > sizeof(g_mock_eeprom)) return DLMS_ERR_NULL_PTR;
    memcpy(&g_mock_eeprom[addr], data, len);
    return DLMS_OK;
}

static dlms_result_t mock_eeprom_read(uint32_t addr, uint8_t *data, size_t len) {
    if (!data || (addr + len) > sizeof(g_mock_eeprom)) return DLMS_ERR_NULL_PTR;
    memcpy(data, &g_mock_eeprom[addr], len);
    return DLMS_OK;
}

int main(void) {
    printf("[TEST RUNNER] Running test_dlms_nvm...\n");

    /* 1. Inisialisasi & Tambah 3 Event Tamper */
    dlms_tamper_log_t log_save;
    assert(dlms_tamper_log_init(&log_save) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log_save, 1700000000, DLMS_TAMPER_TERMINAL_COVER_OPEN, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log_save, 1700000100, DLMS_TAMPER_MAGNETIC_INDUCTION, 1) == DLMS_OK);
    assert(dlms_tamper_log_add_event(&log_save, 1700000200, DLMS_TAMPER_REVERSE_CURRENT, 1) == DLMS_OK);

    /* 2. Simpan Log ke Mock EEPROM pada Alamat 0x0060 */
    assert(dlms_nvm_save_tamper_log(&log_save, mock_eeprom_write, 0x0060) == DLMS_OK);

    /* 3. Simulasi Power Loss: Clear RAM */
    dlms_tamper_log_t log_load;
    memset(&log_load, 0, sizeof(dlms_tamper_log_t));

    /* 4. Restore Log dari Mock EEPROM */
    assert(dlms_nvm_load_tamper_log(&log_load, mock_eeprom_read, 0x0060) == DLMS_OK);

    /* Verifikasi Kesesuaian Data yang Di-restore */
    assert(log_load.entries_in_use == 3);
    assert(log_load.total_events == 3);

    dlms_tamper_record_t rec;
    assert(dlms_tamper_log_get_entry(&log_load, 0, &rec) == DLMS_OK);
    assert(rec.event_code == DLMS_TAMPER_REVERSE_CURRENT);
    assert(rec.timestamp_epoch == 1700000200);

    /* 5. Simulasi Korupsi Data EEPROM (Ubah 1 byte data) */
    g_mock_eeprom[0x0060 + sizeof(dlms_nvm_header_t) + 2] ^= 0xFF;

    /* Pembacaan Harus Gagal Karena CRC16 Mismatch */
    assert(dlms_nvm_load_tamper_log(&log_load, mock_eeprom_read, 0x0060) == DLMS_ERR_INVALID_APDU);

    printf("[TEST SUCCESS] test_dlms_nvm 100%% PASSED!\n");
    return 0;
}