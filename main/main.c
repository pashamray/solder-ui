/*
 * SPDX-FileCopyrightText: 2022-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>
#include <sys/time.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#include "esp_lcd_touch_gt911.h"
#include "ui_init.h"
#include "backlight.h"

/**
 * @brief LCD Resolution and Timing
 */
#define EXAMPLE_LCD_H_RES               (1024)  ///< Horizontal resolution in pixels
#define EXAMPLE_LCD_V_RES               (600)  ///< Vertical resolution in pixels
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (30.85 * 1000 * 1000) ///< Pixel clock frequency in Hz

/**
 * @brief Color and Pixel Configuration
 */
#define EXAMPLE_LCD_BIT_PER_PIXEL       (16)   ///< Bits per pixel (color depth)
#define EXAMPLE_RGB_BIT_PER_PIXEL       (16)   ///< RGB interface color depth
#define EXAMPLE_RGB_DATA_WIDTH          (16)   ///< Data width for RGB interface
#define EXAMPLE_LCD_RGB_BUFFER_NUMS     (2)    ///< Number of frame buffers for double buffering
#define EXAMPLE_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_H_RES * 10) ///< Size of bounce buffer for RGB data

/**
 * @brief GPIO Pins for RGB LCD Signals
 */
#define EXAMPLE_LCD_IO_RGB_DISP         (-1)   ///< DISP signal, -1 if not used
#define EXAMPLE_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)  ///< Vertical sync signal
#define EXAMPLE_LCD_IO_RGB_HSYNC        (GPIO_NUM_46) ///< Horizontal sync signal
#define EXAMPLE_LCD_IO_RGB_DE           (GPIO_NUM_5)  ///< Data enable signal
#define EXAMPLE_LCD_IO_RGB_PCLK         (GPIO_NUM_7)  ///< Pixel clock signal

/**
 * @brief GPIO Pins for RGB Data Signals
 */
// Blue data signals
#define EXAMPLE_LCD_IO_RGB_DATA0        (GPIO_NUM_14) ///< B3
#define EXAMPLE_LCD_IO_RGB_DATA1        (GPIO_NUM_38) ///< B4
#define EXAMPLE_LCD_IO_RGB_DATA2        (GPIO_NUM_18) ///< B5
#define EXAMPLE_LCD_IO_RGB_DATA3        (GPIO_NUM_17) ///< B6
#define EXAMPLE_LCD_IO_RGB_DATA4        (GPIO_NUM_10) ///< B7

// Green data signals
#define EXAMPLE_LCD_IO_RGB_DATA5        (GPIO_NUM_39) ///< G2
#define EXAMPLE_LCD_IO_RGB_DATA6        (GPIO_NUM_0)  ///< G3
#define EXAMPLE_LCD_IO_RGB_DATA7        (GPIO_NUM_45) ///< G4
#define EXAMPLE_LCD_IO_RGB_DATA8        (GPIO_NUM_48) ///< G5
#define EXAMPLE_LCD_IO_RGB_DATA9        (GPIO_NUM_47) ///< G6
#define EXAMPLE_LCD_IO_RGB_DATA10       (GPIO_NUM_21) ///< G7

// Red data signals
#define EXAMPLE_LCD_IO_RGB_DATA11       (GPIO_NUM_1)  ///< R3
#define EXAMPLE_LCD_IO_RGB_DATA12       (GPIO_NUM_2)  ///< R4
#define EXAMPLE_LCD_IO_RGB_DATA13       (GPIO_NUM_42) ///< R5
#define EXAMPLE_LCD_IO_RGB_DATA14       (GPIO_NUM_41) ///< R6
#define EXAMPLE_LCD_IO_RGB_DATA15       (GPIO_NUM_40) ///< R7

/**
 * @brief Reset and Backlight Configuration
 */
#define EXAMPLE_LCD_IO_RST              (-1)   ///< Reset pin, -1 if not used
#define EXAMPLE_PIN_NUM_BK_LIGHT        (-1)   ///< Backlight pin, -1 if not used
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL   (1)    ///< Logic level to turn on backlight
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL  (!EXAMPLE_LCD_BK_LIGHT_ON_LEVEL) ///< Logic level to turn off backlight
#define EXAMPLE_LCD_GPIO_BL         (GPIO_EXIO2)

/* Rotation: 0, 90, 180, 270 (degrees).
 * For 0 and 180: uses avoid_tearing (direct RGB framebuffer, fastest).
 * For 90 and 270: requires sw_rotate — change EXAMPLE_USE_SW_ROTATE to 1 below. */
#define EXAMPLE_LCD_ROTATION            (LV_DISPLAY_ROTATION_0)

#if (EXAMPLE_LCD_ROTATION == LV_DISPLAY_ROTATION_90 || EXAMPLE_LCD_ROTATION == LV_DISPLAY_ROTATION_270)
#define EXAMPLE_USE_SW_ROTATE   (1)
#else
#define EXAMPLE_USE_SW_ROTATE   (0)
#endif

#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE    (0)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT    (EXAMPLE_LCD_V_RES)

/* Touch settings */
#define EXAMPLE_TOUCH_I2C_NUM       (0)
#define EXAMPLE_TOUCH_I2C_CLK_HZ    (400000)

/* LCD touch pins */
#define EXAMPLE_TOUCH_I2C_SCL       (GPIO_NUM_9)
#define EXAMPLE_TOUCH_I2C_SDA       (GPIO_NUM_8)
#define EXAMPLE_TOUCH_GPIO_INT      (GPIO_NUM_4)

/* IO expander (Waveshare custom chip, I2C addr 0x24) */
#define IO_EXT_ADDR         (0x24)
#define IO_EXT_REG_MODE     (0x02)  // all 1 = all outputs
#define IO_EXT_REG_OUTPUT   (0x03)
#define IO_EXT_IO1_CTP_RST  (1 << 1) // IO1 = touch RST

static const char *TAG = "EXAMPLE";

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    // /* LCD backlight */
    // gpio_config_t bk_gpio_config = {
        // .mode = GPIO_MODE_OUTPUT,
        // .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL
    // };
    // ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    /* LCD initialization */
    // Log the start of the RGB LCD panel driver installation
    ESP_LOGI(TAG, "Install RGB LCD panel driver");

    // Configuration structure for the RGB LCD panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // Use the default clock source
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency in Hz
            .h_res = EXAMPLE_LCD_H_RES,            // Horizontal resolution (number of pixels per row)
            .v_res = EXAMPLE_LCD_V_RES,            // Vertical resolution (number of rows)
            .hsync_pulse_width = 162,                // Horizontal sync pulse width
            .hsync_back_porch = 152,                 // Horizontal back porch
            .hsync_front_porch = 48,                // Horizontal front porch
            .vsync_pulse_width = 45,                // Vertical sync pulse width
            .vsync_back_porch = 13,                 // Vertical back porch
            .vsync_front_porch = 3,                // Vertical front porch
            .flags = {
                .pclk_active_neg = 1, // Set pixel clock polarity to active low
            },
        },
        .data_width = EXAMPLE_RGB_DATA_WIDTH,                    // Data width for RGB signals
        #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
                .psram_trans_align = 64,
        #else
                .dma_burst_size = 64,
        #endif
        #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,0,0)
                .in_color_format = LCD_COLOR_FMT_RGB565,
        #else
                .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
        #endif
        .num_fbs = EXAMPLE_LCD_RGB_BUFFER_NUMS,                  // Number of framebuffers for double/triple buffering
        .bounce_buffer_size_px = EXAMPLE_RGB_BOUNCE_BUFFER_SIZE, // Bounce buffer size in pixels
        .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC,              // GPIO for horizontal sync signal
        .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC,              // GPIO for vertical sync signal
        .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE,                    // GPIO for data enable signal
        .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK,                // GPIO for pixel clock signal
        .disp_gpio_num = EXAMPLE_LCD_IO_RGB_DISP,                // GPIO for display enable signal
        .data_gpio_nums = {
            // GPIOs for RGB data signals
            EXAMPLE_LCD_IO_RGB_DATA0,  // Data bit 0
            EXAMPLE_LCD_IO_RGB_DATA1,  // Data bit 1
            EXAMPLE_LCD_IO_RGB_DATA2,  // Data bit 2
            EXAMPLE_LCD_IO_RGB_DATA3,  // Data bit 3
            EXAMPLE_LCD_IO_RGB_DATA4,  // Data bit 4
            EXAMPLE_LCD_IO_RGB_DATA5,  // Data bit 5
            EXAMPLE_LCD_IO_RGB_DATA6,  // Data bit 6
            EXAMPLE_LCD_IO_RGB_DATA7,  // Data bit 7
            EXAMPLE_LCD_IO_RGB_DATA8,  // Data bit 8
            EXAMPLE_LCD_IO_RGB_DATA9,  // Data bit 9
            EXAMPLE_LCD_IO_RGB_DATA10, // Data bit 10
            EXAMPLE_LCD_IO_RGB_DATA11, // Data bit 11
            EXAMPLE_LCD_IO_RGB_DATA12, // Data bit 12
            EXAMPLE_LCD_IO_RGB_DATA13, // Data bit 13
            EXAMPLE_LCD_IO_RGB_DATA14, // Data bit 14
            EXAMPLE_LCD_IO_RGB_DATA15, // Data bit 15
        },
        .flags = {
            .fb_in_psram = 1, // Use PSRAM for framebuffers to save internal SRAM
        },
    };

    // Create and register the RGB LCD panel driver with the configuration above
    ESP_GOTO_ON_ERROR(esp_lcd_new_rgb_panel(&panel_config, &lcd_panel), err, TAG, "RGB init failed");

    // Log the initialization of the RGB LCD panel
    ESP_LOGI(TAG, "Initialize RGB LCD panel");

    // Initialize the RGB LCD panel
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(lcd_panel), err, TAG, "LCD init failed");

    // esp_lcd_rgb_panel_event_callbacks_t cbs = {
// #if EXAMPLE_RGB_BOUNCE_BUFFER_SIZE > 0
        // .on_bounce_frame_finish = rgb_lcd_on_vsync_event, // Callback for bounce frame finish
// #else
        // .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
// #endif
    // };
    // ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(lcd_panel, &cbs, NULL)); // Register event callbacks

    // /* LCD backlight on */
    // ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL));

    return ret;

err:
    if (lcd_panel) {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io) {
        esp_lcd_panel_io_del(lcd_io);
    }
    return ret;
}

static esp_err_t io_ext_gt911_reset(i2c_master_bus_handle_t bus)
{
    i2c_master_dev_handle_t dev;
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IO_EXT_ADDR,
        .scl_speed_hz = EXAMPLE_TOUCH_I2C_CLK_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &dev), TAG, "io_ext add failed");

    /* all pins = output */
    uint8_t mode_cmd[] = {IO_EXT_REG_MODE, 0xFF};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(dev, mode_cmd, sizeof(mode_cmd), 100), TAG, "io_ext mode failed");

    /* RST = LOW */
    uint8_t rst_low[] = {IO_EXT_REG_OUTPUT, (uint8_t)(0xFF & ~IO_EXT_IO1_CTP_RST)};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(dev, rst_low, sizeof(rst_low), 100), TAG, "io_ext rst low failed");
    vTaskDelay(pdMS_TO_TICKS(10));

    /* INT = LOW → selects GT911 addr 0x5D */
    gpio_set_direction(EXAMPLE_TOUCH_GPIO_INT, GPIO_MODE_OUTPUT);
    gpio_set_level(EXAMPLE_TOUCH_GPIO_INT, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* RST = HIGH */
    uint8_t rst_high[] = {IO_EXT_REG_OUTPUT, 0xFF};
    ESP_RETURN_ON_ERROR(i2c_master_transmit(dev, rst_high, sizeof(rst_high), 100), TAG, "io_ext rst high failed");
    vTaskDelay(pdMS_TO_TICKS(200));

    /* release INT back to input */
    gpio_set_direction(EXAMPLE_TOUCH_GPIO_INT, GPIO_MODE_INPUT);

    i2c_master_bus_rm_device(dev);
    return ESP_OK;
}

static i2c_master_bus_handle_t s_i2c_bus = NULL;

static esp_err_t app_touch_init(void)
{
    /* Initilize I2C */
    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = EXAMPLE_TOUCH_I2C_NUM,
        .sda_io_num = EXAMPLE_TOUCH_I2C_SDA,
        .scl_io_num = EXAMPLE_TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_config, &s_i2c_bus), TAG, "");

    /* Reset GT911 via IO expander IO1 (CTP_RST) */
    ESP_RETURN_ON_ERROR(io_ext_gt911_reset(s_i2c_bus), TAG, "GT911 reset via IO ext failed");

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = EXAMPLE_TOUCH_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = EXAMPLE_TOUCH_I2C_CLK_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
}

static esp_err_t app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,         /* LVGL task priority */
        .task_stack = 8192,         /* LVGL task stack size */
        .task_affinity = -1,        /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5        /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = NULL,
        .panel_handle = lcd_panel,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = EXAMPLE_USE_SW_ROTATE,
            .sw_rotate = EXAMPLE_USE_SW_ROTATE,
            .full_refresh = !EXAMPLE_USE_SW_ROTATE, /* full-screen refresh for avoid_tearing */
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = false,
#endif
        }
    };
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = (EXAMPLE_RGB_BOUNCE_BUFFER_SIZE > 0),
            .avoid_tearing = !EXAMPLE_USE_SW_ROTATE,
        }
    };
    lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);

#if (EXAMPLE_LCD_ROTATION != LV_DISPLAY_ROTATION_0)
    lv_display_set_rotation(lvgl_disp, EXAMPLE_LCD_ROTATION);
#endif

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}


void app_main(void)
{
    /* LCD HW initialization */
    ESP_ERROR_CHECK(app_lcd_init());

    /* Touch initialization (also creates s_i2c_bus) */
    ESP_ERROR_CHECK(app_touch_init());

    /* Backlight PWM — must come after app_touch_init creates the I2C bus */
    backlight_init(s_i2c_bus);

    /* LVGL initialization */
    ESP_ERROR_CHECK(app_lvgl_init());

    /* Set RTC to 2026-01-01 12:00:00 */
    {
        struct tm t = {
            .tm_year = 2026 - 1900,
            .tm_mon  = 0,
            .tm_mday = 1,
            .tm_hour = 12,
            .tm_min  = 0,
            .tm_sec  = 0,
        };
        struct timeval tv = { .tv_sec = mktime(&t), .tv_usec = 0 };
        settimeofday(&tv, NULL);
    }

    /* Show LVGL objects */
    app_ui_init();
}
