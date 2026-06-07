#pragma once
#include "driver/i2c_master.h"
#include <stdint.h>

/* Call once after the I2C bus is initialised (after app_touch_init). */
void backlight_init(i2c_master_bus_handle_t bus);

/* Set backlight brightness: 0-100 (%). Thread-safe. */
void backlight_set(uint8_t pct);
