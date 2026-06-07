/*******************************************************************************
 * Size: 32 px
 * Bpp: 4
 * Opts: --no-compress --bpp 4 --size 32 --font /usr/share/fonts/google-roboto/Roboto-Medium.ttf -r 0x002B,0x002D --format lvgl --lv-font-name roboto_pm_32 -o /home/ps/projects/solder-ui/main/fonts/roboto_pm_32.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef ROBOTO_PM_32
#define ROBOTO_PM_32 1
#endif

#if ROBOTO_PM_32

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+002B "+" */
    0x0, 0x0, 0x0, 0x55, 0x54, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa,
    0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa,
    0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa,
    0x56, 0x66, 0x66, 0xff, 0xfd, 0x66, 0x66, 0x64,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xfb, 0x0, 0x0, 0x0,

    /* U+002D "-" */
    0x0, 0x0, 0x0, 0x0, 0xe, 0xff, 0xff, 0xff,
    0xf5, 0xef, 0xff, 0xff, 0xff, 0x5e, 0xff, 0xff,
    0xff, 0xf5
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 286, .box_w = 16, .box_h = 17, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 136, .adv_w = 168, .box_w = 9, .box_h = 4, .ofs_x = 1, .ofs_y = 8}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_0[] = {
    0, 0, 1
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 43, .range_length = 3, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_0, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t roboto_pm_32 = {
#else
lv_font_t roboto_pm_32 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = -2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 2,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if ROBOTO_PM_32*/

