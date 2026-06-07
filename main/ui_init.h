#pragma once
#include "lvgl.h"

/* Shared screen handles — all UI files access these */
extern lv_obj_t *scr_main;
extern lv_obj_t *scr_settings;
extern lv_obj_t *scr_sound;
extern lv_obj_t *scr_sleep;
extern lv_obj_t *scr_display;
extern lv_obj_t *scr_units;
extern lv_obj_t *scr_profiles;
extern lv_obj_t *scr_about;
extern lv_obj_t *scr_ch1_settings;
extern lv_obj_t *scr_ch2_settings;
extern lv_obj_t *scr_profile_edit;

void app_ui_init(void);
void ui_rebuild_all(void);
void ui_rebuild_to_settings(void);   /* like ui_rebuild_all but lands on scr_settings */
