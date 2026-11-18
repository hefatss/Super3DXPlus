"""Read every car's race meters out of `main` by running the arcade's own
clMeterConsole::drawMeter (A(0x85b6bdc)) under the unicorn x86 emulator, and write
src/meterdb.h (where each dial and needle goes, how far the needles turn) and
tools/meters.json (the sprite files, for hudpack.py).

usage: meterdb.py <main>

- GetCarData(model) is stCarDataDefine<car id>::data (pointer table A(0x9736980)): +0x20
  catalogue horsepower, +0x24 forward gears, +0x28 bit 0 the boost gauge, then the
  meter sprites and their centres for both views and both times of day.
- drawMeter submits each sprite to clSpriteManager::entry (A(0x851bbb0)) as a
  clSpriteRootEntry: +4 sprite id (clImageResourceManager table A(0x9551560)), +0x70 the
  blend (1 additive: the R35's night needles glow over its day ones), +0x7c/+0x80 the
  centre on the 640x480 screen, +0x98 the turn in radians (clockwise).
- Its inputs (setMeter, from yt::CSeq::Get2DInfoMeter): km/h, the rev rate
  (CEngine::GetRevRate, shaped 0.96 r + 0.04 above 0.1) and the boost ((p + 0.5) * 0.4).
  A needle's turn is linear in its input: probing each input at 0 and 1 gives it.
- The card's meter (clV386CardData::getMeter +0x26c, the card check's cardcheck_7 row:
  Normal, For Racing, SPECIAL): clStandardConsole's ctor sets +0x299 for 1 and +0x29b for 2,
  and drawMeter draws the racing_* or sp2_* dials then (their sprites parked at (0, 0) are
  not drawn). Each car gets all three, morning and night.
"""
from wmmtaddr import A  # the arcade addresses of the build being read
import json
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cardb import Main  # noqa: E402

from unicorn import UC_HOOK_CODE  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP  # noqa: E402

DRAW_METER = A(0x85B6BDC)
ENTRY = A(0x851BBB0)
MEMCPY, GUARD_ACQ, GUARD_REL = A(0x8422288), A(0x8420D78), A(0x8421C38)
CAR_DATA = A(0x9736980)
CAR_FOLDERS = A(0x9417720)
CAR_MODELS = A(0x9419EA0)
SPRITES = A(0x9551560)
OBJ, MGR = 0x1FE00000, 0x1FE01000
VIEW_BEHIND = 1
VIEW_INSIDE = 0  # (any other value: the first person view's own places for the dials; 0 is what the roof camera shows)
INPUTS = ["none", "speed", "rev", "boost", "lamp_on", "lamp_off"]  # (the last two: the rev lamp lit, and not)
STYLES = [0, 0x299, 0x29B]  # the card's meter 0, 1, 2: the flag drawMeter reads
# the rev lamp (the shift light) of the For Racing (+0x29a) and SPECIAL (+0x29c) meters: clStandardConsole::update (A(0x85bc848)) sets it from
# S2DInfoMeter +0x18, yt::CSeq::Get2DInfoMeter's yt::CMachine::GetRevLamp: CEngine::GetRevRate > 0.86 (A(0x943905c)). Lit, drawMeter
# shows racing_rev_on_* (0x4f1, 0x4f2) in place of racing_rev_off_* (0x63e, 0x63f), or sp2_rev_on_* (0x1cd, 0x1ce) where it showed none.
LAMP_FLAG = {0x299: 0x29A, 0x29B: 0x29C}


class Meter(Main):
    def __init__(self, path):
        super().__init__(path)
        self.mu.mem_map(OBJ, 0x100000)
        self.mu.hook_add(UC_HOOK_CODE, self.plt, begin=A(0x8420000), end=A(0x8423FFF))
        self.mu.hook_add(UC_HOOK_CODE, self.entry, begin=ENTRY, end=ENTRY)
        self.entries = []

    def arg(self, i):
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        return struct.unpack("<I", self.mu.mem_read(sp + 4 + 4 * i, 4))[0]

    def ret(self, val=0):
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EIP, struct.unpack("<I", self.mu.mem_read(sp, 4))[0])
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EAX, val)

    def plt(self, mu, addr, size, _):
        if addr == MEMCPY:
            d, s, n = self.arg(0), self.arg(1), self.arg(2)
            mu.mem_write(d, bytes(mu.mem_read(s, n)))
            self.ret(d)
        elif addr == GUARD_ACQ:  # function statics: initialise them the first time
            self.ret(1 if mu.mem_read(self.arg(0), 1)[0] == 0 else 0)
        elif addr == GUARD_REL:
            mu.mem_write(self.arg(0), b"\1")
            self.ret()
        else:
            raise RuntimeError("drawMeter called PLT %x" % addr)

    def entry(self, mu, addr, size, _):
        e = bytes(mu.mem_read(self.arg(1), 0x9C))
        sid, = struct.unpack_from("<I", e, 4)
        blend, = struct.unpack_from("<i", e, 0x70)
        x, y = struct.unpack_from("<2f", e, 0x7C)
        turn, = struct.unpack_from("<f", e, 0x98)
        self.entries.append((sid, x, y, math.degrees(turn), int(blend == 1)))
        self.ret()

    def draw(self, model, night, speed=0.0, rev=0.0, boost=0.0, style=0, lamp=0, view=VIEW_BEHIND):
        o = bytearray(0x300)
        struct.pack_into("<I", o, 4, MGR)
        struct.pack_into("<3i", o, 0x27C, model, night, view)
        struct.pack_into("<3f", o, 0x28C, speed, rev, boost)
        if style:
            o[style] = 1
        if lamp:
            o[lamp] = 1
        self.mu.mem_write(OBJ, bytes(o))
        self.entries = []
        self.call(DRAW_METER, OBJ)
        return [e for e in self.entries if e[1] or e[2]]

    def sprite_name(self, sid):
        return os.path.basename(self.cstr(self.r32(SPRITES + 44 * sid)))[:-4]


def main():
    m = Meter(sys.argv[1])
    folders = [m.cstr(m.r32(CAR_FOLDERS + 8 * k)) for k in range(56)]
    first_model = {}
    for i in range(0x79):
        car, lo = struct.unpack("<2i", m.mu.mem_read(CAR_MODELS + 20 * i, 8))
        first_model.setdefault(car, lo)
    here = os.path.dirname(os.path.abspath(__file__))
    packs = list(json.load(open(f"{here}/cars.json"))["packs"])  # CARDB_PACKS' order
    cars, sprites, sprites_in = [], {}, {}
    for folder in packs:
        car = [f.lower() for f in folders].index(folder.lower())  # toy_UCF10_taxi is toy_UCF10_TAXI
        data = m.r32(CAR_DATA + 4 * car)
        hp, gears, flags = m.r32(data + 0x20), m.r32(data + 0x24), m.r32(data + 0x28)
        views = []
        for view, store in ((VIEW_BEHIND, sprites), (VIEW_INSIDE, sprites_in)):
            parts = []
            for style, night in [(s, n) for s in STYLES for n in (0, 1)]:
                base = m.draw(first_model[car], night, style=style, view=view)
                probes = [m.draw(first_model[car], night, 320.0, 0, 0, style, view=view), m.draw(first_model[car], night, 0, 1.0, 0, style, view=view),
                          m.draw(first_model[car], night, 0, 0, 1.0, style, view=view)]
                # (the R35 SpecV lights rev lamps near the red zone: sprites of their own, left out)
                probes = [[e for e in p if e[0] in {b[0] for b in base}] for p in probes]
                assert all(len(p) == len(base) for p in probes), folder
                names = [m.sprite_name(e[0]) for e in base]
                these = []
                for i, (sid, x, y, a0, add) in enumerate(base):
                    moved = [k for k, p in enumerate(probes) if abs(p[i][3] - a0) > 1e-3]
                    assert len(moved) <= 1, (folder, names[i])
                    inp = moved[0] + 1 if moved else 0
                    these.append((x, y, a0, probes[moved[0]][i][3] - a0 if moved else 0.0, inp, add))
                if style in LAMP_FLAG:  # the rev lamp: what is drawn when it is lit
                    lit = m.draw(first_model[car], night, 0, 0, 0, style, LAMP_FLAG[style], view=view)
                    lit_ids, base_ids = [e[0] for e in lit], [e[0] for e in base]
                    for i, e in enumerate(base):
                        if e[0] not in lit_ids:  # (replaced when lit)
                            these[i] = these[i][:4] + (5, these[i][5])
                    for k, e in enumerate(lit):
                        if e[0] not in base_ids:
                            at = min(k, len(these))
                            these.insert(at, (e[1], e[2], 0.0, 0.0, 4, int(e[4])))
                            names.insert(at, m.sprite_name(e[0]))
                store.setdefault(folder, []).append(names)
                parts.append(these)
            views.append(parts)
        cars.append(dict(folder=folder, car=car, hp=hp, gears=gears, boost=flags & 1, parts=views[0], parts_in=views[1]))
    json.dump(dict(cars=sprites, cars_in=sprites_in), open(f"{here}/meters.json", "w"), indent=1)

    h = ["/* Generated by tools/meterdb.py from the arcade's clMeterConsole::drawMeter: do not edit. */",
         "#ifndef METERDB_H", "#define METERDB_H", "",
         "/* One sprite of a car's meters (meters.bin has the art), in the order the arcade",
         " * draws them: its centre on the arcade's 640x480 screen (the view behind the car),",
         " * its clockwise turn in degrees, a0 + a1 * the input, and whether it is added on. */",
         "enum { METER_FIXED, METER_SPEED, METER_REV, METER_BOOST, METER_LAMP_ON, METER_LAMP_OFF };",
         "/* (the last two: drawn while the rev lamp is lit, and while it is not) */",
         "/* the inputs: km/h / 320; the rev rate (meter_rev); the boost (meter_boost) */",
         "typedef struct {",
         "  float x, y, a0, a1;",
         "  int input, add;",
         "} meterdb_part;", "",
         "#define METERDB_MAX_PARTS 9",
         "/* the card's meter (0 Normal, 1 For Racing, 2 SPECIAL), morning and night: zone 2 * meter + night */",
         "#define METER_STYLES 3",
         "#define METER_ZONES (2 * METER_STYLES)",
         "typedef struct {",
         "  int car;          /* the arcade's car id (stCarDataDefine) */",
         "  int hp, gears;    /* catalogue horsepower, forward gears */",
         "  int n[METER_ZONES], n_in[METER_ZONES];",
         "  meterdb_part parts[METER_ZONES][METERDB_MAX_PARTS];",
         "  /* the first person view's own (the roof camera): the dials move to the bottom middle of the screen, and some cars show other art */",
         "  meterdb_part parts_in[METER_ZONES][METERDB_MAX_PARTS];",
         "} meterdb_car;", "",
         "/* by CARDB_PACKS index */",
         "static const meterdb_car METERDB[%d] = {" % len(cars)]
    def zones_of(parts):
        return ["{ %s }" % ", ".join("{ %g, %g, %g, %g, METER_%s, %d }" % (x, y, round(a0, 3), round(a1, 3),
                                                                          INPUTS[i].upper() if i else "FIXED", add)
                                      for x, y, a0, a1, i, add in z) for z in parts]
    for c in cars:
        h.append("  { %d, %d, %d, { %s }, { %s }, { %s }, { %s } }, /* %s */" % (
            c["car"], c["hp"], c["gears"], ", ".join(str(len(z)) for z in c["parts"]), ", ".join(str(len(z)) for z in c["parts_in"]),
            ", ".join(zones_of(c["parts"])), ", ".join(zones_of(c["parts_in"])), c["folder"]))
    h += ["};", "", "#endif", ""]
    open(f"{here}/../src/meterdb.h", "w").write("\n".join(h))
    print("%d cars, %d sprites" % (len(cars), sum(sum(len(z) for z in s) for s in sprites.values())))


if __name__ == "__main__":
    main()
