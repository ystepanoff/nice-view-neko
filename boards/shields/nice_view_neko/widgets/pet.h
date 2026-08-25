#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

enum pet_ctrl_direction {
    PET_CTRL_N = 0,
    PET_CTRL_E,
    PET_CTRL_S,
    PET_CTRL_W,
};

/* Register the 68x68 canvas the pet lives on and start the animation timer. */
void pet_attach_canvas(lv_obj_t *canvas);

void pet_set_wpm(uint8_t wpm);
void pet_note_position(bool pressed);
void pet_set_caps(bool caps);
void pet_key_event(uint16_t usage_page, uint32_t keycode, bool pressed);
void pet_sensor_event(uint8_t sensor_index, int32_t delta);
/* Pause/resume the animation timer, e.g. on activity idle/active. */
void pet_set_active(bool active);
