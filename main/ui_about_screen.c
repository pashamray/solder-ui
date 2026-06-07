#include "fonts/fonts.h"
#include "ui_about_screen.h"
#include "ui_init.h"
#include "ui_sub_screen.h"
#include "ui_theme.h"
#include "ui_strings.h"
#include "lvgl.h"
#include "esp_system.h"
#include <stdio.h>

/* Screens are created once at boot and never destroyed — lbl_uptime is valid
   for the program lifetime once ui_about_screen_create() has run. */
static lv_obj_t  *lbl_uptime      = NULL;
static uint32_t   s_uptime_sec    = 0;
static lv_timer_t *s_uptime_timer = NULL;

/* ── Uptime timer ─────────────────────────────────────────────────── */

static void uptime_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_uptime_sec++;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             (unsigned long)(s_uptime_sec / 3600),
             (unsigned long)((s_uptime_sec % 3600) / 60),
             (unsigned long)(s_uptime_sec % 60));
    if (lbl_uptime)
        lv_label_set_text(lbl_uptime, buf);
}

/* ── Factory reset confirm dialog ────────────────────────────────── */

static void do_reset_cb(lv_event_t *e)
{
    (void)e;
    esp_restart();
}

static void cancel_reset_cb(lv_event_t *e)
{
    lv_obj_t *overlay = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_delete(overlay);
}

static void factory_reset_btn_cb(lv_event_t *e)
{
    (void)e;

    /* Full-screen semi-transparent overlay */
    lv_obj_t *overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);

    /* Centered dialog box */
    lv_obj_t *dialog = lv_obj_create(overlay);
    lv_obj_set_size(dialog, 420, 180);
    lv_obj_center(dialog);
    lv_obj_remove_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dialog, ui_color_surface(), 0);
    lv_obj_set_style_border_color(dialog, ui_color_border(), 0);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_radius(dialog, 8, 0);
    lv_obj_set_style_pad_all(dialog, 0, 0);

    /* Prompt text */
    lv_obj_t *lbl = lv_label_create(dialog);
    lv_label_set_text(lbl, ui_lang->confirm_factory_reset);
    lv_obj_set_style_text_color(lbl, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(lbl, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 24);

    /* "Да" button */
    lv_obj_t *btn_yes = lv_obj_create(dialog);
    lv_obj_set_size(btn_yes, 120, 40);
    lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_obj_remove_flag(btn_yes, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn_yes, ui_color_accent(), 0);
    lv_obj_set_style_border_width(btn_yes, 0, 0);
    lv_obj_set_style_radius(btn_yes, 4, 0);
    lv_obj_set_style_pad_all(btn_yes, 0, 0);
    lv_obj_add_flag(btn_yes, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_yes = lv_label_create(btn_yes);
    lv_label_set_text(lbl_yes, ui_lang->confirm_yes);
    lv_obj_set_style_text_color(lbl_yes, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_yes, &roboto_cyrillic_14, 0);
    lv_obj_center(lbl_yes);
    lv_obj_add_event_cb(btn_yes, do_reset_cb, LV_EVENT_CLICKED, NULL);

    /* "Нет" button */
    lv_obj_t *btn_no = lv_obj_create(dialog);
    lv_obj_set_size(btn_no, 120, 40);
    lv_obj_align(btn_no, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    lv_obj_remove_flag(btn_no, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn_no, ui_color_border(), 0);
    lv_obj_set_style_border_width(btn_no, 0, 0);
    lv_obj_set_style_radius(btn_no, 4, 0);
    lv_obj_set_style_pad_all(btn_no, 0, 0);
    lv_obj_add_flag(btn_no, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_no = lv_label_create(btn_no);
    lv_label_set_text(lbl_no, ui_lang->confirm_no);
    lv_obj_set_style_text_color(lbl_no, ui_color_text_primary(), 0);
    lv_obj_set_style_text_font(lbl_no, &roboto_cyrillic_14, 0);
    lv_obj_center(lbl_no);
    lv_obj_add_event_cb(btn_no, cancel_reset_cb, LV_EVENT_CLICKED, overlay);
}

/* ── Screen create ────────────────────────────────────────────────── */

void ui_about_screen_create(void)
{
    lv_obj_t *body = ui_sub_screen_create(&scr_about, ui_lang->about);

    /* Firmware row */
    lv_obj_t *r1 = ui_sub_row_create(body, ui_lang->firmware, NULL);
    lv_obj_t *lbl_fw = lv_label_create(r1);
    lv_label_set_text(lbl_fw, "v1.0.0");
    lv_obj_set_style_text_color(lbl_fw, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_fw, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_fw, LV_ALIGN_CENTER, 0, 0);

    /* Display row */
    lv_obj_t *r2 = ui_sub_row_create(body, ui_lang->display, NULL);
    lv_obj_t *lbl_disp = lv_label_create(r2);
    lv_label_set_text(lbl_disp, "1024\xc3\x97" "600 RGB");  /* × = U+00D7 */
    lv_obj_set_style_text_color(lbl_disp, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_disp, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_disp, LV_ALIGN_CENTER, 0, 0);

    /* MCU row */
    lv_obj_t *r3 = ui_sub_row_create(body, "MCU", NULL);
    lv_obj_t *lbl_mcu = lv_label_create(r3);
    lv_label_set_text(lbl_mcu, "ESP32-S3 @ 240MHz");
    lv_obj_set_style_text_color(lbl_mcu, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_mcu, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_mcu, LV_ALIGN_CENTER, 0, 0);

    /* Uptime row */
    lv_obj_t *r4 = ui_sub_row_create(body, ui_lang->uptime, NULL);
    lbl_uptime = lv_label_create(r4);
    lv_label_set_text(lbl_uptime, "00:00:00");
    lv_obj_set_style_text_color(lbl_uptime, ui_color_text_secondary(), 0);
    lv_obj_set_style_text_font(lbl_uptime, &roboto_cyrillic_14, 0);
    lv_obj_align(lbl_uptime, LV_ALIGN_CENTER, 0, 0);

    /* Factory reset button */
    lv_obj_t *btn_reset = lv_obj_create(body);
    lv_obj_set_size(btn_reset, LV_PCT(100), 48);
    lv_obj_remove_flag(btn_reset, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn_reset, ui_color_accent(), 0);
    lv_obj_set_style_border_width(btn_reset, 0, 0);
    lv_obj_set_style_radius(btn_reset, 6, 0);
    lv_obj_set_style_pad_all(btn_reset, 0, 0);
    lv_obj_add_flag(btn_reset, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_reset = lv_label_create(btn_reset);
    lv_label_set_text(lbl_reset, ui_lang->factory_reset);
    lv_obj_set_style_text_color(lbl_reset, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_reset, &roboto_cyrillic_14, 0);
    lv_obj_center(lbl_reset);
    lv_obj_add_event_cb(btn_reset, factory_reset_btn_cb, LV_EVENT_CLICKED, NULL);

    /* Uptime timer — 1 s period, created once */
    if (!s_uptime_timer)
        s_uptime_timer = lv_timer_create(uptime_timer_cb, 1000, NULL);
}
