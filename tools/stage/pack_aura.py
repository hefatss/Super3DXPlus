"""The car's aura (clMenuCar::setAura, nsMenuCheck::checkAura), for data/aura.wpk.

The arcade shows an aura round the player's car on the mileage tally and the other card
ceremonies once the card's mileage is over 5000 km: the colour (green, purple, red or blue) comes
from two bytes of the card, the strength from the mileage (over 5000 km level 0, 10000 level 1,
30000 level 2, 100000 level 3, 500000 level 4). data/effect/car_aura_<colour>.igb holds five
layers (aura01 .. aura05, stacked rings of a car's length and some 3 m high, units of 0.1 m like the
cars'), level n shows the layers 01 .. 0(n+1); car_aura_glow_<colour>.igb a glow quad.
aura.cg turns the texture's red into a coverage (every sample that is not white counts) and draws it
in the colour with alpha (coverage * 0.25) added.

The pack: one object per colour and layer (object = colour * 5 + layer, colours in checkAura's order
green, purple, red, blue; object 20 + colour the glow quad). The texture is baked to the arcade's
look: the colour of the aura, its alpha the coverage of the veins, drawn added.

usage: pack_aura.py <arcade root (has main, data/)> out.wpk
"""
import os as _os, sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import os
import sys
import tempfile

import numpy as np
from PIL import Image, ImageFilter

import pack
import scene

COLORS = ["green", "purple", "red", "blue"]
TINT = {"green": (80, 255, 110), "purple": (210, 90, 255), "red": (255, 70, 90), "blue": (80, 150, 255)}


def bake(img, tint):
    """the aura texture as the shader sees it: where the picture is not white the coverage (blurred a little,
    as the arcade's eight scaled samples do), the aura's colour"""
    a = np.asarray(img.convert("RGBA")).astype(np.float32)
    cov = np.clip(a[..., 3] / 255.0 * 1.5, 0.0, 1.0)  # (the picture's alpha is the veins; its colour is the same where there are none)
    im = Image.fromarray((cov * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(1.0))
    cov = np.asarray(im).astype(np.float32) / 255.0
    out = np.zeros(a.shape[:2] + (4,), np.uint8)
    out[..., :3] = tint
    out[..., 3] = np.clip(cov * 0.5 * 255.0, 0, 255).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def main():
    root, out_wpk = sys.argv[1:3]
    eff = os.path.join(root, "data", "effect")
    tmp = tempfile.mkdtemp(prefix="aura")
    real_meshes = scene.meshes
    swaps = {}
    paths = []
    layer_of = {}

    def meshes(path):
        real, _, tag = path.partition("#")
        f, ms = real_meshes(real)
        out = []
        for m in ms:
            if tag.startswith("L"):
                if m["path"][-1] != "aura0%d" % (int(tag[1:]) + 1):
                    continue
            m = dict(m)
            if "color" in m["verts"]:
                v = dict(m["verts"])
                c = v["color"].copy()
                c[..., :3] = 255  # (the colour is the texture's; the vertices' alpha stays: it fades the rings out at their top, bottom and ends)
                v["color"] = c
                m["verts"] = v
            m["state"] = dict(m["state"], blend=True, blend_func=(4, 5))  # (alpha blended: added, it whites out a light backdrop)
            out.append(m)
        return f, out

    scene.meshes = meshes
    pack.OBJECTS = -1
    for c, name in enumerate(COLORS):
        base = os.path.join(eff, "car_aura_%s.igb" % name)
        f, ms = real_meshes(base)
        for m in ms:
            if m["image"] is None:
                continue
            tex = f.string(m["image"][22])
            key = pack.tex_key(tex)
            if key not in swaps:
                p = os.path.join(tmp, key + ".png")
                # (load_image_file flips top down again, so flip here to keep the picture's own way up)
                bake(pack.decode_image(f, m["image"]), TINT[name]).transpose(Image.FLIP_TOP_BOTTOM).save(p)
                swaps[key] = p
        for layer in range(5):
            paths.append((3 + c * 5 + layer, base + "#L%d" % layer))
    for c, name in enumerate(COLORS):
        paths.append((3 + 20 + c, os.path.join(eff, "car_aura_glow_%s.igb" % name)))
        f, ms = real_meshes(os.path.join(eff, "car_aura_glow_%s.igb" % name))
        for m in ms:
            if m["image"] is not None:
                key = pack.tex_key(f.string(m["image"][22]))
                p = os.path.join(tmp, key + ".png")
                im = pack.decode_image(f, m["image"]).convert("RGBA")
                a = np.asarray(im).astype(np.float32)
                # (a glow: its brightness is its coverage; added)
                lum = a[..., :3].max(axis=2)
                a[..., 3] = lum
                Image.fromarray(a.astype(np.uint8), "RGBA").transpose(Image.FLIP_TOP_BOTTOM).save(p)
                swaps[key] = p
    textures, order, ms, _ = pack.collect(paths, None, (256, 256, 256), swaps=swaps)
    pack.write(out_wpk, textures, order, pack.split_by_chunk(ms, 0), 0)
    print("aura:", len(paths), "objects,", len(textures), "textures ->", out_wpk)


if __name__ == "__main__":
    main()
