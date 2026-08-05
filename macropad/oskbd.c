// OS keyboard-layout awareness. See oskbd.h for the two host contexts.
#include QMK_KEYBOARD_H
#include "oskbd.h"

// Persisted in the QMK user EEPROM block (survives a reflash).
typedef union {
    uint32_t raw;
    struct {
        bool os_colemak : 1;
    };
} user_config_t;
static user_config_t user_config;

// OS-Colemak compensation LUT (SPEC 1.4): under OS-Colemak, send colemak_lut[c-'a']
// to PRODUCE letter c. Only letters move; digits, '/', '-', space, enter etc. are
// unchanged, so non-letters pass straight through send_char().
static const uint16_t colemak_lut[26] = {
    KC_A,    KC_B, KC_C, KC_G, KC_K, KC_E, KC_T,   // a b c d e f g
    KC_H,    KC_L, KC_Y, KC_N, KC_U, KC_M, KC_J,   // h i j k l m n
    KC_SCLN, KC_R, KC_Q, KC_S, KC_D, KC_F, KC_I,   // o p q r s t u
    KC_V,    KC_W, KC_X, KC_O, KC_Z                // v w x y z
};

void os_layout_init(void) {
    user_config.raw = eeconfig_read_user();
}

bool os_layout_is_colemak(void) {
    return user_config.os_colemak;
}

void os_layout_toggle(void) {
    user_config.os_colemak = !user_config.os_colemak;
    eeconfig_update_user(user_config.raw);  // persist across reflash
}

void cc_type(const char *s) {
    if (!user_config.os_colemak) {
        send_string(s);  // OS-QWERTY: QMK's default sendstring LUT is correct
        return;
    }
    for (const char *p = s; *p; p++) {
        char ch = *p;
        if (ch >= 'a' && ch <= 'z') {
            tap_code(colemak_lut[ch - 'a']);
        } else if (ch >= 'A' && ch <= 'Z') {
            register_code(KC_LSFT);
            tap_code(colemak_lut[ch - 'A']);
            unregister_code(KC_LSFT);
        } else {
            send_char(ch);  // non-letters are unchanged under OS-Colemak
        }
    }
}

void cc_ctrl_letter(char letter) {
    if (letter < 'a' || letter > 'z') return;
    uint16_t kc = user_config.os_colemak ? colemak_lut[letter - 'a']
                                         : (uint16_t)(KC_A + (letter - 'a'));
    register_code(KC_LCTL);
    tap_code(kc);
    unregister_code(KC_LCTL);
}
