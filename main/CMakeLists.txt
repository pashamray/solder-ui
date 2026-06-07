idf_component_register(
    SRCS
        "main.c"
        "backlight.c"
        "ui_init.c"
        "ui_theme.c"
        "ui_strings.c"
        "ui_nvs.c"
        "ui_main_screen.c"
        "ui_settings_screen.c"
        "ui_sound_screen.c"
        "ui_sleep_screen.c"
        "ui_display_screen.c"
        "ui_units_screen.c"
        "ui_profiles_screen.c"
        "ui_profile_edit_screen.c"
        "ui_about_screen.c"
        "ui_ch_settings_screen.c"
        "ui_sub_screen.c"
        "fonts/roboto_cyrillic_12.c"
        "fonts/roboto_cyrillic_14.c"
        "fonts/roboto_cyrillic_16.c"
        "fonts/roboto_cyrillic_24.c"
        "fonts/roboto_pm_32.c"
        "fonts/roboto_48.c"
        "fonts/roboto_160.c"
    INCLUDE_DIRS "." "fonts"
    REQUIRES driver esp_lcd nvs_flash
)

# Font files use "#include lvgl/lvgl.h" unless this flag is set.
# Previously set by lvgl_port_add_images; now set explicitly.
idf_build_set_property(COMPILE_OPTIONS "-DLV_LVGL_H_INCLUDE_SIMPLE=1" APPEND)
