#define DT_DRV_COMPAT zmk_behavior_pet_relay

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include "pet_relay.h"
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include "pet.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int pet_relay_init(const struct device *dev) { return 0; }

static int pet_relay_pressed(struct zmk_behavior_binding *binding,
                             struct zmk_behavior_binding_event event) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (binding->param1) {
    case PET_RELAY_WPM:
        pet_set_wpm(binding->param2);
        break;
    case PET_RELAY_KEY_PRESS:
        pet_key_event(PET_RELAY_HID_USAGE_PAGE_KEY, binding->param2, true);
        break;
    case PET_RELAY_KEY_RELEASE:
        pet_key_event(PET_RELAY_HID_USAGE_PAGE_KEY, binding->param2, false);
        break;
    }
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}

static int pet_relay_released(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api pet_relay_api = {
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
    .binding_pressed = pet_relay_pressed,
    .binding_released = pet_relay_released,
};

BEHAVIOR_DT_INST_DEFINE(0, pet_relay_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pet_relay_api);

#endif
