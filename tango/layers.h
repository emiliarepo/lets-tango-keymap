// Layer + tap-dance identifiers.
// Shared by keymap.c (which builds keymaps[]) and underglow.c (which colours them).
#pragma once

enum layers {
    _COLEMAK = 0,  // base (purple underglow)
    _QWERTY,       // 1  base (green underglow) - toggle on ADJUST
    _LOWER,        // 2  (orange)
    _RAISE,        // 3  (cyan)
    _ADJUST        // 4  control (red)
};

// Tap-dance keys: hold = momentary layer (like MO), double-tap = lock (double-tap again to unlock).
enum tap_dances {
    TD_LOWER,
    TD_RAISE
};
