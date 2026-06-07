#pragma once
#include "lvgl.h"

/* Creates a sub-screen with standard titlebar.
   Returns the body container (below the titlebar) for adding settings rows.
   scr_handle is set to the newly created screen object. */
lv_obj_t *ui_sub_screen_create(lv_obj_t **scr_handle, const char *title);
/* Like ui_sub_screen_create but back button loads *back_scr instead of scr_settings */
lv_obj_t *ui_sub_screen_create2(lv_obj_t **scr_handle, const char *title, lv_obj_t **back_scr);

/* Creates a settings row: label+description on left, returns right container for widget. */
lv_obj_t *ui_sub_row_create(lv_obj_t *body, const char *name, const char *desc);

/* Creates an on/off toggle switch. */
lv_obj_t *ui_toggle_create(lv_obj_t *parent, bool initial_state,
                            lv_event_cb_t cb, void *user_data);
