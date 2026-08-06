// Agent macropad -- Claude Code control pad on a vitamins_included left half.
//
//   Board : vitamins_included/rev2   (ATmega32U4, USB-C, qmk-dfu bootloader)
//   Run   : ONE half, solo over USB (see config.h -> SPLIT_USB_DETECT)
//   Layout: LAYOUT_ortho_4x12 -- the left six columns carry the real keys, the
//           right six are KC_NO. Reusing rev2 avoids authoring a 4x6 keyboard def
//           (which would need the half's matrix pinout, which we don't have).
//
// Like the Tango keymap, this file is glue only: it declares keymaps[] and forwards
// the QMK hooks to the modules. The two concerns live next door:
//   oskbd.c/.h     -- OS keyboard-layout awareness (Colemak/QWERTY compensation)
//   underglow.c/.h -- static "pad is alive" RGB + the Task-2 set_status_color() seam
//
// ── Base layer (the agent loop) ──────────────────────────────────────────────
//    Stop     Approve  Yes-all  Reject   Mode     Rewind
//    Cont     Compact  Clear    Review   Model    Cost
//   ◄Agent    Agent►   New      Jump     Scrl↑    Scrl↓
//    Plan     Verbose  Diff     Resume   ----     Fn
//
// ── Control layer _CTL (hold Fn) ─────────────────────────────────────────────
//    Boot   Reboot  UGtog  Color  Val-   Val+
//    OSLay  Dbg     ----   ----   ----   ----
//    ----   ----    ----   ----   ----   ----
//    ----   ----    ----   ----   ----   (Fn)

#include QMK_KEYBOARD_H
#include "layers.h"
#include "oskbd.h"
#include "underglow.h"
#include "status.h"
#include "raw_hid.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Claude Code control strings  ⚠️ CONFIRM against your installed Claude Code
//  version -- these are the only edits you should ever need. Slash commands
//  verified against Claude Code v2.1.x (/compact /clear /review /model /cost /plan
//  /diff /resume all exist). Two placeholders worth knowing:
//    • Yes-all: Claude Code has NO dedicated "approve & don't ask again" key
//      (it's a /permissions rule or a session mode), so CC_YESALL ships a
//      best-effort "pick the 2nd prompt option" (Down, Enter).
//    • Verbose: the Ctrl-O transcript toggle, not a slash command.
// ─────────────────────────────────────────────────────────────────────────────
#define CC_STR_CONT     "continue\n"
#define CC_STR_COMPACT  "/compact\n"
#define CC_STR_CLEAR    "/clear\n"
#define CC_STR_REVIEW   "/review\n"
#define CC_STR_MODEL    "/model\n"
#define CC_STR_COST     "/cost\n"
#define CC_STR_PLAN     "/plan\n"
#define CC_STR_DIFF     "/diff\n"
#define CC_STR_RESUME   "/resume\n"

// Esc/arrow/Ctrl sequences (not letters -> no OS-layout compensation needed).
#define CC_REWIND_SEQ   SS_TAP(X_ESC) SS_TAP(X_ESC)      // Rewind: double-Esc menu
#define CC_YESALL_SEQ   SS_TAP(X_DOWN) SS_TAP(X_ENT)     // Yes-all: 2nd option ⚠️placeholder

enum custom_keycodes {
    CC_YESALL = SAFE_RANGE,
    CC_REWIND,
    CC_CONT,
    CC_COMPACT,
    CC_CLEAR,
    CC_REVIEW,
    CC_MODEL,
    CC_COST,
    CC_PLAN,
    CC_VERBOSE,
    CC_DIFF,
    CC_RESUME,
    CC_AGENT_PREV,   // ◄Agent  (raw_hid_send fleet event -> daemon)
    CC_AGENT_NEXT,   // Agent►   (raw_hid_send fleet event -> daemon)
    CC_NEW,          // New      (raw_hid_send fleet event -> daemon)
    CC_JUMP,         // Jump     (raw_hid_send fleet event -> daemon focuses waiting agent)
    CC_OSLAY,        // OS keyboard-layout toggle (_CTL)
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

/* _BASE -- agent loop. Left six columns live; right six dead (KC_NO). */
[_BASE] = LAYOUT_ortho_4x12(
    KC_ESC,        KC_ENT,        CC_YESALL, KC_ESC,    LSFT(KC_TAB), CC_REWIND, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    CC_CONT,       CC_COMPACT,    CC_CLEAR,  CC_REVIEW, CC_MODEL,     CC_COST,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    CC_AGENT_PREV, CC_AGENT_NEXT, CC_NEW,    CC_JUMP,   KC_PGUP,      KC_PGDN,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    CC_PLAN,       CC_VERBOSE,    CC_DIFF,   CC_RESUME, KC_NO,        MO(_CTL),  KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO
),

/* _CTL -- controls (hold Fn). QMK/system only; never memorised.
   Boot=DFU  Reboot=warm reset  UGtog/Color/Val± = RGBLIGHT  OSLay=OS-layout  Dbg=console */
[_CTL] = LAYOUT_ortho_4x12(
    QK_BOOT,  QK_RBT,  UG_TOGG, UG_HUEU, UG_VALD, UG_VALU,  KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    CC_OSLAY, DB_TOGG, KC_NO,   KC_NO,   KC_NO,   KC_NO,    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   _______,  KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO
)

};

// ── Underglow colour policy (like Tango, the colour follows the active layer) ──
// Base layer hue also encodes the OS-layout mode, so a glance tells you which mode
// you're in; _CTL shows its own hue while Fn is held. Brightness is preserved
// (rgblight_get_val), so UG_VALU/UG_VALD still work. Task 2's daemon overrides this
// via set_status_color(), driven through the status.c seam -- underglow_repaint()
// is the arbiter: fresh daemon status wins, _CTL always wins over status, and the
// Task-1 layer/OS-mode colour is the fallback when the daemon is absent/stale.
#define UG_HUE_QWERTY   170   // base, OS-QWERTY  (default, VM-safe) -- blue
#define UG_HUE_COLEMAK  213   // base, OS-Colemak (gaming)          -- magenta
#define UG_HUE_CTL        0   // control layer (hold Fn)            -- red

void underglow_repaint(void) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer == _CTL) { set_status_color(UG_HUE_CTL, 255, rgblight_get_val()); return; }
    if (status_is_active()) { status_render(); return; }
    set_status_color(os_layout_is_colemak() ? UG_HUE_COLEMAK : UG_HUE_QWERTY, 255, rgblight_get_val());
}

static void send_fleet(uint8_t key) {
    uint8_t report[32] = {0};  // 32-byte reports (wire protocol, Global Constraints);
                               // RAW_EPSIZE lives in tmk_core/protocol/usb_descriptor.h,
                               // which isn't on keymap.c's include path.
    report[0] = 0x10;  // CMD_FLEET
    report[1] = key;
    raw_hid_send(report, sizeof(report));
}

// ── QMK hooks -> modules ─────────────────────────────────────────────────────
void keyboard_post_init_user(void) {
    status_underglow_init();   // static-light mode
    os_layout_init();          // restore persisted OS-layout mode
    status_init();
    underglow_repaint();       // seed base colour
}

layer_state_t layer_state_set_user(layer_state_t state) {
    // repaint for the NEW layer; can't use underglow_repaint() directly because it
    // reads layer_state (old) -- recompute inline:
    uint8_t layer = get_highest_layer(state);
    if (layer == _CTL) { set_status_color(UG_HUE_CTL, 255, rgblight_get_val()); return state; }
    if (status_is_active()) { status_render(); return state; }
    set_status_color(os_layout_is_colemak() ? UG_HUE_COLEMAK : UG_HUE_QWERTY, 255, rgblight_get_val());
    return state;
}

void housekeeping_task_user(void) { status_tick(); }

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;  // custom keys all fire on press

    switch (keycode) {
        case CC_CONT:    cc_type(CC_STR_CONT);    return false;
        case CC_COMPACT: cc_type(CC_STR_COMPACT); return false;
        case CC_CLEAR:   cc_type(CC_STR_CLEAR);   return false;
        case CC_REVIEW:  cc_type(CC_STR_REVIEW);  return false;
        case CC_MODEL:   cc_type(CC_STR_MODEL);   return false;
        case CC_COST:    cc_type(CC_STR_COST);    return false;
        case CC_PLAN:    cc_type(CC_STR_PLAN);    return false;
        case CC_DIFF:    cc_type(CC_STR_DIFF);    return false;
        case CC_RESUME:  cc_type(CC_STR_RESUME);  return false;

        case CC_REWIND:  SEND_STRING(CC_REWIND_SEQ); return false;
        case CC_YESALL:  SEND_STRING(CC_YESALL_SEQ); return false;
        case CC_VERBOSE: cc_ctrl_letter('o');        return false;  // Ctrl-O transcript toggle

        // Fleet keys -- raw_hid_send events; the daemon drives tmux/Win32 actions.
        case CC_JUMP:       send_fleet(0); return false;  // Jump
        case CC_AGENT_PREV: send_fleet(1); return false;
        case CC_AGENT_NEXT: send_fleet(2); return false;
        case CC_NEW:        send_fleet(3); return false;

        case CC_OSLAY:      os_layout_toggle(); return false;
    }
    return true;
}
