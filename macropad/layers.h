// Layer identifiers for the agent macropad.
//
// A vitamins_included left half run solo over USB: two layers only. The base
// layer is the memorised agent loop; _CTL (held via Fn) carries the QMK/system
// controls that must not be hit by accident.
#pragma once

enum layers {
    _BASE = 0,  // agent loop (calm static underglow)
    _CTL        // 1  controls: boot / RGB / OS-layout toggle (hold Fn)
};
