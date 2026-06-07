#include "fonts/fonts.h"
#include "ui_display_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"
#include "ui_main_screen.h"
#include "backlight.h"

/* ── Language button group ───────────────────────────────────────── */

static lv_obj_t *s_lang_btns[4];
static const char *LANG_NAMES[4] = { "RU", "EN", "DE", "UK" };

static void update_lang_btn_styles(void)
{
    for (int i = 0; i < 4; i++) {
        bool active = (g_settings.language == (uint8_t)i);
        lv_obj_set_style_bg_color(s_lang_btns[i],
            active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_lang_btns[i], 0);
        lv_obj_set_style_text_color(lbl,
            active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void lang_btn_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    g_settings.language = idx;
    ui_strings_set(idx);
    ui_nvs_save_debounced();
    ui_rebuild_all();
}

/* ── Theme button group ──────────────────────────────────────────── */

static lv_obj_t *s_theme_btns[2];

static void update_theme_btn_styles(void)
{
    for (int i = 0; i < 2; i++) {
        bool active = (g_settings.theme == (uint8_t)i);
        lv_obj_set_style_bg_color(s_theme_btns[i],
            active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_theme_btns[i], 0);
        lv_obj_set_style_text_color(lbl,
            active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void theme_btn_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    g_settings.theme = idx;
    ui_theme_apply((ui_theme_t)idx);
    ui_nvs_save_debounced();
    ui_rebuild_all();
}

/* ── Brightness +/- buttons ──────────────────────────────────────── */

static lv_obj_t *s_brightness_label = NULL;

static void brightness_minus_cb(lv_event_t *e)
{
    if (g_settings.brightness > 25)
        g_settings.brightness -= (g_settings.brightness - 5 >= 25) ? 5 : (g_settings.brightness - 25);
    backlight_set(g_settings.brightness);
    if (s_brightness_label)
        lv_label_set_text_fmt(s_brightness_label, "%d%%", g_settings.brightness);
    ui_nvs_save_debounced();
}

static void brightness_plus_cb(lv_event_t *e)
{
    if (g_settings.brightness < 100)
        g_settings.brightness += (g_settings.brightness + 5 <= 100) ? 5 : (100 - g_settings.brightness);
    backlight_set(g_settings.brightness);
    if (s_brightness_label)
        lv_label_set_text_fmt(s_brightness_label, "%d%%", g_settings.brightness);
    ui_nvs_save_debounced();
}

/* ── Backlight timeout button group ─────────────────────────────── */

static lv_obj_t *s_bl_btns[3];

static void update_bl_btn_styles(void)
{
    for (int i = 0; i < 3; i++) {
        bool active = (g_settings.bl_timeout_idx == (uint8_t)i);
        lv_obj_set_style_bg_color(s_bl_btns[i],
            active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_bl_btns[i], 0);
        lv_obj_set_style_text_color(lbl,
            active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void bl_btn_cb(lv_event_t *e)
{
    g_settings.bl_timeout_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    update_bl_btn_styles();
    ui_nvs_save_debounced();
}

/* ── Helper: build a flex-row button group container ─────────────── */

static lv_obj_t *btn_group_create(lv_obj_t *parent, int n_btns,
                                   lv_obj_t **btn_arr,
                                   const char **labels,
                                   lv_event_cb_t cb)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, 4, 0);

    for (int i = 0; i < n_btns; i++) {
        lv_obj_t *btn = lv_obj_create(cont);
        lv_obj_set_size(btn, 80, 32);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        btn_arr[i] = btn;
    }

    return cont;
}

/* ── Screen create ───────────────────────────────────────────────── */

void ui_display_screen_create(void)
{
    lv_obj_t *body = ui_sub_screen_create(&scr_display, ui_lang->display);

    /* ── Language row ── */
    lv_obj_t *r1 = ui_sub_row_create(body, ui_lang->lang_label, NULL);
    btn_group_create(r1, 4, s_lang_btns, LANG_NAMES, lang_btn_cb);
    update_lang_btn_styles();

    /* ── Theme row ── */
    lv_obj_t *r2 = ui_sub_row_create(body, ui_lang->theme_label, NULL);
    const char *theme_labels[2] = { ui_lang->theme_dark, ui_lang->theme_light };
    btn_group_create(r2, 2, s_theme_btns, theme_labels, theme_btn_cb);
    update_theme_btn_styles();

    /* ── Brightness row ── */
    s_brightness_label = NULL;
    lv_obj_t *r3 = ui_sub_row_create(body, ui_lang->brightness, NULL);

    lv_obj_t *br_cont = lv_obj_create(r3);
    lv_obj_remove_style_all(br_cont);
    lv_obj_remove_flag(br_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(br_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(br_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(br_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(br_cont, 8, 0);
    lv_obj_align(br_cont, LV_ALIGN_LEFT_MID, 0, 0);

    /* [-] button */
    lv_obj_t *btn_minus = lv_obj_create(br_cont);
    lv_obj_set_size(btn_minus, 36, 36);
    lv_obj_remove_flag(btn_minus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_minus, 6, 0);
    lv_obj_set_style_border_width(btn_minus, 0, 0);
    lv_obj_set_style_bg_color(btn_minus, ui_color_accent(), 0);
    lv_obj_set_style_pad_all(btn_minus, 0, 0);
    lv_obj_add_flag(btn_minus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_minus, brightness_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_minus = lv_label_create(btn_minus);
    lv_label_set_text(lbl_minus, "-");
    lv_obj_set_style_text_font(lbl_minus, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lbl_minus, lv_color_white(), 0);
    lv_obj_align(lbl_minus, LV_ALIGN_CENTER, 0, 0);

    /* value label */
    s_brightness_label = lv_label_create(br_cont);
    lv_label_set_text_fmt(s_brightness_label, "%d%%", g_settings.brightness);
    lv_obj_set_style_text_font(s_brightness_label, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(s_brightness_label, ui_color_text_primary(), 0);
    lv_obj_set_width(s_brightness_label, 52);
    lv_obj_set_style_text_align(s_brightness_label, LV_TEXT_ALIGN_CENTER, 0);

    /* [+] button */
    lv_obj_t *btn_plus = lv_obj_create(br_cont);
    lv_obj_set_size(btn_plus, 36, 36);
    lv_obj_remove_flag(btn_plus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_plus, 6, 0);
    lv_obj_set_style_border_width(btn_plus, 0, 0);
    lv_obj_set_style_bg_color(btn_plus, ui_color_accent(), 0);
    lv_obj_set_style_pad_all(btn_plus, 0, 0);
    lv_obj_add_flag(btn_plus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_plus, brightness_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_plus = lv_label_create(btn_plus);
    lv_label_set_text(lbl_plus, "+");
    lv_obj_set_style_text_font(lbl_plus, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lbl_plus, lv_color_white(), 0);
    lv_obj_align(lbl_plus, LV_ALIGN_CENTER, 0, 0);

    /* ── Backlight timeout row ── */
    lv_obj_t *r4 = ui_sub_row_create(body, ui_lang->bl_timeout, NULL);
    const char *bl_labels[3] = { ui_lang->bl_1min, ui_lang->bl_5min, ui_lang->bl_never };
    btn_group_create(r4, 3, s_bl_btns, bl_labels, bl_btn_cb);
    update_bl_btn_styles();
}
