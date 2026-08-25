/*
 * Based on nice-view-gem (MIT, © M165437).
 */

#pragma once

#include <lvgl.h>
#include "util.h"

struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

void draw_battery_status(lv_obj_t *canvas, const struct status_state *state);
