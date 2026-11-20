#!/usr/bin/env python3
"""Sprite ids of the build being read.

A sprite id is a place in clImageResourceManager's table in `main` (44-byte entries, +0 the picture's path), and the table is
sorted by name: the Japan and export builds have almost the same pictures but nearly every id differs. The ids that the
arcade's own data and tables hold are the build's own and always right. The ones written by hand in this port (menu.c, hud.c,
tools/menupack.py...) were read off the Japan build; this module turns them into the build's ids through the picture's name:

  C:      JID(2239) -> the id this build gives the picture the Japan build has at 2239 (a constant: tables can hold it),
          PIC_<name> -> the id of the picture of that name
          both resolved by the src/spriteids.h this file writes:  sprids.py <main> <out header> [source file or folder...]
  Python: J(2239), J([..]) -> the same, for the converters' lists  (tools/jpids.json: Japan id -> picture name, for the
          ids the hand-written code uses; it is a dump of the Japan build's ids)

A picture the build does not have (the Japan build's Honda cars' colour swatches, in the export build) has no id: JID gives -1 and
J leaves it out, with a note.
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wmmtaddr import A, lang  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
TABLE_VA, TABLE_N = 0x9551560, 0x1F9B


class _Elf:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        phoff, = struct.unpack_from("<I", self.d, 28)
        phentsize, phnum = struct.unpack_from("<HH", self.d, 42)
        self.segs = []
        for i in range(phnum):
            t, off, va, _, fsz, _, _, _ = struct.unpack_from("<8I", self.d, phoff + i * phentsize)
            if t == 1:
                self.segs.append((va, off, fsz))

    def off(self, va):
        for v, o, n in self.segs:
            if v <= va < v + n:
                return o + va - v
        return None

    def u32(self, va):
        return struct.unpack_from("<I", self.d, self.off(va))[0]

    def cstr(self, va):
        o = self.off(va)
        return self.d[o:self.d.index(b"\0", o)].decode("latin1")


def stem(path):
    """data/sprite/menu_p_x.png, data/sprite-us/menu_p_x.tga -> menu_p_x"""
    return re.sub(r"\.(png|tga)$", "", path.rsplit("/", 1)[-1], flags=re.I)


_tables = {}


def table(main=None):
    """(id -> name, name -> id) of the build whose main is $WMMT_MAIN (or `main`)"""
    main = main or os.environ.get("WMMT_MAIN")
    if not main:
        raise SystemExit("sprids.py: set WMMT_MAIN to the arcade's main")
    if main not in _tables:
        elf = _Elf(main)
        names = {}
        i = 0
        while i < 20000:  # the table's length is the build's own: entries run while +0 points into the file
            o = elf.off(A(TABLE_VA) + 44 * i)
            p = struct.unpack_from("<I", elf.d, o)[0] if o is not None else 0
            if not p or elf.off(p) is None:
                break
            s = stem(elf.cstr(p))
            if s:
                names[i] = s
            i += 1
        by_name = {}
        for i, s in names.items():
            by_name.setdefault(s, i)  # (a name twice: the first)
        _tables[main] = (names, by_name)
    return _tables[main]


_jp = None
RECORD = None  # a set that J() fills with the Japan ids it is asked for


def _jpids():
    global _jp
    if _jp is None:
        path = os.path.join(HERE, "jpids.json")
        _jp = {int(k): v for k, v in json.load(open(path)).items()} if os.path.exists(path) else {}
    return _jp


_noted = set()


def jid(n, by_name=None):
    """The id this build gives the picture the Japan build has at id n, or None"""
    if RECORD is not None:
        RECORD.add(n)
    name = _jpids().get(n)
    if name is None:
        raise KeyError("sprite id %d is not in tools/jpids.json " % n)
    bn = by_name if by_name is not None else table()[1]
    i = bn.get(name)
    if i is None and (n, name) not in _noted:
        _noted.add((n, name))
        print("sprids: this build has no picture %s (the Japan build's %d)" % (name, n), file=sys.stderr)
    return i


def J(x):
    """An id, or an iterable of ids (a list, set, range) written for the Japan build, in this build's: what has no picture
    here is left out of a collection (an id alone gives None)."""
    if isinstance(x, int):
        return jid(x)
    out = [jid(i) for i in x]
    kept = [i for i in out if i is not None]
    return type(x)(kept) if isinstance(x, (list, set, tuple)) else kept


# ---- the C header ----

JID_RE = re.compile(r"\bJID\(\s*(0[xX][0-9a-fA-F]+|[0-9]+)\s*\)")
SPR_RE = re.compile(r"\bPIC_([A-Za-z0-9_]+)\b")  # (not SPR_: hud.c has its own)


def write_header(main, out, sources):
    names, by_name = table(main)
    jp = _jpids()
    files = []
    for s in sources:
        if os.path.isdir(s):
            files += [os.path.join(s, f) for f in sorted(os.listdir(s)) if f.endswith((".c", ".h")) and f != "spriteids.h"]
        else:
            files.append(s)
    jids, sprs = {}, set()
    for f in files:
        text = open(f, encoding="utf8", errors="replace").read()
        for m in JID_RE.finditer(text):
            jids.setdefault(m.group(1), int(m.group(1), 0))
        sprs |= set(SPR_RE.findall(text))
    lines = ["/* Generated by tools/sprids.py from the arcade's main: do not edit. The sprite ids of this build (see sprids.py). */",
             "#ifndef SPRITEIDS_H", "#define SPRITEIDS_H", "",
             "/* JID(n): the id this build gives the picture that the Japan build has at id n */",
             "#define JID(n) JID_##n", "",
             "/* the build's text language (tools/addrs/<name>.json): 1 for English, where a few Japanese layouts differ */",
             "#define WMMT_LANG_EN %d" % (lang() == "en"), ""]
    missing = []
    for tok, n in sorted(jids.items(), key=lambda kv: kv[1]):
        name = jp.get(n)
        if name is None:
            raise SystemExit("sprids.py: JID(%s) is used but tools/jpids.json has no picture for it" % tok)
        i = by_name.get(name)
        if i is None:
            missing.append((n, name))
        lines.append("#define JID_%s %s%s" % (tok, i if i is not None else "(-1)", "" if i is not None else "  /* %s: none in this build */" % name))
    lines += ["", "/* PIC_<name>: the id of the picture of that name */"]
    for s in sorted(sprs):
        i = by_name.get(s)
        if i is None:
            missing.append((None, s))
        lines.append("#define PIC_%s %s" % (s, i if i is not None else "(-1)"))
    lines += ["", "#endif", ""]
    open(out, "w", encoding="utf8").write("\n".join(lines))
    print("%s: %d JID, %d SPR names%s" % (out, len(jids), len(sprs),
                                          (", %d with no picture in this build: %s" % (len(missing), ", ".join(n for _, n in missing[:6]))) if missing else ""))


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    os.environ["WMMT_MAIN"] = sys.argv[1]
    write_header(sys.argv[1], sys.argv[2], sys.argv[3:] or [os.path.join(HERE, "..", "src")])
