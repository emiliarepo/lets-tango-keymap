# Agent macropad: a single vitamins_included half run solo over USB.
#
# Unlike the Tango keymap this pad has no tap dance and does NOT use underglow.c
# (that module is a split-RPC, 5-layer animation -- wrong fit for a lone half).
# The underglow here is plain static RGBLIGHT, seeded in keymap.c.
TAP_DANCE_ENABLE = no

# Extra translation units beyond keymap.c (mirrors the Tango layout).
SRC += oskbd.c underglow.c

# Task 2 will add:
#   RAW_ENABLE = yes        # host daemon <-> pad status + fleet-key events over Raw HID
