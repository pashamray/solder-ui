#include "fonts/fonts.h"
#include "ui_profiles_screen.h"
#include "ui_profile_edit_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "ui_nvs.h"
#include "lvgl.h"

static void open_edit_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ui_profile_edit_open(idx);
}

static void add_cb(lv_event_t *e)
{
    (void)e;
    ui_profile_edit_open(-1);
}

void ui_profiles_screen_create(void)
{
    lv_obj_t *body = ui_sub_screen_create(&scr_profiles, ui_lang->profiles);

    /* Profile list */
    for (int i = 0; i < (int)g_profile_cnt; i++) {
        lv_obj_t *row = lv_obj_create(body);
        lv_obj_set_size(row, LV_PCT(100), 56);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(row, ui_color_surface(), 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_hor(row, 14, 0);
        lv_obj_set_style_pad_ver(row, 0, 0);

        /* Profile name */
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, g_profiles[i].name);
        lv_obj_set_style_text_color(lbl, ui_color_text_primary(), 0);
        lv_obj_set_style_text_font(lbl, &roboto_cyrillic_16, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, -10);

        /* PID + tool type summary */
        char info[56];
        const char *type_str = (g_profiles[i].tool_type == CH_TYPE_IRON)
            ? IRON_SUBTYPE_NAMES[g_profiles[i].iron_subtype < IRON_SUBTYPE_COUNT
                                 ? g_profiles[i].iron_subtype : 0]
            : "Gun";
        lv_snprintf(info, sizeof(info), "%s  Kp%d Ki%d Kd%d  %+d\xc2\xb0",
                    type_str,
                    (int)g_profiles[i].pid_kp,
                    (int)g_profiles[i].pid_ki,
                    (int)g_profiles[i].pid_kd,
                    (int)g_profiles[i].temp_offset);
        lv_obj_t *lbl_info = lv_label_create(row);
        lv_label_set_text(lbl_info, info);
        lv_obj_set_style_text_color(lbl_info, ui_color_text_secondary(), 0);
        lv_obj_set_style_text_font(lbl_info, &roboto_cyrillic_12, 0);
        lv_obj_align(lbl_info, LV_ALIGN_LEFT_MID, 0, 10);

        /* Edit button */
        lv_obj_t *btn = lv_obj_create(row);
        lv_obj_set_size(btn, 80, 36);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn, ui_color_accent(), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(btn, open_edit_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t *lbl_btn = lv_label_create(btn);
        lv_label_set_text(lbl_btn, ui_lang->edit_profile);
        lv_obj_set_style_text_font(lbl_btn, &roboto_cyrillic_14, 0);
        lv_obj_set_style_text_color(lbl_btn, lv_color_white(), 0);
        lv_obj_align(lbl_btn, LV_ALIGN_CENTER, 0, 0);
    }

    /* [+ Add] button */
    if (g_profile_cnt < MAX_PROFILES) {
        lv_obj_t *btn_add = lv_obj_create(body);
        lv_obj_set_size(btn_add, LV_PCT(100), 44);
        lv_obj_remove_flag(btn_add, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(btn_add, ui_color_border(), 0);
        lv_obj_set_style_border_width(btn_add, 0, 0);
        lv_obj_set_style_radius(btn_add, 8, 0);
        lv_obj_set_style_pad_all(btn_add, 0, 0);
        lv_obj_add_flag(btn_add, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_add, add_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl_add = lv_label_create(btn_add);
        lv_label_set_text(lbl_add, ui_lang->add_profile);
        lv_obj_set_style_text_font(lbl_add, &roboto_cyrillic_16, 0);
        lv_obj_set_style_text_color(lbl_add, ui_color_text_primary(), 0);
        lv_obj_align(lbl_add, LV_ALIGN_CENTER, 0, 0);
    }
}
