/*
 * Screen plumbing based on nice-view-gem (MIT, © M165437).
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/activity.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/wpm.h>

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SENSORS)
#include <zmk/events/sensor_event.h>
#include <zmk/sensors.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#define PET_HID_LED_CAPS_LOCK 0x02
#endif

#include "battery.h"
#include "layer.h"
#include "output.h"
#include "pet.h"
#include "profile.h"
#include "screen.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/**
 * Draw buffers
 **/

static void draw_status(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    fill_background(canvas);

    draw_output_status(canvas, state);
    draw_battery_status(canvas, state);

    rotate_canvas(canvas);
}

static void draw_bottom(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);
    fill_background(canvas);

    draw_profile_status(canvas, state);
    draw_layer_status(canvas, state);

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
 * Layer status
 **/

static void set_layer_status(struct zmk_widget_screen *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_bottom(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

/**
 * Output status
 **/

static void set_output_status(struct zmk_widget_screen *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_status(widget->obj, &widget->state);
    draw_bottom(widget->obj, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoint_get_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

/**
 * WPM -> pet
 **/

struct pet_wpm_state {
    uint8_t wpm;
};

static void pet_wpm_update_cb(struct pet_wpm_state state) { pet_set_wpm(state.wpm); }

static struct pet_wpm_state pet_wpm_get_state(const zmk_event_t *eh) {
    return (struct pet_wpm_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pet_wpm, struct pet_wpm_state, pet_wpm_update_cb,
                            pet_wpm_get_state)
ZMK_SUBSCRIPTION(widget_pet_wpm, zmk_wpm_state_changed);

/**
 * Keycodes (ctrl, space) -> pet
 **/

struct pet_key_state {
    uint16_t usage_page;
    uint32_t keycode;
    bool pressed;
    bool valid;
};

static void pet_key_update_cb(struct pet_key_state state) {
    if (state.valid) {
        pet_key_event(state.usage_page, state.keycode, state.pressed);
    }
}

static struct pet_key_state pet_key_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return (struct pet_key_state){.valid = false};
    }
    return (struct pet_key_state){
        .usage_page = ev->usage_page,
        .keycode = ev->keycode,
        .pressed = ev->state,
        .valid = true,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pet_key, struct pet_key_state, pet_key_update_cb,
                            pet_key_get_state)
ZMK_SUBSCRIPTION(widget_pet_key, zmk_keycode_state_changed);

/**
 * Encoders -> pet
 **/

#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SENSORS)
struct pet_sensor_state {
    uint8_t index;
    int32_t delta;
    bool valid;
};

static void pet_sensor_update_cb(struct pet_sensor_state state) {
    if (state.valid) {
        pet_sensor_event(state.index, state.delta);
    }
}

static struct pet_sensor_state pet_sensor_get_state(const zmk_event_t *eh) {
    const struct zmk_sensor_event *ev = as_zmk_sensor_event(eh);
    if (ev == NULL || ev->channel_data_size < 1) {
        return (struct pet_sensor_state){.valid = false};
    }
    return (struct pet_sensor_state){
        .index = ev->sensor_index,
        .delta = ev->channel_data[0].value.val1,
        .valid = true,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pet_sensor, struct pet_sensor_state, pet_sensor_update_cb,
                            pet_sensor_get_state)
ZMK_SUBSCRIPTION(widget_pet_sensor, zmk_sensor_event);
#endif /* IS_ENABLED(CONFIG_ZMK_KEYMAP_SENSORS) */

/**
 * Caps lock -> pet
 **/

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
struct pet_caps_state {
    bool caps;
};

static void pet_caps_update_cb(struct pet_caps_state state) { pet_set_caps(state.caps); }

static struct pet_caps_state pet_caps_get_state(const zmk_event_t *eh) {
    return (struct pet_caps_state){
        .caps = (zmk_hid_indicators_get_current_profile() & PET_HID_LED_CAPS_LOCK) != 0,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_pet_caps, struct pet_caps_state, pet_caps_update_cb,
                            pet_caps_get_state)
ZMK_SUBSCRIPTION(widget_pet_caps, zmk_hid_indicators_changed);
#endif /* IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) */

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

    lv_obj_t *pet_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(pet_canvas, LV_ALIGN_TOP_RIGHT, -SLICE_PET_START, 0);
    lv_canvas_set_buffer(pet_canvas, widget->cbuf_pet, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);

    lv_obj_t *status = lv_canvas_create(widget->obj);
    lv_obj_align(status, LV_ALIGN_TOP_RIGHT, -SLICE_STATUS_START, 0);
    lv_canvas_set_buffer(status, widget->cbuf_status, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_RIGHT, -SLICE_BOTTOM_START, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf_bottom, CANVAS_SIZE, CANVAS_SIZE,
                         CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_layer_status_init();
    widget_output_status_init();
    widget_pet_wpm_init();
    widget_pet_key_init();
#if IS_ENABLED(CONFIG_ZMK_KEYMAP_SENSORS)
    widget_pet_sensor_init();
#endif
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    widget_pet_caps_init();
#endif
    widget_pet_activity_init();

    pet_attach_canvas(pet_canvas);

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
