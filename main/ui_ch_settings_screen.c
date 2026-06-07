#include "fonts/fonts.h"
#include "ui_ch_settings_screen.h"
#include "ui_main_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

#define SWATCH_SZ  36
#define SWATCH_GAP  6

/* ------------------------------------------------------------------ */
/*  Shared state — both channel screens share the same callbacks        */
/* ------------------------------------------------------------------ */

/* Preset value labels: [ch][preset_idx] */
static lv_obj_t *s_preset_lbl[2][4];
/* Airflow value label per channel */
static lv_obj_t *s_af_lbl[2];
/* Tool type buttons: [ch][0=iron, 1=gun] */
static lv_obj_t *s_type_btns[2][2];
/* Airflow row container (shown only for gun) */
static lv_obj_t *s_af_row[2];
/* Color swatches: [ch][color_idx] */
static lv_obj_t *s_color_swatches[2][CH_COLOR_COUNT];

/* °C or °F display of a temperature */
static int to_disp(int c) { return g_settings.units ? c * 9 / 5 + 32 : c; }

/* ------------------------------------------------------------------ */
/*  Style helpers                                                       */
/* ------------------------------------------------------------------ */

static void refresh_type_btn_styles(int ch)
{
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = s_type_btns[ch][i];
        if (!btn) continue;
        bool active = ((ch == 0 ? g_settings.ch1_type : g_settings.ch2_type) == (uint8_t)i);
        lv_obj_set_style_bg_color(btn, active ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (lbl)
            lv_obj_set_style_text_color(lbl,
                active ? lv_color_white() : ui_color_text_secondary(), 0);
    }
}

static void refresh_af_row_visibility(int ch)
{
    if (!s_af_row[ch]) return;
    uint8_t type = (ch == 0) ? g_settings.ch1_type : g_settings.ch2_type;
    if (type == CH_TYPE_GUN)
        lv_obj_remove_flag(s_af_row[ch], LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(s_af_row[ch], LV_OBJ_FLAG_HIDDEN);
}

static void refresh_color_swatches(int ch)
{
    uint8_t sel = (ch == 0) ? g_settings.ch1_color_idx : g_settings.ch2_color_idx;
    for (int i = 0; i < CH_COLOR_COUNT; i++) {
        lv_obj_t *sw = s_color_swatches[ch][i];
        if (!sw) continue;
        if (i == sel) {
            /* dark inner border + white outer outline: visible on any swatch color */
            lv_obj_set_style_border_color(sw, lv_color_hex(0x222222), 0);
            lv_obj_set_style_border_width(sw, 3, 0);
            lv_obj_set_style_outline_color(sw, lv_color_white(), 0);
            lv_obj_set_style_outline_width(sw, 3, 0);
            lv_obj_set_style_outline_pad(sw, 3, 0);
            lv_obj_set_style_outline_opa(sw, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_border_width(sw, 0, 0);
            lv_obj_set_style_outline_width(sw, 0, 0);
        }
    }
}

static void color_swatch_cb(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int ch  = (int)(data >> 8);
    int idx = (int)(data & 0xFF);

    if (ch == 0) g_settings.ch1_color_idx = (uint8_t)idx;
    else         g_settings.ch2_color_idx = (uint8_t)idx;

    ui_theme_set_ch_colors(g_settings.ch1_color_idx, g_settings.ch2_color_idx);
    refresh_color_swatches(ch);
    ui_main_screen_update_ch_color(ch);
    ui_nvs_save_debounced();
}

/* ------------------------------------------------------------------ */
/*  Callbacks                                                           */
/* ------------------------------------------------------------------ */

static void type_btn_cb(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int ch   = (int)(data >> 8);
    int type = (int)(data & 0xFF);

    if (ch == 0) g_settings.ch1_type = (uint8_t)type;
    else         g_settings.ch2_type = (uint8_t)type;

    refresh_type_btn_styles(ch);
    refresh_af_row_visibility(ch);
    ui_nvs_save_debounced();
    /* Channel layout changes (iron vs gun), so rebuild main screen on exit */
    /* The user navigates back; rebuild_to_settings keeps them in settings flow */
    ui_rebuild_to_settings();
}

/* Preset +/- callbacks: user_data = (ch << 12) | (idx << 8) | dir
 *   dir=0 → minus, dir=1 → plus */
static void preset_adj_cb(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int ch  = (int)((data >> 12) & 0xF);
    int idx = (int)((data >>  8) & 0xF);
    int dir = (int)( data        & 0xFF);

    uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;
    if (dir == 1) {
        if (presets[idx] < 450) presets[idx] += 5;
    } else {
        if (presets[idx] > 100) presets[idx] -= 5;
    }

    lv_obj_t *lbl = s_preset_lbl[ch][idx];
    if (lbl)
        lv_label_set_text_fmt(lbl, "%d", to_disp((int)presets[idx]));
    ui_main_screen_refresh_presets(ch);
    ui_nvs_save_debounced();
}

/* Airflow +/- callbacks: user_data = (ch << 8) | dir */
static void af_adj_cb(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int ch  = (int)((data >> 8) & 0xFF);
    int dir = (int)( data       & 0xFF);

    uint8_t *af = (ch == 0) ? &g_settings.ch1_airflow : &g_settings.ch2_airflow;
    if (dir == 1) {
        if (*af < 100) *af += 5;
    } else {
        if (*af > 10) *af -= 5;
    }

    if (s_af_lbl[ch])
        lv_label_set_text_fmt(s_af_lbl[ch], "%d%%", *af);
    ui_nvs_save_debounced();
}

/* ------------------------------------------------------------------ */
/*  Build helpers                                                       */
/* ------------------------------------------------------------------ */

/* Small +/- row: [label_text] [−] [value_lbl] [+]
 * Returns the value label for the caller to store. */
static lv_obj_t *make_adj_row(lv_obj_t *body, const char *name,
                               const char *value_text,
                               lv_event_cb_t cb,
                               void *ud_minus, void *ud_plus)
{
    lv_obj_t *row = ui_sub_row_create(body, name, NULL);

    lv_obj_t *cont = lv_obj_create(row);
    lv_obj_remove_style_all(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(cont, 8, 0);
    lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);

    /* − button */
    lv_obj_t *btn_m = lv_obj_create(cont);
    lv_obj_set_size(btn_m, 36, 36);
    lv_obj_remove_flag(btn_m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_m, 6, 0);
    lv_obj_set_style_border_width(btn_m, 0, 0);
    lv_obj_set_style_bg_color(btn_m, ui_color_border(), 0);
    lv_obj_set_style_pad_all(btn_m, 0, 0);
    lv_obj_add_flag(btn_m, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_m, cb, LV_EVENT_CLICKED, ud_minus);
    lv_obj_t *lm = lv_label_create(btn_m);
    lv_label_set_text(lm, "-");
    lv_obj_set_style_text_font(lm, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lm, ui_color_text_primary(), 0);
    lv_obj_align(lm, LV_ALIGN_CENTER, 0, 0);

    /* Value label */
    lv_obj_t *val = lv_label_create(cont);
    lv_label_set_text(val, value_text);
    lv_obj_set_style_text_font(val, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(val, ui_color_text_primary(), 0);
    lv_obj_set_width(val, 56);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);

    /* + button */
    lv_obj_t *btn_p = lv_obj_create(cont);
    lv_obj_set_size(btn_p, 36, 36);
    lv_obj_remove_flag(btn_p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn_p, 6, 0);
    lv_obj_set_style_border_width(btn_p, 0, 0);
    lv_obj_set_style_bg_color(btn_p, ui_color_border(), 0);
    lv_obj_set_style_pad_all(btn_p, 0, 0);
    lv_obj_add_flag(btn_p, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_p, cb, LV_EVENT_CLICKED, ud_plus);
    lv_obj_t *lp = lv_label_create(btn_p);
    lv_label_set_text(lp, "+");
    lv_obj_set_style_text_font(lp, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lp, ui_color_text_primary(), 0);
    lv_obj_align(lp, LV_ALIGN_CENTER, 0, 0);

    return val;
}

/* ------------------------------------------------------------------ */
/*  Core screen builder                                                 */
/* ------------------------------------------------------------------ */

static void create_ch_settings_screen(lv_obj_t **handle, int ch)
{
    const char *title = (ch == 0) ? ui_lang->channel_1 : ui_lang->channel_2;
    lv_obj_t *body = ui_sub_screen_create(handle, title);

    /* ── Tool type row ── */
    lv_obj_t *r_type = ui_sub_row_create(body, ui_lang->tool_type, NULL);

    lv_obj_t *type_cont = lv_obj_create(r_type);
    lv_obj_remove_style_all(type_cont);
    lv_obj_remove_flag(type_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(type_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(type_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(type_cont, 4, 0);
    lv_obj_align(type_cont, LV_ALIGN_LEFT_MID, 0, 0);

    const char *type_labels[2] = { ui_lang->tool_iron, ui_lang->tool_gun };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_obj_create(type_cont);
        lv_obj_set_size(btn, 110, 32);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        uintptr_t ud = ((uintptr_t)ch << 8) | (uintptr_t)i;
        lv_obj_add_event_cb(btn, type_btn_cb, LV_EVENT_CLICKED, (void *)ud);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, type_labels[i]);
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        s_type_btns[ch][i] = btn;
    }
    refresh_type_btn_styles(ch);

    /* ── Preset rows ── */
    static const char *preset_names[4] = { "P1", "P2", "P3", "P4" };
    uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;

    for (int i = 0; i < 4; i++) {
        char val_buf[16];
        snprintf(val_buf, sizeof(val_buf), "%d", to_disp((int)presets[i]));

        uintptr_t ud_m = ((uintptr_t)ch << 12) | ((uintptr_t)i << 8) | 0;
        uintptr_t ud_p = ((uintptr_t)ch << 12) | ((uintptr_t)i << 8) | 1;

        s_preset_lbl[ch][i] = make_adj_row(body, preset_names[i], val_buf,
                                           preset_adj_cb,
                                           (void *)ud_m, (void *)ud_p);
    }

    /* ── Airflow row (shown only when gun type is active) ── */
    {
        uint8_t af = (ch == 0) ? g_settings.ch1_airflow : g_settings.ch2_airflow;

        /* ui_sub_row_create returns the 'right' container; its parent is the actual row. */
        lv_obj_t *af_right = ui_sub_row_create(body, ui_lang->airflow, NULL);
        s_af_row[ch] = lv_obj_get_parent(af_right);   /* the actual row in body */

        lv_obj_t *cont = lv_obj_create(af_right);
        lv_obj_remove_style_all(cont);
        lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(cont, 8, 0);
        lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);

        uintptr_t ud_m = ((uintptr_t)ch << 8) | 0;
        uintptr_t ud_p = ((uintptr_t)ch << 8) | 1;

        /* − button */
        lv_obj_t *btn_m = lv_obj_create(cont);
        lv_obj_set_size(btn_m, 36, 36);
        lv_obj_remove_flag(btn_m, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn_m, 6, 0);
        lv_obj_set_style_border_width(btn_m, 0, 0);
        lv_obj_set_style_bg_color(btn_m, ui_color_border(), 0);
        lv_obj_set_style_pad_all(btn_m, 0, 0);
        lv_obj_add_flag(btn_m, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_m, af_adj_cb, LV_EVENT_CLICKED, (void *)ud_m);
        lv_obj_t *lm = lv_label_create(btn_m);
        lv_label_set_text(lm, "-");
        lv_obj_set_style_text_font(lm, &roboto_cyrillic_16, 0);
        lv_obj_set_style_text_color(lm, ui_color_text_primary(), 0);
        lv_obj_align(lm, LV_ALIGN_CENTER, 0, 0);

        /* Value label */
        lv_obj_t *val = lv_label_create(cont);
        lv_label_set_text_fmt(val, "%d%%", af);
        lv_obj_set_style_text_font(val, &roboto_cyrillic_16, 0);
        lv_obj_set_style_text_color(val, ui_color_text_primary(), 0);
        lv_obj_set_width(val, 56);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);
        s_af_lbl[ch] = val;

        /* + button */
        lv_obj_t *btn_p = lv_obj_create(cont);
        lv_obj_set_size(btn_p, 36, 36);
        lv_obj_remove_flag(btn_p, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn_p, 6, 0);
        lv_obj_set_style_border_width(btn_p, 0, 0);
        lv_obj_set_style_bg_color(btn_p, ui_color_border(), 0);
        lv_obj_set_style_pad_all(btn_p, 0, 0);
        lv_obj_add_flag(btn_p, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_p, af_adj_cb, LV_EVENT_CLICKED, (void *)ud_p);
        lv_obj_t *lp = lv_label_create(btn_p);
        lv_label_set_text(lp, "+");
        lv_obj_set_style_text_font(lp, &roboto_cyrillic_16, 0);
        lv_obj_set_style_text_color(lp, ui_color_text_primary(), 0);
        lv_obj_align(lp, LV_ALIGN_CENTER, 0, 0);

        refresh_af_row_visibility(ch);
    }

    /* ── Channel color row ── */
    {
        lv_obj_t *row = ui_sub_row_create(body, ui_lang->ch_color, NULL);

        lv_obj_t *cont = lv_obj_create(row);
        lv_obj_remove_style_all(cont);
        lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(cont, SWATCH_GAP, 0);
        lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);

        for (int i = 0; i < CH_COLOR_COUNT; i++) {
            lv_obj_t *sw = lv_obj_create(cont);
            lv_obj_remove_flag(sw, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_size(sw, SWATCH_SZ, SWATCH_SZ);
            lv_obj_set_style_radius(sw, 6, 0);
            lv_obj_set_style_bg_color(sw, CH_COLORS[i], 0);
            lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
            lv_obj_set_style_pad_all(sw, 0, 0);
            lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
            uintptr_t ud = ((uintptr_t)ch << 8) | (uintptr_t)i;
            lv_obj_add_event_cb(sw, color_swatch_cb, LV_EVENT_CLICKED, (void *)ud);
            s_color_swatches[ch][i] = sw;
        }
        refresh_color_swatches(ch);
    }
}

/* ------------------------------------------------------------------ */
/*  Public entry points                                                 */
/* ------------------------------------------------------------------ */

void ui_ch1_settings_screen_create(void)
{
    /* Clear stale handles for ch0 */
    memset(s_preset_lbl[0],      0, sizeof(s_preset_lbl[0]));
    memset(s_type_btns[0],       0, sizeof(s_type_btns[0]));
    memset(s_color_swatches[0],  0, sizeof(s_color_swatches[0]));
    s_af_lbl[0] = NULL;
    s_af_row[0] = NULL;

    create_ch_settings_screen(&scr_ch1_settings, 0);
}

void ui_ch2_settings_screen_create(void)
{
    memset(s_preset_lbl[1],      0, sizeof(s_preset_lbl[1]));
    memset(s_type_btns[1],       0, sizeof(s_type_btns[1]));
    memset(s_color_swatches[1],  0, sizeof(s_color_swatches[1]));
    s_af_lbl[1] = NULL;
    s_af_row[1] = NULL;

    create_ch_settings_screen(&scr_ch2_settings, 1);
}
