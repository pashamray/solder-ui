#include "ui_nvs.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NVS_NS "solder_ui"
static const char *TAG = "ui_nvs";

ui_settings_t g_settings = {
    .ch1_sp          = 350,
    .ch2_sp          = 300,
    .sound_en        = 1,
    .beep_touch      = 0,
    .alert_overheat  = 1,
    .alert_ready     = 1,
    .sleep_timer_idx = 1,
    .sleep_temp      = 150,
    .brightness      = 80,
    .bl_timeout_idx  = 1,
    .theme           = 0,
    .language        = 0,
    .units           = 0,
    .ch1_type        = CH_TYPE_IRON,
    .ch2_type        = CH_TYPE_IRON,
    .ch1_presets     = {180, 250, 320, 400},
    .ch2_presets     = {180, 250, 320, 400},
    .ch1_en          = 1,
    .ch2_en          = 1,
    .ch1_airflow     = 50,
    .ch2_airflow     = 50,
    .temp_step       = 5,
    .ch1_color_idx   = 0,
    .ch2_color_idx   = 1,
};

void ui_nvs_load(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "nvs_flash_init after erase failed: %d", err);
            return;
        }
    }

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "no saved settings, using defaults");
        return;
    }

    nvs_get_u16(h, "ch1_sp",   &g_settings.ch1_sp);
    nvs_get_u16(h, "ch2_sp",   &g_settings.ch2_sp);
    nvs_get_u8 (h, "sound_en", &g_settings.sound_en);
    nvs_get_u8 (h, "beep",     &g_settings.beep_touch);
    nvs_get_u8 (h, "overheat", &g_settings.alert_overheat);
    nvs_get_u8 (h, "ready",    &g_settings.alert_ready);
    nvs_get_u8 (h, "slp_tmr",  &g_settings.sleep_timer_idx);
    nvs_get_u16(h, "slp_temp", &g_settings.sleep_temp);
    nvs_get_u8 (h, "bright",   &g_settings.brightness);
    nvs_get_u8 (h, "bl_tmout", &g_settings.bl_timeout_idx);
    nvs_get_u8 (h, "theme",    &g_settings.theme);
    nvs_get_u8 (h, "lang",     &g_settings.language);  /* fixed: was &g_settings.language passed to nvs_set_u8 */
    nvs_get_u8 (h, "units",    &g_settings.units);
    nvs_get_u8 (h, "ch1_type", &g_settings.ch1_type);
    nvs_get_u8 (h, "ch2_type", &g_settings.ch2_type);
    nvs_get_u16(h, "ch1_p0",   &g_settings.ch1_presets[0]);
    nvs_get_u16(h, "ch1_p1",   &g_settings.ch1_presets[1]);
    nvs_get_u16(h, "ch1_p2",   &g_settings.ch1_presets[2]);
    nvs_get_u16(h, "ch1_p3",   &g_settings.ch1_presets[3]);
    nvs_get_u16(h, "ch2_p0",   &g_settings.ch2_presets[0]);
    nvs_get_u16(h, "ch2_p1",   &g_settings.ch2_presets[1]);
    nvs_get_u16(h, "ch2_p2",   &g_settings.ch2_presets[2]);
    nvs_get_u16(h, "ch2_p3",   &g_settings.ch2_presets[3]);
    nvs_get_u8 (h, "ch1_en",   &g_settings.ch1_en);
    nvs_get_u8 (h, "ch2_en",   &g_settings.ch2_en);
    nvs_get_u8 (h, "ch1_aflo", &g_settings.ch1_airflow);
    nvs_get_u8 (h, "ch2_aflo", &g_settings.ch2_airflow);
    nvs_get_u8 (h, "t_step",   &g_settings.temp_step);
    nvs_get_u8 (h, "ch1_col",  &g_settings.ch1_color_idx);
    nvs_get_u8 (h, "ch2_col",  &g_settings.ch2_color_idx);

    nvs_close(h);
    ESP_LOGI(TAG, "loaded: ch1=%d ch2=%d theme=%d lang=%d",
             g_settings.ch1_sp, g_settings.ch2_sp,
             g_settings.theme, g_settings.language);
}

static lv_timer_t *s_save_timer = NULL;

static void nvs_save_cb(lv_timer_t *t)
{
    s_save_timer = NULL;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;

    nvs_set_u16(h, "ch1_sp",   g_settings.ch1_sp);
    nvs_set_u16(h, "ch2_sp",   g_settings.ch2_sp);
    nvs_set_u8 (h, "sound_en", g_settings.sound_en);
    nvs_set_u8 (h, "beep",     g_settings.beep_touch);
    nvs_set_u8 (h, "overheat", g_settings.alert_overheat);
    nvs_set_u8 (h, "ready",    g_settings.alert_ready);
    nvs_set_u8 (h, "slp_tmr",  g_settings.sleep_timer_idx);
    nvs_set_u16(h, "slp_temp", g_settings.sleep_temp);
    nvs_set_u8 (h, "bright",   g_settings.brightness);
    nvs_set_u8 (h, "bl_tmout", g_settings.bl_timeout_idx);
    nvs_set_u8 (h, "theme",    g_settings.theme);
    nvs_set_u8 (h, "lang",     g_settings.language);
    nvs_set_u8 (h, "units",    g_settings.units);
    nvs_set_u8 (h, "ch1_type", g_settings.ch1_type);
    nvs_set_u8 (h, "ch2_type", g_settings.ch2_type);
    nvs_set_u16(h, "ch1_p0",   g_settings.ch1_presets[0]);
    nvs_set_u16(h, "ch1_p1",   g_settings.ch1_presets[1]);
    nvs_set_u16(h, "ch1_p2",   g_settings.ch1_presets[2]);
    nvs_set_u16(h, "ch1_p3",   g_settings.ch1_presets[3]);
    nvs_set_u16(h, "ch2_p0",   g_settings.ch2_presets[0]);
    nvs_set_u16(h, "ch2_p1",   g_settings.ch2_presets[1]);
    nvs_set_u16(h, "ch2_p2",   g_settings.ch2_presets[2]);
    nvs_set_u16(h, "ch2_p3",   g_settings.ch2_presets[3]);
    nvs_set_u8 (h, "ch1_en",   g_settings.ch1_en);
    nvs_set_u8 (h, "ch2_en",   g_settings.ch2_en);
    nvs_set_u8 (h, "ch1_aflo", g_settings.ch1_airflow);
    nvs_set_u8 (h, "ch2_aflo", g_settings.ch2_airflow);
    nvs_set_u8 (h, "t_step",   g_settings.temp_step);
    nvs_set_u8 (h, "ch1_col",  g_settings.ch1_color_idx);
    nvs_set_u8 (h, "ch2_col",  g_settings.ch2_color_idx);

    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "saved");
}

void ui_nvs_save_debounced(void)
{
    if (s_save_timer) lv_timer_delete(s_save_timer);
    s_save_timer = lv_timer_create(nvs_save_cb, 500, NULL);
    lv_timer_set_repeat_count(s_save_timer, 1);
}
