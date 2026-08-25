#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

struct zmk_widget_screen {
    sys_snode_t node;
    lv_obj_t *obj;
    uint8_t cbuf_pet[CANVAS_BUF_SIZE];
    uint8_t cbuf_status[CANVAS_BUF_SIZE];
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    uint8_t cbuf_bottom[CANVAS_BUF_SIZE];
#endif
    struct status_state state;
};

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget);
