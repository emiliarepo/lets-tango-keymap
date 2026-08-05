// Static underglow for the standalone agent macropad -- plain RGBLIGHT, no
// animation, no split RPC. Deliberately NOT the Tango underglow.c (that one is a
// split-RPC, 5-layer spinning animation, the wrong fit for a lone half).
//
// This module is the mechanism only: put the strip in static mode and expose one
// setter. Colour *policy* lives in keymap.c, which -- like the Tango firmware
// colours the active layer -- sets the hue from the active layer + OS-layout mode.
//
// Task 2 seam: the companion daemon will call set_status_color() over Raw HID to
// show agent status (idle/thinking/waiting/...), overriding the layer colour.
#pragma once

#include <stdint.h>

// Call from keyboard_post_init_user(): put the strip in static-light mode.
void status_underglow_init(void);

// Set the underglow colour without touching EEPROM. The single RGB seam.
void set_status_color(uint8_t hue, uint8_t sat, uint8_t val);
