/**
 * @file display.h
 * @brief Modul Tampilan LCD & Navigasi Carousel Smart Meter 3-Fasa (E3/ENG-3)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DISPLAY_MODE_AUTO_CAROUSEL = 0,
    DISPLAY_MODE_MANUAL_SCROLL
} display_mode_t;

typedef enum {
    BUTTON_DIR_UP = 0,
    BUTTON_DIR_DOWN
} button_dir_t;

typedef enum {
    DISP_PAGE_IDPEL = 0,
    DISP_PAGE_VOLTAGE_R,
    DISP_PAGE_VOLTAGE_S,
    DISP_PAGE_VOLTAGE_T,
    DISP_PAGE_CURRENT_R,
    DISP_PAGE_CURRENT_S,
    DISP_PAGE_CURRENT_T,
    DISP_PAGE_CURRENT_N,
    DISP_PAGE_ACTIVE_POWER,
    DISP_PAGE_REACTIVE_POWER,
    DISP_PAGE_APPARENT_POWER,
    DISP_PAGE_POWER_FACTOR,
    DISP_PAGE_FREQUENCY,
    DISP_PAGE_ACTIVE_ENERGY,
    DISP_PAGE_COUNT
} display_page_id_t;

typedef struct {
    uint32_t voltage_r_dvolts;
    uint32_t voltage_s_dvolts;
    uint32_t voltage_t_dvolts;
    
    uint32_t current_r_mamps;
    uint32_t current_s_mamps;
    uint32_t current_t_mamps;
    uint32_t current_n_mamps;
    
    int32_t  active_power_w;
    int32_t  reactive_power_var;
    uint32_t apparent_power_va;
    
    uint16_t power_factor_ppm;
    uint16_t frequency_mhz;
    
    uint64_t active_energy_wh;
} meter_measurements_t;

typedef struct {
    display_mode_t   mode;
    display_page_id_t current_page;
    uint32_t         carousel_interval_ms;
    uint32_t         timer_accumulator_ms;
    uint32_t         manual_timeout_ms;
    meter_measurements_t meas_buffer;
    char             customer_id[16];
    bool             alarm_icon_active;
} display_context_t;

bool display_init(display_context_t *ctx, const char *customer_id, uint32_t carousel_interval_ms);
void display_render(display_context_t *ctx);
void display_update_measurements(display_context_t *ctx, const meter_measurements_t *meas);
void display_process_tick(display_context_t *ctx, uint32_t elapsed_ms);
void display_handle_button_press(display_context_t *ctx, button_dir_t dir);
void display_render_frame(const display_context_t *ctx, char *out_line1, char *out_line2, char *out_line3, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */