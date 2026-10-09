/**
 * @file test_spln_display_tamper.c
 * @brief Unit Test untuk Tampilan Standar Layar PLN (Gambar 4) dan Alarm Sabotase
 *        Sesuai Standar SPLN D3.006: 2021 + SUP-1: 2022 + SUP-2: 2023
 *        - Tabel 4: Respons meter terhadap alarm internal / hardware (ERR00..ERR07)
 *        - Tabel 6: Respons meter terhadap ketidaknormalan dan tampering (RUSAK, PERIKSA ERR20..ERR27, REVERSE ERR26)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "display.h"
#include "tamper_manager.h"

int main(void) {
    printf("====================================================================\n");
    printf("  TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & KODE ERROR SPLN D3.006   \n");
    printf("====================================================================\n\n");

    display_context_t ctx;
    bool ok = display_init(&ctx, "530000000001", 2000);
    assert(ok);

    display_spln_frame_t frame;

    /* --------------------------------------------------------------------- */
    /* UJI 1: KONDISI NORMAL (Tidak Ada Sabotase / Alarm)                   */
    /* --------------------------------------------------------------------- */
    printf("[UJI 1] Verifikasi Tampilan Normal (alarm_icon_active = false)...\n");
    ctx.alarm_icon_active = false;
    ctx.current_page = DISP_PAGE_IDPEL;

    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    assert(strcmp(frame.scroll_index_zz, "01") == 0);
    assert(strcmp(frame.main_value, "530000000001") == 0);
    assert(strcmp(frame.obis_code, "96.01") == 0);
    printf("  > Page 01 (IDPEL): OK | zz: %s | Val: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.obis_code);

    /* Cek Halaman Tegangan R */
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    ctx.meas_buffer.voltage_r_dvolts = 2300; /* 230.0 V */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "02") == 0);
    assert(strcmp(frame.main_value, "230.0") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(strcmp(frame.obis_code, "32.07") == 0);
    assert(frame.is_large_font == true);
    printf("  > Page 02 (Volt R): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Carousel Wrap saat Normal: Dari Page 14 (Energy) harus langsung ke Page 01 (IDPEL) */
    ctx.current_page = DISP_PAGE_ACTIVE_ENERGY;
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_IDPEL);
    printf("  > Carousel Skip Test (Normal tidak menampilkan halaman sabotase): [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 2: KONDISI ALARM SABOTASE AKTIF (Default / Tombol PC13 Ditekan)   */
    /* --------------------------------------------------------------------- */
    printf("[UJI 2] Verifikasi Tampilan Alarm Sabotase Case Open (Tombol PC13)...\n");
    ctx.alarm_icon_active = true;
    ctx.active_tamper_mask = TAMPER_VECTOR_CASE_OPEN;

    /* Cek banner alarm pada halaman pengukuran normal */
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("  > Banner Alarm Tersemat di Page 02: Respon Alarm = '%s' [PASS]\n", frame.alarm_code);

    /* Cek Halaman Khusus Darurat Sabotase (DISP_PAGE_TAMPER_ALARM) - SPLN Tabel 6 No 1 */
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.scroll_index_zz, "AL") == 0);
    assert(strcmp(frame.main_value, "RUSAK") == 0);
    assert(strcmp(frame.unit, "-") == 0);
    assert(strcmp(frame.obis_code, "96.50") == 0);
    assert(frame.is_large_font == false);
    printf("  > SPLN Tabel 6 No 1 (Case Open): Header: '%s' | zz: '%s' | Teks: '%s' | Kode: '%s' | OBIS: '%s' [PASS]\n",
           frame.alarm_code, frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Siklus Carousel saat Alarm: Dari Page 14 (Energy) harus masuk ke DISP_PAGE_TAMPER_ALARM */
    ctx.current_page = DISP_PAGE_ACTIVE_ENERGY;
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_TAMPER_ALARM);
    printf("  > Carousel Inclusion Test (Halaman sabotase masuk dalam siklus carousel): [PASS]\n");

    /* Dari DISP_PAGE_TAMPER_ALARM lanjut ke DISP_PAGE_IDPEL */
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_IDPEL);
    printf("  > Carousel Loop Completion Test (Kembali ke Page 01): [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 3: PEMULIHAN SABOTASE (Kembali Normal)                            */
    /* --------------------------------------------------------------------- */
    printf("[UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)...\n");
    display_set_tamper_status(&ctx, 0);
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    printf("  > Status Layar Berhasil Pulih ke OK [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 4: VERIFIKASI LENGKAP 9 VEKTOR SABOTASE SPLN D3.006 TABEL 6       */
    /* --------------------------------------------------------------------- */
    printf("[UJI 4] Verifikasi Seluruh 9 Vektor Sabotase SPLN D3.006: 2021 Tabel 6...\n");
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;

    /* 1. Case Open -> RUSAK, Kode: - */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_CASE_OPEN);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "RUSAK") == 0);
    assert(strcmp(frame.unit, "-") == 0);
    printf("  > 4.1 Case Open: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 2. Terminal Cover Open -> PERIKSA, Kode: ERR20 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR20") == 0);
    printf("  > 4.2 Terminal Open: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 3. Wrong Phase Sequence -> PERIKSA, Kode: ERR21 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_WRONG_SEQUENCE);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR21") == 0);
    printf("  > 4.3 Wrong Sequence: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 4. Cross Phase Current/Voltage -> PERIKSA, Kode: ERR22 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_CROSS_PHASE);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR22") == 0);
    printf("  > 4.4 Cross Phase: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 5. Neutral Missing / Broken -> PERIKSA, Kode: ERR23 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_NEUTRAL_BYPASS);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR23") == 0);
    printf("  > 4.5 Neutral Missing: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 6. Missing 1/2 Phase Voltage -> PERIKSA, Kode: ERR24 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_PHASE_LOSS);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR24") == 0);
    printf("  > 4.6 Voltage Loss: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 7. External Magnetic Field Induction -> PERIKSA, Kode: ERR25 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_MAGNETIC_FIELD);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR25") == 0);
    printf("  > 4.7 Magnetic Field: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 8. Reverse Power -> REVERSE, Kode: ERR26 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "REVERSE") == 0);
    assert(strcmp(frame.unit, "ERR26") == 0);
    printf("  > 4.8 Reverse Power: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 9. Quadrant 4 PF < 0.85 -> PERIKSA, Kode: ERR27 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_LOW_PF_QUAD4);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR27") == 0);
    printf("  > 4.9 Quad 4 (Low PF): Teks='%s' | Kode='%s' [PASS]\n\n", frame.main_value, frame.unit);

    /* --------------------------------------------------------------------- */
    /* UJI 5: VERIFIKASI LENGKAP 8 ALARM INTERNAL SPLN D3.006 TABEL 4        */
    /* --------------------------------------------------------------------- */
    printf("[UJI 5] Verifikasi Seluruh 8 Alarm Hardware Internal SPLN D3.006: 2021 Tabel 4...\n");
    display_set_tamper_status(&ctx, 0);

    /* 0. Flash Memory Rusak -> Teks: -, Kode: ERR00 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_FLASH_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR00") == 0);
    printf("  > 5.0 Flash Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 1. RAM Rusak -> Teks: -, Kode: ERR01 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RAM_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR01") == 0);
    printf("  > 5.1 RAM Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 2. Clock Loss / RTC -> Teks: -, Kode: ERR02 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RTC_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR02") == 0);
    printf("  > 5.2 Clock Loss RTC: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 3. Low Battery -> Teks: -, Kode: ERR03 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_LOW_BATTERY);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR03") == 0);
    printf("  > 5.3 Low Battery: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 4. MCU Error -> Teks: -, Kode: ERR04 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_MCU_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR04") == 0);
    printf("  > 5.4 MCU Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 5. ADC Sampling Error -> Teks: -, Kode: ERR05 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_ADC_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR05") == 0);
    printf("  > 5.5 ADC Sampling: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 6. Supercapacitor Fail -> Teks: -, Kode: ERR06 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_SUPERCAP_FAIL);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR06") == 0);
    printf("  > 5.6 Supercap Fail: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 7. Relay Fail -> Teks: -, Kode: ERR07 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RELAY_FAIL);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR07") == 0);
    printf("  > 5.7 Relay Fail: Teks='%s' | Kode='%s' [PASS]\n\n", frame.main_value, frame.unit);

    /* --------------------------------------------------------------------- */
    /* UJI 6: VERIFIKASI FORMAT TAMPILAN ASCII CLI (display_render_frame)    */
    /* --------------------------------------------------------------------- */
    printf("[UJI 6] Verifikasi Format Tampilan ASCII CLI (display_render_frame)...\n");
    char l1[64], l2[64], l3[64];

    display_set_tamper_status(&ctx, TAMPER_VECTOR_CASE_OPEN);
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "STATUS SABOTASE") == 0);
    assert(strcmp(l3, "RUSAK") == 0);
    printf("  > Case Open CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN);
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l3, "PERIKSA ERR20") == 0);
    printf("  > Terminal Open CLI Frame: L3='%s' [PASS]\n", l3);

    display_set_tamper_status(&ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l3, "REVERSE ERR26") == 0);
    printf("  > Reverse Power CLI Frame: L3='%s' [PASS]\n\n");

    printf("====================================================================\n");
    printf("  HASIL: 100%% SELURUH KODE ERROR SPLN D3.006 TABEL 4 & 6 LULUS!      \n");
    printf("====================================================================\n");

    return 0;
}
