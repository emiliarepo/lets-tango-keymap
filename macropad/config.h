#pragma once

// This firmware runs a SINGLE vitamins_included half (the left half) solo over
// USB as a standalone macropad -- there is no TRRS-connected slave.
//
// Without this, a split build waits at startup trying to work out handedness by
// probing the other half over the serial link, which never answers -- the board
// can stall or come up as the "slave" and never enumerate as USB. SPLIT_USB_DETECT
// makes the half that sees USB VBUS declare itself master immediately and skip the
// probe, so the lone half boots straight into a working USB keyboard.
//
// (Documented per SPEC 1.3: add + note if a lone half needs it. Verify on-device;
// if it still stalls, also shorten the split scan with e.g.
// #define SPLIT_MAX_CONNECTION_ERRORS 2 and #define SPLIT_CONNECTION_CHECK_TIMEOUT 500.)
#define SPLIT_USB_DETECT

// Task 2 seam: enabling RAW_ENABLE (rules.mk) + raw_hid_receive() lets the host
// daemon push a status byte to drive the underglow. No config.h change needed here.
