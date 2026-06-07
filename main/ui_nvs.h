#pragma once
#include <stdint.h>

/* Channel tool type */
#define CH_TYPE_IRON   0
#define CH_TYPE_GUN    1   /* hot air gun — has airflow setpoint */

typedef struct {
    uint16_t ch1_sp;
    uint16_t ch2_sp;
    uint8_t  sound_en;
    uint8_t  beep_touch;
    uint8_t  alert_overheat;
    uint8_t  alert_ready;
    uint8_t  sleep_timer_idx;
    uint16_t sleep_temp;
    uint8_t  brightness;
    uint8_t  bl_timeout_idx;
    uint8_t  theme;
    uint8_t  language;
    uint8_t  units;
    /* per-channel */
    uint8_t  ch1_type;           /* CH_TYPE_IRON / CH_TYPE_GUN */
    uint8_t  ch2_type;
    uint16_t ch1_presets[3];     /* quick-access temperature presets */
    uint16_t ch2_presets[3];
    uint8_t  ch1_en;             /* channel enabled */
    uint8_t  ch2_en;
    uint8_t  ch1_airflow;        /* airflow % setpoint (gun only) */
    uint8_t  ch2_airflow;
    uint8_t  temp_step;          /* temperature step in °C (1–20) */
} ui_settings_t;

extern ui_settings_t g_settings;
void ui_nvs_load(void);
void ui_nvs_save_debounced(void);
