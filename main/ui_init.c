#include "ui_init.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "backlight.h"
#include "ui_main_screen.h"
#include "ui_settings_screen.h"
#include "ui_sound_screen.h"
#include "ui_sleep_screen.h"
#include "ui_display_screen.h"
#include "ui_units_screen.h"
#include "ui_profiles_screen.h"
#include "ui_about_screen.h"
#include "ui_ch_settings_screen.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

lv_obj_t *scr_main         = NULL;
lv_obj_t *scr_settings     = NULL;
lv_obj_t *scr_sound        = NULL;
lv_obj_t *scr_sleep        = NULL;
lv_obj_t *scr_display      = NULL;
lv_obj_t *scr_units        = NULL;
lv_obj_t *scr_profiles     = NULL;
lv_obj_t *scr_about        = NULL;
lv_obj_t *scr_ch1_settings = NULL;
lv_obj_t *scr_ch2_settings = NULL;

static bool s_rebuild_to_settings = false;

static void do_rebuild(void *data)
{
    (void)data;

    ui_main_screen_prepare_destroy();

    lv_obj_t *tmp = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(tmp, ui_color_bg(), 0);
    lv_screen_load(tmp);

    if (scr_main)          { lv_obj_delete(scr_main);          scr_main = NULL; }
    if (scr_settings)      { lv_obj_delete(scr_settings);      scr_settings = NULL; }
    if (scr_sound)         { lv_obj_delete(scr_sound);         scr_sound = NULL; }
    if (scr_sleep)         { lv_obj_delete(scr_sleep);         scr_sleep = NULL; }
    if (scr_display)       { lv_obj_delete(scr_display);       scr_display = NULL; }
    if (scr_units)         { lv_obj_delete(scr_units);         scr_units = NULL; }
    if (scr_profiles)      { lv_obj_delete(scr_profiles);      scr_profiles = NULL; }
    if (scr_about)         { lv_obj_delete(scr_about);         scr_about = NULL; }
    if (scr_ch1_settings)  { lv_obj_delete(scr_ch1_settings);  scr_ch1_settings = NULL; }
    if (scr_ch2_settings)  { lv_obj_delete(scr_ch2_settings);  scr_ch2_settings = NULL; }

    ui_main_screen_create();
    ui_settings_screen_create();
    ui_sound_screen_create();
    ui_sleep_screen_create();
    ui_display_screen_create();
    ui_units_screen_create();
    ui_profiles_screen_create();
    ui_about_screen_create();
    ui_ch1_settings_screen_create();
    ui_ch2_settings_screen_create();

    bool to_settings = s_rebuild_to_settings;
    s_rebuild_to_settings = false;

    lv_screen_load(to_settings ? scr_settings : scr_display);
    lv_obj_delete(tmp);
}

void ui_rebuild_all(void)
{
    lv_async_call(do_rebuild, NULL);
}

void ui_rebuild_to_settings(void)
{
    s_rebuild_to_settings = true;
    lv_async_call(do_rebuild, NULL);
}

#define UI_CREATE(fn) do { \
    lvgl_port_lock(0); fn(); lvgl_port_unlock(); \
    vTaskDelay(pdMS_TO_TICKS(10)); \
} while(0)

void app_ui_init(void)
{
    ui_theme_init();
    ui_strings_init();
    ui_nvs_load();
    ui_theme_apply((ui_theme_t)g_settings.theme);
    ui_strings_set(g_settings.language);

    UI_CREATE(ui_main_screen_create);
    UI_CREATE(ui_settings_screen_create);
    UI_CREATE(ui_sound_screen_create);
    UI_CREATE(ui_sleep_screen_create);
    UI_CREATE(ui_display_screen_create);
    UI_CREATE(ui_units_screen_create);
    UI_CREATE(ui_profiles_screen_create);
    UI_CREATE(ui_about_screen_create);
    UI_CREATE(ui_ch1_settings_screen_create);
    UI_CREATE(ui_ch2_settings_screen_create);

    lvgl_port_lock(0);
    backlight_set(g_settings.brightness);
    lv_screen_load(scr_main);
    lvgl_port_unlock();
}
