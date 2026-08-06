#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "status.h"
#include "underglow.h"
#include "layers.h"

#define CMD_STATUS 0x01
#define CMD_FLEET  0x10
#define STATUS_STALE_MS 5000
#define BREATHE_PERIOD_MS 2600

// Every status uses the user's current underglow level (rgblight_get_val), so
// UG_VALU/UG_VALD govern brightness for all of them. Only `running` varies its
// brightness -- that's the breathe animation itself.
enum { ST_NONE=0, ST_IDLE, ST_RUNNING, ST_WAITING, ST_ERROR };

extern void underglow_repaint(void);   // arbiter in keymap.c

static uint8_t  s_code  = ST_NONE;
static uint16_t s_last  = 0;
static bool     s_fresh = false;

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != CMD_STATUS) return;
    s_code = data[1];
    s_last = timer_read();
    s_fresh = true;
    underglow_repaint();               // repaint immediately on change
}

void status_init(void) { s_fresh = false; }
bool status_is_active(void) { return s_fresh; }

// Continuous breathe triangle 0..255..0, advanced by elapsed time rather than
// (timer_read() % period). timer_read() is a 16-bit ms counter that wraps every
// ~65 s and 65536 isn't a multiple of the period, so the old modulo produced a
// visible discontinuity at each wrap. Unsigned 16-bit subtraction gives the correct
// elapsed delta across the wrap, so the accumulated phase stays smooth.
static uint16_t breathe_phase = 0;
static uint16_t breathe_last  = 0;

static uint8_t breathe_tri(void) {
    const uint16_t now = timer_read();
    breathe_phase += (uint16_t)(now - breathe_last);   // wrap-safe elapsed ms
    breathe_last = now;
    while (breathe_phase >= BREATHE_PERIOD_MS) breathe_phase -= BREATHE_PERIOD_MS;

    const uint16_t half = BREATHE_PERIOD_MS / 2;
    const uint16_t d = breathe_phase < half ? breathe_phase : (BREATHE_PERIOD_MS - breathe_phase);
    return (uint8_t)((uint32_t)d * 255u / half);       // 0..255 up, 255..0 down
}

void status_render(void) {
    const uint8_t uval = rgblight_get_val();  // the user's brightness (UG_VALU/VALD)
    switch (s_code) {
        case ST_NONE:    set_status_color(0, 0, 0); break;
        case ST_IDLE:    set_status_color(170, 180, uval); break;
        case ST_WAITING: set_status_color(85, 255, uval); break;
        case ST_ERROR:   set_status_color(0, 255, uval); break;
        case ST_RUNNING: // amber, breathing across the user's full brightness
            set_status_color(28, 255, (uint8_t)((uint16_t)uval * breathe_tri() / 255u));
            break;
        default: s_fresh = false; break;
    }
}

void status_tick(void) {
    if (!s_fresh) return;
    if (timer_elapsed(s_last) > STATUS_STALE_MS) {
        s_fresh = false;
        underglow_repaint();               // hand colour back to local policy
        return;
    }
    if (get_highest_layer(layer_state) == _CTL) return;  // _CTL shows control hue
    status_render();                        // drives the breathe animation
}
