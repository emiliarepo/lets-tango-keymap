#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "status.h"
#include "underglow.h"
#include "layers.h"

#define CMD_STATUS 0x01
#define STATUS_STALE_MS 5000

// Waiting blinks to grab attention: green ON, then backlight fully OFF, repeat.
// Green-dominant so it reads as "green, blinking" rather than "mostly off".
#define WAITING_ON_MS  550
#define WAITING_OFF_MS 300

enum { ST_NONE=0, ST_IDLE, ST_RUNNING, ST_WAITING, ST_ERROR };

extern void underglow_repaint(void);   // arbiter in keymap.c

static uint8_t  s_code  = ST_NONE;
static uint16_t s_last  = 0;
static bool     s_fresh = false;       // true only for running/waiting/error

// Waiting-blink state. We toggle the strip off with rgblight_disable_noeeprom()
// (a real "backlight off"), which does NOT touch the stored brightness -- so the
// blink can never corrupt Val±, unlike scaling the value down to 0. blink_off
// means WE turned it off, so blink_restore() only re-enables our own blink,
// leaving a user UG_TOGG alone.
static uint16_t blink_timer = 0;
static bool     blink_off   = false;

static void blink_restore(void) {
    if (blink_off) { rgblight_enable_noeeprom(); blink_off = false; }
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != CMD_STATUS) return;
    s_code = data[1];
    s_last = timer_read();
    // Only running/waiting/error take over the underglow. none + idle mean
    // "nothing urgent" -> show the pad's normal alive colour (keymap's local policy).
    s_fresh = (s_code == ST_RUNNING || s_code == ST_WAITING || s_code == ST_ERROR);
    blink_restore();               // lights back on before we repaint
    blink_timer = timer_read();    // start any new blink from a full ON phase
    underglow_repaint();
}

void status_init(void) { s_fresh = false; }
bool status_is_active(void) { return s_fresh; }

// Solid colours at the user's current brightness (rgblight_get_val, set to itself
// so it stays put). The waiting blink is driven in status_tick, not here.
void status_render(void) {
    const uint8_t v = rgblight_get_val();
    switch (s_code) {
        case ST_RUNNING: set_status_color(28, 255, v); break;  // amber = working
        case ST_WAITING: set_status_color(85, 255, v); break;  // green = your turn
        case ST_ERROR:   set_status_color(0,  255, v); break;  // red   = error
        default:         break;
    }
}

void status_tick(void) {
    if (!s_fresh) { blink_restore(); return; }              // idle/none -> local colour
    if (timer_elapsed(s_last) > STATUS_STALE_MS) {
        s_fresh = false;
        blink_restore();
        underglow_repaint();                                // daemon quiet -> alive colour
        return;
    }
    if (get_highest_layer(layer_state) == _CTL) { blink_restore(); return; }  // Fn -> control hue
    if (s_code != ST_WAITING) { blink_restore(); return; }  // running/error -> solid

    // WAITING: blink green <-> backlight off.
    const uint16_t phase_ms = blink_off ? WAITING_OFF_MS : WAITING_ON_MS;
    if (timer_elapsed(blink_timer) >= phase_ms) {
        blink_timer = timer_read();
        if (blink_off) {
            rgblight_enable_noeeprom();
            blink_off = false;
            status_render();          // green back on at the user's brightness
        } else {
            rgblight_disable_noeeprom();
            blink_off = true;         // backlight fully off
        }
    }
}
