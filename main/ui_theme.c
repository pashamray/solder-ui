#include "ui_theme.h"

const lv_color_t CH_COLORS[CH_COLOR_COUNT] = {
    { .red = 0xe9, .green = 0x45, .blue = 0x60 },  /* 0 red     */
    { .red = 0x4a, .green = 0x9e, .blue = 0xff },  /* 1 blue    */
    { .red = 0xff, .green = 0x6b, .blue = 0x35 },  /* 2 orange  */
    { .red = 0x2e, .green = 0xcc, .blue = 0x71 },  /* 3 green   */
    { .red = 0x9b, .green = 0x59, .blue = 0xb6 },  /* 4 purple  */
    { .red = 0x1a, .green = 0xbc, .blue = 0x9c },  /* 5 teal    */
    { .red = 0xf1, .green = 0xc4, .blue = 0x0f },  /* 6 yellow  */
    { .red = 0xff, .green = 0x69, .blue = 0xb4 },  /* 7 pink    */
};

ui_palette_t g_pal;
ui_theme_t   g_theme = THEME_DARK;

static const ui_palette_t PAL_DARK = {
    .bg             = { .red = 0x0d, .green = 0x0d, .blue = 0x1a },
    .surface        = { .red = 0x12, .green = 0x12, .blue = 0x2a },
    .border         = { .red = 0x1e, .green = 0x1e, .blue = 0x3a },
    .ch1            = { .red = 0xe9, .green = 0x45, .blue = 0x60 },
    .ch2            = { .red = 0x4a, .green = 0x9e, .blue = 0xff },
    .text_primary   = { .red = 0xee, .green = 0xee, .blue = 0xee },
    .text_secondary = { .red = 0x88, .green = 0x88, .blue = 0x88 },
    .text_muted     = { .red = 0x55, .green = 0x55, .blue = 0x55 },
    .enc_val        = { .red = 0xff, .green = 0xd7, .blue = 0x00 },
    .tab_active_bg  = { .red = 0xe9, .green = 0x45, .blue = 0x60 },
    .tab_idle_bg    = { .red = 0x1a, .green = 0x3a, .blue = 0x5c },
    .tab_idle_text  = { .red = 0x4a, .green = 0x9e, .blue = 0xff },
};

static const ui_palette_t PAL_LIGHT = {
    .bg             = { .red = 0xf0, .green = 0xf2, .blue = 0xf5 },
    .surface        = { .red = 0xff, .green = 0xff, .blue = 0xff },
    .border         = { .red = 0xe0, .green = 0xe0, .blue = 0xe0 },
    .ch1            = { .red = 0xc0, .green = 0x39, .blue = 0x2b },
    .ch2            = { .red = 0x1a, .green = 0x52, .blue = 0x76 },
    .text_primary   = { .red = 0x1a, .green = 0x1a, .blue = 0x1a },
    .text_secondary = { .red = 0x66, .green = 0x66, .blue = 0x66 },
    .text_muted     = { .red = 0xbb, .green = 0xbb, .blue = 0xbb },
    .enc_val        = { .red = 0xd4, .green = 0xac, .blue = 0x0d },
    .tab_active_bg  = { .red = 0xc0, .green = 0x39, .blue = 0x2b },
    .tab_idle_bg    = { .red = 0xd6, .green = 0xea, .blue = 0xf8 },
    .tab_idle_text  = { .red = 0x1a, .green = 0x52, .blue = 0x76 },
};

void ui_theme_init(void)
{
    g_pal = PAL_DARK;
}

void ui_theme_apply(ui_theme_t t)
{
    g_theme = t;
    g_pal   = (t == THEME_LIGHT) ? PAL_LIGHT : PAL_DARK;
    lv_obj_t *scr = lv_screen_active();
    if (scr) lv_obj_invalidate(scr);
}

void ui_theme_set_ch_colors(uint8_t idx1, uint8_t idx2)
{
    if (idx1 < CH_COLOR_COUNT) g_pal.ch1 = CH_COLORS[idx1];
    if (idx2 < CH_COLOR_COUNT) g_pal.ch2 = CH_COLORS[idx2];
}
