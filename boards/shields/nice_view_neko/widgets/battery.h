/*
 * Based on nice-view-gem (MIT, © M165437).
 */

#pragma once

#include <lvgl.h>
#include "util.h"

void draw_battery_status(lv_obj_t *canvas, const struct status_state *state);
