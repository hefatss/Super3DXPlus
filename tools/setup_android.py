"""The Android setup (`python build.py android`, or setup-android.bat / ./setup-android.sh): the one setup this branch has.

It finds or downloads what an Android build needs (a JDK, and the Android SDK with its build tools and NDK), builds ONE self-contained APK with
the game's data inside (Super3DXPlus-<version>-Android.apk), and installs it on a phone connected by USB if you like. Works from Windows, Linux
and macOS. The tools it downloads go into the project's .super3dx folder (about 3 GB for the Android SDK and NDK) and are used again next time."""
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tarfile
import time
import urllib.request
import xml.etree.ElementTree as ET
import zipfile

from setupkit import ROOT, ISSUES, say, ask, yes, step, which, find_game, browse_folder, run_logged, explain_failure

TOOLS = os.path.join(ROOT, ".super3dx")
SDK_PACKAGES = ["platform-tools", "platforms;android-34", "build-tools;35.0.0", "ndk;26.1.10909125"]
REPO_XML = "https://dl.google.com/android/repository/repository2-3.xml"
REPO_BASE = "https://dl.google.com/android/repository/"
# the command-line tools version whose sdkmanager is a plain Java program (the newest ones turn it into a deprecated wrapper around a new tool)
CMDLINE_TOOLS = "cmdline-tools;11.0"
LICENSES = "https://developer.android.com/studio/terms"
WIN = os.name == "nt"
EXE = ".exe" if WIN else ""


def _open(url, timeout=60):
    """urlopen with a client name (some download servers refuse Python's default one with a 403)"""
    return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "Super3DXPlus-setup"}), timeout=timeout)


def _vkey(name):
    return [int(x) if x.isdigit() else 0 for x in name.replace("-", ".").split(".")]


def _download(url, dest, digest=None, algo="sha256", what="a file"):
    say("downloading %s ..." % what)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    h = hashlib.new(algo)
    try:
        with _open(url) as r, open(dest + ".part", "wb") as f:
            total = int(r.headers.get("Content-Length") or 0)
            n, shown = 0, -1
            while True:
                b = r.read(1 << 20)
                if not b:
                    break
                f.write(b)
                h.update(b)
                n += len(b)
                if total and n * 10 // total != shown:
                    shown = n * 10 // total
                    say("  %d%% of %.0f MB" % (shown * 10, total / 1e6))
    except OSError as e:
        raise SystemExit("could not download %s\n  (%s)\n  Check your internet connection (a firewall, VPN or proxy can block it) and run this again." % (what, e))
    if digest and h.hexdigest().lower() != digest.lower():
        os.remove(dest + ".part")
        raise SystemExit("the download of %s is damaged (its %s checksum does not match). Run this again." % (what, algo))
    os.replace(dest + ".part", dest)


def _extract(archive, dest):
    shutil.rmtree(dest, ignore_errors=True)
    os.makedirs(dest)
    if archive.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            for info in z.infolist():
                z.extract(info, dest)
                mode = info.external_attr >> 16
                if mode and not WIN:
                    os.chmod(os.path.join(dest, info.filename), mode & 0o7777 | stat.S_IRUSR)
    else:
        with tarfile.open(archive) as t:
            t.extractall(dest)


# ------------------------------------------------------------------------------------------------------------ the JDK
def _java_ok(home):
    javac = os.path.join(home, "bin", "javac" + EXE)
    if not (home and os.path.isfile(javac) and os.path.isfile(os.path.join(home, "bin", "keytool" + EXE))):
        return False
    r = subprocess.run([javac, "-version"], capture_output=True, text=True)
    try:
        return int((r.stdout + r.stderr).split()[1].split(".")[0]) >= 17
    except (IndexError, ValueError):
        return False


def find_jdk():
    cands = [os.environ.get("JAVA_HOME", "")]
    p = shutil.which("javac")
    if p:
        cands.append(os.path.dirname(os.path.dirname(os.path.realpath(p))))
    jdk_dir = os.path.join(TOOLS, "jdk")
    if os.path.isdir(jdk_dir):
        for dp, dn, fn in os.walk(jdk_dir):
            if "javac" + EXE in fn and os.path.basename(dp) == "bin":
                cands.append(os.path.dirname(dp))
                break
    return next((c for c in cands if _java_ok(c)), None)


def ensure_jdk():
    home = find_jdk()
    if home:
        say("Java (JDK 17 or newer): " + home)
        return home
    say("\nA JDK (Java development kit, version 17 or newer) is needed to build the app, and none was found.")
    if not yes("Download Eclipse Temurin JDK 17 (about 190 MB, free, into this project's .super3dx folder)?"):
        say("Install a JDK 17 yourself (https://adoptium.net) and run this again.")
        raise SystemExit("a JDK is required")
    import platform as _p
    os_ = {"Windows": "windows", "Linux": "linux", "Darwin": "mac"}.get(_p.system())
    arch = {"amd64": "x64", "x86_64": "x64", "arm64": "aarch64", "aarch64": "aarch64"}.get(_p.machine().lower())
    if not os_ or not arch:
        raise SystemExit("no JDK download for this machine: install a JDK 17 yourself (https://adoptium.net)")
    url = "https://api.adoptium.net/v3/assets/latest/17/hotspot?architecture=%s&image_type=jdk&os=%s&vendor=eclipse" % (arch, os_)
    try:
        pkg = json.load(_open(url))[0]["binary"]["package"]
    except (OSError, ValueError, KeyError, IndexError) as e:
        raise SystemExit("could not look up the JDK download (%s). Check your internet connection, or install a JDK 17 yourself." % e)
    arch_file = os.path.join(TOOLS, pkg["name"])
    _download(pkg["link"], arch_file, pkg.get("checksum"), "sha256", "the JDK")
    _extract(arch_file, os.path.join(TOOLS, "jdk"))
    os.remove(arch_file)
    home = find_jdk()
    if not home:
        raise SystemExit("the JDK was downloaded but could not be used; delete the .super3dx/jdk folder and run this again")
    say("Java: " + home)
    return home


# ------------------------------------------------------------------------------------------------------------ the Android SDK
def _sdk_ok(sdk):
    try:
        bt, pl, nd = (os.path.join(sdk, d) for d in ("build-tools", "platforms", "ndk"))
        newest = sorted(os.listdir(bt), key=_vkey)[-1]
        return (_vkey(newest) >= [35] and any(os.path.isfile(os.path.join(pl, p, "android.jar")) for p in os.listdir(pl))
                and any(os.path.isdir(os.path.join(nd, n, "toolchains")) for n in os.listdir(nd)))
    except (OSError, IndexError):
        return False


def find_sdk():
    cands = [os.environ.get("ANDROID_HOME", ""), os.environ.get("ANDROID_SDK_ROOT", ""), os.path.join(TOOLS, "android-sdk"), _sdk_home(),
             os.path.join(os.environ.get("LOCALAPPDATA", ""), "Android", "Sdk"), os.path.expanduser("~/Android/Sdk"), os.path.expanduser("~/Library/Android/sdk"),
             "/opt/android-sdk"]
    return next((c for c in cands if c and _sdk_ok(c)), None)


def _sdk_home():
    """where the Android SDK this setup downloads goes: the project's .super3dx folder, but on Windows, in a short place when that folder is already deep (a game
    unzipped inside Downloads\\some folder\\the same folder again): the SDK's own files go 150 characters down, and Windows stops at 260 (issue 34)"""
    home = os.path.join(TOOLS, "android-sdk")
    if WIN and len(home) > 70:
        base = os.environ.get("LOCALAPPDATA") or os.path.expanduser("~")
        home = os.path.join(base, "Super3DX", "android-sdk")
    return home


def _cmdline_tools(sdk, jdk):
    """the sdkmanager from Google's command-line tools (downloaded from the address in Google's own repository list)"""
    sm = os.path.join(sdk, "cmdline-tools", "latest", "bin", "sdkmanager" + (".bat" if WIN else ""))
    if os.path.isfile(sm):
        return sm
    host = {"nt": "windows", "posix": "linux"}[os.name]
    import platform as _p
    if _p.system() == "Darwin":
        host = "macosx"
    try:
        root = ET.fromstring(_open(REPO_XML).read())
    except (OSError, ET.ParseError) as e:
        raise SystemExit("could not read Google's Android download list (%s). Check your internet connection and run this again." % e)
    for pkg in root.iter():
        if pkg.tag.endswith("remotePackage") and pkg.get("path") == CMDLINE_TOOLS:
            for ar in pkg.iter():
                if ar.tag.endswith("archive") and next((c.text for c in ar if c.tag.endswith("host-os")), None) == host:
                    comp = next(c for c in ar if c.tag.endswith("complete"))
                    url = next(c.text for c in comp if c.tag.endswith("url"))
                    sha = next(c.text for c in comp if c.tag.endswith("checksum"))
                    z = os.path.join(TOOLS, url)
                    _download(REPO_BASE + url, z, sha, "sha1", "the Android command-line tools")
                    tmp = os.path.join(os.path.dirname(sdk), "cmdline-tools.tmp")  # (beside the SDK: as short as it is)
                    os.makedirs(os.path.dirname(sdk), exist_ok=True)
                    _extract(z, tmp)
                    os.makedirs(os.path.join(sdk, "cmdline-tools"), exist_ok=True)
                    shutil.rmtree(os.path.join(sdk, "cmdline-tools", "latest"), ignore_errors=True)
                    shutil.move(os.path.join(tmp, "cmdline-tools"), os.path.join(sdk, "cmdline-tools", "latest"))
                    shutil.rmtree(tmp, ignore_errors=True)
                    os.remove(z)
                    if not WIN:
                        for f in os.listdir(os.path.dirname(sm)):
                            os.chmod(os.path.join(os.path.dirname(sm), f), 0o755)
                    return sm
    raise SystemExit("Google's download list has no command-line tools for this system; install the Android SDK yourself (Android Studio) and set ANDROID_HOME")


def _sdkmanager_cmd(sm, jdk, sdk, *args):
    """the command line for the SDK manager, started through java directly (what its script does): the sdkmanager.bat of Windows splits
    package names like "ndk;26.1.10909125" at the semicolon"""
    home = os.path.dirname(os.path.dirname(sm))
    java = os.path.join(jdk, "bin", "java" + EXE)
    return [java, "-Dcom.android.sdklib.toolsdir=" + home, "-classpath", os.path.join(home, "lib", "sdkmanager-classpath.jar"),
            "com.android.sdklib.tool.sdkmanager.SdkManagerCli", "--sdk_root=" + sdk] + list(args)


def ensure_sdk(jdk):
    sdk = find_sdk()
    if sdk:
        say("Android SDK: " + sdk)
        return sdk
    sdk = _sdk_home()
    say("\nThe Android build tools were not found. The setup can download them into this project's .super3dx folder:")
    say("the Android SDK build tools, platform and NDK (the C compiler for Android): about 3 GB to download, 4 GB on disk, free of charge.")
    say("Using them means accepting Google's Android SDK licence terms (%s)." % LICENSES)
    if not yes("Download them and accept the licence terms?", False):
        say("Install the Android SDK yourself (Android Studio's SDK Manager: build-tools 35, an NDK, a platform), set ANDROID_HOME, and run this again.")
        raise SystemExit("the Android SDK is required")
    env = dict(os.environ, JAVA_HOME=jdk, PATH=os.path.join(jdk, "bin") + os.pathsep + os.environ.get("PATH", ""))
    sm = _cmdline_tools(sdk, jdk)
    subprocess.run(_sdkmanager_cmd(sm, jdk, sdk, "--licenses"), input="y\n" * 60, text=True, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    say("\nInstalling the SDK packages (several minutes)...")
    r = subprocess.run(_sdkmanager_cmd(sm, jdk, sdk, *SDK_PACKAGES), env=env)
    if r.returncode or not _sdk_ok(sdk):
        raise SystemExit("the Android SDK could not be installed (see the messages above). Check your internet connection and free disk space, then run this again.")
    say("Android SDK: " + sdk)
    return sdk


# ------------------------------------------------------------------------------------------------------------ a phone
def _adb(sdk):
    for c in (os.path.join(sdk, "platform-tools", "adb" + EXE), shutil.which("adb")):
        if c and os.path.isfile(c):
            return c
    return None


def install_on_phone(sdk, apk, pkg):
    adb = _adb(sdk)
    if not adb:
        return False
    r = subprocess.run([adb, "devices"], capture_output=True, text=True)
    devs = [l.split()[0] for l in r.stdout.splitlines()[1:] if l.strip().endswith("device")]
    unauth = [l.split()[0] for l in r.stdout.splitlines()[1:] if l.strip().endswith("unauthorized")]
    if unauth and not devs:
        say("\nA phone is connected but has not allowed this PC: look at the phone's screen and tap \"Allow USB debugging\", then run this again.")
        return False
    if not devs:
        return False
    serial = devs[0]
    if not yes("\nA phone is connected by USB (%s). Install the game on it now? (it takes a few minutes; the phone must stay unlocked)" % serial):
        return False
    r = subprocess.run([adb, "-s", serial, "install", "-r", apk], capture_output=True, text=True)
    out = (r.stdout + r.stderr).strip()
    if r.returncode == 0 and "Success" in out:
        say("Installed. Open \"Super3DX+\" on the phone; the first start copies the game's data (a minute or two).")
        return True
    if "UPDATE_INCOMPATIBLE" in out or "signatures do not match" in out:
        say("\nThe phone already has a copy of this app that was built with a different signing key (for example by another PC), so Android refuses the update.")
        say("To replace it you must uninstall it first, which DELETES its saves. If you want them, back them up first, then remove the app and run this again.")
        say("(Future updates from this PC work, because its signing key is kept in .super3dx/android/debug.keystore: keep a copy of that file.)")
    elif "INSUFFICIENT_STORAGE" in out or "No space" in out:
        say("\nThe phone does not have enough free storage (the app needs about 3 GB while installing). Free some space and try again.")
    else:
        say("\nThe install failed:\n  " + out[-400:])
    return False


# ------------------------------------------------------------------------------------------------------------ the setup
KNOWN = {"22b9bbcc5a0cefd42a4e83b2dd892c98": "Japan", "aac96862372ee774a14096792fc40c13": "Export"}


def find_md5(game):
    return hashlib.md5(open(os.path.join(game, "main"), "rb").read()).hexdigest()


def run(build_py, jobs):
    total = 4
    say("")
    say("Super3DX+ setup for Android")
    say("This downloads what an Android build needs (a JDK, and the Android SDK with its build tools and NDK), asks for your copy of the game,")
    say("converts the data and builds ONE self-contained APK with the game inside it, for phones and handhelds with Android 9 or newer (a 64-bit")
    say("ARM device with Vulkan or GLES3). You can leave it alone while it works.")

    step(1, total, "find your copy of the game")
    say("I need the folder that holds your copy of the game: it has a file called `main` and a folder called `data` inside.")
    game = None
    while not game:
        say("\nPress Enter to browse for the folder, or paste / drag the folder into this window and press Enter (q to quit).")
        a = ask("> ")
        if a.lower() == "q":
            return 1
        game = find_game(a or browse_folder())
    md5 = find_md5(game)
    region = KNOWN.get(md5)
    say("\nGame folder: " + game)
    if region:
        say("Recognised: the %s version of 3DX+." % region)
    else:
        say("This `main` is not one of the versions this project was tested with (docs/BUILDS.md lists them).")
        say("It may still work, but expect problems.")
        if not yes("Carry on anyway?", False):
            return 1
        region = "Custom"
    pkg = "com.wmmt.port" if region == "Japan" else "com.wmmt.port." + region.lower()
    label = "Super3DX+ (%s)" % region
    free = shutil.disk_usage(ROOT).free / 2**30
    if free < 8:
        say("\nOnly %.0f GB is free on this drive; the build needs about 8 GB (plus 4 GB if the Android tools must be downloaded)." % free)
        if not yes("Carry on anyway?", False):
            return 1

    step(2, total, "get the Android tools")
    jdk = ensure_jdk()
    sdk = ensure_sdk(jdk)

    step(3, total, "convert the data and build the app")
    log = os.path.join(ROOT, "build", "logs", "setup-android-%s.txt" % time.strftime("%Y%m%d-%H%M%S"))
    say("(everything below is also saved in %s)\n" % os.path.relpath(log, ROOT))
    say("Converting the game's data and regenerating the sources (about an hour)...")
    rc = run_logged([sys.executable, build_py, "data", game], log)
    if rc:
        explain_failure(log, rc)
        return 1
    out_apk = os.path.join(ROOT, "Super3DXPlus-%s-Android.apk" % region)
    keystore = os.path.join(TOOLS, "android", "debug.keystore")
    cmd = [sys.executable, os.path.join(ROOT, "platform", "android", "build_apk.py"), "--sdk", sdk, "--jdk", jdk, "--out", out_apk, "--pkg", pkg,
           "--label", label, "--embed", os.path.join(ROOT, "data"), "--keystore", keystore]
    rc = run_logged(cmd, log)
    if rc:
        explain_failure(log, rc)
        return 1

    step(4, total, "done")
    say("Your Android app is:\n\n    %s\n" % out_apk)
    if not install_on_phone(sdk, out_apk, pkg):
        say("To put it on a phone: copy that one file to the phone (USB cable, cloud drive, ...), open it there, and allow \"install unknown apps\" when asked.")
        say("Or turn on USB debugging on the phone, plug it in, and run this again: the setup can install it for you.")
    say("\nThe file contains the game's data, so do not share it. Saves live on the phone; installing a newer build over the old one keeps them.")
    say("Keep .super3dx/android/debug.keystore: it is the key the app is signed with, and updates only install over an app signed with the same key.")
    return 0
