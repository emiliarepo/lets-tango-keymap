import os, re
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.colors import HexColor, white

W, H = landscape(A4)  # 841.89 x 595.28
OUT = os.environ.get("CARD_OUT", "lets_tango_layer_card.pdf")
KEYMAP_SRC = os.environ.get("KEYMAP_SRC", "keymap.c")
c = canvas.Canvas(OUT, pagesize=(W, H))

CELL_W = 63.5
CELL_H = 20
GAP = 12                      # split gap after column 6
GRID_W = 12 * CELL_W + GAP    # 774
START_X = round((W - GRID_W) / 2)
INK = HexColor("#2C2C2A")
MUTE = HexColor("#73726C")
TRANS_FILL = HexColor("#F4F3EF")
TRANS_STROKE = HexColor("#E4E3DD")

def col_x(i):
    x = START_X + i * CELL_W
    if i >= 6:
        x += GAP
    return x

# ---- Parse keymap.c: pull the 48 keycodes out of each LAYOUT_ortho_4x12 block ----

def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    s = re.sub(r"//[^\n]*", "", s)
    return s

def extract_layer(src, sym):
    i = src.index("[" + sym + "]")
    i = src.index("LAYOUT_ortho_4x12(", i) + len("LAYOUT_ortho_4x12(")
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
    assert len(tokens) == 48, "%s: expected 48 keys, got %d" % (sym, len(tokens))
    return [tokens[r*12:(r+1)*12] for r in range(4)]

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

LAYER_LABEL = {"_COLEMAK": "Colemk", "_QWERTY": "QWERTY", "_LOWER": "LOWER", "_RAISE": "RAISE", "_ADJUST": "ADJUST"}
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

# ---- Presentation (names / colours / triggers stay here; keys come from keymap.c) ----

LAYER_META = [
    {"name": "Colemak", "sym": "_COLEMAK", "trigger": "base \u00b7 default",
     "header": "#6B2FB0", "tint": "#EFE9FA", "text": "#2E1560", "shades": ["#5A00FF", "#A200FF", "#CA00DC"]},
    {"name": "QWERTY", "sym": "_QWERTY", "trigger": "base \u00b7 toggle on Adjust",
     "header": "#2E9E3E", "tint": "#E7F5E7", "text": "#1C5C24", "shades": ["#4EFF00", "#00FF00", "#00FF4E"]},
    {"name": "Raise", "sym": "_RAISE", "trigger": "hold RAISE",
     "header": "#0E8C99", "tint": "#E4F6F8", "text": "#0A4A53", "shades": ["#00FFD2", "#00F0FF", "#00A3E6"]},
    {"name": "Lower", "sym": "_LOWER", "trigger": "hold LOWER   \u00b7   symbols + numpad",
     "header": "#C67200", "tint": "#FBEFDC", "text": "#6E4200", "shades": ["#FF6C00", "#FFA800", "#E6CD00"]},
    {"name": "Adjust", "sym": "_ADJUST", "trigger": "hold RAISE + LOWER",
     "header": "#BF2A20", "tint": "#FBE8E6", "text": "#6E140E", "shades": ["#FF0024", "#FF0000", "#E62B00"]},
]

src = strip_comments(open(KEYMAP_SRC).read())
layers = []
for meta in LAYER_META:
    rows = [[label_for(t) for t in row] for row in extract_layer(src, meta["sym"])]
    layers.append(dict(meta, rows=rows))
if _unmapped:
    print("WARNING: unmapped keycodes ->", ", ".join(sorted(_unmapped)))

# ---- Title ----
y = H - 34
c.setFillColor(INK)
c.setFont("Helvetica-Bold", 14)
c.drawString(START_X, y, "Let's Tango \u2013 Colemak layer map")
c.setFont("Helvetica", 8.5)
c.setFillColor(MUTE)
c.drawRightString(START_X + GRID_W, y + 1, "vitamins_included/rev2  \u00b7  split 6 | 6")
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
    return rows_top - 4 * CELL_H

for L in layers:
    bottom = draw_grid(y, L)
    y = bottom - 9

c.showPage()
c.save()
print("wrote", OUT, "from", KEYMAP_SRC)