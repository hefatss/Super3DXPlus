"""What the tools need from the system that is not a Python package, found the same way on Windows, Linux and macOS:
a C++ demangler (c++filt, or tools/demangle.cpp built with any C++ compiler, which is what Windows uses) and fonts (any of a list of file names in the usual font folders, or in
the folders named by MT3DX_FONT_DIRS, `;` or `:` separated)."""
import glob
import os
import shutil
import subprocess


def _helper_compilers():
    """C++ compilers that can build tools/demangle.cpp: one on PATH, or a C++ compiler on PATH"""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    exe = "zig.exe" if os.name == "nt" else "zig"
    for z in (os.path.join(root, ".super3dx", "zig", exe), shutil.which("zig")):
        if z and os.path.isfile(z):
            tgt = ["-target", "x86_64-windows-gnu"] if os.name == "nt" else []
            yield [z, "c++", "-O2"] + tgt
    for cxx in ("g++", "clang++", "x86_64-w64-mingw32-g++"):
        p = shutil.which(cxx)
        if p:
            yield [p, "-O2"]


def _demangle_helper():
    """tools/demangle.cpp built once into .super3dx/ (libc++abi's demangler, from any C++ compiler); None if it cannot be built"""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = os.path.join(root, "tools", "demangle.cpp")
    out = os.path.join(root, ".super3dx", "demangle" + (".exe" if os.name == "nt" else ""))
    if os.path.isfile(out) and os.path.getmtime(out) >= os.path.getmtime(src):
        return out
    os.makedirs(os.path.dirname(out), exist_ok=True)
    for cmd in _helper_compilers():
        if subprocess.run(cmd + ["-o", out, src], capture_output=True).returncode == 0 and os.path.isfile(out):
            return out
    return None


def demangle_many(names):
    """the demangled names in the same count and order, like `c++filt -n`: with c++filt (binutils) or llvm-cxxfilt where they are on PATH, else
    with tools/demangle.cpp (built once from a C++ compiler's libc++abi: that is what Windows uses)"""
    names = list(names)
    exe = shutil.which("c++filt") or shutil.which("llvm-cxxfilt")
    if not exe:
        exe = _demangle_helper()
    if not exe:
        raise SystemExit("no C++ demangler: install binutils (Linux: sudo apt install binutils), or a C++ compiler (g++ / clang++) on Windows, then try again")
    cmd = [exe, "-n"] if os.path.basename(exe).lower().startswith(("c++filt", "llvm-cxxfilt")) else [exe]
    r = subprocess.run(cmd, input="\n".join(names).encode("utf-8"), capture_output=True)
    out = r.stdout.decode("utf-8", "replace").replace("\r", "").split("\n")
    if r.returncode or len(out) < len(names):
        raise SystemExit("the C++ demangler (%s) failed (exit %d): %s" % (exe, r.returncode, r.stderr.decode("utf-8", "replace")[:300]))
    return out[:len(names)]


def _font_dirs():
    dirs = [p for p in os.environ.get("MT3DX_FONT_DIRS", "").replace(";", os.pathsep).split(os.pathsep) if p]
    if os.name == "nt":
        dirs += [os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts"), os.path.join(os.environ.get("LOCALAPPDATA", ""), "Microsoft", "Windows", "Fonts")]
    if os.environ.get("PREFIX"):  # (Termux keeps its packages' fonts under its own prefix)
        dirs.append(os.path.join(os.environ["PREFIX"], "share", "fonts"))
    dirs += ["/usr/share/fonts", "/usr/local/share/fonts", os.path.expanduser("~/.fonts"), os.path.expanduser("~/.local/share/fonts"),
             "/Library/Fonts", "/System/Library/Fonts", "/System/Library/Fonts/Supplemental", os.path.expanduser("~/Library/Fonts")]
    return dirs


def find_font(*names):
    """the first existing font file among `names` (file names, case-insensitive, or absolute paths), searched in the font folders"""
    for n in names:
        if os.path.isabs(n) and os.path.exists(n):
            return n
    for d in _font_dirs():
        for n in names:
            if os.path.isabs(n):
                continue
            for p in glob.glob(os.path.join(d, "**", n), recursive=True) + glob.glob(os.path.join(d, "**", n.lower()), recursive=True):
                return p
    raise SystemExit("no font found: tried %s (put one in a folder named by MT3DX_FONT_DIRS)" % ", ".join(names))
