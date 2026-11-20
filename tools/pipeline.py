#!/usr/bin/env python3
"""The data pipeline, in Python so that it runs the same on Windows, Linux and macOS (no shell, no WSL). It feeds the Android build
(`python build.py data <arcade root>` / `python build.py android`):

    python tools/pipeline.py gen <arcade root> [--phys]       tables, lists and (--phys) the recompiled physics, from the arcade's `main`
    python tools/pipeline.py convert <arcade root> [--no-media] [--only STEP] [--keep-going]
                                                              the arcade's art, sound, movies and menus -> data/
    python tools/pipeline.py steps                            list the conversion steps

<arcade root> is a folder with `main` and `data/`: a dump of your own copy of the game, never changed here. `gen` must run
before `convert`. Every conversion step has a time limit and a
log (build/logs/<step>.log); the first failed step stops the run and says which (--keep-going carries on and lists them all),
so a half-converted data set cannot pass for a finished one. Needs the Python packages of tools/requirements.txt, and for
`convert` an ffmpeg with the Bink decoder and libx264/libvorbis (found on PATH, or named by FFMPEG).
"""
import argparse
import os
import platform
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PY = sys.executable


def sh(*cmd, env=None, cwd=ROOT, stdout=None, check=True):
    try:
        return subprocess.run([str(c) for c in cmd], cwd=cwd, env=env, stdout=stdout, check=check)
    except FileNotFoundError as e:  # (Windows' bare "[WinError 2]" does not say what was not found)
        what = "the program " + str(cmd[0]) if not os.path.isfile(str(cmd[0])) and not shutil.which(str(cmd[0])) else "the folder " + str(cwd) if not os.path.isdir(str(cwd)) else str(e)
        sys.exit("could not start a program: %s was not found (%s)."
                 "\nIf it is ffmpeg, install it (Windows: winget install Gyan.FFmpeg) or set FFMPEG to its full path, then run setup again." % (what, e))


# ---------------------------------------------------------------------------------------------- the dump's view
def _has_files(path):
    for _, _, files in os.walk(path):
        if files:
            return True
    return False


def _link_dir(target, link):
    """a directory link: a symbolic link, or on Windows (where those need a privilege) a junction"""
    try:
        os.symlink(target, link, target_is_directory=True)
    except (OSError, NotImplementedError):
        if platform.system() != "Windows":
            raise
        subprocess.run(["cmd", "/c", "mklink", "/J", link, target], check=True, stdout=subprocess.DEVNULL)


def _link_file(target, link):
    try:
        os.symlink(target, link)
    except (OSError, NotImplementedError):
        shutil.copyfile(target, link)  # (`main` is one file of some 40 MB)


def arcade_view(arcade, view):
    """A read-only view of an arcade dump where the region's art and text sit in the folders the tools read: the Japan
    dump keeps them in `sprite`, `anime`, `movie` and `cutin/jp`, the export dump in `sprite-us`, `anime-us`, `movie-us`
    and `cutin/us` (the plain ones empty). Nothing is copied and the dump is not changed. Returns "japan" or "english"."""
    arcade = os.path.abspath(arcade)
    if os.path.lexists(view):
        try:
            _unlink(view)
        except OSError as e:
            sys.exit("could not clear %s (%s): close any window or program using that folder (the Explorer, a virus scanner), or delete the folder by hand, "
                     "and run the setup again" % (view, e))
    os.makedirs(os.path.join(view, "data"))
    _link_file(os.path.join(arcade, "main"), os.path.join(view, "main"))
    for e in os.listdir(os.path.join(arcade, "data")):
        _link_dir(os.path.join(arcade, "data", e), os.path.join(view, "data", e)) if os.path.isdir(os.path.join(arcade, "data", e)) \
            else _link_file(os.path.join(arcade, "data", e), os.path.join(view, "data", e))
    lang = "japan"
    for n in ("sprite", "anime", "movie"):
        plain, us = os.path.join(arcade, "data", n), os.path.join(arcade, "data", n + "-us")
        if not (os.path.isdir(plain) and _has_files(plain)) and os.path.isdir(us) and _has_files(us):
            _unlink(os.path.join(view, "data", n))
            _link_dir(us, os.path.join(view, "data", n))
            lang = "english"
            print("region art: %s -> %s-us" % (n, n))
    cut = os.path.join(arcade, "data", "cutin")
    if os.path.isdir(cut) and not _has_files(os.path.join(cut, "jp")) and _has_files(os.path.join(cut, "us")):
        _unlink(os.path.join(view, "data", "cutin"))
        os.makedirs(os.path.join(view, "data", "cutin"))
        for e in os.listdir(cut):
            if e != "jp":
                _link_dir(os.path.join(cut, e), os.path.join(view, "data", "cutin", e))
        _link_dir(os.path.join(cut, "us"), os.path.join(view, "data", "cutin", "jp"))
        lang = "english"
        print("region text: cutin/jp -> cutin/us")
    return lang


def _is_link(p):
    """a symbolic link or a Windows junction (Python's own islink does not see junctions before 3.12, and a reparse point is one either way)"""
    if os.path.islink(p):
        return True
    try:
        return bool(getattr(os.lstat(p), "st_file_attributes", 0) & 0x400)  # FILE_ATTRIBUTE_REPARSE_POINT
    except OSError:
        return False


def _retry(fn, p):
    """a file an antivirus scanner or the Explorer holds for a moment: try again a few times before giving up"""
    for i in range(6):
        try:
            return fn(p)
        except OSError:
            if i == 5:
                raise
            time.sleep(0.5)


def _unlink(p):
    """remove a link (never what it points at), a file, or a directory of those. Never goes into a link: a junction that would not come off
    must stop the run, not have the dump it points at emptied."""
    if _is_link(p):
        try:
            _retry(os.remove, p)
        except OSError:
            _retry(os.rmdir, p)  # (a link to a directory comes off with rmdir on Windows)
    elif os.path.isfile(p):
        _retry(os.remove, p)
    elif os.path.isdir(p):
        for name in os.listdir(p):
            _unlink(os.path.join(p, name))
        _retry(os.rmdir, p)


def tool_env(main):
    env = dict(os.environ)
    env["WMMT_MAIN"] = main
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"  # Windows Python otherwise reads and writes files in the regional code page (cp1252) and the Japanese text fails
    return env


# ---------------------------------------------------------------------------------------------------------- gen
def gen(arcade, phys):
    real = os.path.abspath(arcade)
    for need in ("main", "data"):
        if not os.path.exists(os.path.join(real, need)):
            sys.exit("%s has no %s: it must be a dump of the game with `main` and `data/`" % (real, need))
    view = os.path.join(ROOT, "build", "arcade-view-gen")
    arcade_view(real, view)
    main = os.path.join(view, "main")
    d = os.path.join(view, "data")
    env = tool_env(main)

    def run(*cmd):
        print("  " + " ".join(os.path.relpath(str(c), ROOT) if str(c).startswith(ROOT) else str(c) for c in cmd), flush=True)
        sh(PY, *cmd, env=env)

    print("sprite ids (this build's, for the ids written by hand: JID(n) in the C code)...")
    run("tools/sprids.py", main, "src/spriteids.h", "src")
    print("the car select, lamps and meters...")
    run("tools/cardb.py", main, os.path.join(d, "car"))
    run("tools/brakelampdb.py", main)
    run("tools/meterdb.py", main)
    run("tools/neondb.py", main)
    run("tools/stage/pack_cars.py", view, "--wheel-table")
    print("dress-up, names, sounds, sparks, titles...")
    for t in ("dressdb", "weartexts", "sounddb", "sparkdb", "titledb"):
        run("tools/%s.py" % t, main)
    run("tools/uvtab.py")
    print("story and 10-in-a-row...")
    run("tools/rivaldb.py", main, os.path.join(d, "characar"))
    run("tools/halodb.py", main, d)
    for t in ("partdb", "storysel", "storyinfo", "storymovie", "storypre", "storydemo"):
        run("tools/%s.py" % t, main)
    run("tools/storycity.py", main, os.path.join(d, "carconfig", "ytConfigStory.txt"))
    run("tools/sttune.py", main)
    run("tools/steff.py", main)
    run("tools/storycut.py", view)
    run("tools/storybgm.py", main)
    # (namedb.py and tendb.py import menupack.py, which reads the story headers above)
    run("tools/tendb.py", main, os.path.join(d, "carconfig", "ytConfigTenFight.txt"))
    run("tools/namedb.py", main)
    print("3D bits...")
    os.makedirs(os.path.join(ROOT, "data"), exist_ok=True)
    run("tools/stage/pack_battle.py", view, "src/battle.h")
    run("tools/stage/pack_tenline.py", d, "src/tenline.h")
    run("tools/stage/pack_backfire.py", view, "src/backfiredb.h")
    if os.path.isdir(os.path.join(d, "menu3d", "course")):
        run("tools/stage/pack_mini.py", view, "data/minicourse.wpk", "src/minicourse.h")
    if os.path.isdir(os.path.join(d, "effect")):
        run("tools/stage/pack_aura.py", view, "data/aura.wpk")
    print("  tools/stage/barricades.py main > tools/stage/barricades.txt")
    with open(os.path.join(ROOT, "tools", "stage", "barricades.txt"), "w", encoding="utf-8", newline="\n") as f:
        sh(PY, "tools/stage/barricades.py", main, env=env, stdout=f)
    if phys:
        print("the recompiled physics (a few minutes)...")
        rc = os.path.join(ROOT, "tools", "recomp")
        sh(PY, "recomp.py", "../../src/phys/phys_rc.c", *open(os.path.join(rc, "roots.txt")).read().split(), env=env, cwd=rc)
        sh(PY, "mkimage.py", "../../data/phys.img", env=env, cwd=rc)
        sh(PY, "mkrcaddr.py", "../../src", "../../src/phys/rc_addr.h", env=env, cwd=rc)
    print("done")


# ------------------------------------------------------------------------------------------------------- convert
def ffmpeg_exe():
    exe = os.environ.get("FFMPEG") or shutil.which("ffmpeg")
    if exe and not os.path.isfile(exe) and not shutil.which(exe):
        sys.exit("FFMPEG names %s, which does not exist. Unset it, or point it at ffmpeg.exe itself (not its folder)." % exe)
    if not exe:
        sys.exit("no ffmpeg: install one with the Bink decoder (a full build, e.g. gyan.dev's on Windows) and put it on PATH, or set FFMPEG")
    return exe


class Ctx:
    def __init__(self, arcade, view, real, lang, media):
        self.arcade, self.view, self.real, self.lang, self.media = arcade, view, real, lang, media
        self.d = os.path.join(view, "data")
        self.main = os.path.join(view, "main")
        self.env = tool_env(self.main)
        self.env.update(ARCADE=view, REAL_ARCADE=real, LANGTAG=lang, D=self.d)


def pack(ctx, out, *args):
    sh(PY, "tools/stage/pack.py", out, *args, env=ctx.env)


def pack_tokyo(ctx, tod):
    """the Tokyo map the way the arcade loads it, for its morning (other_stage/c1_daytime.lua + the makers' signboards of
    tokyo/daytime/daytime.lua) or its night (tokyo/night/night.lua): the ground, the short and long range buildings with the
    map's own textures, the OtherLight models, the guardrails across the closed junctions (barricades.txt), the junction arrow
    boards (arrows.wpk + arrows.names) and the sky box (sky.wpk)"""
    a = os.path.join(ctx.d)
    t = os.path.join(a, "tokyo", tod)
    makers = ("sk_makerkanban_C1_kandabashi sk_makerkanban_C1_shibapark sk_makerkanban_C1_shiodome sk_makerkanban_shinkanjo_ariake "
              "sk_makerkanban_shinkanjo_kiba sk_makerkanban_Wangan_13gouchi wg_makerkanban_Wangan_daikokuhuto wg_makerkanban_Wangan_kukochuo "
              "sk_makerkanban_Yokohane_shibaura yh_makerkanban_Yokohane_haneda yh_makerkanban_Yokohane_shioiri").split()
    other = "c1_otherlight sk_otherlight wg_otherlight yh_otherlight".split()
    if tod == "night":
        out, suffix = "data/tokyo_night.wpk", "_night"
        g = ("sk_area01_N sk_area02_N sk_area03_N sk_area04_N sk_area05_N sk_area06_N sk_area07_N sk_area08_N sk_area09_N sk_area10_N sk_area11_N "
             "sk_area12_N sk_area13_N sk_area15_N sk_area16_N sk_area17_N sk_area18_N wg_suimen_ni wg_area01_michi_ni wg_area02_michi_ni "
             "wg_area04_michi_ni wg_area06_michi_ni wg_area09_michi_ni wg_area11_michi_ni wg_area12_michi_ni yh_area01_michi_ni yh_area02_michi_ni "
             "yh_area05_michi_ni yh_area06_michi_ni yh_area07_michi_ni yh_area08_michi_ni").split() + makers + other
        s = ("sk_ginza_Ns sk_shibapark10_Ns sk_kasumigaseki_N sk_edagawa_N sk_odaiba_Ns sk_oi_Ns sk_rb_N wg_area01_building_ni wg_area02_building_ni "
             "wg_area04_building_ni wg_area09_building_ni wg_area11_building_ni wg_area12_building_ni yh_area01_building_ni yh_area02_building_ni "
             "yh_area05_building_ni yh_area06_building_ni yh_area07_building_ni yh_area08_building_ni").split()
        l = ("sk_area14_N sk_ginza_Nl sk_shibapark10_Nl sk_odaiba_N_beam sk_odaiba_Nl sk_oi_Nl sk_searchlight sk_ginza_glow sk_edagawa_glow "
             "sk_kasumigaseki_glow sk_shibapark10_glow sk_rb_glow sk_oi_glow sk_odaiba_glow sk_tower_N sk_ginza_sign sk_shibapark10_sign "
             "sk_kasumigaseki_sign sk_oi_sign sk_edagawa_sign sk_rb_sign wg_area01_kanban_ni wg_area02_kanban_ni wg_area04_kanban_ni "
             "wg_area06_kanban_ni wg_area09_kanban_ni wg_area11_kanban_ni yh_area01_kanban_ni yh_area02_kanban_ni yh_area05_kanban_ni "
             "yh_area06_kanban_ni yh_area07_kanban_ni yh_area08_kanban_ni").split()
        sky = "sk_sky1_N sk_sky2_N".split()
    else:
        out, suffix = "data/tokyo.wpk", ""
        g = ("sk_area01_m sk_area02_m sk_area03_m sk_area04_m sk_area05_m sk_area06_m sk_area07_m sk_area08_m sk_area09_m sk_area10_m sk_area11_m "
             "sk_area12_m sk_area13_m sk_area15_m sk_area16_m sk_area17_m sk_area18_m wg_suimen_m wg_area01_michi_m wg_area02_michi_m "
             "wg_area04_michi_m wg_area06_michi_m wg_area09_michi_m wg_area11_michi_m wg_area12_michi_m yh_area01_michi_m yh_area02_michi_m "
             "yh_area05_michi_m yh_area06_michi_m yh_area07_michi_m yh_area08_michi_m").split() + makers + other
        s = ("sk_ginza_ms sk_shibapark10_ms sk_kasumigaseki_m sk_edagawa_m sk_odaiba_ms sk_oi_ms sk_rb_m wg_area01_building_m wg_area02_building_m "
             "wg_area04_building_m wg_area09_building_m wg_area11_building_m wg_area12_building_m wg_area01_kanban wg_area01_kanban_m "
             "wg_area02_kanban_m wg_area04_kanban_m wg_area06_kanban_m yh_area01_building_m yh_area02_building_m yh_area05_building_m "
             "yh_area06_building_m yh_area07_building_m yh_area08_building_m yh_area01_kanban_m yh_area02_kanban_m yh_area05_kanban_m "
             "yh_area06_kanban_m yh_area07_kanban_m yh_area08_kanban_m").split()
        l = ("sk_tower_m sk_area14_m sk_ginza_ml sk_ginza_sign sk_shibapark10_ml sk_shibapark10_sign sk_kasumigaseki_sign sk_odaiba_ml "
             "sk_odaiba_m_beam sk_oi_ml sk_oi_sign sk_edagawa_sign sk_rb_sign").split()
        sky = "sk_sky1 sk_sky2".split()
    igb = lambda n: os.path.join(t, n + ".igb")
    # (the buildings' textures at the arcade's own size, up to 1024: 256 smeared the windows into stripes)
    args = ["--textures", os.path.join(t, "textures.igb"), "--max-tex-build", "1024", "--place",
            os.path.join(a, "stage_common", "gurdrail_%s.igb" % tod), "tools/stage/barricades.txt"]
    args += [igb(n) for n in g] + ["--short"] + [igb(n) for n in s] + ["--long"] + [igb(n) for n in l]
    pack(ctx, out, *args)
    pack(ctx, "data/arrows%s.wpk" % suffix, "--textures", os.path.join(t, "textures.igb"), "--objects",
         os.path.join(a, "coursearrow", "tk_coursearrow_distribution.igb"))
    pack(ctx, "data/sky%s.wpk" % suffix, "--textures", os.path.join(t, "textures.igb"), "--chunk", "1000000", "--max-tex", "1024", "--dither",
         *[igb(n) for n in sky])


def pack_traffic(ctx):
    """the traffic's four models (data/othercar) into data/traffic/<name>.wpk: one chunk each, textures red/blue swapped, lit, each with
    its shadow volume and its lamps' glow"""
    d = os.path.join(ctx.d, "othercar")
    os.makedirs(os.path.join(ROOT, "data", "traffic"), exist_ok=True)
    for model, lamp in (("NZE121", "toy_NZE121_Other"), ("KZH100G", "toy_KZH100G_Other"), ("Truck", "truck_other"), ("RC2", "sub_RC2_Other")):
        pack(ctx, "data/traffic/%s.wpk" % model.lower(), "--chunk", "0", "--lit", "--solid-opaque", os.path.join(d, "LOD_" + model, "body_low.igb"),
             os.path.join(d, "brakelamp", "brakelamp_%s.igb" % lamp), "--shadow", os.path.join(d, "LOD_" + model, "shadow.igb"))


def pack_starter(ctx, paint=1):
    """the player's FD3S as car.lua dresses it: the white body with the chosen paint swapped in, sky light baked from the normals"""
    c = os.path.join(ctx.d, "car", "maz_FD3S")
    pack(ctx, "data/fd3s.wpk", "--chunk", "100000", os.path.join(c, "normal", "body.igb"), os.path.join(c, "tire.igb"), "--shade", "--swap",
         "fd3s_body01_white=" + os.path.join(c, "normal", "color", "fd3s_body01_colA0%d.tga" % paint))


def copy_runtime(ctx):
    """plain copies the game reads at run time: the story's in-race lines (every language the dump has), the car configs, the realtime
    movies' tables"""
    data = os.path.join(ROOT, "data")
    for sub in ("cutin", "carconfig", "rtmovie"):
        os.makedirs(os.path.join(data, sub), exist_ok=True)
    src = os.path.join(ctx.real, "data")
    if os.path.normcase(os.path.realpath(src)) == os.path.normcase(os.path.realpath(data)):
        return  # the dump already sits in the game folder (the files are where they need to be); copying onto itself fails on Windows

    def same(a, b):
        return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))

    shutil.copytree(os.path.join(src, "cutin"), os.path.join(data, "cutin"), dirs_exist_ok=True)
    for f in os.listdir(os.path.join(src, "carconfig")):
        if f.startswith("ytConfig") and f.endswith(".txt"):
            a, b = os.path.join(src, "carconfig", f), os.path.join(data, "carconfig", f)
            if not same(a, b):
                shutil.copyfile(a, b)
    for sub in ("table", "trail"):
        shutil.copytree(os.path.join(src, "rtmovie", sub), os.path.join(data, "rtmovie", sub), dirs_exist_ok=True)


def prepare_sound(ctx):
    src = os.path.join(ctx.d, "sound")
    out = os.path.join(ROOT, "data", "sound")
    os.makedirs(os.path.join(out, "engine"), exist_ok=True)
    for f in os.listdir(os.path.join(src, "engine")):
        if f.endswith(".wav"):
            shutil.copyfile(os.path.join(src, "engine", f), os.path.join(out, "engine", f))
    race = ("slip wall crash1 crash4 valb1 valb2 valb3 gyap phone2 turbin1 turbin2 turbin3 air1 backfire1 backfire2").split()
    system = ("cursor select cancel gamestart ti_count ti_count2 turning6 finish1 turning turning2 turning3 start321 start_go finish2 cutin2 "
              "tuning_bar result2 result_pass approach2 approach3 turning3_cutin door_open door_close engine_start rybal_intro parts_equip tuning "
              "levelup jack_up jack_down tuning1 tuning2 parts_comp rank win win_bigstar approach5 approach4 approach6 turning5 odo_count").split()
    for f in race:
        shutil.copyfile(os.path.join(src, "race_se", f + ".wav"), os.path.join(out, f + ".wav"))
    for f in system:
        shutil.copyfile(os.path.join(src, "system_se", f + ".wav"), os.path.join(out, f + ".wav"))


BGM = ("maxi3plus/wm3DX_P_sys_01_ent maxi3plus/wm3DX_P_sys_02_sel maxi3/sys_04 maxi3/sys_05 maxi3plus/wm3DX_P_race_01 maxi3plus/wm3DX_P_race_02 "
       "maxi3plus/wm3DX_P_race_03 maxi3plus/wm3DX_P_race_04 maxi3dx/wm3DX_race_01_mainD "
       + " ".join("maxi3/wm3_%02d" % n for n in range(1, 16)) + " maxi1/wm2_03 maxi1/wm2_05 maxi1/wm2_08 maxi1/wm2_18 "
       + " ".join("maxi2/wm2_%02d" % n for n in (1, 2, 4, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23)) + " "
       + " ".join("maxi3/ten_%s%d" % (c, n) for c, last in (("a", 9), ("b", 8), ("c", 8), ("d", 6), ("e", 7)) for n in range(1, last + 1)) + " "
       + " ".join("r/wmr_%02d" % n for n in range(1, 13))).split()


def prepare_bgm(ctx, formats=("m4a", "ogg")):
    """the menu and race BGM to AAC (m4a: Android's) and Ogg Vorbis (the SDL2 player's: Windows, Linux, the Raspberry Pi), the tracks
    already encoded kept (a stopped run leaves no half-written track behind)"""
    ff = ffmpeg_exe()
    src = os.path.join(ctx.d, "sound", "bgm")
    out = os.path.join(ROOT, "data", "sound", "bgm")
    os.makedirs(out, exist_ok=True)
    for f in BGM:
        for fmt in formats:
            dst = os.path.join(out, "%s.%s" % (os.path.basename(f), fmt))
            if os.path.isfile(dst):
                continue
            wav = os.path.join(src, f + ".wav")
            codec = ["-c:a", "aac", "-b:a", "128k", "-f", "ipod"] if fmt == "m4a" else ["-c:a", "libvorbis", "-q:a", "4", "-ar", "44100", "-ac", "2", "-f", "ogg"]
            sh(ff, "-y", "-v", "error", "-i", wav, *codec, dst + ".part")
            os.replace(dst + ".part", dst)


def _movie_label_filter(mp4):
    f = ("[{i}:v]format=rgba,geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='alpha(X,Y)*abs(1-2*mod(T,1))'[l];[0:v][l]overlay=29:18:shortest=1{fps}[v]")
    return f.format(i=2 if mp4 else 1, fps="" if mp4 else ",fps=30")


def prepare_movies(ctx, mp4=True):
    """the attract movies: .mp4 (H.264 + AAC) for Android. The Bink videos are silent 640x480; the arcade plays sound/bgm/attract/*_<language>.wav over them and blinks
    a label over playdemo and tutorial (opacity |1 - 2 frac(t)| over 60 frames), burned in here."""
    ff = ffmpeg_exe()
    d = ctx.d
    lang = ctx.lang
    movie = [("atruct.bik", "opening_%s.wav" % lang, "intro", None),
             ("playdemo.bik", "demo_%s.wav" % lang, "playdemo", "atract_p_movie_playdemo.png"),
             ("tutorial.bik", "tutorial_%s.wav" % lang, "tutorial", "atract_p_movie_tutorial.png")]
    enc = ["-c:v", "libx264", "-profile:v", "main", "-level", "3.1", "-pix_fmt", "yuv420p", "-preset", "slow", "-crf", "20", "-c:a", "aac", "-b:a", "128k",
           "-ar", "44100", "-movflags", "+faststart"]
    for bik, wav, name, label in movie:
        bik_p, wav_p = os.path.join(d, "movie", bik), os.path.join(d, "sound", "bgm", "attract", wav)
        if mp4 and not os.path.isfile(os.path.join(ROOT, "data", name + ".mp4")):
            out = os.path.join(ROOT, "data", name + ".mp4")
            if label:
                sh(ff, "-y", "-v", "error", "-i", bik_p, "-i", wav_p, "-loop", "1", "-framerate", "30", "-i", os.path.join(d, "sprite", label),
                   "-filter_complex", _movie_label_filter(True), "-map", "[v]", "-map", "1:a", "-shortest", *enc, out)
            else:
                sh(ff, "-y", "-v", "error", "-i", bik_p, "-i", wav_p, "-map", "0:v", "-map", "1:a", "-shortest", *enc, out)



def steps(ctx):
    """(name, time limit in seconds, function) for every conversion step, in order"""
    d, main, view = ctx.d, ctx.main, ctx.view
    py = lambda *a: (lambda: sh(PY, *a, env=ctx.env))
    st = [("tokyo-day", 1800, lambda: pack_tokyo(ctx, "daytime")), ("tokyo-night", 1800, lambda: pack_tokyo(ctx, "night"))]
    for city in ("oosaka", "hakone", "nagoya", "fukuoka"):
        for tod in ("daytime", "night"):
            st.append(("%s-%s" % (city, tod), 1800, py("tools/stage/pack_city.py", d, city, tod, "data")))
    st += [("lights", 1800, py("tools/stage/pack_lights.py", d, "data")),
           ("neon", 1800, py("tools/stage/pack_neon.py", d, "data/neon.wpk")),
           ("cars", 3600, py("tools/stage/pack_cars.py", view, "data/cars"))]
    for a in "abc":
        st.append(("cars-aero-" + a, 3600, py("tools/stage/pack_cars.py", view, "data/cars", "--aero", a)))
    st += [("wheels", 3600, py("tools/stage/pack_cars.py", view, "data/wheels", "--wheels")),
           ("characters", 3600, py("tools/stage/pack_cars.py", view, "data/chara", "--chara")),
           ("vinyl", 3600, py("tools/stage/pack_vinyl.py", view, "data/vinyl")),
           ("traffic", 1800, lambda: pack_traffic(ctx)),
           ("starter-car", 1800, lambda: pack_starter(ctx)),
           ("hud", 1800, py("tools/hudpack.py", os.path.join(d, "sprite"), main, "data")),
           ("minimap", 1800, py("tools/navipack.py", d, main, "data"))]
    for pre, city in (("Os", "oosaka"), ("Hk", "hakone"), ("Nk", "nagoya"), ("Fk", "fukuoka")):
        st.append(("minimap-" + city, 1800, py("tools/navipack.py", d, main, "data", pre + "Navi.igb", "navi_%s.bin" % city)))
    st += [("menu-discs", 1800, py("tools/vsdiscs.py", os.path.join(d, "sprite"))),
           ("menu", 3600, py("tools/menupack.py", view, "data/menu.bin")),
           ("extras", 600, py("tools/extrapack.py", view, "data/menu_extra.bin")),
           ("runtime-files", 300, lambda: copy_runtime(ctx))]
    if ctx.media:
        st += [("sound", 600, lambda: prepare_sound(ctx)), ("bgm", 3600, lambda: prepare_bgm(ctx)), ("movies", 3600, lambda: prepare_movies(ctx))]
    return st


def convert(arcade, media, only, keep_going):
    real = os.path.abspath(arcade)
    if not (os.path.isfile(os.path.join(real, "main")) and os.path.isdir(os.path.join(real, "data"))):
        sys.exit("%s has no main and data/" % real)
    if not os.path.isfile(os.path.join(ROOT, "src", "phys", "phys_rc.c")):
        sys.exit("run `python tools/pipeline.py gen %s --phys` first" % arcade)
    view = os.path.join(ROOT, "build", "arcade-view")
    lang = arcade_view(real, view)
    ctx = Ctx(real, view, real, lang, media)
    os.makedirs(os.path.join(ROOT, "data"), exist_ok=True)
    logs = os.path.join(ROOT, "build", "logs")
    os.makedirs(logs, exist_ok=True)
    done, failed = 0, []
    for name, limit, fn in steps(ctx):
        if only and only != name:
            continue
        t0 = time.time()
        print("%-28s " % name, end="", flush=True)
        log = os.path.join(logs, name + ".log")
        try:
            with open(log, "w", encoding="utf-8", errors="replace") as lf:
                old = sys.stdout, sys.stderr
                # (the tools' own output goes to the step's log: the child processes inherit these handles)
                sys.stdout.flush()
                saved = os.dup(1), os.dup(2)
                os.dup2(lf.fileno(), 1)
                os.dup2(lf.fileno(), 2)
                try:
                    fn()
                finally:
                    os.dup2(saved[0], 1)
                    os.dup2(saved[1], 2)
                    os.close(saved[0])
                    os.close(saved[1])
            print("ok (%d s)" % (time.time() - t0))
            done += 1
        except (subprocess.CalledProcessError, OSError, SystemExit) as e:
            print("FAILED (%s)" % e)
            print("--- last lines of %s:" % os.path.relpath(log, ROOT))
            print("".join(open(log, encoding="utf-8", errors="replace").readlines()[-15:]))
            if keep_going:
                failed.append(name)
                continue
            sys.exit("CONVERSION STOPPED at step '%s'" % name)
    print("converted: %d steps ok. Output in data/" % done)
    if failed:
        sys.exit("FAILED STEPS: " + " ".join(failed))
    print("CONVERSION DONE")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    g = sub.add_parser("gen")
    g.add_argument("arcade")
    g.add_argument("--phys", action="store_true", help="also the recompiled physics (src/phys/phys_rc.c, data/phys.img, src/phys/rc_addr.h)")
    c = sub.add_parser("convert")
    c.add_argument("arcade")
    c.add_argument("--no-media", action="store_true", help="skip the sounds, the music and the movies (they need ffmpeg)")
    c.add_argument("--only", help="one step only (see `steps`)")
    c.add_argument("--keep-going", action="store_true")
    sub.add_parser("steps")
    a = ap.parse_args()
    if a.cmd == "gen":
        try:
            gen(a.arcade, a.phys)
        except subprocess.CalledProcessError as e:
            tool = [os.path.relpath(str(c), ROOT) if str(c).startswith(ROOT) else str(c) for c in e.cmd[1:3]]
            sys.exit("GENERATION STOPPED: a generator step failed (exit code %s): %s\n  The tool's own error message is just above." % (e.returncode, " ".join(tool)))
    elif a.cmd == "convert":
        convert(a.arcade, not a.no_media, a.only, a.keep_going)
    else:
        for name, limit, _ in steps(Ctx("", "", "", "japan", True)):
            print(name)


if __name__ == "__main__":
    main()
