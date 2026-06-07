#include "fonts/fonts.h"
#include "ui_profiles_screen.h"
#include "ui_main_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"

typedef struct { const char *name; uint16_t ch1; uint16_t ch2; } preset_t;

static const preset_t PRESETS[3] = {
    { "Lead-free", 250, 250 },
    { "Leaded",    320, 320 },
    { "Fine work", 350, 350 },
};

static void apply_preset_cb(lv_event_t *e)
{
    const preset_t *p = (const preset_t *)lv_event_get_user_data(e);
    g_settings.ch1_sp = p->ch1;
    g_settings.ch2_sp = p->ch2;
    ui_main_screen_refresh_sp();
    ui_nvs_save_debounced();
}

static void save_current_cb(lv_event_t *e)
{
    (void)e;
    ui_nvs_save_debounced();
}

void ui_profiles_screen_create(void)
{
    lv_obj_t *body = ui_sub_screen_create(&scr_profiles, ui_lang->profiles);

    for (int i = 0; i < 3; i++) {
        /* Clickable row */
        lv_obj_t *row = lv_obj_create(body);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(row, ui_color_surface(), 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_all(row, 14, 0);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(row, lv_color_mix(ui_color_surface(),
                                                     lv_color_white(), 220),
                                   LV_STATE_PRESSED);
        lv_obj_add_event_cb(row, apply_preset_cb, LV_EVENT_CLICKED,
                            (void *)&PRESETS[i]);

        /* Preset name — left side */
        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, PRESETS[i].name);
        lv_obj_set_style_text_color(lbl_name, ui_color_text_primary(), 0);
        lv_obj_set_style_text_font(lbl_name, &roboto_cyrillic_14, 0);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 0, 0);

        /* Temperature values — right side */
        char temp_buf[32];
        lv_snprintf(temp_buf, sizeof(temp_buf),
                    "CH1: %u\xc2\xb0  CH2: %u\xc2\xb0",
                    (unsigned)PRESETS[i].ch1, (unsigned)PRESETS[i].ch2);
        lv_obj_t *lbl_temps = lv_label_create(row);
        lv_label_set_text(lbl_temps, temp_buf);
        lv_obj_set_style_text_color(lbl_temps, ui_color_ch2(), 0);
        lv_obj_set_style_text_font(lbl_temps, &roboto_cyrillic_14, 0);
        lv_obj_align(lbl_temps, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    /* "Save current" button — full-width, 40 px height */
    lv_obj_t *btn_save = lv_btn_create(body);
    lv_obj_set_size(btn_save, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(btn_save, ui_color_accent(), 0);
    lv_obj_set_style_radius(btn_save, 8, 0);
    lv_obj_set_style_border_width(btn_save, 0, 0);
    lv_obj_add_event_cb(btn_save, save_current_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, ui_lang->save_current);
    lv_obj_set_style_text_color(lbl_save, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_save, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_save, LV_ALIGN_CENTER, 0, 0);
}
