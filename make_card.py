import os, re, json
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.colors import HexColor, white

# ---- Board config (per-board JSON) + env ---------------------------------------
# BOARD_CONFIG : path to the board's card.json (geometry, title, layers, labels)
# KEYMAP_SRC   : the keymap.c to parse
# CARD_OUT     : output PDF
# Defaults target the Tango board from its new home so `python make_card.py` still
# renders the Tango card locally after the restructure.
BOARD_CONFIG = os.environ.get("BOARD_CONFIG", "tango/card.json")
KEYMAP_SRC = os.environ.get("KEYMAP_SRC", "tango/keymap.c")
OUT = os.environ.get("CARD_OUT", "lets_tango_layer_card.pdf")

with open(BOARD_CONFIG, encoding="utf-8") as f:
    CFG = json.load(f)

LAYOUT_MACRO = CFG["layout_macro"]
ROWS = CFG["rows"]
COLS = CFG["cols"]                       # columns in the LAYOUT macro (parse width)
COLS_SHOWN = CFG.get("cols_shown", COLS)  # columns actually drawn (macropad = left 6)
SPLIT_AFTER = CFG.get("split_after")      # insert a gap after this column, or null
CELL_W = CFG.get("cell_w", 63.5)
CELL_H = CFG.get("cell_h", 20)
GAP = CFG.get("gap", 12)

W, H = landscape(A4)  # 841.89 x 595.28
c = canvas.Canvas(OUT, pagesize=(W, H))

HAS_GAP = SPLIT_AFTER is not None and 0 < SPLIT_AFTER < COLS_SHOWN
GRID_W = COLS_SHOWN * CELL_W + (GAP if HAS_GAP else 0)
START_X = round((W - GRID_W) / 2)
INK = HexColor("#2C2C2A")
MUTE = HexColor("#73726C")
TRANS_FILL = HexColor("#F4F3EF")
TRANS_STROKE = HexColor("#E4E3DD")

def col_x(i):
    x = START_X + i * CELL_W
    if HAS_GAP and i >= SPLIT_AFTER:
        x += GAP
    return x

# ---- Parse keymap.c: pull the ROWS*COLS keycodes out of each LAYOUT block ----

def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    s = re.sub(r"//[^\n]*", "", s)
    return s

def extract_layer(src, sym):
    i = src.index("[" + sym + "]")
    i = src.index(LAYOUT_MACRO + "(", i) + len(LAYOUT_MACRO + "(")
    depth, buf = 1, []
    while depth > 0:
        ch = src[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                break
        buf.append(ch)
        i += 1
    inner = "".join(buf)
    tokens, cur, d = [], "", 0
    for ch in inner:
        if ch == "(":
            d += 1; cur += ch
        elif ch == ")":
            d -= 1; cur += ch
        elif ch == "," and d == 0:
            tokens.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        tokens.append(cur)
    tokens = [" ".join(t.split()) for t in tokens if t.strip()]
    assert len(tokens) == ROWS * COLS, "%s: expected %d keys, got %d" % (sym, ROWS * COLS, len(tokens))
    return [tokens[r*COLS:(r+1)*COLS] for r in range(ROWS)]

# ---- Keycode -> display label ----

PLAIN = {
    "_______": "", "KC_TRNS": "", "XXXXXXX": "", "KC_NO": "",
    "KC_TAB": "Tab", "KC_ESC": "Esc", "KC_BSPC": "Bksp", "KC_ENT": "Enter",
    "KC_SPC": "Space", "KC_DEL": "Del",
    "KC_LSFT": "Shift", "KC_RSFT": "Shift", "KC_LCTL": "Ctrl", "KC_LALT": "Alt", "KC_LGUI": "Gui",
    "KC_LEFT": "Left", "KC_RGHT": "Right", "KC_UP": "Up", "KC_DOWN": "Down",
    "KC_HOME": "Home", "KC_END": "End",
    "KC_MPLY": "Play", "KC_MPRV": "Prev", "KC_MNXT": "Next", "KC_VOLD": "Vol-", "KC_VOLU": "Vol+",
    "KC_SCLN": ";", "KC_QUOT": "'", "KC_COMM": ",", "KC_DOT": ".", "KC_SLSH": "/",
    "KC_GRV": "`", "KC_BSLS": "\\", "KC_MINS": "-", "KC_EQL": "=", "KC_LBRC": "[", "KC_RBRC": "]",
    "KC_TILD": "~", "KC_EXLM": "!", "KC_AT": "@", "KC_HASH": "#", "KC_DLR": "$", "KC_PERC": "%",
    "KC_CIRC": "^", "KC_AMPR": "&", "KC_ASTR": "*", "KC_LPRN": "(", "KC_RPRN": ")", "KC_PIPE": "|",
    "KC_UNDS": "_", "KC_PLUS": "+", "KC_LCBR": "{", "KC_RCBR": "}", "KC_LT": "<", "KC_GT": ">",
    "KC_PDOT": ".", "KC_PPLS": "+", "KC_PMNS": "-", "KC_PAST": "*", "KC_PSLS": "/",
    "KC_PENT": "Enter", "KC_NUM": "Num Lk",
    "QK_BOOT": "Boot", "QK_RBT": "Reboot", "DB_TOGG": "Dbg", "NK_TOGG": "NKRO",
    "AU_TOGG": "Audio", "MU_TOGG": "Music", "CK_TOGG": "Clicky",
    "UG_TOGG": "UGtog", "UG_NEXT": "UGmod", "UG_HUEU": "Hue+", "UG_HUED": "Hue-",
    "UG_SATU": "Sat+", "UG_SATD": "Sat-", "UG_VALU": "Val+", "UG_VALD": "Val-",
}
for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    PLAIN["KC_" + ch] = ch
for n in range(10):
    PLAIN["KC_" + str(n)] = str(n)   # KC_0..KC_9
    PLAIN["KC_P" + str(n)] = str(n)  # KC_P0..KC_P9 (keypad)
for n in range(1, 25):
    PLAIN["KC_F" + str(n)] = "F" + str(n)

# Per-board overrides win over the defaults above (friendly labels for the board's
# custom keycodes, e.g. CC_CONT -> "Cont"). Empty for Tango -> output unchanged.
PLAIN.update(CFG.get("label_overrides", {}))

# Layer keys (MO/PDF/...) render with the board's short layer labels.
LAYER_LABEL = dict(CFG.get("layer_labels", {}))
MOD_LABEL = {"LSFT": "Sft", "RSFT": "Sft", "LCTL": "Ctl", "RCTL": "Ctl",
             "LALT": "Alt", "RALT": "Alt", "LGUI": "Gui", "RGUI": "Gui"}
TAP_ABBR = {"Enter": "Ent"}
_unmapped = set()

def label_for(tok):
    tok = " ".join(tok.split())
    if tok in PLAIN:
        return PLAIN[tok]
    m = re.match(r"^(?:MO|TG|TT|TO|DF|PDF)\((_\w+)\)$", tok)      # layer keys
    if m:
        return LAYER_LABEL.get(m.group(1), m.group(1).lstrip("_"))
    m = re.match(r"^TD\(TD_(\w+)\)$", tok)                        # tap dance
    if m:
        return m.group(1)
    m = re.match(r"^([LR](?:SFT|CTL|ALT|GUI))_T\((KC_\w+)\)$", tok)  # mod-tap
    if m:
        tap = PLAIN.get(m.group(2), m.group(2).replace("KC_", ""))
        return MOD_LABEL[m.group(1)] + "/" + TAP_ABBR.get(tap, tap)
    m = re.match(r"^LT\(\s*\w+\s*,\s*(KC_\w+)\)$", tok)           # layer-tap
    if m:
        return PLAIN.get(m.group(1), m.group(1).replace("KC_", ""))
    _unmapped.add(tok)
    return tok.replace("KC_", "")

# ---- Presentation (names / colours / triggers come from the board config) ----

LAYER_META = CFG["layers"]

src = strip_comments(open(KEYMAP_SRC, encoding="utf-8").read())
layers = []
for meta in LAYER_META:
    rows = [[label_for(t) for t in row[:COLS_SHOWN]] for row in extract_layer(src, meta["sym"])]
    layers.append(dict(meta, rows=rows))
if _unmapped:
    print("WARNING: unmapped keycodes ->", ", ".join(sorted(_unmapped)))

# ---- Title ----
y = H - 34
c.setFillColor(INK)
c.setFont("Helvetica-Bold", 14)
c.drawString(START_X, y, CFG["title"])
c.setFont("Helvetica", 8.5)
c.setFillColor(MUTE)
c.drawRightString(START_X + GRID_W, y + 1, CFG.get("subtitle", ""))
y -= 9
c.setStrokeColor(HexColor("#B4B2A9")); c.setLineWidth(0.5)
c.line(START_X, y, START_X + GRID_W, y)
y -= 13

def draw_grid(y_top, L):
    header = HexColor(L["header"]); tint = HexColor(L["tint"]); txtc = HexColor(L["text"])
    bar_h = 15
    c.setFillColor(header)
    c.rect(START_X, y_top - bar_h, GRID_W, bar_h, stroke=0, fill=1)
    c.setFillColor(white)
    c.setFont("Helvetica-Bold", 9)
    c.drawString(START_X + 6, y_top - bar_h + 4.5, L["name"])
    c.setFont("Helvetica", 8)
    c.drawString(START_X + 58, y_top - bar_h + 4.5, L["trigger"])
    sw = 10
    sx = START_X + GRID_W - (3 * (sw + 2)) - 6
    c.setFont("Helvetica", 7)
    c.drawRightString(sx - 5, y_top - bar_h + 4.5, "underglow")
    for k, hexv in enumerate(L["shades"]):
        c.setFillColor(HexColor(hexv))
        c.rect(sx + k * (sw + 2), y_top - bar_h + 2.5, sw, sw, stroke=0, fill=1)
    rows_top = y_top - bar_h - 1
    for r, row in enumerate(L["rows"]):
        cy_top = rows_top - r * CELL_H
        for i, cell in enumerate(row):
            x = col_x(i); yb = cy_top - CELL_H
            if cell == "":
                c.setFillColor(TRANS_FILL); c.setStrokeColor(TRANS_STROKE)
            else:
                c.setFillColor(tint); c.setStrokeColor(HexColor("#D8D6CE"))
            c.setLineWidth(0.5)
            c.rect(x, yb, CELL_W, CELL_H, stroke=1, fill=1)
            if cell:
                fs = 9 if len(cell) <= 4 else (8 if len(cell) <= 6 else 7)
                c.setFont("Helvetica", fs)
                c.setFillColor(txtc)
                c.drawCentredString(x + CELL_W / 2, yb + CELL_H / 2 - fs / 2 + 1.4, cell)
    return rows_top - ROWS * CELL_H

for L in layers:
    bottom = draw_grid(y, L)
    y = bottom - 9

c.showPage()
c.save()
print("wrote", OUT, "from", KEYMAP_SRC)
