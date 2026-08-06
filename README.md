# Let's Tango — QMK keymaps

Personal QMK firmware for the **Let's Tango / Vitamins Included rev 2.1** — a split
4×12 ortholinear (integrated ATmega32U4, USB-C, 12× WS2812 underglow, speaker).

![build](https://github.com/emiliarepo/lets-tango-keymap/actions/workflows/build.yml/badge.svg)

Two boards live here, each with its own firmware and printable layer card:

| Dir         | Board                                   | What it is                                            |
| ----------- | --------------------------------------- | ----------------------------------------------------- |
| `tango/`    | `vitamins_included/rev2` (both halves)  | The full split Colemak keyboard.                      |
| `macropad/` | `vitamins_included/rev2` (left half solo)| A standalone **agent macropad** for driving Claude Code. |

---

## `tango/` — Colemak split keyboard

The everyday keyboard: Colemak base with a QWERTY alternate, symbol/number layers,
and a per-layer spinning underglow synced across both halves.

| #   | Layer      | What it is                                                       | Underglow |
| --- | ---------- | ---------------------------------------------------------------- | --------- |
| 0   | `_COLEMAK` | Base — Colemak                                                   | purple    |
| 1   | `_QWERTY`  | Base — QWERTY (toggle from Adjust)                               | green     |
| 2   | `_LOWER`   | Symbols on the left, NumLock keypad on the right                 | orange    |
| 3   | `_RAISE`   | Symbols / numbers                                                | cyan      |
| 4   | `_ADJUST`  | F1–F12, base toggles, underglow + audio/NKRO toggles, reset/boot | red       |

- **Base switch:** `PDF()` keys on Adjust persistently swap Colemak ↔ QWERTY.
- **Adjust** is reached by holding both thumb keys (Lower + Raise).
- **Per-layer spinning underglow** — each layer keeps its own hue family and
  rotates it around the 12-LED ring; the master computes it and syncs hue/phase to
  the slave over a split RPC so both halves stay in step.
- **Tap-dance layer lock** — double-tap Lower or Raise to lock that layer;
  double-tap again to unlock. Holding still works as a momentary layer.

The six firmware files (`keymap.c`, `layers.h`, `underglow.c/.h`, `config.h`,
`rules.mk`) are unchanged from before the restructure — only their path moved.

---

## `macropad/` — agent pad (standalone, zero host software)

A single `vitamins_included` **left half** run solo over USB (see the
`SPLIT_USB_DETECT` note in `macropad/config.h`) as a dedicated Claude Code control
pad. It's a `LAYOUT_ortho_4x12` where the **left six columns** carry the keys and
the right six are `KC_NO`, so it reuses the existing keyboard definition — no 4×6
matrix-pinout guesswork.

### Base layer — the agent loop (memorised)

```
 Stop     Approve  Yes-all  Reject   Mode     Rewind
 Cont     Compact  Clear    Review   Model    Cost
◄Agent    Agent►   New      Jump     Scrl↑    Scrl↓
 Plan     Verbose  Diff     Resume   ----     Fn
```

`Fn` (`MO(_CTL)`, hold) reaches the control layer. `Stop`=Esc, `Approve`=Enter,
`Mode`=Shift-Tab, `Rewind`=Esc Esc, `Scrl↑/↓`=PgUp/PgDn. `Cont` and the
slash-command keys (`Compact`/`Clear`/`Review`/`Model`/`Cost`/`Plan`/`Diff`/
`Resume`) type the command + Enter. `Verbose` is Ctrl-O (transcript toggle).

> **Fleet keys drive the companion daemon:** `◄Agent`/`Agent►`/`New`/`Jump` send Raw
> HID events to the host daemon (`daemon/`), which maps them to tmux + window focus
> — see `daemon/README.md`. `Yes-all` is still a best-effort placeholder (Claude Code
> has no dedicated "approve & don't ask again" key — this sends a best-effort *pick
> 2nd option*). All Claude-Code control strings are `#define`s at the top of
> `macropad/keymap.c`.

### Control layer `_CTL` — hold `Fn`

```
 Boot   Reboot  UGtog  Color  Val-   Val+
 OSLay  Dbg     ----   ----   ----   ----
 ----   ----    ----   ----   ----   ----
 ----   ----    ----   ----   ----   (Fn)
```

Bootloader (`QK_BOOT` → DFU to reflash), warm reset, RGB controls, debug toggle,
and the OS-layout toggle — kept off the base layer so they can't be hit by accident.

### OS keyboard-layout awareness (the core feature)

The pad emits raw HID keycodes; the OS turns them into characters using its *active*
layout. You always type Colemak but switch **where** Colemak lives per task, so
`OSLay` flips between two modes (persisted to EEPROM):

- **`os_qwerty`** (default, VM-safe): keyboard does Colemak, OS stays US-QWERTY —
  the pad's keycodes land as-is, macros sent unchanged.
- **`os_colemak`** (gaming): the OS applies a Colemak override, so it re-maps the
  pad's keycodes — the firmware pre-compensates every letter back to intent
  (`oskbd.c`'s LUT). Only letters and `;`/`'` move; digits, `/`, `-`, space, enter
  pass through.

The base-layer underglow hue encodes the mode (blue = OS-QWERTY, magenta =
OS-Colemak) so a glance tells you which mode you're in; the `_CTL` layer shows red
while `Fn` is held (like the Tango firmware colours its active layer).

### Firmware shape

Mirrors the Tango layout — `keymap.c` is glue (`keymaps[]` + hooks), with small
modules beside it:

- `oskbd.c/.h` — OS keyboard-layout awareness (LUT, EEPROM persistence, typing).
- `underglow.c/.h` — static RGBLIGHT. Not the Tango `underglow.c` (that's a
  split-RPC animation). Exposes `set_status_color()`, driven by the companion
  daemon over Raw HID; falls back to the static Task-1 colour when the daemon is
  absent or stale.
- `status.c/.h` — Raw HID status in, underglow out: receives the daemon's status
  byte and renders it (idle dim / running amber-breathe / waiting green / error
  red) through `set_status_color()`. Needs `RAW_ENABLE = yes` (set in
  `macropad/rules.mk`).

---

## Repo layout

```
├── .github/workflows/build.yml   # CI: firmware + card for BOTH boards, then release
├── tango/                        # full split keyboard (keymap.c, layers.h, underglow.c/.h, config.h, rules.mk, card.json)
├── macropad/                     # agent pad (keymap.c, layers.h, oskbd.c/.h, underglow.c/.h, status.c/.h, config.h, rules.mk, card.json)
├── daemon/                       # companion daemon (Python: reporter, listener, aggregator, hid_link, actions, main loop)
├── make_card.py                  # board-config-driven layer-card generator
├── README.md
└── .gitignore
```

## Build

Copy a board's files into a QMK checkout as that keymap, then compile:

```bash
# Tango
cp tango/*.c tango/*.h tango/rules.mk ~/qmk_firmware/keyboards/vitamins_included/keymaps/colemak/
qmk compile -kb vitamins_included/rev2 -km colemak

# Agent macropad
cp macropad/*.c macropad/*.h macropad/rules.mk ~/qmk_firmware/keyboards/vitamins_included/keymaps/macropad/
qmk compile -kb vitamins_included/rev2 -km macropad
```

Render a layer card locally (needs `reportlab`):

```bash
pip install reportlab
BOARD_CONFIG=tango/card.json    KEYMAP_SRC=tango/keymap.c    CARD_OUT=lets_tango_layer_card.pdf python make_card.py
BOARD_CONFIG=macropad/card.json KEYMAP_SRC=macropad/keymap.c CARD_OUT=macropad_card.pdf         python make_card.py
```

`make_card.py` reads all board-specific presentation (layout, columns shown, split
gap, title, layers, keycode-label overrides) from the per-board `card.json`.

## CI

`.github/workflows/build.yml` runs on every push / PR (and on demand). It matrixes
the firmware and card jobs over both boards and, on push to `main`, publishes a
`build-<run_number>` release with all four artifacts:

- `firmware-hex-tango` — `vitamins_included_rev2_colemak.hex`
- `firmware-hex-macropad` — `vitamins_included_rev2_macropad.hex`
- `layer-card-tango` — `lets_tango_layer_card.pdf`
- `layer-card-macropad` — `macropad_card.pdf`

## Flash

QMK Toolbox, bootloader **qmk-dfu** (enter it with the `Boot` key or the reset
pads). For **Tango**, flash both halves with the same `.hex`; USB goes to the left
(master) half, join with TRRS while unpowered. For the **macropad**, flash the
single half and run it solo over USB.

---

## `daemon/` — agent-aware companion

A host daemon makes the macropad *agent-aware*. It reads Claude Code hook events
and drives the pad's underglow over Raw HID (idle dim / running amber-breathe /
waiting green / error red), and maps the fleet keys to real actions: `Jump`
focuses the waiting session, `◄Agent`/`Agent►` cycle tmux windows, and `New` spawns
one. The pad falls back to its static Task-1 colour whenever the daemon is off.
See `daemon/README.md` for install and config.
