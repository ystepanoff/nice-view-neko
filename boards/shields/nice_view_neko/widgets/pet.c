#include <zephyr/kernel.h>
#include "pet.h"
#include "util.h"
#include "../assets/neko.h"

/* HID Keyboard/Keypad usage page and the keys the pet reacts to */
#define PET_HID_USAGE_PAGE_KEY 0x07
#define PET_HID_KEY_SPACE 0x2C
#define PET_HID_KEY_LCTRL 0xE0
#define PET_HID_KEY_RCTRL 0xE4

/* Sensor index of each encoder within the keymap sensors node */
#define PET_SENSOR_LEFT 0
#define PET_SENSOR_RIGHT 1

static struct {
    lv_obj_t *canvas;
    lv_timer_t *timer;
    uint8_t frame;
    uint8_t wpm;
    bool caps;
    uint8_t ctrl_held;
    enum pet_ctrl_direction ctrl_dir;
    int8_t scroll_x; /* -1 = west, +1 = east */
    int8_t scroll_y; /* -1 = south, +1 = north */
    int64_t last_scroll_x;
    int64_t last_scroll_y;
    bool jumping;
} pet = {.ctrl_dir = PET_CTRL_N};

/* neko_scroll direction order: N, NE, E, SE, S, SW, W, NW */
static const uint8_t scroll_dir_table[3][3] = {
    /* y = -1 (south): x = -1, 0, +1 */ {5, 4, 3},
    /* y =  0:                        */ {6, 0, 2}, /* [1][1] unused */
    /* y = +1 (north):                */ {7, 0, 1},
};

static const lv_img_dsc_t *pick_frame(void) {
    if (pet.caps) {
        return neko_caps[pet.frame];
    }
    if (pet.ctrl_held > 0) {
        return neko_ctrl[pet.ctrl_dir * NEKO_FRAMES_PER_STATE + pet.frame];
    }
    if (pet.scroll_x != 0 || pet.scroll_y != 0) {
        uint8_t dir = scroll_dir_table[pet.scroll_y + 1][pet.scroll_x + 1];
        return neko_scroll[dir * NEKO_FRAMES_PER_STATE + pet.frame];
    }
    if (pet.wpm <= CONFIG_NICE_VIEW_NEKO_WPM_LOW) {
        return neko_idle[pet.frame];
    }
    if (pet.wpm <= CONFIG_NICE_VIEW_NEKO_WPM_HIGH) {
        return neko_walk[pet.frame];
    }
    return neko_run[pet.frame];
}

static void pet_draw(void) {
    if (pet.canvas == NULL) {
        return;
    }

    fill_background(pet.canvas);

    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);
    lv_point_t ground[] = {{4, PET_GROUND_Y + 2}, {CANVAS_SIZE - 4, PET_GROUND_Y + 2}};
    canvas_draw_line(pet.canvas, ground, 2, &line_dsc);

    int32_t y = PET_GROUND_Y - NEKO_FRAME_H;
    if (pet.jumping && pet.ctrl_held == 0) {
        y -= PET_JUMP_PX;
    }

    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    canvas_draw_img(pet.canvas, (CANVAS_SIZE - NEKO_FRAME_W) / 2, y, pick_frame(), &img_dsc);

    rotate_canvas(pet.canvas);
}

static void pet_timer_cb(lv_timer_t *timer) {
    int64_t now = k_uptime_get();
    if (pet.scroll_x != 0 && now - pet.last_scroll_x > CONFIG_NICE_VIEW_NEKO_SCROLL_TIMEOUT_MS) {
        pet.scroll_x = 0;
    }
    if (pet.scroll_y != 0 && now - pet.last_scroll_y > CONFIG_NICE_VIEW_NEKO_SCROLL_TIMEOUT_MS) {
        pet.scroll_y = 0;
    }

    pet.frame = (pet.frame + 1) % NEKO_FRAMES_PER_STATE;
    pet_draw();
}

void pet_attach_canvas(lv_obj_t *canvas) {
    pet.canvas = canvas;
    pet_draw();
    if (pet.timer == NULL) {
        pet.timer = lv_timer_create(pet_timer_cb, CONFIG_NICE_VIEW_NEKO_FRAME_MS, NULL);
    }
}

void pet_set_wpm(uint8_t wpm) { pet.wpm = wpm; }

void pet_set_caps(bool caps) { pet.caps = caps; }

void pet_key_event(uint16_t usage_page, uint32_t keycode, bool pressed) {
    if (usage_page != PET_HID_USAGE_PAGE_KEY) {
        return;
    }

    switch (keycode) {
    case PET_HID_KEY_LCTRL:
    case PET_HID_KEY_RCTRL:
        if (pressed) {
            pet.ctrl_held++;
        } else if (pet.ctrl_held > 0) {
            pet.ctrl_held--;
        }
        break;
#if IS_ENABLED(CONFIG_NICE_VIEW_NEKO_JUMP)
    case PET_HID_KEY_SPACE:
        pet.jumping = pressed && pet.wpm > CONFIG_NICE_VIEW_NEKO_WPM_LOW;
        break;
#endif
    default:
        break;
    }
}

void pet_sensor_event(uint8_t sensor_index, int32_t delta) {
    if (delta == 0) {
        return;
    }

    int64_t now = k_uptime_get();
    if (sensor_index == PET_SENSOR_LEFT) {
        pet.scroll_x = delta > 0 ? 1 : -1;
        pet.ctrl_dir = delta > 0 ? PET_CTRL_E : PET_CTRL_W;
        pet.last_scroll_x = now;
    } else if (sensor_index == PET_SENSOR_RIGHT) {
        pet.scroll_y = delta > 0 ? 1 : -1;
        pet.ctrl_dir = delta > 0 ? PET_CTRL_N : PET_CTRL_S;
        pet.last_scroll_y = now;
    }
}

void pet_set_active(bool active) {
    if (pet.timer == NULL) {
        return;
    }
    if (active) {
        lv_timer_resume(pet.timer);
    } else {
        lv_timer_pause(pet.timer);
    }
}
