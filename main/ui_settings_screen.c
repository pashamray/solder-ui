#include "fonts/fonts.h"
#include "ui_settings_screen.h"
#include "ui_init.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "lvgl.h"

#define LCD_W    1024
#define LCD_H    600
#define TB_H     40
/* 4-column × 2-row tile grid: 20px outer margin, 12px gaps */
#define TILE_COLS   4
#define TILE_GAP    12
#define TILE_W      ((LCD_W - 40 - (TILE_COLS - 1) * TILE_GAP) / TILE_COLS)   /* 237 */
#define TILE_H      (((LCD_H - TB_H - 40) - TILE_GAP) / 2)                    /* 254 */

static void back_to_main_cb(lv_event_t *e) {
    lv_screen_load_anim(scr_main, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false);
}

static void nav_ch1_cb(lv_event_t *e)      { lv_screen_load_anim(scr_ch1_settings, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_ch2_cb(lv_event_t *e)      { lv_screen_load_anim(scr_ch2_settings, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_sound_cb(lv_event_t *e)    { lv_screen_load_anim(scr_sound,        LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_sleep_cb(lv_event_t *e)    { lv_screen_load_anim(scr_sleep,        LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_display_cb(lv_event_t *e)  { lv_screen_load_anim(scr_display,      LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_units_cb(lv_event_t *e)    { lv_screen_load_anim(scr_units,        LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_profiles_cb(lv_event_t *e) { lv_screen_load_anim(scr_profiles,     LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }
static void nav_about_cb(lv_event_t *e)    { lv_screen_load_anim(scr_about,        LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false); }

static void make_tile(lv_obj_t *parent, const char *icon_text, const char *label_text, lv_event_cb_t cb)
{
    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_set_size(tile, TILE_W, TILE_H);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(tile, ui_color_surface(), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_border_color(tile, ui_color_border(), 0);
    lv_obj_set_style_radius(tile, 10, 0);
    lv_obj_set_style_pad_all(tile, 0, 0);

    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(tile, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(tile, LV_FLEX_ALIGN_CENTER, 0);

    lv_obj_t *icon = lv_label_create(tile);
    lv_label_set_text(icon, icon_text);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, ui_color_text_primary(), 0);
    lv_obj_set_width(icon, LV_PCT(100));
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *lbl = lv_label_create(tile);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(lbl, ui_color_text_primary(), 0);
    lv_obj_set_style_pad_top(lbl, 6, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tile, cb, LV_EVENT_CLICKED, NULL);
}

void ui_settings_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    scr_settings = scr;

    /* ── Titlebar ── */
    lv_obj_t *titlebar = lv_obj_create(scr);
    lv_obj_set_size(titlebar, LCD_W, TB_H);
    lv_obj_align(titlebar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_remove_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(titlebar, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(titlebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(titlebar, 0, 0);
    lv_obj_set_style_pad_all(titlebar, 0, 0);
    lv_obj_set_style_radius(titlebar, 0, 0);

    lv_obj_t *btn_back = lv_button_create(titlebar);
    lv_obj_set_size(btn_back, LV_SIZE_CONTENT, 28);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_bg_color(btn_back, ui_color_accent(), 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_set_style_pad_hor(btn_back, 10, 0);
    lv_obj_set_style_pad_ver(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, back_to_main_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_back_lbl = lv_label_create(btn_back);
    lv_label_set_text(btn_back_lbl, ui_lang->back);
    lv_obj_set_style_text_font(btn_back_lbl, &roboto_cyrillic_16, 0);
    lv_obj_set_style_text_color(btn_back_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(btn_back_lbl);

    lv_obj_t *title = lv_label_create(titlebar);
    lv_label_set_text(title, ui_lang->settings);
    lv_obj_set_style_text_font(title, &roboto_cyrillic_24, 0);
    lv_obj_set_style_text_color(title, ui_color_text_primary(), 0);
    lv_obj_center(title);

    /* ── Tile grid container ── */
    int tile_container_h = LCD_H - TB_H - 40;   /* 520 */

    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, LCD_W - 40, tile_container_h);
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 20, TB_H + 20);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_radius(grid, 0, 0);

    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(grid, TILE_GAP, 0);
    lv_obj_set_style_pad_row(grid, TILE_GAP, 0);

    /* Row 1: CH1, CH2, Sound, Sleep */
    make_tile(grid, LV_SYMBOL_EDIT,    ui_lang->channel_1, nav_ch1_cb);
    make_tile(grid, LV_SYMBOL_EDIT,    ui_lang->channel_2, nav_ch2_cb);
    make_tile(grid, LV_SYMBOL_AUDIO,   ui_lang->sound,     nav_sound_cb);
    make_tile(grid, LV_SYMBOL_POWER,   ui_lang->sleep,     nav_sleep_cb);
    /* Row 2: Display, Units, Profiles, About */
    make_tile(grid, LV_SYMBOL_IMAGE,   ui_lang->display,   nav_display_cb);
    make_tile(grid, LV_SYMBOL_CHARGE,  ui_lang->units,     nav_units_cb);
    make_tile(grid, LV_SYMBOL_LIST,    ui_lang->profiles,  nav_profiles_cb);
    make_tile(grid, LV_SYMBOL_FILE,    ui_lang->about,     nav_about_cb);
}
