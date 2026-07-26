// Vitamins Included (Let's Tango) - Colemak keymap
// Migrated 1:1 from the original ZMK ALU40 keymap; base layer converted QWERTY -> standard Colemak.
//
//   Board : vitamins_included/rev2   (integrated ATmega32U4, USB-C, qmk-dfu bootloader,
//                                     12 WS2812 underglow LEDs = 6 per half)
//   Layout: LAYOUT_ortho_4x12  (4x12 split ortholinear)
//
// Two base layers (COLEMAK / QWERTY); switch between them persistently from ADJUST (PDF keys).
// Bases sit below the momentary layers so RAISE/LOWER/ADJUST overlay whichever base is active.
//
// ADJUST is reached like in ZMK: hold one thumb layer key (RAISE or LOWER) and press the
// other thumb key, which is MO(ADJUST) on that layer.
//
// Layer enums live in layers.h. The underglow lives in underglow.c -- this file contains no
// RGB code, it just forwards the three hooks the animation needs.

#include QMK_KEYBOARD_H
#include "layers.h"
#include "underglow.h"

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
 *  QMK hooks -> underglow module
 * ------------------------------------------------------------------ */

void keyboard_post_init_user(void) {
    underglow_init();
}

void housekeeping_task_user(void) {
    underglow_task();
}

// Tap dance defers layer_on() until the dance resolves, so tell the underglow about the
// physical press directly and it can colour the held layer immediately.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TD(TD_LOWER): underglow_hold_hint(_LOWER, record->event.pressed); break;
        case TD(TD_RAISE): underglow_hold_hint(_RAISE, record->event.pressed); break;
    }
    return true;  // let tap dance handle the key as usual
}
