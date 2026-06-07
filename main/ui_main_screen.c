#include "fonts/fonts.h"
#include "ui_main_screen.h"
#include "ui_init.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Layout constants                                                   */
/* ------------------------------------------------------------------ */
#define LCD_W          1024
#define LCD_H          600
#define TB_H           40
#define CH_W           (LCD_W / 2)    /* 512 */
#define CH_H           (LCD_H - TB_H) /* 560 */

#define CH_HDR_H       50   /* channel header height   */
#define PRESET_BAR_H   66   /* preset bar height       */
#define CTRL_BTN_SZ    64   /* ±button / value box     */
#define PRESET_BTN_W   CTRL_BTN_SZ  /* square */
#define PRESET_BTN_H   CTRL_BTN_SZ
#define EN_BTN_W       CTRL_BTN_SZ  /* square, pushed to right by spacer */

#define PWR_BAR_H      14   /* horizontal power bar height */

#define SP_MIN  100
#define SP_MAX  450
#define AF_MIN   10
#define AF_MAX  100

static int to_disp(int c) { return g_settings.units ? c * 9 / 5 + 32 : c; }

/* ------------------------------------------------------------------ */
/*  Per-channel state                                                  */
/* ------------------------------------------------------------------ */
typedef struct {
    lv_obj_t *lbl_temp;
    lv_obj_t *lbl_sp;
    lv_obj_t *lbl_af;
    lv_obj_t *power_cover;
    lv_obj_t *lbl_pwr;
    lv_obj_t *preset_btns[4];
    lv_obj_t *ch_en_btn;
    lv_obj_t *profile_dd;
    int        dd_map[MAX_PROFILES]; /* dd_map[i] = g_profiles index for list item i+1 */
    int        dd_map_cnt;
    int        mock_pwr;
} ch_state_t;

static ch_state_t s_ch[2];
static lv_obj_t  *lbl_time;
static lv_obj_t  *lbl_unit_ind;
static lv_obj_t  *lbl_bell;
static int        s_clock_sec = 43200;

/* ------------------------------------------------------------------ */
/*  Preset highlight                                                   */
/* ------------------------------------------------------------------ */
static void update_ch_en_btn(int ch)
{
    lv_obj_t *btn = s_ch[ch].ch_en_btn;
    if (!btn) return;
    bool en = (ch == 0) ? g_settings.ch1_en : g_settings.ch2_en;
    lv_color_t ch_col = (ch == 0) ? ui_color_ch1() : ui_color_ch2();
    lv_obj_set_style_bg_color(btn, en ? ch_col : ui_color_border(), 0);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl)
        lv_obj_set_style_text_color(lbl, en ? lv_color_white() : ui_color_text_primary(), 0);
}

static void update_preset_highlight(int ch)
{
    uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;
    uint16_t  sp      = (ch == 0) ? g_settings.ch1_sp      : g_settings.ch2_sp;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = s_ch[ch].preset_btns[i];
        if (!btn) continue;
        bool active = (presets[i] == sp);
        lv_color_t ch_col = (ch == 0) ? ui_color_ch1() : ui_color_ch2();
        lv_obj_set_style_bg_color(btn, active ? ch_col : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        if (lbl) {
            lv_label_set_text_fmt(lbl, "%d", to_disp((int)presets[i]));
            lv_obj_set_style_text_color(lbl,
                active ? lv_color_white() : ui_color_text_primary(), 0);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Label refresh                                                      */
/* ------------------------------------------------------------------ */
static void refresh_sp_label(int ch)
{
    if (!s_ch[ch].lbl_sp) return;
    uint16_t sp = (ch == 0) ? g_settings.ch1_sp : g_settings.ch2_sp;
    lv_label_set_text_fmt(s_ch[ch].lbl_sp, "%d", to_disp((int)sp));
}

static void refresh_af_label(int ch)
{
    if (!s_ch[ch].lbl_af) return;
    uint8_t af = (ch == 0) ? g_settings.ch1_airflow : g_settings.ch2_airflow;
    lv_label_set_text_fmt(s_ch[ch].lbl_af, "%d%%", af);
}

/* ------------------------------------------------------------------ */
/*  Callbacks                                                          */
/* ------------------------------------------------------------------ */
static void on_sp_minus(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    uint16_t *sp = (ch == 0) ? &g_settings.ch1_sp : &g_settings.ch2_sp;
    uint16_t step = g_settings.temp_step ? g_settings.temp_step : 1;
    if (*sp > SP_MIN + step - 1) { *sp -= step; refresh_sp_label(ch); update_preset_highlight(ch); ui_nvs_save_debounced(); }
    else if (*sp > SP_MIN)       { *sp  = SP_MIN; refresh_sp_label(ch); update_preset_highlight(ch); ui_nvs_save_debounced(); }
}
static void on_sp_plus(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    uint16_t *sp = (ch == 0) ? &g_settings.ch1_sp : &g_settings.ch2_sp;
    uint16_t step = g_settings.temp_step ? g_settings.temp_step : 1;
    if (*sp + step <= SP_MAX) { *sp += step; refresh_sp_label(ch); update_preset_highlight(ch); ui_nvs_save_debounced(); }
    else if (*sp < SP_MAX)    { *sp  = SP_MAX; refresh_sp_label(ch); update_preset_highlight(ch); ui_nvs_save_debounced(); }
}
static void on_af_minus(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    uint8_t *af = (ch == 0) ? &g_settings.ch1_airflow : &g_settings.ch2_airflow;
    if (*af > AF_MIN) { *af -= 5; refresh_af_label(ch); ui_nvs_save_debounced(); }
}
static void on_af_plus(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    uint8_t *af = (ch == 0) ? &g_settings.ch1_airflow : &g_settings.ch2_airflow;
    if (*af < AF_MAX) { *af += 5; refresh_af_label(ch); ui_nvs_save_debounced(); }
}
static void on_preset(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int ch  = (int)(data >> 8);
    int idx = (int)(data & 0xFF);
    uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;
    uint16_t *sp      = (ch == 0) ? &g_settings.ch1_sp     : &g_settings.ch2_sp;
    *sp = presets[idx];
    refresh_sp_label(ch);
    update_preset_highlight(ch);
    ui_nvs_save_debounced();
}
static void on_ch_toggle(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    uint8_t *en = (ch == 0) ? &g_settings.ch1_en : &g_settings.ch2_en;
    *en = !*en;
    update_ch_en_btn(ch);
    ui_nvs_save_debounced();
}

static void on_open_settings(lv_event_t *e)
{
    (void)e;
    lv_screen_load_anim(scr_settings, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false);
}


/* ------------------------------------------------------------------ */
/*  Clock timer                                                        */
/* ------------------------------------------------------------------ */
static void timer_clock_cb(lv_timer_t *t)
{
    (void)t;
    s_clock_sec++;
    if (s_clock_sec >= 86400) s_clock_sec = 0;
    if (!lbl_time) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             s_clock_sec/3600, (s_clock_sec%3600)/60, s_clock_sec%60);
    lv_label_set_text(lbl_time, buf);
}

/* ------------------------------------------------------------------ */
/*  Titlebar                                                           */
/* ------------------------------------------------------------------ */
static void create_titlebar(lv_obj_t *scr)
{
    lv_obj_t *tb = lv_obj_create(scr);
    lv_obj_remove_flag(tb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(tb, LCD_W, TB_H);
    lv_obj_align(tb, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(tb, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tb, 0, 0);
    lv_obj_set_style_border_width(tb, 0, 0);
    lv_obj_set_style_border_color(tb, ui_color_border(), 0);
    lv_obj_set_style_border_side(tb, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(tb, 1, 0);
    lv_obj_set_style_pad_all(tb, 0, 0);

    lbl_time = lv_label_create(tb);
    lv_label_set_text(lbl_time, "12:00:00");
    lv_obj_set_style_text_color(lbl_time, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(lbl_time, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, 0);

    lbl_unit_ind = lv_label_create(tb);
    lv_label_set_text(lbl_unit_ind, g_settings.units == 0 ? "\xc2\xb0""C" : "\xc2\xb0""F");
    lv_obj_set_style_text_color(lbl_unit_ind, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_unit_ind, &roboto_cyrillic_16, 0);
    lv_obj_align(lbl_unit_ind, LV_ALIGN_LEFT_MID, 12, 0);

    lbl_bell = lv_label_create(tb);
    lv_label_set_text(lbl_bell, LV_SYMBOL_BELL);
    lv_obj_align(lbl_bell, LV_ALIGN_RIGHT_MID, -54, 0);
    ui_main_screen_update_sound();

    lv_obj_t *btn_gear = lv_button_create(tb);
    lv_obj_set_size(btn_gear, 40, 32);
    lv_obj_align(btn_gear, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_style_bg_color(btn_gear, ui_color_border(), 0);
    lv_obj_set_style_bg_opa(btn_gear, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_gear, 0, 0);
    lv_obj_set_style_radius(btn_gear, 4, 0);
    lv_obj_set_style_pad_all(btn_gear, 0, 0);
    lv_obj_set_style_shadow_width(btn_gear, 0, 0);
    lv_obj_add_event_cb(btn_gear, on_open_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_gear = lv_label_create(btn_gear);
    lv_label_set_text(lbl_gear, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(lbl_gear, lv_color_hex(0x7ec8e3), 0);
    lv_obj_align(lbl_gear, LV_ALIGN_CENTER, 0, 0);
}

/* ------------------------------------------------------------------ */
/*  Helper: strip all default container styles                        */
/* ------------------------------------------------------------------ */
static void obj_clear(lv_obj_t *o)
{
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
}

/* ------------------------------------------------------------------ */
/*  Helper: ± square button                                           */
/* ------------------------------------------------------------------ */
static lv_obj_t *make_pm_btn(lv_obj_t *parent, const char *sym,
                              lv_event_cb_t cb, void *ud)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, CTRL_BTN_SZ, CTRL_BTN_SZ);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, ui_color_border(), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, sym);
    lv_obj_set_style_text_font(lbl, &roboto_pm_32, 0);
    lv_obj_set_style_text_color(lbl, ui_color_text_primary(), 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

/* ------------------------------------------------------------------ */
/*  Control row  [-]  value  [+]                                      */
/*  Returns the value label (plain, no box).                          */
/* ------------------------------------------------------------------ */
static lv_obj_t *create_ctrl_row(lv_obj_t *body,
                                  lv_event_cb_t cb_m, lv_event_cb_t cb_p,
                                  void *ud, const char *val_text,
                                  lv_color_t val_color, const lv_font_t *val_font)
{
    lv_obj_t *row = lv_obj_create(body);
    obj_clear(row);
    lv_obj_set_size(row, LV_PCT(100), CTRL_BTN_SZ);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(row, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 0, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);

    make_pm_btn(row, "-", cb_m, ud);

    lv_obj_t *lbl_val = lv_label_create(row);
    lv_label_set_text(lbl_val, val_text);
    lv_obj_set_style_text_color(lbl_val, val_color, 0);
    lv_obj_set_style_text_font(lbl_val, val_font, 0);

    make_pm_btn(row, "+", cb_p, ud);

    return lbl_val;
}

/* ------------------------------------------------------------------ */
/*  Control cell: title label + ctrl row in a flex-column box.         */
/*  Returns the cell object; *out_lbl receives the value label.        */
/*  Caller sets the cell width (flex_grow or explicit).                */
/* ------------------------------------------------------------------ */
static lv_obj_t *make_ctrl_cell(lv_obj_t *parent, const char *title,
                                  lv_event_cb_t cb_m, lv_event_cb_t cb_p,
                                  void *ud, const char *val,
                                  lv_color_t color, const lv_font_t *font,
                                  lv_obj_t **out_lbl)
{
    lv_obj_t *cell = lv_obj_create(parent);
    obj_clear(cell);
    lv_obj_set_height(cell, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_cross_place(cell, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(cell, 4, 0);

    lv_obj_t *lbl = lv_label_create(cell);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, ui_color_text_muted(), 0);
    lv_obj_set_style_text_font(lbl, &roboto_cyrillic_12, 0);

    *out_lbl = create_ctrl_row(cell, cb_m, cb_p, ud, val, color, font);
    return cell;
}

/* ------------------------------------------------------------------ */
/*  Profile dropdown helpers                                           */
/* ------------------------------------------------------------------ */
static void build_dd_opts(char *buf, int sz, int ch, int *map, int *map_cnt)
{
    uint8_t ch_type = (ch == 0) ? g_settings.ch1_type : g_settings.ch2_type;
    uint8_t ch_sub  = (ch == 0) ? g_settings.ch1_iron_subtype : g_settings.ch2_iron_subtype;
    int pos = snprintf(buf, sz, "---");
    int cnt = 0;
    for (int i = 0; i < (int)g_profile_cnt && pos < sz - 2; i++) {
        if (g_profiles[i].tool_type != ch_type) continue;
        if (ch_type == CH_TYPE_IRON && g_profiles[i].iron_subtype != ch_sub) continue;
        buf[pos++] = '\n';
        int n = snprintf(buf + pos, sz - pos, "%s", g_profiles[i].name);
        if (n > 0) pos += n;
        if (map && cnt < MAX_PROFILES) map[cnt] = i;
        cnt++;
    }
    if (map_cnt) *map_cnt = cnt;
}

static void apply_profile_to_ch(int ch, int idx)
{
    if (idx < 0 || idx >= (int)g_profile_cnt) return;
    const ui_profile_t *p = &g_profiles[idx];
    if (ch == 0) {
        g_settings.ch1_pid_kp       = p->pid_kp;
        g_settings.ch1_pid_ki       = p->pid_ki;
        g_settings.ch1_pid_kd       = p->pid_kd;
        g_settings.ch1_temp_offset  = p->temp_offset;
        g_settings.ch1_iron_subtype = p->iron_subtype;
    } else {
        g_settings.ch2_pid_kp       = p->pid_kp;
        g_settings.ch2_pid_ki       = p->pid_ki;
        g_settings.ch2_pid_kd       = p->pid_kd;
        g_settings.ch2_temp_offset  = p->temp_offset;
        g_settings.ch2_iron_subtype = p->iron_subtype;
    }
    ui_nvs_save_debounced();
}

static void on_profile_dd(lv_event_t *e)
{
    int ch = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *dd = lv_event_get_target_obj(e);
    uint32_t sel = lv_dropdown_get_selected(dd);
    if (sel == 0) return;
    int map_idx = (int)sel - 1;
    if (map_idx >= s_ch[ch].dd_map_cnt) return;
    apply_profile_to_ch(ch, s_ch[ch].dd_map[map_idx]);
}

/* ------------------------------------------------------------------ */
/*  Channel panel                                                      */
/*                                                                     */
/*  Flex column layout:                                                */
/*    [header  CH_HDR_H px]                                           */
/*    [divider 1 px       ]                                           */
/*    [body    flex_grow=1]  ← contains temp, unit, controls          */
/*    [preset  PRESET_BAR_H] ← 4 preset buttons                      */
/*                                                                     */
/*  Power bar: LV_OBJ_FLAG_FLOATING on panel, aligned RIGHT_MID       */
/*  with y-offset = -8 to center within the body area.                */
/* ------------------------------------------------------------------ */
static void create_channel_panel(lv_obj_t *scr, int ch)
{
    uint8_t    ch_type  = (ch == 0) ? g_settings.ch1_type : g_settings.ch2_type;
    lv_color_t ch_color = (ch == 0) ? ui_color_ch1() : ui_color_ch2();
    void      *ud = (void *)(uintptr_t)ch;

    /* ── Panel (flex column) ── */
    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(panel, CH_W, CH_H);
    lv_obj_align(panel, (ch == 0) ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_RIGHT, 0, TB_H);
    lv_obj_set_style_bg_color(panel, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 0, 0);

    /* ── Header ── */
    lv_obj_t *hdr = lv_obj_create(panel);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(hdr, LV_PCT(100), CH_HDR_H);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    /* Channel name — left */
    lv_obj_t *lbl_name = lv_label_create(hdr);
    lv_label_set_text(lbl_name, (ch == 0) ? ui_lang->channel_1 : ui_lang->channel_2);
    lv_obj_set_style_text_color(lbl_name, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_name, &roboto_cyrillic_12, 0);
    lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 12, 0);

    /* Tool type / subtype — right */
    {
        char type_buf[24];
        if (ch_type == CH_TYPE_GUN) {
            snprintf(type_buf, sizeof(type_buf), "%s", ui_lang->tool_gun);
        } else {
            uint8_t sub = (ch == 0) ? g_settings.ch1_iron_subtype : g_settings.ch2_iron_subtype;
            if (sub >= IRON_SUBTYPE_COUNT) sub = 0;
            snprintf(type_buf, sizeof(type_buf), "%s", IRON_SUBTYPE_NAMES[sub]);
        }
        lv_obj_t *lbl_type = lv_label_create(hdr);
        lv_label_set_text(lbl_type, type_buf);
        lv_obj_set_style_text_color(lbl_type, ch_color, 0);
        lv_obj_set_style_text_font(lbl_type, &roboto_cyrillic_12, 0);
        lv_obj_align(lbl_type, LV_ALIGN_RIGHT_MID, -12, 0);
    }

    /* Profile dropdown — centered in header */
    {
        char dd_opts[MAX_PROFILES * (PROFILE_NAME_LEN + 1) + 8];
        build_dd_opts(dd_opts, sizeof(dd_opts), ch,
                      s_ch[ch].dd_map, &s_ch[ch].dd_map_cnt);

        lv_obj_t *dd = lv_dropdown_create(hdr);
        lv_dropdown_set_options(dd, dd_opts);
        lv_obj_set_size(dd, 220, 32);
        lv_obj_align(dd, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(dd, on_profile_dd, LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)ch);

        lv_obj_set_style_bg_color(dd, ui_color_surface(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(dd, ui_color_border(), LV_PART_MAIN);
        lv_obj_set_style_border_width(dd, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(dd, 6, LV_PART_MAIN);
        lv_obj_set_style_text_color(dd, ui_color_text_primary(), LV_PART_MAIN);
        lv_obj_set_style_text_font(dd, &roboto_cyrillic_14, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(dd, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(dd, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(dd, 0, LV_PART_MAIN);
        lv_obj_set_style_text_color(dd, ui_color_text_secondary(), LV_PART_INDICATOR);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, LV_PART_INDICATOR);

        lv_obj_t *list = lv_dropdown_get_list(dd);
        if (list) {
            lv_obj_set_style_bg_color(list, ui_color_surface(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(list, ui_color_border(), LV_PART_MAIN);
            lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
            lv_obj_set_style_text_color(list, ui_color_text_primary(), LV_PART_MAIN);
            lv_obj_set_style_text_font(list, &roboto_cyrillic_14, LV_PART_MAIN);
            lv_obj_set_style_bg_color(list, ui_color_accent(), LV_PART_SELECTED);
            lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
            lv_obj_set_style_text_color(list, lv_color_white(), LV_PART_SELECTED);
        }

        s_ch[ch].profile_dd = dd;
    }

    /* ── Header divider (1 px) ── */
    lv_obj_t *div_h = lv_obj_create(panel);
    obj_clear(div_h);
    lv_obj_set_size(div_h, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(div_h, ui_color_border(), 0);
    lv_obj_set_style_bg_opa(div_h, LV_OPA_COVER, 0);

    /* ── Body (flex grow, column, SPACE_BETWEEN) ── */
    lv_obj_t *body = lv_obj_create(panel);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_height(body, 0);      /* height determined by flex_grow */
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_radius(body, 0, 0);
    lv_obj_set_style_pad_hor(body,    16, 0);
    lv_obj_set_style_pad_top(body,    10, 0);
    lv_obj_set_style_pad_bottom(body, 10, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(body, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_flex_cross_place(body, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(body, 0, 0);

    /* ── Power bar: full body width, % overlaid as floating label ── */
    lv_obj_t *pwr_row = lv_obj_create(body);
    obj_clear(pwr_row);
    lv_obj_set_size(pwr_row, LV_PCT(100), PWR_BAR_H);

    lv_obj_t *pwr_bar = lv_bar_create(pwr_row);
    lv_obj_set_size(pwr_bar, LV_PCT(100), PWR_BAR_H);
    lv_obj_align(pwr_bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(pwr_bar, ui_color_surface(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pwr_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(pwr_bar, PWR_BAR_H / 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(pwr_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(pwr_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(pwr_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(pwr_bar, lv_color_hex(0x1144ee), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(pwr_bar, lv_color_hex(0xdd2222), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(pwr_bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(pwr_bar, PWR_BAR_H / 2, LV_PART_INDICATOR);
    lv_bar_set_range(pwr_bar, 0, 100);
    lv_bar_set_value(pwr_bar, s_ch[ch].mock_pwr, LV_ANIM_OFF);
    s_ch[ch].power_cover = pwr_bar;

    lv_obj_t *lbl_pwr = lv_label_create(pwr_row);
    lv_obj_add_flag(lbl_pwr, LV_OBJ_FLAG_FLOATING);
    lv_label_set_text_fmt(lbl_pwr, "%d%%", s_ch[ch].mock_pwr);
    lv_obj_set_style_text_font(lbl_pwr, &roboto_cyrillic_12, 0);
    lv_obj_set_style_text_color(lbl_pwr, lv_color_white(), 0);
    lv_obj_align(lbl_pwr, LV_ALIGN_RIGHT_MID, -6, 0);
    s_ch[ch].lbl_pwr = lbl_pwr;

    /* ── Temperature — full width, centered ── */
    lv_obj_t *lbl_temp = lv_label_create(body);
    {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", to_disp((ch == 0) ? 250 : 300));
        lv_label_set_text(lbl_temp, buf);
    }
    lv_obj_set_style_text_color(lbl_temp, ch_color, 0);
    lv_obj_set_style_text_font(lbl_temp, &roboto_160, 0);
    lv_obj_set_width(lbl_temp, LV_PCT(100));
    lv_obj_set_style_text_align(lbl_temp, LV_TEXT_ALIGN_CENTER, 0);
    s_ch[ch].lbl_temp = lbl_temp;

    /* ── Controls ── */
    if (ch_type == CH_TYPE_GUN) {
        /* GUN: airflow + setpoint side by side; flex_grow=1 splits width evenly */
        lv_obj_t *ctrl_pair = lv_obj_create(body);
        obj_clear(ctrl_pair);
        lv_obj_set_size(ctrl_pair, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(ctrl_pair, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_flex_cross_place(ctrl_pair, LV_FLEX_ALIGN_START, 0);

        uint16_t sp = (ch == 0) ? g_settings.ch1_sp : g_settings.ch2_sp;
        char sp_buf[8];
        snprintf(sp_buf, sizeof(sp_buf), "%d", to_disp((int)sp));
        lv_obj_t *sp_cell = make_ctrl_cell(ctrl_pair, ui_lang->setpoint,
                                            on_sp_minus, on_sp_plus, ud,
                                            sp_buf, ch_color, &roboto_cyrillic_24,
                                            &s_ch[ch].lbl_sp);
        lv_obj_set_width(sp_cell, 0);
        lv_obj_set_flex_grow(sp_cell, 1);

        uint8_t af = (ch == 0) ? g_settings.ch1_airflow : g_settings.ch2_airflow;
        char af_buf[8];
        snprintf(af_buf, sizeof(af_buf), "%d%%", af);
        lv_obj_t *af_cell = make_ctrl_cell(ctrl_pair, ui_lang->airflow,
                                            on_af_minus, on_af_plus, ud,
                                            af_buf, ui_color_text_secondary(), &roboto_cyrillic_24,
                                            &s_ch[ch].lbl_af);
        lv_obj_set_width(af_cell, 0);
        lv_obj_set_flex_grow(af_cell, 1);
    } else {
        /* IRON: setpoint full width */
        uint16_t sp = (ch == 0) ? g_settings.ch1_sp : g_settings.ch2_sp;
        char sp_buf[8];
        snprintf(sp_buf, sizeof(sp_buf), "%d", to_disp((int)sp));
        lv_obj_t *sp_cell = make_ctrl_cell(body, ui_lang->setpoint,
                                            on_sp_minus, on_sp_plus, ud,
                                            sp_buf, ch_color, &roboto_cyrillic_24,
                                            &s_ch[ch].lbl_sp);
        lv_obj_set_width(sp_cell, LV_PCT(100));
    }

    /* ── Preset bar: 4 presets + 1 channel-enable button (2× wide) ── */
    lv_obj_t *pbar = lv_obj_create(panel);
    lv_obj_remove_flag(pbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(pbar, LV_PCT(100), PRESET_BAR_H);
    lv_obj_set_style_bg_opa(pbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pbar, 0, 0);
    lv_obj_set_style_radius(pbar, 0, 0);
    lv_obj_set_style_pad_hor(pbar, 16, 0);
    lv_obj_set_style_pad_ver(pbar, 0, 0);
    lv_obj_set_flex_flow(pbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(pbar, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_style_flex_cross_place(pbar, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(pbar, 8, 0);

    uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;
    uint16_t sp = (ch == 0) ? g_settings.ch1_sp : g_settings.ch2_sp;
    lv_color_t ch_col = (ch == 0) ? ui_color_ch1() : ui_color_ch2();
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_obj_create(pbar);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(btn, PRESET_BTN_W, PRESET_BTN_H);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        bool active = (presets[i] == sp);
        lv_obj_set_style_bg_color(btn, active ? ch_col : ui_color_border(), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        uintptr_t pud = ((uintptr_t)ch << 8) | (uintptr_t)i;
        lv_obj_add_event_cb(btn, on_preset, LV_EVENT_CLICKED, (void *)pud);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "%d", to_disp((int)presets[i]));
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
        lv_obj_set_style_text_color(lbl, active ? lv_color_white() : ui_color_text_primary(), 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        s_ch[ch].preset_btns[i] = btn;
    }

    /* Spacer pushes en_btn to the right */
    lv_obj_t *spacer = lv_obj_create(pbar);
    lv_obj_remove_style_all(spacer);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(spacer, 0, PRESET_BTN_H);
    lv_obj_set_flex_grow(spacer, 1);

    /* Channel enable/disable button — square, right side */
    lv_obj_t *en_btn = lv_obj_create(pbar);
    lv_obj_remove_flag(en_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(en_btn, EN_BTN_W, PRESET_BTN_H);
    lv_obj_set_style_radius(en_btn, 8, 0);
    lv_obj_set_style_border_width(en_btn, 0, 0);
    lv_obj_set_style_pad_all(en_btn, 0, 0);
    lv_obj_add_flag(en_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(en_btn, on_ch_toggle, LV_EVENT_CLICKED, ud);
    lv_obj_t *en_lbl = lv_label_create(en_btn);
    lv_label_set_text(en_lbl, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(en_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(en_lbl, lv_color_white(), 0);
    lv_obj_align(en_lbl, LV_ALIGN_CENTER, 0, 0);
    s_ch[ch].ch_en_btn = en_btn;
    update_ch_en_btn(ch);
}

/* ------------------------------------------------------------------ */
/*  Vertical channel divider                                           */
/* ------------------------------------------------------------------ */
static void create_divider(lv_obj_t *scr)
{
    lv_obj_t *div = lv_obj_create(scr);
    obj_clear(div);
    lv_obj_set_size(div, 1, CH_H);
    lv_obj_align(div, LV_ALIGN_TOP_MID, 0, TB_H);
    lv_obj_set_style_bg_color(div, ui_color_border(), 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void ui_main_screen_update_sound(void)
{
    if (!lbl_bell) return;
    bool muted = !g_settings.sound_en;
    lv_obj_set_style_text_color(lbl_bell,
        muted ? lv_color_hex(0x555555) : lv_color_hex(0x7ec8e3), 0);
    lv_obj_set_style_text_decor(lbl_bell,
        muted ? LV_TEXT_DECOR_STRIKETHROUGH : LV_TEXT_DECOR_NONE, 0);
}

void ui_main_screen_prepare_destroy(void)
{
    lbl_time     = NULL;
    lbl_unit_ind = NULL;
    lbl_bell     = NULL;
    memset(s_ch, 0, sizeof(s_ch)); /* clears preset_btns, ch_en_btn, etc. */
}

void ui_main_screen_refresh_sp(void)
{
    for (int ch = 0; ch < 2; ch++) {
        refresh_sp_label(ch);
        update_preset_highlight(ch);
        update_ch_en_btn(ch);
    }
}

void ui_main_screen_refresh_presets(int ch)
{
    update_preset_highlight(ch);
}

void ui_main_screen_update_ch_color(int ch)
{
    lv_color_t col = (ch == 0) ? ui_color_ch1() : ui_color_ch2();
    if (s_ch[ch].lbl_temp)
        lv_obj_set_style_text_color(s_ch[ch].lbl_temp, col, 0);
    if (s_ch[ch].lbl_sp)
        lv_obj_set_style_text_color(s_ch[ch].lbl_sp, col, 0);
    update_preset_highlight(ch);
    update_ch_en_btn(ch);
}

void ui_main_screen_update_units(void)
{
    if (!lbl_unit_ind) return;
    lv_label_set_text(lbl_unit_ind, g_settings.units == 0 ? "\xc2\xb0""C" : "\xc2\xb0""F");
    for (int ch = 0; ch < 2; ch++) {
        if (s_ch[ch].lbl_temp)
            lv_label_set_text_fmt(s_ch[ch].lbl_temp, "%d",
                                  to_disp((ch == 0) ? 250 : 300));
        refresh_sp_label(ch);
        uint16_t *presets = (ch == 0) ? g_settings.ch1_presets : g_settings.ch2_presets;
        for (int i = 0; i < 4; i++) {
            lv_obj_t *btn = s_ch[ch].preset_btns[i];
            if (!btn) continue;
            lv_obj_t *lbl = lv_obj_get_child(btn, 0);
            if (lbl)
                lv_label_set_text_fmt(lbl, "%d", to_disp((int)presets[i]));
        }
    }
}

void ui_main_screen_refresh_profile_dropdowns(void)
{
    char opts[MAX_PROFILES * (PROFILE_NAME_LEN + 1) + 8];
    for (int ch = 0; ch < 2; ch++) {
        lv_obj_t *dd = s_ch[ch].profile_dd;
        if (!dd) continue;
        build_dd_opts(opts, sizeof(opts), ch,
                      s_ch[ch].dd_map, &s_ch[ch].dd_map_cnt);
        lv_dropdown_set_options(dd, opts);
        lv_dropdown_set_selected(dd, 0);
    }
}

void ui_main_screen_create(void)
{
    s_ch[0].mock_pwr = 65;
    s_ch[1].mock_pwr = 40;

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    scr_main = scr;

    create_titlebar(scr);
    create_channel_panel(scr, 0);
    create_channel_panel(scr, 1);
    create_divider(scr);

    s_clock_sec = 43200;
    static lv_timer_t *s_clock_timer = NULL;
    if (!s_clock_timer)
        s_clock_timer = lv_timer_create(timer_clock_cb, 1000, NULL);
}
