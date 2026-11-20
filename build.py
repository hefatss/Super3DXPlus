#!/usr/bin/env python3
"""Builds the game for Android (64-bit ARM, Android 9+). One script: Python 3.9+ on a PC, a JDK and the Android SDK/NDK.

    python build.py                              the guided setup: installs what it can, asks where your game is, converts the data, builds the APK, says where it is
    python build.py android                      the same guided Android setup
    python build.py gensrc <arcade root>         the generated sources only: the tables, the lists and the recompiled physics
    python build.py data <arcade root> [--no-media]
                                                 gen + convert: the generated sources and the game's data (data/)
    python build.py doctor                       what is installed, what is missing and how to get it

The data (gen + convert) is made by tools/pipeline.py; the APK by platform/android/build_apk.py. Both are also
checked against the game on a device (docs/ANDROID_ON_DEVICE.md).
"""
import argparse
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(ROOT, ".super3dx")  # downloaded toolchain pieces (git-ignored)
sys.path.insert(0, os.path.join(ROOT, "tools"))
from setupkit import ISSUES  # noqa: E402  (the guided setups' shared helpers)

REQ_MODULES = ["capstone", "unicorn", "PIL", "numpy"]


def say(*a):
    print(*a, flush=True)


def die(msg):
    sys.exit("build.py: " + msg)


def which(*names):
    for n in names:
        p = shutil.which(n)
        if p:
            return p
    return None


def run(cmd, **kw):
    try:
        r = subprocess.run(cmd, **kw)
    except OSError as e:
        die("could not start %s: %s" % (cmd[0], e))
    if r.returncode:
        die("a build command failed (exit code %d): %s ...\n  The tool's own message is just above this line." % (r.returncode, " ".join(str(c) for c in cmd[:4])))


def _has(mod):
    try:
        __import__(mod)
        return True
    except ImportError:
        return False


def pipeline(*args):
    run([sys.executable, os.path.join(ROOT, "tools", "pipeline.py")] + list(args), cwd=ROOT)


def ensure_modules():
    """the Python packages the converters use; installs them, in a private virtual environment if the system Python refuses"""
    missing = [m for m in REQ_MODULES if not _has(m)]
    if not missing:
        say("Python packages: all there.")
        return
    req = os.path.join(ROOT, "tools", "requirements.txt")
    say("Installing the Python packages the converters need (%s)..." % ", ".join(missing))
    if subprocess.run([sys.executable, "-m", "pip", "install", "-r", req]).returncode == 0:
        import importlib
        importlib.invalidate_caches()
        return
    if sys.prefix != getattr(sys, "base_prefix", sys.prefix):
        die("could not install the Python packages (pip failed, see above)")
    say("The system Python would not take them; making a private environment in .super3dx/venv ...")
    venv = os.path.join(TOOLS, "venv")
    if subprocess.run([sys.executable, "-m", "venv", venv]).returncode:
        die("could not make a virtual environment. On Debian/Ubuntu run: sudo apt install python3-venv python3-pip")
    py = os.path.join(venv, "Scripts" if os.name == "nt" else "bin", "python" + (".exe" if os.name == "nt" else ""))
    if subprocess.run([py, "-m", "pip", "install", "-r", req]).returncode:
        die("could not install the Python packages (pip failed, see above)")
    sys.exit(subprocess.run([py] + sys.argv).returncode)  # carry on inside it


# --------------------------------------------------------------------------------------------------------- doctor
def doctor():
    ok = True

    def line(name, found, hint):
        nonlocal ok
        say("  %-26s %s" % (name, found if found else "MISSING - " + hint))
        ok &= bool(found)

    say("python:", sys.version.split()[0], sys.executable)
    for m in REQ_MODULES:
        try:
            __import__(m)
            line("python module " + m, "ok", "")
        except ImportError:
            line("python module " + m, None, "pip install -r tools/requirements.txt")
    demangler = which("c++filt", "llvm-cxxfilt") or os.path.isfile(os.path.join(TOOLS, "demangle.exe" if os.name == "nt" else "demangle")) or which("g++", "clang++")
    line("C++ demangler", demangler, "apt install binutils (or a C++ compiler)")
    line("ffmpeg (for the sound/movies)", os.environ.get("FFMPEG") or which("ffmpeg"), "a full build with the Bink decoder: winget install Gyan.FFmpeg, apt install ffmpeg")
    line("JDK (javac, keytool)", os.environ.get("JAVA_HOME") or which("javac"), "set JAVA_HOME to a JDK 17+")
    line("Android SDK", os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT"), "python build.py android (it installs the SDK bits it needs)")
    say("Android needs the JDK, the SDK/NDK and ffmpeg. The rest is for the data.")


def _guided(jobs):
    """the Android setup (tools/setup_android.py): the data, the SDK/NDK and the APK"""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except Exception:
            pass
    try:
        ensure_modules()
        mod = __import__("setup_android")
        sys.exit(mod.run(os.path.abspath(__file__), jobs))
    except KeyboardInterrupt:
        sys.exit("\nStopped. Run it again whenever you like.")
    except SystemExit as e:
        if isinstance(e.code, str):
            say("\n" + "!" * 78 + "\n" + e.code.replace("build.py: ", "", 1) + "\n" + "!" * 78)
            sys.exit(1)
        raise
    except Exception:
        import traceback
        err = os.path.join(ROOT, "build", "logs", "setup-error.txt")
        os.makedirs(os.path.dirname(err), exist_ok=True)
        open(err, "w", encoding="utf-8").write(traceback.format_exc())
        say("\n" + "!" * 78 + "\nThe setup hit an unexpected problem (it is a bug in the setup, not your fault).\n"
            "Please open an issue at %s and attach this file:\n\n    %s\n" % (ISSUES, err) + "!" * 78)
        sys.exit(1)


def main():
    os.environ["PYTHONUTF8"] = "1"  # every Python tool this starts reads and writes UTF-8 (the Windows default, cp1252, cannot hold the Japanese text)
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", nargs="?", default="android", choices=["android", "gensrc", "data", "doctor"])
    ap.add_argument("arcade", nargs="?", help="the arcade dump: a folder with main and data/ (for gensrc and data)")
    ap.add_argument("-j", dest="jobs", type=int, default=os.cpu_count() or 2, help="processes at once")
    ap.add_argument("--no-media", action="store_true", help="data: skip the sounds, music and movies")
    a = ap.parse_args()
    if a.cmd == "gensrc":
        if not a.arcade:
            die("gensrc needs the arcade dump folder")
        pipeline("gen", a.arcade, "--phys")
        return
    if a.cmd == "android":
        _guided(a.jobs)
        return
    if a.cmd == "doctor":
        doctor()
        return
    if a.cmd == "data":
        if not a.arcade:
            die("data needs the arcade dump folder")
        pipeline("gen", a.arcade, "--phys")
        pipeline("convert", a.arcade, *(["--no-media"] if a.no_media else []))
        return


if __name__ == "__main__":
    main()
