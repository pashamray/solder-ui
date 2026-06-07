#pragma once
#include "lvgl.h"
typedef enum { THEME_DARK = 0, THEME_LIGHT = 1 } ui_theme_t;
typedef struct {
    lv_color_t bg;
    lv_color_t surface;
    lv_color_t border;
    lv_color_t ch1;
    lv_color_t ch2;
    lv_color_t text_primary;
    lv_color_t text_secondary;
    lv_color_t text_muted;
    lv_color_t enc_val;
    lv_color_t tab_active_bg;
    lv_color_t tab_idle_bg;
    lv_color_t tab_idle_text;
} ui_palette_t;
extern ui_palette_t g_pal;
extern ui_theme_t   g_theme;
void ui_theme_init(void);
void ui_theme_apply(ui_theme_t t);
static inline lv_color_t ui_color_bg(void)             { return g_pal.bg; }
static inline lv_color_t ui_color_surface(void)        { return g_pal.surface; }
static inline lv_color_t ui_color_border(void)         { return g_pal.border; }
static inline lv_color_t ui_color_ch1(void)            { return g_pal.ch1; }
static inline lv_color_t ui_color_ch2(void)            { return g_pal.ch2; }
static inline lv_color_t ui_color_text_primary(void)   { return g_pal.text_primary; }
static inline lv_color_t ui_color_text_secondary(void) { return g_pal.text_secondary; }
static inline lv_color_t ui_color_text_muted(void)     { return g_pal.text_muted; }
static inline lv_color_t ui_color_enc_val(void)        { return g_pal.enc_val; }
static inline lv_color_t ui_color_tab_active_bg(void)  { return g_pal.tab_active_bg; }
static inline lv_color_t ui_color_tab_idle_bg(void)    { return g_pal.tab_idle_bg; }
static inline lv_color_t ui_color_tab_idle_text(void)  { return g_pal.tab_idle_text; }
static inline lv_color_t ui_color_accent(void)         { return g_pal.tab_active_bg; }
