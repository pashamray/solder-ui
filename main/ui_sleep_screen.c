#include "fonts/fonts.h"
#include "ui_sleep_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"
#include <stdio.h>

#define SLEEP_TEMP_MIN  50
#define SLEEP_TEMP_MAX  200
#define SLEEP_TEMP_STEP 5

static lv_obj_t *s_timer_btns[4];
static lv_obj_t *s_sleep_temp_lbl;

static int sleep_to_disp(int c)
{
    return g_settings.units ? c * 9 / 5 + 32 : c;
}

/* ── Timer buttons ───────────────────────────────────────────────── */

static void update_timer_btn_styles(void)
{
    for (int i = 0; i < 4; i++) {
        bool active = (i == g_settings.sleep_timer_idx);
        lv_obj_set_style_bg_color(s_timer_btns[i],
            active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_timer_btns[i], 0);
        if (lbl)
            lv_obj_set_style_text_color(lbl,
                active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void timer_btn_cb(lv_event_t *e)
{
    g_settings.sleep_timer_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    update_timer_btn_styles();
    ui_nvs_save_debounced();
}

/* ── Sleep temperature ±  ────────────────────────────────────────── */

static void refresh_sleep_temp(void)
{
    if (!s_sleep_temp_lbl) return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", sleep_to_disp(g_settings.sleep_temp));
    lv_label_set_text(s_sleep_temp_lbl, buf);
}

static void sleep_temp_minus_cb(lv_event_t *e)
{
    (void)e;
    if (g_settings.sleep_temp > SLEEP_TEMP_MIN) {
        g_settings.sleep_temp -= SLEEP_TEMP_STEP;
        refresh_sleep_temp();
        ui_nvs_save_debounced();
    }
}

static void sleep_temp_plus_cb(lv_event_t *e)
{
    (void)e;
    if (g_settings.sleep_temp < SLEEP_TEMP_MAX) {
        g_settings.sleep_temp += SLEEP_TEMP_STEP;
        refresh_sleep_temp();
        ui_nvs_save_debounced();
    }
}

/* ── Screen create ───────────────────────────────────────────────── */

void ui_sleep_screen_create(void)
{
    s_sleep_temp_lbl = NULL;
    lv_obj_t *body = ui_sub_screen_create(&scr_sleep, ui_lang->sleep);

    /* Row 1: sleep timer selector */
    lv_obj_t *r1 = ui_sub_row_create(body, ui_lang->sleep_timer_label, NULL);

    lv_obj_t *btn_row = lv_obj_create(r1);
    lv_obj_remove_style_all(btn_row);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btn_row, 4, 0);

    const char *timer_labels[4] = {
        ui_lang->sl_2min,
        ui_lang->bl_5min,
        ui_lang->sl_10min,
        ui_lang->bl_never,
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(btn_row);
        lv_obj_set_size(btn, 80, 32);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_event_cb(btn, timer_btn_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, timer_labels[i]);
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
        lv_obj_center(lbl);
        s_timer_btns[i] = btn;
    }
    update_timer_btn_styles();

    /* Row 2: sleep temperature — [−] value [+] */
    lv_obj_t *r2 = ui_sub_row_create(body, ui_lang->sleep_temp_label, NULL);

    lv_obj_t *ctrl = lv_obj_create(r2);
    lv_obj_remove_style_all(ctrl);
    lv_obj_remove_flag(ctrl, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ctrl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(ctrl, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(ctrl, 8, 0);

    /* Minus */
    lv_obj_t *btn_m = lv_button_create(ctrl);
    lv_obj_set_size(btn_m, 36, 36);
    lv_obj_set_style_bg_color(btn_m, ui_color_border(), 0);
    lv_obj_set_style_radius(btn_m, 6, 0);
    lv_obj_set_style_border_width(btn_m, 0, 0);
    lv_obj_set_style_pad_all(btn_m, 0, 0);
    lv_obj_add_event_cb(btn_m, sleep_temp_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_m = lv_label_create(btn_m);
    lv_label_set_text(lbl_m, "-");
    lv_obj_set_style_text_font(lbl_m, &roboto_pm_32, 0);
    lv_obj_set_style_text_color(lbl_m, ui_color_text_primary(), 0);
    lv_obj_center(lbl_m);

    /* Value */
    s_sleep_temp_lbl = lv_label_create(ctrl);
    lv_obj_set_style_text_font(s_sleep_temp_lbl, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(s_sleep_temp_lbl, ui_color_text_primary(), 0);
    lv_obj_set_style_min_width(s_sleep_temp_lbl, 48, 0);
    lv_obj_set_style_text_align(s_sleep_temp_lbl, LV_TEXT_ALIGN_CENTER, 0);
    refresh_sleep_temp();

    /* Plus */
    lv_obj_t *btn_p = lv_button_create(ctrl);
    lv_obj_set_size(btn_p, 36, 36);
    lv_obj_set_style_bg_color(btn_p, ui_color_border(), 0);
    lv_obj_set_style_radius(btn_p, 6, 0);
    lv_obj_set_style_border_width(btn_p, 0, 0);
    lv_obj_set_style_pad_all(btn_p, 0, 0);
    lv_obj_add_event_cb(btn_p, sleep_temp_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, "+");
    lv_obj_set_style_text_font(lbl_p, &roboto_pm_32, 0);
    lv_obj_set_style_text_color(lbl_p, ui_color_text_primary(), 0);
    lv_obj_center(lbl_p);
}
