#include "fonts/fonts.h"
#include "ui_sub_screen.h"
#include "ui_init.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "lvgl.h"

#define LCD_W  1024
#define LCD_H  600
#define TB_H   40

static void back_to_settings_cb(lv_event_t *e)
{
    lv_screen_load_anim(scr_settings, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false);
}

static void back_to_custom_cb(lv_event_t *e)
{
    lv_obj_t **target = (lv_obj_t **)lv_event_get_user_data(e);
    if (target && *target)
        lv_screen_load_anim(*target, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, false);
}

static lv_obj_t *build_sub_screen(lv_obj_t **scr_handle, const char *title,
                                   lv_event_cb_t back_cb, void *back_ud)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    *scr_handle = scr;

    lv_obj_t *tb = lv_obj_create(scr);
    lv_obj_remove_style_all(tb);
    lv_obj_set_size(tb, LCD_W, TB_H);
    lv_obj_set_pos(tb, 0, 0);
    lv_obj_remove_flag(tb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(tb, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(tb, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(tb, ui_color_border(), LV_PART_MAIN);
    lv_obj_set_style_border_width(tb, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tb, 0, 0);

    lv_obj_t *btn_back = lv_button_create(tb);
    lv_obj_set_size(btn_back, LV_SIZE_CONTENT, 28);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_bg_color(btn_back, ui_color_accent(), 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_pad_hor(btn_back, 10, 0);
    lv_obj_set_style_pad_ver(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, back_cb, LV_EVENT_CLICKED, back_ud);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, ui_lang->back);
    lv_obj_set_style_text_color(lbl_back, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_back, &roboto_cyrillic_16, 0);
    lv_obj_center(lbl_back);

    lv_obj_t *lbl_title = lv_label_create(tb);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(lbl_title, &roboto_cyrillic_24, 0);
    lv_obj_center(lbl_title);

    lv_obj_t *body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(body, LCD_W, LCD_H - TB_H);
    lv_obj_set_pos(body, 0, TB_H);
    lv_obj_set_style_bg_color(body, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(body, 16, 0);
    lv_obj_set_style_pad_bottom(body, 16, 0);
    lv_obj_set_style_pad_left(body, 20, 0);
    lv_obj_set_style_pad_right(body, 20, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, 0);

    return body;
}

lv_obj_t *ui_sub_screen_create(lv_obj_t **scr_handle, const char *title)
{
    return build_sub_screen(scr_handle, title, back_to_settings_cb, NULL);
}

lv_obj_t *ui_sub_screen_create2(lv_obj_t **scr_handle, const char *title, lv_obj_t **back_scr)
{
    return build_sub_screen(scr_handle, title, back_to_custom_cb, back_scr);
}

lv_obj_t *ui_sub_row_create(lv_obj_t *body, const char *name, const char *desc)
{
    lv_obj_t *row = lv_obj_create(body);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(row, ui_color_surface(), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 14, 0);

    /* Left: name + optional desc, vertically centred in the row */
    lv_obj_t *col = lv_obj_create(row);
    lv_obj_remove_style_all(col);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(col, LV_PCT(65), LV_SIZE_CONTENT);
    lv_obj_align(col, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *lbl_name = lv_label_create(col);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_color(lbl_name, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(lbl_name, &roboto_cyrillic_16, 0);

    if (desc) {
        lv_obj_t *lbl_desc = lv_label_create(col);
        lv_label_set_text(lbl_desc, desc);
        lv_obj_set_style_text_color(lbl_desc, ui_color_text_muted(), 0);
        lv_obj_set_style_text_font(lbl_desc, &roboto_cyrillic_12, 0);
    }

    /* Right: container for the setting widget */
    lv_obj_t *right = lv_obj_create(row);
    lv_obj_remove_style_all(right);
    lv_obj_remove_flag(right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);

    return right;
}

lv_obj_t *ui_toggle_create(lv_obj_t *parent, bool initial_state,
                            lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 40, 22);
    if (initial_state)
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, ui_color_border(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xe94560),
                               LV_PART_MAIN | LV_STATE_CHECKED);
    if (cb)
        lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, user_data);
    return sw;
}
