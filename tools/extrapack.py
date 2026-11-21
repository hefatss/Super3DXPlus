#!/usr/bin/env python3
"""menu_extra.bin: the port's own menu art, baked in seconds (menu.bin takes up to 45 minutes).

The player (src/anm.c extra_load) reads it after menu.bin: same sprite table ("WMN3", count, then id / w / h / zsize / pixels), no
animations. A sprite whose id menu.bin already has takes its place ("extras win"); the others are added. Without the file, or a
sprite, menu.c falls back to its drawn discs and text.

  EX_DISC_BASE + k   the selectors' discs (256 x 256, in the mode discs' style, tools/vsdiscs.py): the pictogram box and title
  EX_TITLE_BASE + k  the selectors' top bar titles (512 x 64, the arcade's title layout, tools/headline.py)
  OPTION_TEXT_BASE + 10   "Game options" (replaces the title menu.bin has until it is regenerated)

  NAME_GLYPH1_BASE + k  the card's name characters in the game's font 1 (white, 20 px cells: the race plates' name), from the arcade's own
                        sheets: the build's game folder is needed for these (its `main` and sprites)

    python3 tools/extrapack.py <the build's game folder> <out menu_extra.bin> [font.otf]
"""
import math
import os
import struct
import sys
import zlib

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import headline  # noqa: E402
import vsdiscs  # noqa: E402

EX_DISC_BASE, EX_TITLE_BASE, OPTION_TEXT_BASE, NAME_GLYPH1_BASE = 110000, 110100, 101900, 108100
# main menu, settings, tuning option (menu.c draw_selmenu TEXT[][])
DISCS = ["Play", "Settings", "Exit", "Game", "Controls", "Sound", "Stock", "Hell's\nTuning",
         # the mode select (8..10), the car's settings (11..14) and the delete question (15, 16)
         "Solo", "Versus\nbattle", "Car\nsettings", "Transmission", "Customize", "Meter\n& BGM", "Delete\ncar", "Keep", "Delete",
         # the customize select (17, 18)
         "Dress-up\nparts", "Paint\ncolor"]
# index = menu.c's sel_menu - 1 (main, settings, starting point, transmission, mode, car settings, delete), then the solo game select's
TITLES = ["Please select an option", "Settings", "Select your starting point", "Select transmission", "Please select an option", "Car settings",
          "Delete this car?", "Customize", "Select game mode"]
CLASS_BASE = 107200  # (menu.c CARD_CLASS; CARD_CLASSP is 100 on)
TEN_BGM_BASE = 110200  # (the 10 out run's soundtracks' names, menu.c EX_TENBGM)
OPTION_TITLE_ID = OPTION_TEXT_BASE + 10
OPTION_TITLE = "Game options"
SS = vsdiscs.SS


def car(d, w, h, tuned):
    """a car in side view, black: the body, the window cut out, wheels; tuned: a wing and speed lines"""
    s = min(w / 112.0, h / 48.0)
    ox, oy = (w - 112 * s) / 2, (h - 48 * s) / 2 + 2 * s

    def P(x, y):
        return (ox + x * s, oy + y * s)
    body = [(6, 36), (8, 27), (26, 23), (40, 10), (74, 10), (88, 23), (104, 26), (107, 36)]
    d.polygon([P(*p) for p in body], fill=0)
    d.polygon([P(*p) for p in ((43, 14), (72, 14), (82, 23), (36, 23))], fill=255)
    d.line([P(58, 14), P(58, 23)], fill=0, width=int(2 * s))
    for cx in (28, 86):
        r = 9 * s
        c = P(cx, 36)
        d.ellipse((c[0] - r - 2 * s, c[1] - r - 2 * s, c[0] + r + 2 * s, c[1] + r + 2 * s), fill=255)
        d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), fill=0)
        r2 = 3.4 * s
        d.ellipse((c[0] - r2, c[1] - r2, c[0] + r2, c[1] + r2), fill=255)
    if tuned:
        d.line([P(100, 25), P(102, 11)], fill=0, width=int(3 * s))
        d.rectangle((P(90, 7)[0], P(90, 7)[1], P(111, 11)[0], P(111, 11)[1]), fill=0)
        for y, x1 in ((15, 14), (23, 9)):  # (speed lines behind it)
            d.line([P(0, y), P(x1, y)], fill=0, width=int(2.2 * s))


def picto(kind, w, h):
    """the black pictogram on white (w x h, SS times over)"""
    im = Image.new("L", (w, h), 255)
    d = ImageDraw.Draw(im)
    cx, cy = w / 2, h / 2
    lw = int(5 * SS)
    r = h * 0.40
    if kind == 0:  # PLAY: a play triangle in a ring
        d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=0, width=lw)
        t = r * 0.5
        d.polygon([(cx - t * 0.7, cy - t * 1.05), (cx - t * 0.7, cy + t * 1.05), (cx + t * 1.2, cy)], fill=0)
    elif kind == 1:  # SETTINGS: a gear
        n = 8
        pts = []
        for i in range(n * 4):
            a = math.pi * 2 * i / (n * 4)
            rr = r if (i % 4) in (1, 2) else r * 0.78
            pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
        d.polygon(pts, fill=0)
        hr = r * 0.40
        d.ellipse((cx - hr, cy - hr, cx + hr, cy + hr), fill=255)
    elif kind == 2:  # EXIT: the power symbol
        rr = r * 0.88
        d.arc((cx - rr, cy - rr, cx + rr, cy + rr), -60, 240, fill=0, width=lw + 2 * SS)
        d.line([(cx, cy - rr * 1.12), (cx, cy)], fill=0, width=lw + 2 * SS)
    elif kind == 3:  # GAME: a steering wheel
        d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=0, width=lw + SS)
        hr = r * 0.30
        d.ellipse((cx - hr, cy - hr, cx + hr, cy + hr), fill=0)
        d.line([(cx - r, cy), (cx - hr, cy)], fill=0, width=lw)
        d.line([(cx + hr, cy), (cx + r, cy)], fill=0, width=lw)
        d.line([(cx, cy + hr), (cx, cy + r)], fill=0, width=lw)
    elif kind == 4:  # CONTROLS: a game pad
        bw, bh = w * 0.62, h * 0.60
        d.rounded_rectangle((cx - bw / 2, cy - bh / 2, cx + bw / 2, cy + bh / 2), radius=bh * 0.42, fill=0)
        pl = bh * 0.20
        px, py = cx - bw * 0.26, cy
        d.rectangle((px - pl, py - pl * 0.35, px + pl, py + pl * 0.35), fill=255)
        d.rectangle((px - pl * 0.35, py - pl, px + pl * 0.35, py + pl), fill=255)
        for dx, dy in ((0.24, -0.12), (0.34, 0.06)):
            br = bh * 0.085
            bx, by = cx + bw * dx, cy + bh * dy
            d.ellipse((bx - br, by - br, bx + br, by + br), fill=255)
    elif kind == 5:  # SOUND: a speaker and waves
        sx = cx - w * 0.14
        d.rectangle((sx - r * 0.55, cy - r * 0.32, sx - r * 0.1, cy + r * 0.32), fill=0)
        d.polygon([(sx - r * 0.12, cy - r * 0.32), (sx + r * 0.5, cy - r * 0.8), (sx + r * 0.5, cy + r * 0.8), (sx - r * 0.12, cy + r * 0.32)], fill=0)
        for i in range(3):
            rr = r * (0.45 + 0.3 * i)
            d.arc((sx + r * 0.5 - rr, cy - rr, sx + r * 0.5 + rr, cy + rr), -45, 45, fill=0, width=int(3.6 * SS))
    elif kind == 6:
        car(d, w, h, False)
    else:
        car(d, w, h, True)
    return im


def disc(tg, kind, title, font):
    """vsdiscs.icon with this module's pictograms and the title on the "mode" line's room too"""
    out = Image.new("RGBA", tg.size, (0, 0, 0, 0))
    if kind >= 6:  # (the starting point's discs: no picture, the words centred in the disc)
        lines = title.split("\n")
        big = Image.new("RGBA", (tg.size[0] * SS, tg.size[1] * SS), (0, 0, 0, 0))
        f = min((vsdiscs.fit_font(font, t, (52 if len(lines) == 1 else 44) * SS, 176 * SS) for t in lines), key=lambda ft: ft.size)
        step = (46 if len(lines) == 1 else 39) * SS
        for i, t in enumerate(lines):
            y = 128 * SS + (i - (len(lines) - 1) / 2) * step
            vsdiscs.text_edge(big, (128 * SS, y), t, f, width=2 * SS)
        # the words' ink centred in the disc whatever the font's metrics (the baseline offsets of one font sat them low)
        bb = big.getchannel("A").getbbox()
        if bb:
            from PIL import ImageChops
            big = ImageChops.offset(big, 0, int(round(128 * SS - (bb[1] + bb[3]) / 2)))
        out.alpha_composite(big.resize(tg.size, Image.LANCZOS))
        return out
    x0, y0, x1, y1 = vsdiscs.BOX
    big = Image.new("RGBA", (tg.size[0] * SS, tg.size[1] * SS), (0, 0, 0, 0))
    ImageDraw.Draw(big).rounded_rectangle((x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1), radius=6 * SS,
                                          fill=(255, 255, 255, 255), outline=(40, 40, 40, 255), width=2 * SS)
    out.alpha_composite(big.resize(tg.size, Image.LANCZOS))
    inner = (x0 + 4, y0 + 4, x1 - 4, y1 - 4)
    w, h = inner[2] - inner[0], inner[3] - inner[1]
    p = picto(kind, w * SS, h * SS).resize((w, h), Image.LANCZOS)
    out.paste(Image.new("RGBA", (w, h), (0, 0, 0, 255)), inner[:2], Image.eval(p, lambda v: 255 - v))
    big = Image.new("RGBA", (tg.size[0] * SS, tg.size[1] * SS), (0, 0, 0, 0))
    vsdiscs.text_edge(big, (128 * SS, 99 * SS), title, vsdiscs.fit_font(font, title, 40 * SS, 170 * SS), width=2 * SS)
    out.alpha_composite(big.resize(tg.size, Image.LANCZOS))
    return out


def registration_labels(elf, names, root):
    """The machine registration page's label column (menu_p_card_1_menu, and the R35's) without its last two rows, "Mileage password" and
    "Card life", and their underlines: the rows after the sixth underline made clear. Where the column is not as expected, left out."""
    import sprids
    out = []
    for jid in (0x846, 0x11ED):
        sid = sprids.J(jid)
        if sid is None or not names.get(sid):
            continue
        im = Image.open(f"{root}/{names[sid]}").convert("RGBA")
        a = im.getchannel("A")
        full = [sum(1 for x in range(im.width) if a.getpixel((x, y)) > 8) for y in range(im.height)]
        top = max(full)
        ends, run = [], False
        for y, n in enumerate(full):  # (the underlines: rows as wide as the sprite's widest)
            if n >= 0.9 * top and not run:
                run = True
            elif n < 0.9 * top and run:
                run = False
                ends.append(y)
        if len(ends) != 8:
            print("registration labels", sid, "not as expected, left as they are:", len(ends), "underlines")
            continue
        im.paste((255, 255, 255, 0), (0, ends[5], im.width, im.height))
        out.append((sid, im))
    return out


# arcade animations the port draws from this file (their records name sprites by id; those menu.bin lacks come with them): the bottom
# bar's variants with the animated key glyphs, the card check's pages
EXTRA_ANIMS = [
    "menu_layout_wait_1", "menu_layout_wait_2",  # (the "Please wait!" screens: waiting on the others, the black one)
    "menu_g_finish_vs",  # (the versus' place at the goal: 1st/4)
    # the race's route choice (clJunctionSelectConsole) and the last stretch's call (clFinishConsole)
    "cons_g_bunki_select", "cons_g_bunki_mes_select", "cons_g_bunki_mes_next", "cons_g_bunki_flash",
    "cons_g_last_000m", "cons_g_last_tonikaku", "cons_g_last_saigo", "cons_g_last_hasire", "cons_g_last_nokori",
    "menu_g_bottombar_bb_gd", "menu_g_bottombar_bb_hs_gd", "menu_g_bottombar_bp_gc", "menu_g_bottombar_bp_gc_vk_hs", "menu_g_bottombar_bp_gc_vs",
    "menu_g_bottombar_gc", "menu_g_bottombar_gd", "menu_g_bottombar_hc_gd", "menu_g_bottombar_hs_bb_gd", "menu_g_bottombar_hs_bb_gd_sc",
    "menu_g_bottombar_hs_bc_gd", "menu_g_bottombar_hs_bc_gd2", "menu_g_bottombar_hs_gd", "menu_g_bottombar_vp_gc_bk_hs",
    # the card check's pages 2 to 9 (src/menu.c, CARD PAGES): their frames, and the animated choices of the two settings pages
    "menu_g_cardcheck_2_waku", "menu_g_cardcheck_2_chara", "menu_g_cardcheck_2_line", "menu_layout_cardcheck_2",
    "menu_g_cardcheck_3_waku", "menu_layout_cardcheck_3_crown",
    "menu_layout_cardcheck_14_waku", "menu_layout_cardcheck_14_2_waku",
    "menu_g_cardcheck_4_waku", "menu_g_cardcheck_4_lv",
    "menu_g_cardcheck_5_waku", "menu_g_cardcheck_5_point",
    "menu_g_cardcheck_6_waku", "menu_g_cardcheck_6_time", "menu_g_cardcheck_6_mask", "menu_g_cardcheck_6_scrollstop",
    "menu_g_cardcheck_7_waku", "menu_g_cardcheck_7_title_transmission", "menu_g_cardcheck_7_sel_transmission", "menu_g_cardcheck_7_title_retire",
    "menu_g_cardcheck_7_sel_retire", "menu_g_cardcheck_7_title_meter", "menu_g_cardcheck_7_sel_meter", "menu_g_cardcheck_7_sel_meter1",
    "menu_g_cardcheck_7_sel_meter2",
    "menu_g_cardcheck_8_waku", "menu_g_cardcheck_8_title_volume", "menu_g_cardcheck_8_sel_volume", "menu_g_cardcheck_8_title_sound",
    "menu_g_cardcheck_8_sound_sel", "menu_g_cardcheck_8_sound_sel_2", "menu_g_cardcheck_8_sound_sel_no", "menu_g_cardcheck_8_title_password",
    "menu_g_cardcheck_8_sel_password",
]
# sprites to take whole from the arcade (over menu.bin's: the card check's title with its page number, which menu.bin has without)
EXTRA_WHOLE = [0x1DB2 + k for k in range(7)] + [1385, 2005, 2006, 3346, 3347, 3348, 3350]  # (+ the route console walls: divergence_frame, _arrow01, _arrow02; the ranking plates' class letters A, B, C, S)  # (the arcade's JP ids: sprids.J gives the build's)
# sprites the card pages draw themselves, by picture name (taken whole): the page 5 graph and other single pictures
EXTRA_NAMED = ["menu_p_card_8_sound_1_1", "menu_p_card_8_sound_2_1", "menu_p_card_8_sound_3_2",  # (the Meter & BGM's sound logos lit: the selected one)
               "menu_p_yn_waku_blue", "menu_p_yn_waku_red", "menu_p_yn_waku_backlight_blue", "menu_p_yn_waku_backlight_red", "menu_p_yn_waku_light_blue",
               "menu_p_yn_mission_at", "menu_p_yn_mission_mt", "menu_p_topbar_mes_carsel_transmission_no_card",  # (the new car's Select transmission)
               "menu_p_carsel_maker_waku", "menu_p_carsel_maker_mitsubishi_name",  # (the maker select's Special Vehicles button)
               "menu_p_card_5_graph","menu_p_card_5_menu", "menu_p_card_6_bar", "menu_p_card_6_password", "menu_p_card_3_menu",
               "menu_p_card_3_menu_1", "menu_p_card_3_menu_2", "menu_p_card_3_menu_3", "menu_p_card_3_menu_4", "menu_p_card_3_menu_5",
               "menu_p_card_4_lv_gold", "menu_p_card_4_lv_none", "menu_p_card_4_lv_bronze", "menu_p_card_4_lv_silver", "menu_p_card_2_chara_xxx_00",
               "menu_p_card_2_chara_waku", "menu_p_card_2_chara_hatena", "menu_p_card_2_clear_kai", "menu_p_card_2_clear", "menu_p_card_2_clear_shu",
               "menu_p_card_2_clear_wa", "menu_p_card_2_chara_ramp_12", "menu_p_card_2_chara_ramp_14"] + ["menu_p_milage_bg", "menu_p_milage_title_bar", "menu_p_milage_km", "menu_p_milage_comma"] + ["cons_p_last_remaining"] + ["menu_p_milage_%d" % k for k in range(10)] + ["menu_p_card_2_st%02d" % k for k in range(1, 100)] + ["menu_p_card_2_st%d" % k for k in range(100, 111)] + [
    "menu_p_card_2_chara_ramp_" + k for k in ("01A", "02A", "03A", "04A", "05A", "06A", "07A", "08A", "09A", "10A", "11A", "13A", "13B", "13c")] + ["menu_p_card_4_lv%02d" % k for k in range(1, 11)] + ["menu_p_card_4_stage%02d" % k for k in range(1, 16)]
# the arcade's own fonts as ASCII glyphs (src/menu.c EXF): id EXF_BASE + 100 * font + (char - 32)
EXF_BASE, EXF_FONTS = 120000, (1, 2, 5)


def menu_bin_ids(path):
    """the sprite ids menu.bin holds (its table walked by seeks)"""
    ids = set()
    if not os.path.isfile(path):
        return ids
    with open(path, "rb") as f:
        if f.read(4) != b"WMN3":
            return ids
        n, = struct.unpack("<I", f.read(4))
        for _ in range(n):
            sid, w, h, z = struct.unpack("<iHHI", f.read(12))
            ids.add(sid)
            f.seek(z if z else w * h * 4, 1)
    return ids


def strip_red_edge(im):
    """some of the English pictures (cons_p_last_remaining, cons_p_finish_hasire...) have a 1 px solid red frame round them, an edge marker the arcade's
    own drawing leaves out: without it the picture is not boxed in red"""
    w, h = im.size
    if w < 4 or h < 4:
        return im
    px = im.load()
    edge = [px[x, 0] for x in range(w)] + [px[x, h - 1] for x in range(w)] + [px[0, y] for y in range(h)] + [px[w - 1, y] for y in range(h)]
    if all(p == (255, 0, 0, 255) for p in edge):
        return im.crop((1, 1, w - 1, h - 1))
    return im


def arcade_sprite(mp, elf, names, root, sid):
    """a sprite as menupack.py packs it (its table size), or None"""
    name = names.get(sid, "")
    if not name:
        return None
    try:
        im = Image.open(f"{root}/{name}").convert("RGBA")
    except OSError:
        return None
    orig = im.size
    im = strip_red_edge(im)
    if sid in mp.ON_BLACK:
        im = mp.unblend_black(im)
    size = tuple(round(v) for v in struct.unpack_from("<2f", elf.d, elf.off(mp.TABLE_VA + 44 * sid + 16)))
    if size != im.size and min(size) > 0 and not (size == orig and im.size != orig):  # (the table counts the red frame in: the picture inside it is the picture)
        im = im.resize(size, Image.LANCZOS)
    return im


def pack_anims(mp, elf, names, root, have):
    """(sprites missing from menu.bin, the id table, the animations) for EXTRA_ANIMS"""
    idpos, idlist, anims, new = {}, [], [], {}
    for name in EXTRA_ANIMS:
        path = next((p for p in (f"{root}/data/anime/{name}.anm", f"{root}/data/anime-us/{name}.anm") if os.path.isfile(p)), "")
        if not path:
            print("no animation", name)
            continue
        frames, recs = mp.load_anm(path, 0)
        packed = b""
        for r in recs:
            sid = r[0]
            if sid >= 0 and sid not in idpos:
                idpos[sid] = len(idlist)
                idlist.append(sid)
                if sid not in have and sid not in new:
                    im = arcade_sprite(mp, elf, names, root, sid)
                    if im is not None:
                        new[sid] = im
            packed += struct.pack("<HH6f4h4h", idpos.get(sid, 0xFFFF), mp.BLEND.get(r[15], 0), *r[1:7],
                                  *(max(-32768, min(32767, v)) for v in r[7:15]))
        anims.append((name, frames, len(recs), packed))
    return sorted(new.items()), idlist, anims


UV_BASE = 102000  # (menupack.py: the UV table's cell n)
QMARK_ID = 123000  # (src/menu.c T_QMARK: the Special Vehicles button's logo)
BLACKREF_ID = 123001  # (src/menu.c T_BLACKREF: the display options' reference picture, tools/media/black_level_ref.jpg)


def qmark():
    """a question mark in the makers' grey, 128 x 128 (the Special Vehicles button's logo: the arcade has none)"""
    from PIL import ImageFont, ImageChops, ImageFilter
    s = 4
    from sysdeps import find_font
    font = ImageFont.truetype(find_font("DejaVuSans-Bold.ttf", "arialbd.ttf", "LiberationSans-Bold.ttf"), 100 * s)
    mask = Image.new("L", (128 * s, 128 * s), 0)
    d = ImageDraw.Draw(mask)
    bb = d.textbbox((0, 0), "?", font=font)
    d.text(((128 * s - (bb[2] - bb[0])) // 2 - bb[0], (128 * s - (bb[3] - bb[1])) // 2 - bb[1]), "?", font=font, fill=255)
    mask = mask.resize((128, 128), Image.LANCZOS)
    # grey, darker at the top (the logos' metal), a light edge under it
    out = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    grad = Image.new("RGBA", (128, 128))
    gp = grad.load()
    for y in range(128):
        v = int(70 + 60 * y / 127)
        for x in range(128):
            gp[x, y] = (v, v, v + 4, 255)
    hi = ImageChops.offset(mask, 0, 2).filter(ImageFilter.GaussianBlur(0.6))
    out.paste((255, 255, 255, 255), (0, 0), hi)
    out.paste(grad, (0, 0), mask)
    return out


def main():
    root, out = sys.argv[1], sys.argv[2]
    font = sys.argv[3] if len(sys.argv) > 3 else vsdiscs.FONT or vsdiscs.default_font()
    os.environ.setdefault("WMMT_MAIN", os.path.join(root, "main"))  # (the build's addresses, tools/wmmtaddr.py)
    src = next(d for d in (os.path.join(root, "data", "sprite-us"), os.path.join(root, "data", "sprite"))
               if os.path.isfile(os.path.join(d, "menu_p_mode_icon_tg.png")))
    tg = Image.open(os.path.join(src, "menu_p_mode_icon_tg.png")).convert("RGBA")
    sprites = [(EX_DISC_BASE + k, disc(tg, k, t, font)) for k, t in enumerate(DISCS)]
    sprites += [(EX_TITLE_BASE + k, headline.render_title_sprite(t)) for k, t in enumerate(TITLES)]
    sprites.append((OPTION_TITLE_ID, headline.render_title_sprite(OPTION_TITLE)))
    # the top bars of the port's own screens (menu.c draw_title_bar: 0 key bindings, 1 volume, 2 meter and BGM, 3 HUD options, 4 information, 5 debug options, 6 vibration, 7 display options, 8 network options, 9 graphics)
    sprites += [(EX_TITLE_BASE + 20 + k, headline.render_title_sprite(t)) for k, t in enumerate(("Key bindings", "Volume", "Meter & BGM", "HUD options", "Information", "Debug options", "Vibration", "Display options", "Network options", "Graphics"))]
    # the card check's pages ("Card data confirmed (n/5)": the arcade's nine, less the settings pages the discs replaced and the two ghost battle pages, hidden; the ids are the seven pages', the ghosts' unused)
    page_no = [1, 2, 0, 0, 3, 4, 5]
    sprites += [(EX_TITLE_BASE + 9 + k, headline.render_title_sprite("Card data confirmed (%d/5)" % page_no[k])) for k in range(7)]
    import menupack as mp
    elf = mp.Elf(os.path.join(root, "main"))
    names = mp.sprite_names(elf)
    sprites += registration_labels(elf, names, root)
    sprites.append((QMARK_ID, qmark()))
    sprites.append((BLACKREF_ID, Image.open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "media", "black_level_ref.jpg")).convert("RGBA")))
    # the 10 out run's five soundtracks' names for the BGM select (the table's titles of their first loops, in app.c's TEN_LOOP order d, e, a, b, c),
    # in font 0 as the select's other names
    # the card's title rows by class (menu.c CARD_CLASS, CARD_CLASSP): menu.bin's own sit at 105400 and 105450, which the Special Vehicles' colour names
    # (105200 + their text number, up to 105434) run over (a full tuned Corolla's Body color showed "B3 Class /Wangan Beginner"): moved clear of them
    for k, name in enumerate(mp.CLASS_NAMES):
        sprites.append((CLASS_BASE + k, mp.render_text(elf, names, root, 5, name + mp.LANG_TEXT["class"] + "/" + mp.LANG_TEXT["title1"])))
        sprites.append((CLASS_BASE + 100 + k, mp.render_text(elf, names, root, 5, name + mp.LANG_TEXT["class"] + "/")))
    for k, song in enumerate((80, 86, 55, 64, 72)):
        sprites.append((TEN_BGM_BASE + k, mp.render_text(elf, names, root, 0, elf.wstr(elf.u32(mp.BGM_MUSIC + 12 * song + 8)))))
    sprites += [(NAME_GLYPH1_BASE + k, mp.render_text(elf, names, root, 1, c)) for k, c in enumerate(mp.name_chars(elf))]
    have = menu_bin_ids(os.path.join(os.path.dirname(os.path.abspath(out)), "menu.bin"))
    import sprids
    for jid in EXTRA_WHOLE:  # (whole, over menu.bin's: the card check's titles with their page numbers)
        sid = sprids.J(jid)
        im = arcade_sprite(mp, elf, names, root, sid) if sid is not None else None
        if im is not None:
            sprites.append((sid, im))
    by_name = {n.split("/")[-1][:-4]: sid for sid, n in names.items()}
    for nm in EXTRA_NAMED + [n for n in by_name if n.startswith("menu_p_card_2_chara_ramp_")]:
        sid = by_name.get(nm)
        im = arcade_sprite(mp, elf, names, root, sid) if sid is not None else None
        if im is not None and sid not in have:
            sprites.append((sid, im))
    for font in EXF_FONTS:
        sprites += [(EXF_BASE + 100 * font + (ord(c) - 32), mp.render_text(elf, names, root, font, c)) for c in map(chr, range(33, 127))]
    # the UV table's cells the race consoles swap in (hud.c UV_BASE + n): the last stretch's big digits and lines (120..139: the km and Remaining of the distance callouts too), the route choice's arrows (218..231), boards and names (246..263), Wrong way (268)
    cells = mp.uv_cells()
    for n in [*range(120, 140), *range(218, 232), *range(246, 264), 268]:
        sid, (u0, v0, u1, v1) = cells[n]
        im = Image.open(f"{root}/{names[sid]}").convert("RGBA")
        w, h = im.size
        sprites.append((UV_BASE + n, im.crop((round(u0 * w), round(v0 * h), round(u1 * w), round(v1 * h)))))
    new, idlist, anims = pack_anims(mp, elf, names, root, have | {sid for sid, _ in sprites})
    sprites += new
    with open(out, "wb") as f:
        f.write(b"WMN3" + struct.pack("<I", len(sprites)))
        for sid, im in sprites:
            px = im.convert("RGBA").tobytes()
            z = zlib.compress(px, 9)
            f.write(struct.pack("<iHHI", sid, *im.size, len(z)) + z)
        f.write(struct.pack("<I", len(idlist)) + b"".join(struct.pack("<i", i) for i in idlist) + struct.pack("<I", len(anims)))
        for name, frames, nrec, packed in anims:
            f.write(name.encode().ljust(48, b"\0") + struct.pack("<II", len(frames), nrec))
            f.write(b"".join(struct.pack("<II", st, n) for st, n in frames))
            f.write(packed)
    print(f"{len(sprites)} sprites -> {out} ({os.path.getsize(out)} bytes)")
    if os.environ.get("EXTRA_PREVIEW"):
        d = os.environ["EXTRA_PREVIEW"]
        os.makedirs(d, exist_ok=True)
        for sid, im in sprites:
            im.save(os.path.join(d, f"{sid}.png"))


if __name__ == "__main__":
    main()
