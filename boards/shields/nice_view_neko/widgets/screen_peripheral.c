/*
 * Screen plumbing based on nice-view-gem (MIT, © M165437).
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/activity.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/usb.h>

#include "battery.h"
#include "output.h"
#include "pet.h"
#include "screen_peripheral.h"

/* Put the napping cat at the bottom of the peripheral screen. */
#define SLICE_PET_PERIPHERAL_START 92

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/**
 * Draw buffers
 **/

static void draw_status(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    fill_background(canvas);

    draw_output_status(canvas, state);
    draw_battery_status(canvas, state);

    rotate_canvas(canvas);
}

/**
 * Battery status
 **/

static void set_battery_status(struct zmk_widget_screen *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_status(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state);

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

/**
 * Peripheral status
 **/

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_screen *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;

    draw_status(widget->obj, &widget->state);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

/**
 * Activity -> pause/resume pet animation
 **/

struct pet_activity_state {
    bool active;
};

static void pet_activity_update_cb(struct pet_activity_state state) {
    pet_set_active(state.active);
}

static struct pet_activity_state pet_activity_get_state(const zmk_event_t *eh) {
    return (struct pet_activity_state){.active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pet_activity, struct pet_activity_state,
                            pet_activity_update_cb, pet_activity_get_state)
ZMK_SUBSCRIPTION(widget_pet_activity, zmk_activity_state_changed);

/**
 * Initialization
 **/

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, SCREEN_HEIGHT, SCREEN_WIDTH);
    lv_obj_set_style_bg_color(widget->obj, LVGL_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *status = lv_canvas_create(widget->obj);
    lv_obj_align(status, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(status, widget->cbuf_status, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);

    lv_obj_t *pet_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(pet_canvas, LV_ALIGN_TOP_RIGHT, -SLICE_PET_PERIPHERAL_START, 0);
    lv_canvas_set_buffer(pet_canvas, widget->cbuf_pet, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();
    widget_pet_activity_init();

    pet_attach_canvas(pet_canvas);

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
