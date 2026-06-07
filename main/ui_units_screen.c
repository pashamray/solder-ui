#include "fonts/fonts.h"
#include "ui_units_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "ui_main_screen.h"
#include "lvgl.h"

static lv_obj_t *s_unit_btns[2];
static lv_obj_t *s_step_lbl;

/* Step stored in °C; display in current units (delta: no +32 offset) */
static int step_to_disp(void)
{
    return g_settings.units ? (int)((g_settings.temp_step * 9 + 2) / 5)
                            : (int)g_settings.temp_step;
}

static void update_unit_btn_styles(void) {
    for (int i = 0; i < 2; i++) {
        bool active = (g_settings.units == i);
        lv_obj_set_style_bg_color(s_unit_btns[i],
            active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_unit_btns[i], 0);
        if (lbl) lv_obj_set_style_text_color(lbl,
            active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void unit_btn_cb(lv_event_t *e) {
    g_settings.units = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    update_unit_btn_styles();
    if (s_step_lbl) lv_label_set_text_fmt(s_step_lbl, "%d", step_to_disp());
    ui_nvs_save_debounced();
    ui_main_screen_update_units();
}

static void step_adj_cb(lv_event_t *e) {
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    int disp = step_to_disp() + dir;
    int max_disp = g_settings.units ? 36 : 20;
    if (disp < 1) disp = 1;
    if (disp > max_disp) disp = max_disp;
    if (g_settings.units) {
        uint8_t step_c = (uint8_t)((disp * 5 + 4) / 9);
        if (step_c < 1) step_c = 1;
        g_settings.temp_step = step_c;
    } else {
        g_settings.temp_step = (uint8_t)disp;
    }
    if (s_step_lbl) lv_label_set_text_fmt(s_step_lbl, "%d", step_to_disp());
    ui_nvs_save_debounced();
}

void ui_units_screen_create(void)
{
    s_step_lbl = NULL;

    lv_obj_t *body = ui_sub_screen_create(&scr_units, ui_lang->units);

    lv_obj_t *row = ui_sub_row_create(body, ui_lang->units, NULL);

    /* Button group container */
    lv_obj_t *grp = lv_obj_create(row);
    lv_obj_remove_flag(grp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(grp, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grp, 0, 0);
    lv_obj_set_style_pad_all(grp, 0, 0);
    lv_obj_set_flex_flow(grp, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(grp, 4, 0);

    const char *labels[2] = { ui_lang->units_c, ui_lang->units_f };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_obj_create(grp);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(btn, 80, 32);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, unit_btn_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        s_unit_btns[i] = btn;
    }
    update_unit_btn_styles();

    /* ── Temperature step row ── */
    lv_obj_t *step_row = ui_sub_row_create(body, ui_lang->temp_step, NULL);

    lv_obj_t *cont = lv_obj_create(step_row);
    lv_obj_remove_style_all(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(cont, 8, 0);

    lv_obj_t *btn_m = lv_obj_create(cont);
    lv_obj_set_size(btn_m, 36, 36);
    lv_obj_remove_flag(btn_m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_m, 6, 0);
    lv_obj_set_style_border_width(btn_m, 0, 0);
    lv_obj_set_style_bg_color(btn_m, ui_color_border(), 0);
    lv_obj_set_style_pad_all(btn_m, 0, 0);
    lv_obj_add_flag(btn_m, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_m, step_adj_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
    lv_obj_t *lm = lv_label_create(btn_m);
    lv_label_set_text(lm, "-");
    lv_obj_set_style_text_font(lm, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lm, ui_color_text_primary(), 0);
    lv_obj_align(lm, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *val = lv_label_create(cont);
    lv_label_set_text_fmt(val, "%d", step_to_disp());
    lv_obj_set_style_text_font(val, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(val, ui_color_text_primary(), 0);
    lv_obj_set_width(val, 56);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
    s_step_lbl = val;

    lv_obj_t *btn_p = lv_obj_create(cont);
    lv_obj_set_size(btn_p, 36, 36);
    lv_obj_remove_flag(btn_p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_p, 6, 0);
    lv_obj_set_style_border_width(btn_p, 0, 0);
    lv_obj_set_style_bg_color(btn_p, ui_color_border(), 0);
    lv_obj_set_style_pad_all(btn_p, 0, 0);
    lv_obj_add_flag(btn_p, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_p, step_adj_cb, LV_EVENT_CLICKED, (void *)(intptr_t)+1);
    lv_obj_t *lp = lv_label_create(btn_p);
    lv_label_set_text(lp, "+");
    lv_obj_set_style_text_font(lp, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lp, ui_color_text_primary(), 0);
    lv_obj_align(lp, LV_ALIGN_CENTER, 0, 0);
}
