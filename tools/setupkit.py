"""Helpers shared by the guided Android setup (build.py android, tools/setup_android.py): prompts, a logged child process, and plain-language
explanations of failures."""
import os
import re
import subprocess
import sys

ISSUES = "https://github.com/smolderyguest/Super3DXPlus/issues"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def say(*a):
    print(*a, flush=True)


def which(*names):
    import shutil
    for n in names:
        p = shutil.which(n)
        if p:
            return p
    return None


def ask(question, default=""):
    try:
        a = input(question).strip().strip('"').strip("'")
    except EOFError:
        return default
    return a or default


def yes(question, default=True):
    a = ask(question + (" [Y/n] " if default else " [y/N] "), "y" if default else "n").lower()
    return a.startswith("y")


def step(n, total, title):
    say("")
    say("=" * 78)
    say("Step %d of %d: %s" % (n, total, title))
    say("=" * 78)



def run_logged(cmd, logpath):
    """run a command, showing its output as it comes and keeping a copy in logpath; returns its exit code"""
    os.makedirs(os.path.dirname(logpath), exist_ok=True)
    env = dict(os.environ, PYTHONUNBUFFERED="1", PYTHONUTF8="1")
    with open(logpath, "w", encoding="utf-8", errors="replace") as lf:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, cwd=ROOT,
                             text=True, encoding="utf-8", errors="replace", bufsize=1)
        try:
            for line in p.stdout:
                say(line.rstrip("\r\n"))
                lf.write(line)
                lf.flush()
            return p.wait()
        except BaseException:
            p.terminate()
            raise


# what a failure's log says, and what to do about it (first match wins; the plain advice comes before the technical lines)
DIAGNOSES = [
    (r"no ffmpeg", "ffmpeg is not installed (it converts the game's sound and movies).\n"
                   "  Windows: open a terminal and run   winget install Gyan.FFmpeg   (or get the 'full' build from https://www.gyan.dev/ffmpeg/builds/)\n"
                   "  Linux: sudo apt install ffmpeg\n  Then run setup again."),
    (r"no compiler for Windows|no SDL2 development files|no C compiler", "A build tool is missing or was not downloaded properly.\n"
                   "  Run   python build.py doctor   to see what is missing, then run setup again."),
    (r"WinError 206|filename or extension is too long|File name too long|Filename too long|WinError 3\]|No such file or directory: '[A-Za-z]:\\\\.{200,}",
     "A file path got too long for Windows. Move the whole project folder somewhere short, such as C:\\Super3DXPlus, and run setup again."),
    (r"No space left|WinError 112|Errno 28|not enough space|disk is full",
     "The drive ran out of space. The build needs about 15 GB free. Free some space (or move the project folder to a bigger drive) and run setup again."),
    (r"MemoryError|Cannot allocate memory|out of memory",
     "The computer ran out of memory. Close other programs (browsers especially) and run setup again."),
    (r"PermissionError|Access is denied|WinError 5\]|WinError 32|being used by another process|Permission denied",
     "Windows (or an antivirus program) would not let the build write or delete a file.\n"
     "  Close anything that may be using this folder (File Explorer windows inside it, a running copy of the game), pause your antivirus for the\n"
     "  build or add this folder to its exclusions, make sure the folder is somewhere you can write to (not Program Files), and run setup again."),
    (r"URLError|getaddrinfo|Temporary failure in name resolution|timed out|could not download",
     "A download failed. Check your internet connection (a firewall, VPN or proxy can block it) and run setup again."),
    (r"UnicodeEncodeError|UnicodeDecodeError",
     "A text-encoding problem. This is a bug in the setup, not something you did wrong: please report it (details below)."),
    (r"has no main and data|has no main|has no data",
     "The game folder you chose is not what the setup expects: it must contain a file called `main` and a folder called `data`."),
    (r"phys_rc\.c",
     "The physics source was not generated. Run setup again from the start (it makes it in the first minutes)."),
    (r"GENERATION STOPPED",
     "The tools could not read your game's `main` file. The usual causes are a version of the game this project does not support\n"
     "  (docs/BUILDS.md lists the ones that work) or an incomplete or damaged copy of the game. Check that you copied all of it, and that `main` is the\n"
     "  game's real executable (tens of MB), not a shortcut or an empty file."),
    (r"CONVERSION STOPPED",
     "One of the data conversion steps failed. The line above this one says which; the lines before it show the converter's own message.\n"
     "  Run   python tools/pipeline.py convert <your game folder> --only <that step>   after fixing the cause to retry just that step."),
    (r"No module named",
     "A Python package is missing. Run   python -m pip install -r tools/requirements.txt   and then setup again."),
]


def explain_failure(log, rc):
    """print what went wrong in plain words, from the log of a failed build"""
    text = open(log, encoding="utf-8", errors="replace").read()
    lines = [l for l in text.splitlines() if l.strip()]
    hint = next((h for pat, h in DIAGNOSES if re.search(pat, text)), None)
    step = None
    for l in reversed(lines):
        m = re.match(r"^(\S+)\s+FAILED", l) or re.search(r"CONVERSION STOPPED at step '([^']+)'", l)
        if m:
            step = m.group(1)
            break
    say("\n" + "!" * 78)
    say("The build stopped before it finished.%s" % ((" It failed at: " + step) if step else ""))
    say("")
    if hint:
        say("What probably happened, and what to do:\n  " + hint)
    else:
        say("I do not recognise this error. The last lines of the log were:\n")
        for l in lines[-12:]:
            say("    " + l[:160])
    say("")
    say("Nothing was damaged. Once the cause is fixed, run setup again (it redoes the data step from the beginning).")
    say("The full log is in:  " + log)
    say("If you are still stuck, open an issue at %s and attach that log file (or paste its last 50 lines)." % ISSUES)
    say("!" * 78)


def find_game(path):
    """the folder holding `main` and `data/`: the path itself, or one level inside it"""
    path = os.path.abspath(os.path.expanduser(path))
    if not os.path.isdir(path):
        return None
    for cand in [path] + sorted(os.path.join(path, d) for d in os.listdir(path)):
        if os.path.isfile(os.path.join(cand, "main")) and os.path.isdir(os.path.join(cand, "data")):
            return cand
    return None


def browse_folder():
    try:
        import tkinter
        from tkinter import filedialog
        root = tkinter.Tk()
        root.withdraw()
        root.attributes("-topmost", True)
        p = filedialog.askdirectory(title="Choose the folder with your copy of the game (it has main and data inside)")
        root.destroy()
        return p
    except Exception:
        return ""


