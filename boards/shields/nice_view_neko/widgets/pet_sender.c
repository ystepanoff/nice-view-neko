#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/wpm_state_changed.h>

#include "pet_relay.h"

static void pet_relay_invoke(uint32_t type, uint32_t value) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(pet_rly)),
        .param1 = type,
        .param2 = value,
    };
    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
    };
    zmk_behavior_invoke_binding(&binding, event, true);
}

static int pet_sender_wpm_listener(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    if (ev != NULL) {
        pet_relay_invoke(PET_RELAY_WPM, ev->state);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(pet_sender_wpm, pet_sender_wpm_listener);
ZMK_SUBSCRIPTION(pet_sender_wpm, zmk_wpm_state_changed);

static int pet_sender_key_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || ev->usage_page != PET_RELAY_HID_USAGE_PAGE_KEY) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    switch (ev->keycode) {
    case PET_RELAY_HID_KEY_LCTRL:
    case PET_RELAY_HID_KEY_RCTRL:
    case PET_RELAY_HID_KEY_SPACE:
        pet_relay_invoke(ev->state ? PET_RELAY_KEY_PRESS : PET_RELAY_KEY_RELEASE, ev->keycode);
        break;
    default:
        break;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(pet_sender_key, pet_sender_key_listener);
ZMK_SUBSCRIPTION(pet_sender_key, zmk_keycode_state_changed);
