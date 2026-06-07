#include "backlight.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "BL";

/*
 * Waveshare custom I2C expander @ 0x24
 *
 * Register map (from board examples):
 *   0x02 = mode      (0xFF = all outputs)
 *   0x03 = IO output (bit N = pin N state)
 *   0x04 = IO input
 *   0x05 = PWM       (0x00-0xFF duty cycle, chip generates signal)
 *   0x06 = ADC
 *
 * Backlight = IO2 (bit 2 of reg 0x03).
 * Hardware PWM output on reg 0x05 — one write sets duty, no polling needed.
 *
 * I2C writes are deferred to a dedicated task so they never run inside
 * the LVGL task and cannot collide with the GT911 touch reads.
 */
#define IO_EXT_ADDR     0x24
#define IO_EXT_REG_OUT  0x03
#define IO_EXT_REG_PWM  0x05
#define IO_EXIO2_BIT    ((uint8_t)(1 << 2))

static i2c_master_dev_handle_t s_dev    = NULL;
static uint8_t                 s_io_out = 0xFF;
static QueueHandle_t           s_queue  = NULL;

static void ext_write(uint8_t reg, uint8_t val)
{
    if (!s_dev) return;
    uint8_t cmd[2] = {reg, val};
    esp_err_t err = i2c_master_transmit(s_dev, cmd, 2, 50);
    if (err != ESP_OK)
        ESP_LOGW(TAG, "i2c err reg=0x%02x val=0x%02x: %s", reg, val, esp_err_to_name(err));
}

/*
 * Gamma=1.5 lookup for pct = 10,15,20,...,100 (step 5, index = (pct-10)/5).
 * duty = round(255 * (1 - (pct/100)^1.5)), active-low, max 247 per chip limit.
 */
static const uint8_t s_duty_table[19] = {
    247, 240, 232, 223, 213, 202, 190, 178,
    165, 151, 136, 121, 106,  89,  73,  55,
     37,  19,   0
};

static void backlight_task(void *arg)
{
    uint8_t pct;
    while (1) {
        xQueueReceive(s_queue, &pct, portMAX_DELAY);
        if (pct > 100) pct = 100;
        if (pct < 25)  pct = 25;
        uint8_t idx = (pct - 10) / 5;
        uint8_t duty = s_duty_table[idx];
        ESP_LOGI(TAG, "pct=%d duty=0x%02x", pct, duty);
        ext_write(IO_EXT_REG_PWM, duty);
    }
}

void backlight_init(i2c_master_bus_handle_t bus)
{
    const i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = IO_EXT_ADDR,
        .scl_speed_hz    = 400000,
    };
    i2c_master_bus_add_device(bus, &cfg, &s_dev);

    s_io_out |= IO_EXIO2_BIT;
    ext_write(IO_EXT_REG_OUT, s_io_out);
    ext_write(IO_EXT_REG_PWM, 0x00);

    s_queue = xQueueCreate(1, sizeof(uint8_t));
    xTaskCreate(backlight_task, "bl", 2048, NULL, 3, NULL);
}

void backlight_set(uint8_t pct)
{
    if (s_queue)
        xQueueOverwrite(s_queue, &pct);
}
