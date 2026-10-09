/**
 * @file display.c
 * @brief Implementasi Modul Tampilan LCD & Carousel UI (E3/ENG-3)
 */

#include "display.h"
#include <stdio.h>
#include <string.h>

bool display_init(display_context_t *ctx, const char *customer_id, uint32_t carousel_interval_ms) {
    if (ctx == NULL) return false;
    memset(ctx, 0, sizeof(display_context_t));
    
    ctx->mode = DISPLAY_MODE_AUTO_CAROUSEL;
    ctx->current_page = DISP_PAGE_IDPEL;
    ctx->carousel_interval_ms = (carousel_interval_ms > 0) ? carousel_interval_ms : 5000;
    ctx->manual_timeout_ms = 10000; // Timeout manual scroll 10 detik

    if (customer_id != NULL) {
        strncpy(ctx->customer_id, customer_id, sizeof(ctx->customer_id) - 1);
    } else {
        strncpy(ctx->customer_id, "12345678901", sizeof(ctx->customer_id) - 1);
    }
    ctx->customer_id[sizeof(ctx->customer_id)-1]='\0';
    return true;
}

void display_update_measurements(display_context_t *ctx, const meter_measurements_t *meas) {
    if (ctx != NULL && meas != NULL) {
        memcpy(&ctx->meas_buffer, meas, sizeof(meter_measurements_t));
    }
}

void display_process_tick(display_context_t *ctx, uint32_t elapsed_ms) {
    if (ctx == NULL) return;

    ctx->timer_accumulator_ms += elapsed_ms;

    if (ctx->mode == DISPLAY_MODE_AUTO_CAROUSEL) {
        if (ctx->timer_accumulator_ms >= ctx->carousel_interval_ms) {
            ctx->timer_accumulator_ms = 0;
            display_page_id_t next_page = (display_page_id_t)((ctx->current_page + 1) % DISP_PAGE_COUNT);
            if (!ctx->alarm_icon_active && next_page == DISP_PAGE_TAMPER_ALARM) {
                next_page = DISP_PAGE_IDPEL;
            }
            ctx->current_page = next_page;
        }
    } else if (ctx->mode == DISPLAY_MODE_MANUAL_SCROLL) {
        if (ctx->timer_accumulator_ms >= ctx->manual_timeout_ms) {
            ctx->timer_accumulator_ms = 0;
            ctx->mode = DISPLAY_MODE_AUTO_CAROUSEL;
            ctx->current_page = DISP_PAGE_IDPEL;
        }
    }
}

void display_handle_button_press(display_context_t *ctx, button_dir_t dir) {
    if (ctx == NULL) return;

    ctx->mode = DISPLAY_MODE_MANUAL_SCROLL;
    ctx->timer_accumulator_ms = 0;

    if (dir == BUTTON_DIR_DOWN) {
        display_page_id_t next_page = (display_page_id_t)((ctx->current_page + 1) % DISP_PAGE_COUNT);
        if (!ctx->alarm_icon_active && next_page == DISP_PAGE_TAMPER_ALARM) {
            next_page = DISP_PAGE_IDPEL;
        }
        ctx->current_page = next_page;
    } else if (dir == BUTTON_DIR_UP) {
        display_page_id_t prev_page = (ctx->current_page == 0) ? (display_page_id_t)(DISP_PAGE_COUNT - 1) 
                                                               : (display_page_id_t)(ctx->current_page - 1);
        if (!ctx->alarm_icon_active && prev_page == DISP_PAGE_TAMPER_ALARM) {
            prev_page = DISP_PAGE_ACTIVE_ENERGY;
        }
        ctx->current_page = prev_page;
    }
}

void display_set_tamper_status(display_context_t *ctx, uint32_t active_tamper_mask) {
    if (ctx == NULL) return;
    ctx->active_tamper_mask = active_tamper_mask;
    ctx->alarm_icon_active = (active_tamper_mask != 0 || ctx->active_alarm_mask != 0);
}

void display_set_internal_alarm(display_context_t *ctx, uint32_t active_alarm_mask) {
    if (ctx == NULL) return;
    ctx->active_alarm_mask = active_alarm_mask;
    ctx->alarm_icon_active = (ctx->active_tamper_mask != 0 || active_alarm_mask != 0);
}

void display_get_spln_tamper_info(uint32_t tamper_mask, char *out_text, size_t text_sz, char *out_code, size_t code_sz) {
    if (out_text == NULL || text_sz == 0 || out_code == NULL || code_sz == 0) return;

    /* Sesuai SPLN D3.006: 2021 Tabel 6 (Respons meter terhadap tampering):
     * 1. Tutup meter dibuka: Teks = "RUSAK", Kode = "-"
     * 2. Tutup terminal dibuka: Teks = "PERIKSA", Kode = "ERR20"
     * 3. Urutan fase terbalik: Teks = "PERIKSA", Kode = "ERR21"
     * 4. Arus dan tegangan tidak sefase: Teks = "PERIKSA", Kode = "ERR22"
     * 5. Kawat netral putus: Teks = "PERIKSA", Kode = "ERR23"
     * 6. Hilang tegangan 1/2 fase: Teks = "PERIKSA", Kode = "ERR24"
     * 7. Induksi medan magnet: Teks = "PERIKSA", Kode = "ERR25"
     * 8. Reverse power: Teks = "REVERSE", Kode = "ERR26"
     * 9. Kuadran 4 / PF < 0.85: Teks = "PERIKSA", Kode = "ERR27"
     */
    if (tamper_mask & TAMPER_VECTOR_CASE_OPEN || tamper_mask == 0) {
        snprintf(out_text, text_sz, "RUSAK");
        snprintf(out_code, code_sz, "-");
    } else if (tamper_mask & TAMPER_VECTOR_REVERSE_POWER) {
        snprintf(out_text, text_sz, "REVERSE");
        snprintf(out_code, code_sz, "ERR26");
    } else if (tamper_mask & TAMPER_VECTOR_TERMINAL_OPEN) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR20");
    } else if (tamper_mask & TAMPER_VECTOR_WRONG_SEQUENCE) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR21");
    } else if (tamper_mask & TAMPER_VECTOR_CROSS_PHASE) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR22");
    } else if (tamper_mask & TAMPER_VECTOR_NEUTRAL_BYPASS) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR23");
    } else if (tamper_mask & TAMPER_VECTOR_PHASE_LOSS) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR24");
    } else if (tamper_mask & TAMPER_VECTOR_MAGNETIC_FIELD) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR25");
    } else if (tamper_mask & TAMPER_VECTOR_LOW_PF_QUAD4) {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "ERR27");
    } else {
        snprintf(out_text, text_sz, "PERIKSA");
        snprintf(out_code, code_sz, "-");
    }
}

void display_get_spln_alarm_info(uint32_t alarm_mask, char *out_text, size_t text_sz, char *out_code, size_t code_sz) {
    if (out_text == NULL || text_sz == 0 || out_code == NULL || code_sz == 0) return;

    /* Sesuai SPLN D3.006: 2021 Tabel 4 (Respons meter terhadap alarm internal):
     * Teks pada Tabel 4 adalah "-" dan Kodenya ERR00..ERR07 (berkedip) */
    snprintf(out_text, text_sz, "-");

    if (alarm_mask & SPLN_ALARM_FLASH_ERROR) {
        snprintf(out_code, code_sz, "ERR00");
    } else if (alarm_mask & SPLN_ALARM_RAM_ERROR) {
        snprintf(out_code, code_sz, "ERR01");
    } else if (alarm_mask & SPLN_ALARM_RTC_ERROR) {
        snprintf(out_code, code_sz, "ERR02");
    } else if (alarm_mask & SPLN_ALARM_LOW_BATTERY) {
        snprintf(out_code, code_sz, "ERR03");
    } else if (alarm_mask & SPLN_ALARM_MCU_ERROR) {
        snprintf(out_code, code_sz, "ERR04");
    } else if (alarm_mask & SPLN_ALARM_ADC_ERROR) {
        snprintf(out_code, code_sz, "ERR05");
    } else if (alarm_mask & SPLN_ALARM_SUPERCAP_FAIL) {
        snprintf(out_code, code_sz, "ERR06");
    } else if (alarm_mask & SPLN_ALARM_RELAY_FAIL) {
        snprintf(out_code, code_sz, "ERR07");
    } else {
        snprintf(out_code, code_sz, "-");
    }
}

void display_render_frame(const display_context_t *ctx, char *out_line1, char *out_line2, char *out_line3, size_t max_len) {
    if (ctx == NULL || out_line1 == NULL || out_line2 == NULL || out_line3 == NULL) return;

    snprintf(out_line1, max_len, "[%s] %s", 
             (ctx->mode == DISPLAY_MODE_AUTO_CAROUSEL) ? "AUTO" : "MANUAL",
             ctx->alarm_icon_active ? "(!)" : " OK ");

    switch (ctx->current_page) {
        case DISP_PAGE_IDPEL:
            snprintf(out_line2, max_len, "IDPEL:");
            snprintf(out_line3, max_len, "%s", ctx->customer_id);
            break;
        case DISP_PAGE_VOLTAGE_R:
            snprintf(out_line2, max_len, "VOLTAGE PHASE R");
            snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(ctx->meas_buffer.voltage_r_dvolts / 10), (unsigned long)(ctx->meas_buffer.voltage_r_dvolts % 10));
            break;
        case DISP_PAGE_VOLTAGE_S:
            snprintf(out_line2, max_len, "VOLTAGE PHASE S");
            snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(ctx->meas_buffer.voltage_s_dvolts / 10), (unsigned long)(ctx->meas_buffer.voltage_s_dvolts % 10));
            break;
        case DISP_PAGE_VOLTAGE_T:
            snprintf(out_line2, max_len, "VOLTAGE PHASE T");
            snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(ctx->meas_buffer.voltage_t_dvolts / 10), (unsigned long)(ctx->meas_buffer.voltage_t_dvolts % 10));
            break;
        case DISP_PAGE_CURRENT_R:
            snprintf(out_line2, max_len, "CURRENT PHASE R");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_r_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_S:
            snprintf(out_line2, max_len, "CURRENT PHASE S");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_s_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_s_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_T:
            snprintf(out_line2, max_len, "CURRENT PHASE T");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_t_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_t_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_N:
            snprintf(out_line2, max_len, "CURRENT NEUTRAL");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_n_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_n_mamps % 1000));
            break;
        case DISP_PAGE_ACTIVE_POWER: {
            int32_t pw = ctx->meas_buffer.active_power_w;
            uint32_t abs_pw = (pw < 0) ? (uint32_t)(-pw) : (uint32_t)pw;
            snprintf(out_line2, max_len, "ACTIVE POWER");
            snprintf(out_line3, max_len, "%s%lu.%03lu kW", (pw < 0) ? "-" : "", (unsigned long)(abs_pw / 1000), (unsigned long)(abs_pw % 1000));
            break;
        }
        case DISP_PAGE_REACTIVE_POWER: {
            int32_t pvar = ctx->meas_buffer.reactive_power_var;
            uint32_t abs_pvar = (pvar < 0) ? (uint32_t)(-pvar) : (uint32_t)pvar;
            snprintf(out_line2, max_len, "REACTIVE POWER");
            snprintf(out_line3, max_len, "%s%lu.%03lu kvar", (pvar < 0) ? "-" : "", (unsigned long)(abs_pvar / 1000), (unsigned long)(abs_pvar % 1000));
            break;
        }
        case DISP_PAGE_APPARENT_POWER:
            snprintf(out_line2, max_len, "APPARENT POWER");
            snprintf(out_line3, max_len, "%lu.%03lu kVA", (unsigned long)(ctx->meas_buffer.apparent_power_va / 1000), (unsigned long)(ctx->meas_buffer.apparent_power_va % 1000));
            break;
        case DISP_PAGE_POWER_FACTOR:
            snprintf(out_line2, max_len, "POWER FACTOR");
            snprintf(out_line3, max_len, "%lu.%03lu", (unsigned long)(ctx->meas_buffer.power_factor_ppm / 1000), (unsigned long)(ctx->meas_buffer.power_factor_ppm % 1000));
            break;
        case DISP_PAGE_FREQUENCY:
            snprintf(out_line2, max_len, "GRID FREQUENCY");
            snprintf(out_line3, max_len, "%lu.%02lu Hz", (unsigned long)(ctx->meas_buffer.frequency_mhz / 1000), (unsigned long)((ctx->meas_buffer.frequency_mhz % 1000) / 10));
            break;
        case DISP_PAGE_ACTIVE_ENERGY:
            snprintf(out_line2, max_len, "TOTAL ENERGY");
            snprintf(out_line3, max_len, "%lu.%02lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_wh / 1000), (unsigned long)((ctx->meas_buffer.active_energy_wh % 1000) / 10));
            break;
        case DISP_PAGE_TAMPER_ALARM: {
            char spln_text[16] = "RUSAK";
            char spln_code[10] = "-";
            if (ctx->active_tamper_mask != 0 || (ctx->active_tamper_mask == 0 && ctx->active_alarm_mask == 0)) {
                display_get_spln_tamper_info(ctx->active_tamper_mask, spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));
            } else {
                display_get_spln_alarm_info(ctx->active_alarm_mask, spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));
            }
            snprintf(out_line2, max_len, "STATUS SABOTASE");
            if (spln_code[0] != '-' && spln_code[0] != '\0') {
                snprintf(out_line3, max_len, "%s %s", spln_text, spln_code);
            } else {
                snprintf(out_line3, max_len, "%s", spln_text);
            }
            break;
        }
        default:
            snprintf(out_line2, max_len, "DISPLAY PAGE %d", ctx->current_page);
            snprintf(out_line3, max_len, "---");
            break;
    }
}

void display_render_spln_frame(const display_context_t *ctx, display_spln_frame_t *frame) {
    if (ctx == NULL || frame == NULL) return;
    memset(frame, 0, sizeof(display_spln_frame_t));

    const char *obis_code = "00.00";
    frame->is_large_font = true;
    frame->is_alarm_active = ctx->alarm_icon_active;

    if (ctx->alarm_icon_active) {
        snprintf(frame->alarm_code, sizeof(frame->alarm_code), "!ALM");
    } else {
        snprintf(frame->alarm_code, sizeof(frame->alarm_code), "OK");
    }

    switch (ctx->current_page) {
        case DISP_PAGE_IDPEL:
            obis_code = "96.01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "01");
            snprintf(frame->main_value, sizeof(frame->main_value), "%s", ctx->customer_id);
            frame->unit[0] = '\0';
            frame->is_large_font = false; /* 12 digit IDPEL menggunakan Font 7x10 */
            break;
        case DISP_PAGE_VOLTAGE_R:
            obis_code = "32.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "02");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                     (unsigned long)(ctx->meas_buffer.voltage_r_dvolts / 10),
                     (unsigned long)(ctx->meas_buffer.voltage_r_dvolts % 10));
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        case DISP_PAGE_VOLTAGE_S:
            obis_code = "52.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "03");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                     (unsigned long)(ctx->meas_buffer.voltage_s_dvolts / 10),
                     (unsigned long)(ctx->meas_buffer.voltage_s_dvolts % 10));
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        case DISP_PAGE_VOLTAGE_T:
            obis_code = "72.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "04");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                     (unsigned long)(ctx->meas_buffer.voltage_t_dvolts / 10),
                     (unsigned long)(ctx->meas_buffer.voltage_t_dvolts % 10));
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        case DISP_PAGE_CURRENT_R:
            obis_code = "31.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "05");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000),
                     (unsigned long)((ctx->meas_buffer.current_r_mamps % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_S:
            obis_code = "51.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "06");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.current_s_mamps / 1000),
                     (unsigned long)((ctx->meas_buffer.current_s_mamps % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_T:
            obis_code = "71.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "07");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.current_t_mamps / 1000),
                     (unsigned long)((ctx->meas_buffer.current_t_mamps % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_N:
            obis_code = "91.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "08");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.current_n_mamps / 1000),
                     (unsigned long)((ctx->meas_buffer.current_n_mamps % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_ACTIVE_POWER: {
            obis_code = "01.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "09");
            int32_t pw = ctx->meas_buffer.active_power_w;
            uint32_t abs_pw = (pw < 0) ? (uint32_t)(-pw) : (uint32_t)pw;
            snprintf(frame->main_value, sizeof(frame->main_value), "%s%lu.%03lu",
                     (pw < 0) ? "-" : "",
                     (unsigned long)(abs_pw / 1000),
                     (unsigned long)(abs_pw % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        }
        case DISP_PAGE_REACTIVE_POWER: {
            obis_code = "03.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "10");
            int32_t pvar = ctx->meas_buffer.reactive_power_var;
            uint32_t abs_pvar = (pvar < 0) ? (uint32_t)(-pvar) : (uint32_t)pvar;
            snprintf(frame->main_value, sizeof(frame->main_value), "%s%lu.%03lu",
                     (pvar < 0) ? "-" : "",
                     (unsigned long)(abs_pvar / 1000),
                     (unsigned long)(abs_pvar % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kvar");
            break;
        }
        case DISP_PAGE_APPARENT_POWER:
            obis_code = "09.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "11");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.apparent_power_va / 1000),
                     (unsigned long)(ctx->meas_buffer.apparent_power_va % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kVA");
            break;
        case DISP_PAGE_POWER_FACTOR:
            obis_code = "13.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "12");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.power_factor_ppm / 1000),
                     (unsigned long)(ctx->meas_buffer.power_factor_ppm % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "PF");
            break;
        case DISP_PAGE_FREQUENCY:
            obis_code = "14.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "13");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.frequency_mhz / 1000),
                     (unsigned long)((ctx->meas_buffer.frequency_mhz % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "Hz");
            break;
        case DISP_PAGE_ACTIVE_ENERGY:
            obis_code = "01.08";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "14");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_wh / 1000),
                     (unsigned long)((ctx->meas_buffer.active_energy_wh % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_TAMPER_ALARM: {
            obis_code = "96.50";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "AL");
            frame->is_large_font = false;

            char spln_text[16] = "RUSAK";
            char spln_code[10] = "-";

            if (ctx->active_tamper_mask != 0 || (ctx->active_tamper_mask == 0 && ctx->active_alarm_mask == 0)) {
                display_get_spln_tamper_info(ctx->active_tamper_mask, spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));
            } else {
                display_get_spln_alarm_info(ctx->active_alarm_mask, spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));
            }

            snprintf(frame->main_value, sizeof(frame->main_value), "%s", spln_text);
            snprintf(frame->unit, sizeof(frame->unit), "%s", spln_code);
            break;
        }
        default:
            obis_code = "00.00";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "--");
            snprintf(frame->main_value, sizeof(frame->main_value), "---");
            frame->unit[0] = '\0';
            break;
    }

    /* Simpan obis_code untuk penataan pixel terpisah */
    snprintf(frame->obis_code, sizeof(frame->obis_code), "%s", obis_code);

    /* Format Baris 1: Simbol & Kode OBIS gabungan */
    snprintf(frame->header_symbols, sizeof(frame->header_symbols),
             "%s [B] L123 I123 %s",
             frame->alarm_code,
             obis_code);
}