// Static underglow -- plain RGBLIGHT, no animation, no split RPC.
// See underglow.h for why this is not the Tango underglow.c. Mechanism only;
// keymap.c chooses the colour from the active layer + OS-layout mode.
#include QMK_KEYBOARD_H
#include "underglow.h"

void set_status_color(uint8_t hue, uint8_t sat, uint8_t val) {
    rgblight_sethsv_noeeprom(hue, sat, val);
}

void status_underglow_init(void) {
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);  // silence built-in effects
}
