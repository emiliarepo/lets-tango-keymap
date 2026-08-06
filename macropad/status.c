#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "status.h"
#include "underglow.h"
#include "layers.h"

#define CMD_STATUS 0x01
#define STATUS_STALE_MS 5000

enum { ST_NONE=0, ST_IDLE, ST_RUNNING, ST_WAITING, ST_ERROR };

extern void underglow_repaint(void);   // arbiter in keymap.c

static uint8_t  s_code  = ST_NONE;
static uint16_t s_last  = 0;
static bool     s_fresh = false;       // true only for the "active" states below

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != CMD_STATUS) return;
    s_code = data[1];
    s_last = timer_read();
    // Only running/waiting/error take over the underglow. none + idle mean
    // "nothing urgent" -> show the pad's normal alive colour (keymap's local policy).
    s_fresh = (s_code == ST_RUNNING || s_code == ST_WAITING || s_code == ST_ERROR);
    underglow_repaint();               // paint once, on change
}

void status_init(void) { s_fresh = false; }
bool status_is_active(void) { return s_fresh; }

// Solid colours only -- no animation. Each renders at the user's current
// brightness (rgblight_get_val), which stays put because we set it to itself.
void status_render(void) {
    const uint8_t v = rgblight_get_val();
    switch (s_code) {
        case ST_RUNNING: set_status_color(28, 255, v); break;  // amber = working
        case ST_WAITING: set_status_color(85, 255, v); break;  // green = your turn
        case ST_ERROR:   set_status_color(0,  255, v); break;  // red   = error
        default:         break;                                // not reached when active
    }
}

void status_tick(void) {
    if (!s_fresh) return;
    if (timer_elapsed(s_last) > STATUS_STALE_MS) {
        s_fresh = false;
        underglow_repaint();           // daemon went quiet -> back to the alive colour
    }
    // Solid colour: nothing to animate; it was painted on change / layer switch.
}
