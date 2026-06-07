#include "fonts/fonts.h"
#include "ui_profile_edit_screen.h"
#include "ui_profiles_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  State                                                               */
/* ------------------------------------------------------------------ */
static int            s_edit_idx;   /* -1 = new */
static ui_profile_t   s_buf;        /* working copy */

/* Value labels / widgets for live update */
static lv_obj_t *s_kp_lbl;
static lv_obj_t *s_ki_lbl;
static lv_obj_t *s_kd_lbl;
static lv_obj_t *s_off_lbl;
static lv_obj_t *s_ta;
static lv_obj_t *s_kb;
static lv_obj_t *s_type_btns[2];      /* [0]=Iron [1]=Gun */
static lv_obj_t *s_subtype_btns[IRON_SUBTYPE_COUNT];
static lv_obj_t *s_subtype_row;       /* row hidden when tool_type == GUN */
static lv_obj_t *s_apply_btn[2];      /* Apply CH1, Apply CH2 */

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */
static void refresh_type_ui(void)
{
    /* Tool type toggle buttons */
    for (int i = 0; i < 2; i++) {
        if (!s_type_btns[i]) continue;
        bool sel = ((int)s_buf.tool_type == i);
        lv_obj_set_style_bg_color(s_type_btns[i], sel ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_type_btns[i], 0);
        if (lbl) lv_obj_set_style_text_color(lbl,
            sel ? lv_color_white() : ui_color_text_secondary(), 0);
    }
    /* Iron subtype row visibility */
    if (s_subtype_row) {
        if (s_buf.tool_type == CH_TYPE_IRON)
            lv_obj_remove_flag(s_subtype_row, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(s_subtype_row, LV_OBJ_FLAG_HIDDEN);
    }
    /* Iron subtype buttons */
    for (int i = 0; i < IRON_SUBTYPE_COUNT; i++) {
        if (!s_subtype_btns[i]) continue;
        bool sel = ((int)s_buf.iron_subtype == i);
        lv_obj_set_style_bg_color(s_subtype_btns[i], sel ? ui_color_accent() : ui_color_border(), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_subtype_btns[i], 0);
        if (lbl) lv_obj_set_style_text_color(lbl,
            sel ? lv_color_white() : ui_color_text_secondary(), 0);
    }
    /* Apply buttons: disabled if channel type mismatches profile type */
    for (int ch = 0; ch < 2; ch++) {
        if (!s_apply_btn[ch]) continue;
        uint8_t ch_type = (ch == 0) ? g_settings.ch1_type : g_settings.ch2_type;
        bool ok = (ch_type == s_buf.tool_type);
        if (ok) {
            lv_obj_remove_state(s_apply_btn[ch], LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(s_apply_btn[ch],
                (ch == 0) ? ui_color_ch1() : ui_color_ch2(), 0);
        } else {
            lv_obj_add_state(s_apply_btn[ch], LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(s_apply_btn[ch], ui_color_border(), 0);
        }
    }
}

/* Make a small ± adjustment row, returns value label */
static lv_obj_t *make_adj(lv_obj_t *body, const char *title,
                           lv_event_cb_t cb, void *ud_m, void *ud_p,
                           const char *init_val)
{
    lv_obj_t *row = lv_obj_create(body);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(row, ui_color_surface(), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 12, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lbl, ui_color_text_secondary(), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* right side: [−] val [+] */
    lv_obj_t *cont = lv_obj_create(row);
    lv_obj_remove_style_all(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(cont, 8, 0);
    lv_obj_align(cont, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *bm = lv_obj_create(cont);
    lv_obj_set_size(bm, 36, 36);
    lv_obj_remove_flag(bm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bm, 6, 0);
    lv_obj_set_style_border_width(bm, 0, 0);
    lv_obj_set_style_bg_color(bm, ui_color_border(), 0);
    lv_obj_set_style_pad_all(bm, 0, 0);
    lv_obj_add_flag(bm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bm, cb, LV_EVENT_CLICKED, ud_m);
    lv_obj_t *lm = lv_label_create(bm);
    lv_label_set_text(lm, "-");
    lv_obj_set_style_text_font(lm, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lm, ui_color_text_primary(), 0);
    lv_obj_align(lm, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *val = lv_label_create(cont);
    lv_label_set_text(val, init_val);
    lv_obj_set_style_text_font(val, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(val, ui_color_text_primary(), 0);
    lv_obj_set_width(val, 56);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *bp = lv_obj_create(cont);
    lv_obj_set_size(bp, 36, 36);
    lv_obj_remove_flag(bp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bp, 6, 0);
    lv_obj_set_style_border_width(bp, 0, 0);
    lv_obj_set_style_bg_color(bp, ui_color_border(), 0);
    lv_obj_set_style_pad_all(bp, 0, 0);
    lv_obj_add_flag(bp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bp, cb, LV_EVENT_CLICKED, ud_p);
    lv_obj_t *lp = lv_label_create(bp);
    lv_label_set_text(lp, "+");
    lv_obj_set_style_text_font(lp, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lp, ui_color_text_primary(), 0);
    lv_obj_align(lp, LV_ALIGN_CENTER, 0, 0);

    return val;
}

/* ------------------------------------------------------------------ */
/*  Callbacks                                                           */
/* ------------------------------------------------------------------ */
#define CLAMP(v,lo,hi) do { if ((v)<(lo)) (v)=(lo); else if ((v)>(hi)) (v)=(hi); } while(0)

static void kp_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    int v = (int)s_buf.pid_kp + d;
    CLAMP(v, 0, 99);
    s_buf.pid_kp = (uint8_t)v;
    if (s_kp_lbl) lv_label_set_text_fmt(s_kp_lbl, "%d", s_buf.pid_kp);
}
static void ki_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    int v = (int)s_buf.pid_ki + d;
    CLAMP(v, 0, 99);
    s_buf.pid_ki = (uint8_t)v;
    if (s_ki_lbl) lv_label_set_text_fmt(s_ki_lbl, "%d", s_buf.pid_ki);
}
static void kd_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    int v = (int)s_buf.pid_kd + d;
    CLAMP(v, 0, 99);
    s_buf.pid_kd = (uint8_t)v;
    if (s_kd_lbl) lv_label_set_text_fmt(s_kd_lbl, "%d", s_buf.pid_kd);
}
static void off_cb(lv_event_t *e) {
    int d = (int)(intptr_t)lv_event_get_user_data(e);
    int v = (int)s_buf.temp_offset + d;
    CLAMP(v, -20, 20);
    s_buf.temp_offset = (int8_t)v;
    if (s_off_lbl) lv_label_set_text_fmt(s_off_lbl, "%+d", (int)s_buf.temp_offset);
}

static void type_btn_cb(lv_event_t *e)
{
    int t = (int)(intptr_t)lv_event_get_user_data(e);
    s_buf.tool_type = (uint8_t)t;
    refresh_type_ui();
}

static void subtype_btn_cb(lv_event_t *e)
{
    int st = (int)(intptr_t)lv_event_get_user_data(e);
    s_buf.iron_subtype = (uint8_t)st;
    refresh_type_ui();
}

static void save_cb(lv_event_t *e)
{
    (void)e;
    /* sync name from textarea */
    if (s_ta) {
        const char *txt = lv_textarea_get_text(s_ta);
        strncpy(s_buf.name, txt, PROFILE_NAME_LEN - 1);
        s_buf.name[PROFILE_NAME_LEN - 1] = '\0';
    }
    if (s_edit_idx < 0) {
        /* new profile */
        if (g_profile_cnt < MAX_PROFILES) {
            g_profiles[g_profile_cnt] = s_buf;
            g_profile_cnt++;
        }
    } else {
        g_profiles[s_edit_idx] = s_buf;
    }
    ui_nvs_save_profiles();
    if (scr_profiles) { lv_obj_delete(scr_profiles); scr_profiles = NULL; }
    ui_profiles_screen_create();
    lv_screen_load(scr_profiles);
}

static void delete_cb(lv_event_t *e)
{
    (void)e;
    if (s_edit_idx >= 0 && s_edit_idx < (int)g_profile_cnt) {
        for (int i = s_edit_idx; i < (int)g_profile_cnt - 1; i++)
            g_profiles[i] = g_profiles[i + 1];
        g_profile_cnt--;
        ui_nvs_save_profiles();
    }
    if (scr_profiles) { lv_obj_delete(scr_profiles); scr_profiles = NULL; }
    ui_profiles_screen_create();
    lv_screen_load(scr_profiles);
}

static void apply_ch_cb(lv_event_t *e)
{
    int ch = (int)(intptr_t)lv_event_get_user_data(e);
    if (ch == 0) {
        g_settings.ch1_pid_kp       = s_buf.pid_kp;
        g_settings.ch1_pid_ki       = s_buf.pid_ki;
        g_settings.ch1_pid_kd       = s_buf.pid_kd;
        g_settings.ch1_temp_offset  = s_buf.temp_offset;
        g_settings.ch1_iron_subtype = s_buf.iron_subtype;
    } else {
        g_settings.ch2_pid_kp       = s_buf.pid_kp;
        g_settings.ch2_pid_ki       = s_buf.pid_ki;
        g_settings.ch2_pid_kd       = s_buf.pid_kd;
        g_settings.ch2_temp_offset  = s_buf.temp_offset;
        g_settings.ch2_iron_subtype = s_buf.iron_subtype;
    }
    ui_nvs_save_debounced();
}

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (!s_kb) return;
    if (code == LV_EVENT_FOCUSED)
        lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    else if (code == LV_EVENT_DEFOCUSED)
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(s_ta, LV_STATE_FOCUSED);
    }
}

/* ------------------------------------------------------------------ */
/*  Screen builder                                                      */
/* ------------------------------------------------------------------ */
static void build_screen(void)
{
    lv_obj_t *body = ui_sub_screen_create2(&scr_profile_edit,
                                            s_edit_idx < 0 ? ui_lang->add_profile
                                                           : ui_lang->edit_profile,
                                            &scr_profiles);

    /* Name textarea */
    lv_obj_t *name_row = lv_obj_create(body);
    lv_obj_remove_style_all(name_row);
    lv_obj_set_size(name_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_remove_flag(name_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(name_row, ui_color_surface(), 0);
    lv_obj_set_style_bg_opa(name_row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(name_row, 8, 0);
    lv_obj_set_style_pad_all(name_row, 12, 0);

    lv_obj_t *name_lbl = lv_label_create(name_row);
    lv_label_set_text(name_lbl, ui_lang->profile_name);
    lv_obj_set_style_text_font(name_lbl, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(name_lbl, ui_color_text_secondary(), 0);
    lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *ta = lv_textarea_create(name_row);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, PROFILE_NAME_LEN - 1);
    lv_textarea_set_text(ta, s_buf.name);
    lv_obj_set_width(ta, 320);
    lv_obj_align(ta, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(ta, ui_color_surface(), 0);
    lv_obj_set_style_border_color(ta, ui_color_border(), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 6, 0);
    lv_obj_set_style_text_color(ta, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(ta, &roboto_cyrillic_16, 0);
    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, NULL);
    s_ta = ta;

    /* Tool type row */
    {
        lv_obj_t *type_row = lv_obj_create(body);
        lv_obj_remove_style_all(type_row);
        lv_obj_set_size(type_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_remove_flag(type_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(type_row, ui_color_surface(), 0);
        lv_obj_set_style_bg_opa(type_row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(type_row, 8, 0);
        lv_obj_set_style_pad_all(type_row, 12, 0);

        lv_obj_t *tlbl = lv_label_create(type_row);
        lv_label_set_text(tlbl, ui_lang->tool_type);
        lv_obj_set_style_text_font(tlbl, &roboto_cyrillic_16, 0);
        lv_obj_set_style_text_color(tlbl, ui_color_text_secondary(), 0);
        lv_obj_align(tlbl, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *tcont = lv_obj_create(type_row);
        lv_obj_remove_style_all(tcont);
        lv_obj_remove_flag(tcont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(tcont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(tcont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(tcont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(tcont, 8, 0);
        lv_obj_align(tcont, LV_ALIGN_RIGHT_MID, 0, 0);

        const char *tlabels[2] = { ui_lang->tool_iron, ui_lang->tool_gun };
        for (int i = 0; i < 2; i++) {
            lv_obj_t *tb = lv_obj_create(tcont);
            lv_obj_set_size(tb, LV_SIZE_CONTENT, 36);
            lv_obj_remove_flag(tb, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(tb, 6, 0);
            lv_obj_set_style_border_width(tb, 0, 0);
            lv_obj_set_style_bg_color(tb, ui_color_border(), 0);
            lv_obj_set_style_pad_hor(tb, 14, 0);
            lv_obj_set_style_pad_ver(tb, 0, 0);
            lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(tb, type_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
            lv_obj_t *tl = lv_label_create(tb);
            lv_label_set_text(tl, tlabels[i]);
            lv_obj_set_style_text_font(tl, &roboto_cyrillic_16, 0);
            lv_obj_set_style_text_color(tl, ui_color_text_secondary(), 0);
            lv_obj_align(tl, LV_ALIGN_CENTER, 0, 0);
            s_type_btns[i] = tb;
        }
    }

    /* Iron subtype row */
    {
        lv_obj_t *srow = lv_obj_create(body);
        lv_obj_remove_style_all(srow);
        lv_obj_set_size(srow, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_remove_flag(srow, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(srow, ui_color_surface(), 0);
        lv_obj_set_style_bg_opa(srow, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(srow, 8, 0);
        lv_obj_set_style_pad_all(srow, 12, 0);
        lv_obj_set_flex_flow(srow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(srow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(srow, 8, 0);
        s_subtype_row = srow;

        for (int i = 0; i < IRON_SUBTYPE_COUNT; i++) {
            lv_obj_t *sb = lv_obj_create(srow);
            lv_obj_set_size(sb, LV_SIZE_CONTENT, 36);
            lv_obj_remove_flag(sb, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(sb, 6, 0);
            lv_obj_set_style_border_width(sb, 0, 0);
            lv_obj_set_style_bg_color(sb, ui_color_border(), 0);
            lv_obj_set_style_pad_hor(sb, 14, 0);
            lv_obj_set_style_pad_ver(sb, 0, 0);
            lv_obj_add_flag(sb, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(sb, subtype_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
            lv_obj_t *sl = lv_label_create(sb);
            lv_label_set_text(sl, IRON_SUBTYPE_NAMES[i]);
            lv_obj_set_style_text_font(sl, &roboto_cyrillic_16, 0);
            lv_obj_set_style_text_color(sl, ui_color_text_secondary(), 0);
            lv_obj_align(sl, LV_ALIGN_CENTER, 0, 0);
            s_subtype_btns[i] = sb;
        }
    }

    /* PID and offset rows */
    char buf[16];

    snprintf(buf, sizeof(buf), "%d", s_buf.pid_kp);
    s_kp_lbl = make_adj(body, ui_lang->pid_kp, kp_cb, (void*)(intptr_t)-1, (void*)(intptr_t)1, buf);

    snprintf(buf, sizeof(buf), "%d", s_buf.pid_ki);
    s_ki_lbl = make_adj(body, ui_lang->pid_ki, ki_cb, (void*)(intptr_t)-1, (void*)(intptr_t)1, buf);

    snprintf(buf, sizeof(buf), "%d", s_buf.pid_kd);
    s_kd_lbl = make_adj(body, ui_lang->pid_kd, kd_cb, (void*)(intptr_t)-1, (void*)(intptr_t)1, buf);

    snprintf(buf, sizeof(buf), "%+d", (int)s_buf.temp_offset);
    s_off_lbl = make_adj(body, ui_lang->temp_offset, off_cb, (void*)(intptr_t)-1, (void*)(intptr_t)1, buf);

    /* Apply / Save / Delete buttons */
    lv_obj_t *btn_row = lv_obj_create(body);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, LV_PCT(100), 44);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_row, 8, 0);

    /* Apply CH1 */
    s_apply_btn[0] = lv_obj_create(btn_row);
    lv_obj_set_size(s_apply_btn[0], LV_SIZE_CONTENT, 40);
    lv_obj_set_style_bg_color(s_apply_btn[0], ui_color_ch1(), 0);
    lv_obj_set_style_bg_opa(s_apply_btn[0], LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_apply_btn[0], 8, 0);
    lv_obj_set_style_border_width(s_apply_btn[0], 0, 0);
    lv_obj_set_style_pad_hor(s_apply_btn[0], 16, 0);
    lv_obj_set_style_pad_ver(s_apply_btn[0], 0, 0);
    lv_obj_remove_flag(s_apply_btn[0], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_apply_btn[0], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_apply_btn[0], apply_ch_cb, LV_EVENT_CLICKED, (void*)(intptr_t)0);
    lv_obj_t *l1 = lv_label_create(s_apply_btn[0]);
    lv_label_set_text(l1, ui_lang->apply_ch1);
    lv_obj_set_style_text_font(l1, &roboto_cyrillic_14, 0);
    lv_obj_set_style_text_color(l1, lv_color_white(), 0);
    lv_obj_align(l1, LV_ALIGN_CENTER, 0, 0);

    /* Apply CH2 */
    s_apply_btn[1] = lv_obj_create(btn_row);
    lv_obj_set_size(s_apply_btn[1], LV_SIZE_CONTENT, 40);
    lv_obj_set_style_bg_color(s_apply_btn[1], ui_color_ch2(), 0);
    lv_obj_set_style_bg_opa(s_apply_btn[1], LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_apply_btn[1], 8, 0);
    lv_obj_set_style_border_width(s_apply_btn[1], 0, 0);
    lv_obj_set_style_pad_hor(s_apply_btn[1], 16, 0);
    lv_obj_set_style_pad_ver(s_apply_btn[1], 0, 0);
    lv_obj_remove_flag(s_apply_btn[1], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_apply_btn[1], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_apply_btn[1], apply_ch_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);
    lv_obj_t *l2 = lv_label_create(s_apply_btn[1]);
    lv_label_set_text(l2, ui_lang->apply_ch2);
    lv_obj_set_style_text_font(l2, &roboto_cyrillic_14, 0);
    lv_obj_set_style_text_color(l2, lv_color_white(), 0);
    lv_obj_align(l2, LV_ALIGN_CENTER, 0, 0);

    /* Save */
    lv_obj_t *bsave = lv_obj_create(btn_row);
    lv_obj_set_size(bsave, LV_SIZE_CONTENT, 40);
    lv_obj_set_style_bg_color(bsave, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(bsave, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bsave, 8, 0);
    lv_obj_set_style_border_width(bsave, 0, 0);
    lv_obj_set_style_pad_hor(bsave, 16, 0);
    lv_obj_set_style_pad_ver(bsave, 0, 0);
    lv_obj_remove_flag(bsave, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bsave, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bsave, save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lsave = lv_label_create(bsave);
    lv_label_set_text(lsave, ui_lang->save_current);
    lv_obj_set_style_text_font(lsave, &roboto_cyrillic_14, 0);
    lv_obj_set_style_text_color(lsave, lv_color_white(), 0);
    lv_obj_align(lsave, LV_ALIGN_CENTER, 0, 0);

    /* Delete (only for existing) */
    if (s_edit_idx >= 0) {
        lv_obj_t *bdel = lv_obj_create(btn_row);
        lv_obj_set_size(bdel, LV_SIZE_CONTENT, 40);
        lv_obj_set_style_bg_color(bdel, lv_color_hex(0x8b0000), 0);
        lv_obj_set_style_bg_opa(bdel, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bdel, 8, 0);
        lv_obj_set_style_border_width(bdel, 0, 0);
        lv_obj_set_style_pad_hor(bdel, 16, 0);
        lv_obj_set_style_pad_ver(bdel, 0, 0);
        lv_obj_remove_flag(bdel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(bdel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(bdel, delete_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *ldel = lv_label_create(bdel);
        lv_label_set_text(ldel, ui_lang->delete_profile);
        lv_obj_set_style_text_font(ldel, &roboto_cyrillic_14, 0);
        lv_obj_set_style_text_color(ldel, lv_color_white(), 0);
        lv_obj_align(ldel, LV_ALIGN_CENTER, 0, 0);
    }

    /* Set initial visual state for type/subtype/apply buttons */
    refresh_type_ui();

    /* Keyboard — child of the screen itself, anchored to bottom, hidden until ta focused */
    lv_obj_t *kb = lv_keyboard_create(scr_profile_edit);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);
    s_kb = kb;
}

/* ------------------------------------------------------------------ */
/*  Public                                                              */
/* ------------------------------------------------------------------ */
void ui_profile_edit_open(int idx)
{
    /* tear down old screen if any */
    if (scr_profile_edit) {
        lv_obj_delete(scr_profile_edit);
        scr_profile_edit = NULL;
    }

    s_edit_idx = idx;
    s_kp_lbl = s_ki_lbl = s_kd_lbl = s_off_lbl = NULL;
    s_ta = s_kb = NULL;
    s_type_btns[0] = s_type_btns[1] = NULL;
    s_subtype_row = NULL;
    s_apply_btn[0] = s_apply_btn[1] = NULL;
    for (int i = 0; i < IRON_SUBTYPE_COUNT; i++) s_subtype_btns[i] = NULL;

    if (idx < 0) {
        /* new profile defaults */
        memset(&s_buf, 0, sizeof(s_buf));
        snprintf(s_buf.name, sizeof(s_buf.name), "Profile %d", (int)g_profile_cnt + 1);
        s_buf.pid_kp = 10; s_buf.pid_ki = 5; s_buf.pid_kd = 2;
    } else {
        s_buf = g_profiles[idx];
    }

    build_screen();
    lv_screen_load(scr_profile_edit);
}
