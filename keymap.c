// Vitamins Included (Let's Tango) - Colemak keymap
// Migrated 1:1 from the original ZMK ALU40 keymap; base layer converted QWERTY -> standard Colemak.
//
//   Board : vitamins_included/rev2   (integrated ATmega32U4, USB-C, qmk-dfu bootloader,
//                                     12 WS2812 underglow LEDs = 6 per half)
//   Layout: LAYOUT_ortho_4x12  (4x12 split ortholinear)
//
// Layers:
//   0 = COLEMAK (base)   1 = QWERTY (base)   2 = LOWER   3 = RAISE   4 = ADJUST
// Two base layers; switch between them persistently from ADJUST (PDF keys). Bases sit
// below the momentary layers so RAISE/LOWER/ADJUST overlay whichever base is active.
//
// ADJUST is reached like in ZMK: hold one thumb layer key (RAISE or LOWER) and press the
// other thumb key, which is MO(ADJUST) on that layer.
//
// Per-layer RGB underglow: each layer has its own hue family (Colemak purple / QWERTY green /
// LOWER orange / RAISE cyan / ADJUST red) that slowly SPINS around the 12-LED ring. The master
// computes the animation and syncs hue + phase to the slave over a split RPC so both halves
// stay locked together. Flash BOTH halves whenever this changes.

#include QMK_KEYBOARD_H
#include "transactions.h"

enum layers {
    _COLEMAK = 0,  // base (purple underglow)
    _QWERTY,       // 1  base (green underglow) - toggle on Adjust
    _LOWER,        // 2
    _RAISE,        // 3
    _ADJUST        // 4  (control)
};

// Tap-dance keys: hold = momentary layer (like MO), double-tap = lock that layer (double-tap again to unlock).
enum tap_dances {
    TD_LOWER,
    TD_RAISE
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

/* COLEMAK - base (default). Switch bases on the ADJUST layer.
 * ,-----------------------------------------------------------------------------------.
 * | Tab  |  Q   |  W   |  F   |  P   |  G   |  J   |  L   |  U   |  Y   |  ;   | Bksp |
 * | Esc  |  A   |  R   |  S   |  T   |  D   |  H   |  N   |  E   |  I   |  O   |  '   |
 * | Shift|  Z   |  X   |  C   |  V   |  B   |  K   |  M   |  ,   |  .   |  /   |Sft/Ent|
 * | Play | Ctrl | Alt  | Gui  |LOWER |    Space    |RAISE | Left | Down |  Up  |Right |
 * `-----------------------------------------------------------------------------------'
 */
[_COLEMAK] = LAYOUT_ortho_4x12(
    KC_TAB,  KC_Q,    KC_W,    KC_F,    KC_P,       KC_G,     KC_J,    KC_L,       KC_U,    KC_Y,    KC_SCLN, KC_BSPC,
    KC_ESC,  KC_A,    KC_R,    KC_S,    KC_T,       KC_D,     KC_H,    KC_N,       KC_E,    KC_I,    KC_O,    KC_QUOT,
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,       KC_B,     KC_K,    KC_M,       KC_COMM, KC_DOT,  KC_SLSH, RSFT_T(KC_ENT),
    KC_MPLY, KC_LCTL, KC_LALT, KC_LGUI, TD(TD_LOWER), KC_SPC, KC_SPC, TD(TD_RAISE), KC_LEFT, KC_DOWN, KC_UP, KC_RGHT
),

/* QWERTY - alternate base (identical frame, QWERTY alphas). Green underglow.
 * ,-----------------------------------------------------------------------------------.
 * | Tab  |  Q   |  W   |  E   |  R   |  T   |  Y   |  U   |  I   |  O   |  P   | Bksp |
 * | Esc  |  A   |  S   |  D   |  F   |  G   |  H   |  J   |  K   |  L   |  ;   |  '   |
 * | Shift|  Z   |  X   |  C   |  V   |  B   |  N   |  M   |  ,   |  .   |  /   |Sft/Ent|
 * | Play | Ctrl | Alt  | Gui  |LOWER |    Space    |RAISE | Left | Down |  Up  |Right |
 * `-----------------------------------------------------------------------------------'
 */
[_QWERTY] = LAYOUT_ortho_4x12(
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,       KC_T,     KC_Y,    KC_U,       KC_I,    KC_O,    KC_P,    KC_BSPC,
    KC_ESC,  KC_A,    KC_S,    KC_D,    KC_F,       KC_G,     KC_H,    KC_J,       KC_K,    KC_L,    KC_SCLN, KC_QUOT,
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,       KC_B,     KC_N,    KC_M,       KC_COMM, KC_DOT,  KC_SLSH, RSFT_T(KC_ENT),
    KC_MPLY, KC_LCTL, KC_LALT, KC_LGUI, TD(TD_LOWER), KC_SPC, KC_SPC, TD(TD_RAISE), KC_LEFT, KC_DOWN, KC_UP, KC_RGHT
),

/* RAISE
 * ,-----------------------------------------------------------------------------------.
 * |  ~   |  !   |  @   |  #   |  $   |  %   |  ^   |  &   |  *   |  (   |  )   |  |   |
 * |  `   |  1   |  2   |  3   |  4   |  5   |  6   |  7   |  8   |  9   |  0   |  \   |
 * |  ,   |  <   |  >   |  =   |  -   |  _   |  +   |  {   |  }   |  [   |  ]   |  .   |
 * | Esc  | Ctrl | Alt  | Gui  |ADJUST|             |      | Prev | End  | Home | Next |
 * `-----------------------------------------------------------------------------------'
 */
[_RAISE] = LAYOUT_ortho_4x12(
    KC_TILD, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,      KC_PERC,  KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_PIPE,
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,        KC_5,     KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSLS,
    KC_COMM, KC_LT,   KC_GT,   KC_EQL,  KC_MINS,     KC_UNDS,  KC_PLUS, KC_LCBR, KC_RCBR, KC_LBRC, KC_RBRC, KC_DOT,
    KC_ESC,  KC_LCTL, KC_LALT, KC_LGUI, MO(_ADJUST), _______,  _______, _______, KC_MPRV, KC_END,  KC_HOME, KC_MNXT
),

/* LOWER - symbols (left) + NumLock keypad (right)
 * Left hand mirrors RAISE's symbol block; Shift keeps its home spot (row 3, col 1).
 * F-keys moved to the ADJUST layer.
 * ,-----------------------------------------------------------------------------------.
 * |  ~   |  !   |  @   |  #   |  $   |  %   |  7   |  8   |  9   |NumLk |  /   |  -   |
 * |  `   |  1   |  2   |  3   |  4   |  5   |  4   |  5   |  6   |  [   |  ]   |  +   |
 * | Shift|  <   |  >   |  =   |  -   |  _   |  1   |  2   |  3   |  0   |  .   |KP Ent|
 * |      |      |      |      |      |             |ADJUST| Prev | Vol- | Vol+ | Next |
 * `-----------------------------------------------------------------------------------'
 */
[_LOWER] = LAYOUT_ortho_4x12(
    KC_TILD, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,  KC_P7,   KC_P8,       KC_P9,   KC_NUM,  KC_PSLS, KC_PMNS,
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,     KC_P4,   KC_P5,       KC_P6,   KC_LBRC, KC_RBRC, KC_PPLS,
    KC_LSFT, KC_LT,   KC_GT,   KC_EQL,  KC_MINS, KC_UNDS,  KC_P1,   KC_P2,       KC_P3,   KC_P0,   KC_PDOT, KC_PENT,
    _______, _______, _______, _______, _______, _______,  _______, MO(_ADJUST), KC_MPRV, KC_VOLD, KC_VOLU, KC_MNXT
),

/* ADJUST (control). Wired board, so the ZMK Bluetooth keys are dropped.
 *   reset -> QK_RBT   bootloader -> QK_BOOT   underglow on/off + brightness only (hue/sat/mode do
 *   nothing under the custom spin)   base -> QWERTY/Colemak   F1-F12 on row 3
 * ,-----------------------------------------------------------------------------------.
 * | Boot |Reboot| Dbg  |      |      |      |      |      |UGtog |Val-  |Val+  | Del  |
 * |      |Audio |Music |NKRO  |      |      |      |QWERTY|Colemk|      |      |      |
 * |  F1  |  F2  |  F3  |  F4  |  F5  |  F6  |  F7  |  F8  |  F9  | F10  | F11  | F12  |
 * |      |      |      |      |      |             |      |      |      |      |      |
 * `-----------------------------------------------------------------------------------'
 */
[_ADJUST] = LAYOUT_ortho_4x12(
    QK_BOOT, QK_RBT,  DB_TOGG, _______, _______, _______,  _______, _______,      UG_TOGG,       UG_VALD, UG_VALU, KC_DEL,
    _______, AU_TOGG, MU_TOGG, NK_TOGG, _______, _______,  _______, PDF(_QWERTY), PDF(_COLEMAK), _______, _______, _______,
    KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,    KC_F7,   KC_F8,        KC_F9,         KC_F10,  KC_F11,  KC_F12,
    _______, _______, _______, _______, _______, _______,  _______, _______,      _______,       _______, _______, _______
)

};

/* ------------------------------------------------------------------ *
 *  Tap dance: LOWER / RAISE layer keys
 *    hold        -> momentary layer (same as MO)
 *    double-tap  -> lock that layer on; double-tap again to unlock
 *  (A quick single tap does nothing.) The active/locked layer also shows
 *  in the underglow, so a locked layer is easy to see.
 * ------------------------------------------------------------------ */

typedef enum { TD_NONE, TD_SINGLE_TAP, TD_SINGLE_HOLD, TD_DOUBLE_TAP } td_kind_t;

static td_kind_t td_lower_kind = TD_NONE;
static td_kind_t td_raise_kind = TD_NONE;

static td_kind_t td_classify(tap_dance_state_t *state) {
    if (state->count == 1) return state->pressed ? TD_SINGLE_HOLD : TD_SINGLE_TAP;
    if (state->count == 2) return TD_DOUBLE_TAP;
    return TD_NONE;
}

static void td_layer_finished(tap_dance_state_t *state, td_kind_t *kind, uint8_t layer) {
    *kind = td_classify(state);
    if (*kind == TD_SINGLE_HOLD) {
        layer_on(layer);                                   // momentary
    } else if (*kind == TD_DOUBLE_TAP) {
        if (layer_state_is(layer)) layer_off(layer);       // unlock
        else                       layer_on(layer);        // lock
    }
}

static void td_layer_reset(td_kind_t *kind, uint8_t layer) {
    if (*kind == TD_SINGLE_HOLD) layer_off(layer);         // release the momentary hold
    *kind = TD_NONE;                                       // a double-tap lock stays on
}

static void td_lower_finished(tap_dance_state_t *state, void *user_data) { td_layer_finished(state, &td_lower_kind, _LOWER); }
static void td_lower_reset(tap_dance_state_t *state, void *user_data)    { td_layer_reset(&td_lower_kind, _LOWER); }
static void td_raise_finished(tap_dance_state_t *state, void *user_data) { td_layer_finished(state, &td_raise_kind, _RAISE); }
static void td_raise_reset(tap_dance_state_t *state, void *user_data)    { td_layer_reset(&td_raise_kind, _RAISE); }

tap_dance_action_t tap_dance_actions[] = {
    [TD_LOWER] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_lower_finished, td_lower_reset),
    [TD_RAISE] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_raise_finished, td_raise_reset),
};

/* ------------------------------------------------------------------ *
 *  Per-layer SPINNING underglow
 *
 *  Each layer picks a base hue; a narrow gradient (+/- SPIN_SPAN/2 around
 *  that hue, so the colour family is preserved) is swept around the ring
 *  by a slowly advancing phase. The master runs the clock and sends
 *  {hue, phase, val} to the slave every frame so both halves stay in step.
 * ------------------------------------------------------------------ */

#define SPIN_LEDS     RGBLIGHT_LED_COUNT  // 12 (6 per half)
#define SPIN_INTERVAL 40                  // ms per frame (~25 fps)
#define SPIN_SPEED    1                   // phase units per frame -> ~10 s per revolution
#define SPIN_SPAN     34                  // hue spread across the strip (keeps the family)

typedef struct __attribute__((packed)) {
    uint8_t hue;
    uint8_t phase;
    uint8_t val;
} spin_sync_t;

static uint8_t  spin_phase = 0;
static uint16_t spin_timer = 0;

// Physical hold state of the LOWER/RAISE tap-dance keys, used so the underglow reflects
// a held layer immediately (tap dance defers the actual layer-on until the dance resolves).
static bool td_lower_down = false;
static bool td_raise_down = false;

static uint8_t hue_for_layer(uint8_t layer) {
    switch (layer) {
        case _QWERTY: return 85;   // green
        case _LOWER:  return 28;   // orange
        case _RAISE:  return 130;  // cyan
        case _ADJUST: return 0;    // red
        default:      return 197;  // _COLEMAK, purple
    }
}

// Paint the whole ring from a base hue + rotation phase.
static void spin_render(uint8_t base_hue, uint8_t phase, uint8_t val) {
    for (uint8_t i = 0; i < SPIN_LEDS; i++) {
        uint8_t pos = (uint8_t)(i * (256 / SPIN_LEDS) + phase);
        // triangle wave 0..255..0 so the sweep wraps smoothly around the ring
        uint8_t tri = (pos < 128) ? (uint8_t)(pos << 1) : (uint8_t)((255 - pos) << 1);
        uint8_t hue = (uint8_t)(base_hue - (SPIN_SPAN / 2) + ((uint16_t)tri * SPIN_SPAN / 255));
        rgblight_sethsv_at(hue, 255, val, i);
    }
}

// Slave side: render from the values the master sent.
static void spin_sync_handler(uint8_t in_len, const void *in_data, uint8_t out_len, void *out_data) {
    if (in_len < sizeof(spin_sync_t)) return;
    const spin_sync_t *d = (const spin_sync_t *)in_data;
    spin_render(d->hue, d->phase, d->val);
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(RGB_SYNC, spin_sync_handler);
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);  // silence the built-in effects
    rgblight_sethsv_noeeprom(197, 255, 255);             // seed a colour
}

// Master side: advance the animation and push it to the slave.
void housekeeping_task_user(void) {
    if (!is_keyboard_master()) return;
    if (timer_elapsed(spin_timer) < SPIN_INTERVAL) return;
    spin_timer = timer_read();
    spin_phase += SPIN_SPEED;

    uint8_t active = get_highest_layer(layer_state | default_layer_state);
    // If a layer thumb key is physically held, show it now rather than waiting for the
    // tap dance to resolve. A truly active higher layer (e.g. ADJUST) still wins.
    uint8_t hint = td_raise_down ? _RAISE : td_lower_down ? _LOWER : 0;
    if (hint > active) active = hint;
    uint8_t hue = hue_for_layer(active);
    uint8_t val = rgblight_is_enabled() ? rgblight_get_val() : 0;  // respect RGB_TOG / brightness keys

    spin_render(hue, spin_phase, val);                            // this half
    spin_sync_t payload = { hue, spin_phase, val };
    transaction_rpc_send(RGB_SYNC, sizeof(payload), &payload);    // the other half
}

// Track the physical up/down of the tap-dance layer keys (for the immediate underglow hint).
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TD(TD_LOWER): td_lower_down = record->event.pressed; break;
        case TD(TD_RAISE): td_raise_down = record->event.pressed; break;
    }
    return true;  // let tap dance handle the key as usual
}
