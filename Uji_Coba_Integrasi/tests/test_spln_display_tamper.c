/**
 * @file test_spln_display_tamper.c
 * @brief Unit Test untuk Tampilan Standar Layar PLN (Gambar 4) dan Alarm Sabotase
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "display.h"

int main(void) {
    printf("====================================================================\n");
    printf("  TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & ALARM SABOTASE OLED     \n");
    printf("====================================================================\n\n");

    display_context_t ctx;
    bool ok = display_init(&ctx, "530000000001", 2000);
    assert(ok);

    display_spln_frame_t frame;

    /* --------------------------------------------------------------------- */
    /* UJI 1: KONDISI NORMAL (Tidak Ada Sabotase)                            */
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
    /* UJI 2: KONDISI ALARM SABOTASE AKTIF (Tombol PC13 Ditekan)            */
    /* --------------------------------------------------------------------- */
    printf("[UJI 2] Verifikasi Tampilan Alarm Sabotase (alarm_icon_active = true)...\n");
    ctx.alarm_icon_active = true;

    /* Cek banner alarm pada halaman pengukuran normal */
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("  > Banner Alarm Tersemat di Page 02: Respon Alarm = '%s' [PASS]\n", frame.alarm_code);

    /* Cek Halaman Khusus Darurat Sabotase (DISP_PAGE_TAMPER_ALARM) */
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.scroll_index_zz, "AL") == 0);
    assert(strcmp(frame.main_value, "SABOTASE") == 0);
    assert(strcmp(frame.unit, "E01") == 0);
    assert(strcmp(frame.obis_code, "96.50") == 0);
    assert(frame.is_large_font == false);
    printf("  > Halaman Sabotase E01: Header: '%s' | zz: '%s' | Teks: '%s' | Kode: '%s' | OBIS: '%s' [PASS]\n",
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
    /* UJI 3: PEMULIHAN SABOTASE (Tombol PC13 Ditekan Kembali / Normal)     */
    /* --------------------------------------------------------------------- */
    printf("[UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)...\n");
    ctx.alarm_icon_active = false;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    printf("  > Status Layar Berhasil Pulih ke OK [PASS]\n\n");

    printf("====================================================================\n");
    printf("  HASIL: 100%% SELURUH SPESIFIKASI LAYAR ALARM SPLN GAMBAR 4 LULUS!   \n");
    printf("====================================================================\n");

    return 0;
}

