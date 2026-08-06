#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "status.h"
#include "underglow.h"
#include "layers.h"

#define CMD_STATUS 0x01
#define CMD_FLEET  0x10
#define STATUS_STALE_MS 5000
#define BREATHE_PERIOD_MS 2600

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

void status_render(void) {
    switch (s_code) {
        case ST_NONE:    set_status_color(0, 0, 0); break;
        case ST_IDLE:    set_status_color(170, 180, 40); break;
        case ST_WAITING: set_status_color(85, 255, 200); break;
        case ST_ERROR:   set_status_color(0, 255, 200); break;
        case ST_RUNNING: {
            uint16_t t = timer_read() % BREATHE_PERIOD_MS;
            uint8_t phase = (uint8_t)((uint32_t)t * 255u / BREATHE_PERIOD_MS);
            uint8_t tri = phase < 128 ? (uint8_t)(phase * 2) : (uint8_t)((255 - phase) * 2);
            uint8_t val = 30 + (uint8_t)((uint16_t)tri * 200u / 255u);
            set_status_color(28, 255, val);   // amber breathe
            break;
        }
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
