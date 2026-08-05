// Per-layer spinning underglow for the Let's Tango (vitamins_included/rev2).
//
// ---------------------------------------------------------------------------
// How rgblight indices work on this board (this is the bit that bit us)
// ---------------------------------------------------------------------------
// keyboard.json declares:  rgblight.led_count = 12, rgblight.split_count = [6, 6]
// which generates RGBLED_SPLIT {6,6}, which implicitly defines RGBLIGHT_SPLIT.
//
// That means the two halves share ONE 12-entry logical index space, and each half
// is given a "clipping range" into it by split_pre_init() based on handedness:
//
//     left  half : rgblight_set_clipping_range(0, 6)   -> owns logical 0..5
//     right half : rgblight_set_clipping_range(6, 6)   -> owns logical 6..11
//
// rgblight_led_index(i) then returns `i - clipping_start_pos`. So on the right half,
// writing logical index 0 resolves to physical index (0 - 6) = 250 as a uint8_t, and
// ws2812_set_color() does NOT bounds check -> an out-of-bounds write ~750 bytes past
// ws2812_leds[12]. A half that paints all 12 indices corrupts RAM 25x a second.
//
// So: each half paints ONLY its own six, and we index the physical chain directly.
// ---------------------------------------------------------------------------

#include QMK_KEYBOARD_H
#include "transactions.h"
#include "layers.h"
#include "underglow.h"

#define SPIN_INTERVAL 40  // ms per frame (~25 fps)
#define SPIN_SPEED    2   // phase units per frame -> ~5 s per revolution of one half's ring
#define SPIN_SPAN     34  // hue spread across the ring (keeps the colour family)

// Physical LED order, measured on the board (cable in the left half, TRRS across the top).
// Each half's six LEDs form a perimeter ring, and BOTH chains run clockwise from that half's
// own top-right corner -- the flipped PCB does not reverse the chain:
//
//   left  chain 0..5 : top-right(inner), right, bottom-right, bottom-left, left, top-left(outer)
//   right chain 0..5 : top-right(outer), right, bottom-right, bottom-left, left, top-left(inner)
//
// The animation is one ring per half, six slots, driven by the same phase on both -- so slot s
// is lit identically on both halves at the same instant. Because the chains are translations of
// each other rather than reflections, the second half's write order has to be reversed to turn
// that into a mirror image about the centreline. That pairs up:
//
//   left top-right(inner) <-> right top-left(inner)     (drawing: 1 <-> 7)
//   left right            <-> right left                (            2 <-> 8)
//   ... out to ...
//   left top-left(outer)  <-> right top-right(outer)    (            6 <-> 12)
//
// 1 = butterfly, halves reflect each other. 0 = both halves in the same absolute orientation.
#define SPIN_MIRROR_HALVES 1

// Hue curve: triangle wave, 0 -> 255 -> 0 around the ring.
//
// It has to be symmetric. The ring is a closed loop, so any curve used here must return to its
// starting value -- a sawtooth ramp snaps back at the wrap and you get a hard edge (on RAISE,
// hue 147 jumping straight to 113: azure to lime, no transition). The triangle is continuous
// all the way around, which is why the original read as smooth.


typedef struct __attribute__((packed)) {
    uint8_t hue;
    uint8_t phase;
    uint8_t val;
} spin_sync_t;

static uint8_t  spin_phase     = 0;
static uint16_t spin_timer     = 0;
static uint8_t  spin_hold_mask = 0;  // bit N set = layer N's key is physically down

static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case _QWERTY: return 85;   // green
        case _LOWER:  return 28;   // orange
        case _RAISE:  return 130;  // cyan
        case _ADJUST: return 0;    // red
        default:      return 197;  // _COLEMAK, purple
    }
}

// Paint this half's six LEDs for one frame, then flush once.
//
// Deliberately bypasses rgblight_sethsv_at(): that helper calls rgblight_set() ->
// rgblight_driver.flush() on EVERY led, so a 12-led loop pushed the whole strip 12x
// per frame. On AVR each flush runs with interrupts disabled (~360 us + 80 us reset),
// so at 25 fps that was ~4 ms/frame of cli() -- enough to starve the bitbang split
// serial link on D0 and drop the sync transactions below. One flush per frame instead.
static void spin_render(uint8_t base_hue, uint8_t phase, uint8_t val) {
    const uint8_t count = rgblight_ranges.clipping_num_leds;  // 6 = this half's perimeter ring

#if RGBLIGHT_LIMIT_VAL < 255
    if (val > RGBLIGHT_LIMIT_VAL) val = RGBLIGHT_LIMIT_VAL;
#endif

    for (uint8_t slot = 0; slot < count; slot++) {
        // Position around this half's ring. Same slot numbering and same phase on both halves,
        // so slot s is always the same colour on both -- that is what keeps them locked.
        // Multiply before dividing: 256/6 truncates to 42 and only spans 252, which leaves a
        // visibly wider hue step at the wrap.
        const uint8_t pos = (uint8_t)(((uint16_t)slot * 256u) / count + phase);

        // triangle wave 0..255..0 so the sweep wraps smoothly around the ring
        const uint8_t ramp = (pos < 128) ? (uint8_t)(pos << 1) : (uint8_t)((255 - pos) << 1);
        const uint8_t hue  = (uint8_t)(base_hue - (SPIN_SPAN / 2) + (((uint16_t)ramp * SPIN_SPAN) / 255));
        const rgb_t   rgb  = hsv_to_rgb((hsv_t){hue, 255, val});

        // Physical chain index. Writing slot s to chain s on both halves would translate the
        // pattern sideways; reversing the right half reflects it instead.
#if SPIN_MIRROR_HALVES
        const uint8_t phys = is_keyboard_left() ? slot : (uint8_t)(count - 1 - slot);
#else
        const uint8_t phys = slot;
#endif
        rgblight_driver.set_color(phys, rgb.r, rgb.g, rgb.b);
    }

    rgblight_driver.flush();
}

// Slave side: render from whatever the master last sent.
static void spin_sync_handler(uint8_t in_len, const void *in_data, uint8_t out_len, void *out_data) {
    if (in_len < sizeof(spin_sync_t)) return;
    const spin_sync_t *d = (const spin_sync_t *)in_data;
    spin_render(d->hue, d->phase, d->val);
}

void underglow_init(void) {
    transaction_register_rpc(RGB_SYNC, spin_sync_handler);
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);  // silence the built-in effects
    rgblight_sethsv_noeeprom(hue_for_layer(_COLEMAK), 255, rgblight_get_val());
}

void underglow_hold_hint(uint8_t layer, bool held) {
    if (layer >= 8) return;
    if (held) {
        spin_hold_mask |= (uint8_t)(1u << layer);
    } else {
        spin_hold_mask &= (uint8_t) ~(1u << layer);
    }
}

// Which layer's colour to show. A genuinely active higher layer (e.g. ADJUST) beats a
// merely-held key, so take the max of the two.
static uint8_t spin_active_layer(void) {
    uint8_t active = get_highest_layer(layer_state | default_layer_state);
    for (uint8_t l = 7; l > active; l--) {
        if (spin_hold_mask & (uint8_t)(1u << l)) return l;
    }
    return active;
}

void underglow_task(void) {
    if (!is_keyboard_master()) return;  // the slave is driven by spin_sync_handler
    if (timer_elapsed(spin_timer) < SPIN_INTERVAL) return;
    spin_timer = timer_read();
    spin_phase += SPIN_SPEED;

    const uint8_t hue = hue_for_layer(spin_active_layer());
    const uint8_t val = rgblight_is_enabled() ? rgblight_get_val() : 0;  // respect UG_TOGG / brightness

    // Send first and check the result: if the other half missed this frame, drop it here
    // too and retry the same phase next tick. Better a shared stutter than a visible drift.
    const spin_sync_t payload = {hue, spin_phase, val};
    if (!transaction_rpc_send(RGB_SYNC, sizeof(payload), &payload)) {
        spin_phase -= SPIN_SPEED;
        return;
    }

    spin_render(hue, spin_phase, val);
}
