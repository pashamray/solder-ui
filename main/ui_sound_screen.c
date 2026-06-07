#include "ui_sound_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"

static void sound_en_cb(lv_event_t *e)
{
    g_settings.sound_en = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0;
    ui_nvs_save_debounced();
}

static void beep_touch_cb(lv_event_t *e)
{
    g_settings.beep_touch = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0;
    ui_nvs_save_debounced();
}

static void alert_overheat_cb(lv_event_t *e)
{
    g_settings.alert_overheat = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0;
    ui_nvs_save_debounced();
}

static void alert_ready_cb(lv_event_t *e)
{
    g_settings.alert_ready = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0;
    ui_nvs_save_debounced();
}

void ui_sound_screen_create(void)
{
    lv_obj_t *body = ui_sub_screen_create(&scr_sound, ui_lang->sound);

    lv_obj_t *r1 = ui_sub_row_create(body, ui_lang->sound, NULL);
    ui_toggle_create(r1, g_settings.sound_en, sound_en_cb, NULL);

    lv_obj_t *r2 = ui_sub_row_create(body, "Бип при касании", NULL);
    ui_toggle_create(r2, g_settings.beep_touch, beep_touch_cb, NULL);

    lv_obj_t *r3 = ui_sub_row_create(body, "Алерт перегрева", NULL);
    ui_toggle_create(r3, g_settings.alert_overheat, alert_overheat_cb, NULL);

    lv_obj_t *r4 = ui_sub_row_create(body, "Сигнал прогрева", NULL);
    ui_toggle_create(r4, g_settings.alert_ready, alert_ready_cb, NULL);
}
