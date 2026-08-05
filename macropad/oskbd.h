// OS keyboard-layout awareness -- the only layout logic the pad needs.
//
// The pad emits raw HID keycodes; the OS turns them into characters using its
// ACTIVE layout. The user always types Colemak but switches WHERE Colemak lives
// per task, so this is flipped often (from OSLay on _CTL) and persisted to EEPROM:
//
//   OS-QWERTY (default, VM-safe): keyboard does Colemak, OS stays US-QWERTY. Our
//                                 raw keycodes land as intended -> send unchanged.
//   OS-Colemak (gaming):          OS applies a Colemak override, so it re-maps our
//                                 keycodes -> pre-compensate letters back to intent.
//
// This module owns OS-mode state + typing only; keymap.c reflects the mode in the
// underglow (base layer hue), so no RGB dependency lives here.
#pragma once

#include <stdbool.h>

// Read the persisted state. Call from keyboard_post_init_user().
void os_layout_init(void);

bool os_layout_is_colemak(void);

// Flip the mode and persist to EEPROM. (The underglow updates via keymap.c's
// layer hook when you release Fn back to the base layer.)
void os_layout_toggle(void);

// Type an ASCII string as the user INTENDS it, compensating for the OS layout.
void cc_type(const char *s);

// Ctrl+<letter>, layout-compensated (e.g. Verbose = Ctrl-O).
void cc_ctrl_letter(char letter);
