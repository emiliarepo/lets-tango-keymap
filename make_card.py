import os
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.colors import HexColor, white

W, H = landscape(A4)  # 841.89 x 595.28
OUT = os.environ.get("CARD_OUT", "colemak_layer_card.pdf")
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

layers = [
    {
        "name": "Colemak", "trigger": "base \u00b7 default",
        "header": "#6B2FB0", "tint": "#EFE9FA", "text": "#2E1560",
        "shades": ["#5A00FF", "#A200FF", "#CA00DC"],
        "rows": [
            ["Tab","Q","W","F","P","G","J","L","U","Y",";","Bksp"],
            ["Esc","A","R","S","T","D","H","N","E","I","O","'"],
            ["Shift","Z","X","C","V","B","K","M",",",".","/","Sft/Ent"],
            ["Play","Ctrl","Alt","Gui","LOWER","Space","Space","RAISE","Left","Down","Up","Right"],
        ],
    },
    {
        "name": "QWERTY", "trigger": "base \u00b7 toggle on Adjust",
        "header": "#2E9E3E", "tint": "#E7F5E7", "text": "#1C5C24",
        "shades": ["#4EFF00", "#00FF00", "#00FF4E"],
        "rows": [
            ["Tab","Q","W","E","R","T","Y","U","I","O","P","Bksp"],
            ["Esc","A","S","D","F","G","H","J","K","L",";","'"],
            ["Shift","Z","X","C","V","B","N","M",",",".","/","Sft/Ent"],
            ["Play","Ctrl","Alt","Gui","LOWER","Space","Space","RAISE","Left","Down","Up","Right"],
        ],
    },
    {
        "name": "Raise", "trigger": "hold RAISE",
        "header": "#0E8C99", "tint": "#E4F6F8", "text": "#0A4A53",
        "shades": ["#00FFD2", "#00F0FF", "#00A3E6"],
        "rows": [
            ["~","!","@","#","$","%","^","&","*","(",")","|"],
            ["`","1","2","3","4","5","6","7","8","9","0","\\"],
            [",","<",">","=","-","_","+","{","}","[","]","."],
            ["Esc","Ctrl","Alt","Gui","ADJUST","","","","Prev","End","Home","Next"],
        ],
    },
    {
        "name": "Lower", "trigger": "hold LOWER   \u00b7   symbols + numpad",
        "header": "#C67200", "tint": "#FBEFDC", "text": "#6E4200",
        "shades": ["#FF6C00", "#FFA800", "#E6CD00"],
        "rows": [
            ["~","!","@","#","$","%","7","8","9","Num Lk","/","-"],
            ["`","1","2","3","4","5","4","5","6","[","]","+"],
            ["Shift","<",">","=","-","_","1","2","3","0",".","Enter"],
            ["","","","","","","","ADJUST","Prev","Vol-","Vol+","Next"],
        ],
    },
    {
        "name": "Adjust", "trigger": "hold RAISE + LOWER",
        "header": "#BF2A20", "tint": "#FBE8E6", "text": "#6E140E",
        "shades": ["#FF0024", "#FF0000", "#E62B00"],
        "rows": [
            ["Boot","Reboot","Dbg","","","","","","UGtog","Val-","Val+","Del"],
            ["","Audio","Music","NKRO","","","","QWERTY","Colemk","","",""],
            ["F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"],
            ["","","","","","","","","","","",""],
        ],
    },
]

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
    # underglow swatches
    sw = 10
    sx = START_X + GRID_W - (3 * (sw + 2)) - 6
    c.setFont("Helvetica", 7)
    c.drawRightString(sx - 5, y_top - bar_h + 4.5, "underglow")
    for k, hexv in enumerate(L["shades"]):
        c.setFillColor(HexColor(hexv))
        c.rect(sx + k * (sw + 2), y_top - bar_h + 2.5, sw, sw, stroke=0, fill=1)
    # cells
    rows_top = y_top - bar_h - 1
    for r, row in enumerate(L["rows"]):
        cy_top = rows_top - r * CELL_H
        for i, cell in enumerate(row):
            x = col_x(i); yb = cy_top - CELL_H
            inherited = isinstance(cell, tuple)
            shown = cell[1] if inherited else cell
            if shown == "" or inherited:
                c.setFillColor(TRANS_FILL); c.setStrokeColor(TRANS_STROKE)
            else:
                c.setFillColor(tint); c.setStrokeColor(HexColor("#D8D6CE"))
            c.setLineWidth(0.5)
            c.rect(x, yb, CELL_W, CELL_H, stroke=1, fill=1)
            if shown:
                fs = 9 if len(shown) <= 4 else (8 if len(shown) <= 6 else 7)
                c.setFont("Helvetica", fs)
                c.setFillColor(MUTE if inherited else txtc)
                c.drawCentredString(x + CELL_W / 2, yb + CELL_H / 2 - fs / 2 + 1.4, shown)
    return rows_top - 4 * CELL_H

for L in layers:
    bottom = draw_grid(y, L)
    y = bottom - 9

c.showPage()
c.save()
print("wrote", OUT, "page:", round(W), "x", round(H))
