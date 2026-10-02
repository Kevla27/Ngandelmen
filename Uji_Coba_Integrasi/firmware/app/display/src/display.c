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
            ctx->current_page = (display_page_id_t)((ctx->current_page + 1) % DISP_PAGE_COUNT);
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
        ctx->current_page = (display_page_id_t)((ctx->current_page + 1) % DISP_PAGE_COUNT);
    } else if (dir == BUTTON_DIR_UP) {
        ctx->current_page = (ctx->current_page == 0) ? (display_page_id_t)(DISP_PAGE_COUNT - 1) 
                                                     : (display_page_id_t)(ctx->current_page - 1);
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
            snprintf(out_line3, max_len, "%u.%u V", ctx->meas_buffer.voltage_r_dvolts / 10, ctx->meas_buffer.voltage_r_dvolts % 10);
            break;
        case DISP_PAGE_CURRENT_R:
            snprintf(out_line2, max_len, "CURRENT PHASE R");
            snprintf(out_line3, max_len, "%u.%03u A", ctx->meas_buffer.current_r_mamps / 1000, ctx->meas_buffer.current_r_mamps % 1000);
            break;
        case DISP_PAGE_ACTIVE_POWER:
            snprintf(out_line2, max_len, "ACTIVE POWER");
            snprintf(out_line3, max_len, "%.3f kW", ctx->meas_buffer.active_power_w / 1000.0);
            break;
        default:
            snprintf(out_line2, max_len, "DISPLAY PAGE %d", ctx->current_page);
            snprintf(out_line3, max_len, "---");
            break;
    }
}