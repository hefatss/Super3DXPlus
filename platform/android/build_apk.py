#!/usr/bin/env python3
"""Builds the Android app (the same as build.sh, in Python, so it also runs on Windows and macOS): an arm64-v8a APK for Android 9+, Vulkan through
src's GLES calls translated by platform/android/vkgl.c, AAudio sound. No Gradle: the SDK's build-tools and the NDK.

    python platform/android/build_apk.py --sdk <Android SDK> --jdk <JDK 17> --out <file.apk> [--pkg com.wmmt.port.export --label "Name"] [--embed <data folder>]

  --sdk    the Android SDK (build-tools 35 or newer, a platform, an NDK in ndk/)
  --jdk    a JDK 17 or newer (javac, keytool)
  --embed  put the converted data folder inside the APK (one self-contained file of about 1 GB; the app copies it to its own storage on first start)
  --pkg / --label   another application id and name, so two builds (Japan, Export) install side by side
  --novk   OpenGL ES 3 instead of Vulkan
  --keystore  the signing key (made if it does not exist). An update only installs over an app signed with the SAME key, so keep it.

Needs the generated sources (build.py gensrc <game folder>) and tools/vkprogs. It installs nothing."""
import argparse
import glob
import io
import os
import re
import shutil
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
API = 28
WIN = os.name == "nt"
TERMUX = os.environ.get("PREFIX", "").startswith("/data/data/com.termux")  # building on the phone itself, in Termux (docs/ANDROID_ON_DEVICE.md)
NATIVE_SRCS = ("platform/android/main_android.c platform/sdl2/bgm.c src/app.c src/render.c src/hud.c src/car.c src/collide.c src/sound.c src/anm.c "
               "src/menu.c src/bind.c src/lights.c src/spark.c src/haptic.c src/ffb.c src/net.c src/vs_host.c src/nat.c src/title.c src/phys/rc_runtime.c").split()
STORE_PASS = "super3dxdebug"


def say(*a):
    print(*a, flush=True)


def vkey(name):
    return [int(x) if x.isdigit() else 0 for x in re.split(r"[.\-]", name)]


def latest(folder):
    names = [n for n in os.listdir(folder) if not n.startswith(".")]
    if not names:
        raise SystemExit("%s is empty" % folder)
    return sorted(names, key=vkey)[-1]


def tool(directory, name, kinds=(".exe", ".bat", ".cmd", "")):
    """an executable in directory: name.exe / name.bat on Windows, name elsewhere (in Termux: the one on the PATH, `pkg install` put it there)"""
    if directory is None:
        p = shutil.which(name)
        if not p:
            raise SystemExit("%s was not found: in Termux run `pkg install aapt2 apksigner d8 openjdk-17 clang vulkan-headers vulkan-loader-android shaderc`" % name)
        return p
    for k in kinds:
        p = os.path.join(directory, name + k)
        if os.path.isfile(p):
            return p
    raise SystemExit("%s was not found in %s: is the Android SDK complete? (build-tools 35+, a platform and an NDK are needed)" % (name, directory))


def run(cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], cwd=kw.pop("cwd", ROOT), **kw)
    if r.returncode:
        raise SystemExit("a build command failed (exit code %d): %s ...\n  Its own message is just above." % (r.returncode, " ".join(str(c) for c in cmd[:3])))


def newer(a, b):
    return not os.path.exists(b) or os.path.getmtime(a) > os.path.getmtime(b)


PLATFORM_URL = "https://dl.google.com/android/repository/platform-34-ext7_r03.zip"  # the SDK's "platforms;android-34" (the same file for every host)
PLATFORM_SHA1 = "1f2e9478d6a7601425ceaa553311dc43191f103d"


def fetch_android_jar(dest):
    """Termux has no SDK: android.jar (the Android API for javac, d8 and aapt2) comes from Google's platform package, checked against its published checksum"""
    import hashlib
    import urllib.request
    say("downloading the Android platform 34 (63 MB; Google's Android SDK licence terms apply: https://developer.android.com/studio/terms) ...")
    try:
        data = urllib.request.urlopen(urllib.request.Request(PLATFORM_URL, headers={"User-Agent": "Super3DXPlus-build"}), timeout=120).read()
    except OSError as e:
        raise SystemExit("could not download the Android platform (%s): check the internet connection and run this again" % e)
    if hashlib.sha1(data).hexdigest() != PLATFORM_SHA1:
        raise SystemExit("the Android platform download is damaged (its checksum does not match): run this again")
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        name = [n for n in z.namelist() if n.endswith("/android.jar")][0]
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as f:
            f.write(z.read(name))


def build(sdk, jdk, out_apk, pkg=None, label=None, embed=None, vulkan=True, keystore=None, work=None, jobs=None, android_jar=None):
    work = work or os.path.join(ROOT, "build", "android")
    if TERMUX and not sdk:  # (the tools came with `pkg install`: no SDK folder, no NDK; the platform's android.jar is a download)
        bt = plat = ndk = host = llvm = None
        android_jar = android_jar or os.environ.get("ANDROID_JAR") or os.path.join(ROOT, ".super3dx", "android", "android.jar")
        if not os.path.isfile(android_jar):
            fetch_android_jar(android_jar)
    else:
        bt = os.path.join(sdk, "build-tools", latest(os.path.join(sdk, "build-tools")))
        plat = os.path.join(sdk, "platforms", latest(os.path.join(sdk, "platforms")))
        ndk = os.path.join(sdk, "ndk", latest(os.path.join(sdk, "ndk")))
        host = latest(os.path.join(ndk, "toolchains", "llvm", "prebuilt"))
        llvm = os.path.join(ndk, "toolchains", "llvm", "prebuilt", host, "bin")
        if vkey(os.path.basename(bt)) < [35]:
            raise SystemExit("build-tools %s is too old: zipalign -P needs 35 or newer" % os.path.basename(bt))
        android_jar = os.path.join(plat, "android.jar")
    for f in ("src/phys/phys_rc.c", "src/phys/rc_addr.h", "src/cardb.h"):
        if not os.path.isfile(os.path.join(ROOT, f)):
            raise SystemExit("missing %s: run `python build.py gensrc <your game folder>` first (the generated sources)" % f)
    env = dict(os.environ, JAVA_HOME=jdk, PATH=os.path.join(jdk, "bin") + os.pathsep + os.environ.get("PATH", ""))
    clang = [tool(llvm, "clang"), "--target=aarch64-linux-android%d" % API]
    if ndk is None:  # (Termux's sysroot has no GLES2 or EGL headers: the Khronos ones in third_party are looked at last)
        clang += ["-idirafter", "third_party/khronos"]

    shutil.rmtree(os.path.join(work, "apk"), ignore_errors=True)
    shutil.rmtree(os.path.join(work, "classes"), ignore_errors=True)
    for d in ("apk/lib/arm64-v8a", "classes", "obj", "res"):
        os.makedirs(os.path.join(work, d), exist_ok=True)

    gfx, gfx_libs = [], ["-lEGL", "-lGLESv2"]
    if vulkan:  # the programs' SPIR-V (tools/vkglsl.py) from their sources, by glslangValidator or the NDK's glslc
        spv = os.path.join(work, "vkgl_spv.c")
        glslc = shutil.which("glslc") if ndk is None else os.path.join(ndk, "shader-tools", host, "glslc" + (".exe" if WIN else ""))
        if not shutil.which("glslangValidator") and glslc and os.path.isfile(glslc):
            env["VKGLSL_CC"] = glslc
        newest = max([os.path.getmtime(f) for f in glob.glob(os.path.join(ROOT, "tools", "vkprogs", "*"))] + [os.path.getmtime(os.path.join(ROOT, "tools", "vkglsl.py"))])
        if not os.path.exists(spv) or os.path.getmtime(spv) < newest:
            say("the programs to SPIR-V...")
            run([sys.executable, os.path.join(ROOT, "tools", "vkglsl.py"), "table", os.path.join(ROOT, "tools", "vkprogs"), spv], env=env)
        gfx = ["-DWMMT_VK", "platform/android/vkgl.c", os.path.relpath(spv, ROOT)]
        gfx_libs = ["/system/lib64/libvulkan.so" if ndk is None else "-lvulkan"]  # (Termux's own libvulkan is another loader, with another soname)

    phys_o = os.path.join(work, "obj", "phys_rc.o")
    if newer(os.path.join(ROOT, "src/phys/phys_rc.c"), phys_o):
        say("compiling the arcade physics (a minute or two)...")
        run(clang + ["-O2", "-fPIC", "-ffp-contract=off", "-DRC_FPT=double", "-w", "-Isrc/phys", "-c", "src/phys/phys_rc.c", "-o", phys_o], env=env)
    say("compiling the game...")
    so = os.path.join(work, "apk", "lib", "arm64-v8a", "libwmmt.so")
    run(clang + ["-O2", "-fPIC", "-shared", "-std=gnu99", "-Wall", "-Wno-unused-function", "-DRC_FPT=double", "-DWMMT_EGL"] + gfx +
        ["-mcpu=cortex-a73", "-Isrc", "-Iplatform/android", "-Iplatform/sdl2", "-Ithird_party/stb"] + NATIVE_SRCS + [phys_o, "-o", so,
         "-landroid"] + gfx_libs + ["-laaudio", "-llog", "/system/lib64/libz.so" if ndk is None else "-lz", "-lm", "-Wl,-z,max-page-size=16384"], env=env)

    say("the Java side...")
    classes = os.path.join(work, "classes")
    run([tool(os.path.join(jdk, "bin"), "javac"), "-source", "8", "-target", "8", "-nowarn", "-classpath", android_jar, "-d", classes,
         "platform/android/java/com/wmmt/port/MainActivity.java"], env=env)
    class_files = [os.path.join(dp, f) for dp, _, fn in os.walk(classes) for f in fn if f.endswith(".class")]
    run([tool(bt, "d8"), "--min-api", str(API), "--lib", android_jar, "--output", os.path.join(work, "apk")] + class_files, env=env)

    icon = os.path.join(ROOT, "data", "icon", "Icon-76@2x.png")  # a plain placeholder unless you put your own artwork there
    if not os.path.isfile(icon):
        os.makedirs(os.path.dirname(icon), exist_ok=True)
        run([sys.executable, os.path.join(ROOT, "tools", "dev", "placeholder_icon.py"), icon], env=env)
    os.makedirs(os.path.join(work, "res", "mipmap"), exist_ok=True)
    shutil.copy(icon, os.path.join(work, "res", "mipmap", "icon.png"))
    res_zip = os.path.join(work, "res.zip")
    run([tool(bt, "aapt2"), "compile", "--dir", os.path.join(work, "res"), "-o", res_zip], env=env)
    manifest = os.path.join(ROOT, "platform", "android", "AndroidManifest.xml")
    if pkg:  # another application id: a second build (Export) installs beside this one, with a data folder of its own
        text = open(manifest, encoding="utf-8").read()
        text = text.replace('package="com.wmmt.port"', 'package="%s"' % pkg).replace('android:name=".MainActivity"', 'android:name="com.wmmt.port.MainActivity"')
        text = re.sub(r'android:label="[^"]*"', 'android:label="%s"' % (label or "Super3DX+"), text)
        manifest = os.path.join(work, "AndroidManifest.xml")
        open(manifest, "w", encoding="utf-8").write(text)
    base = os.path.join(work, "base.apk")
    run([tool(bt, "aapt2"), "link", "-o", base, "-I", android_jar, "--manifest", manifest, "--min-sdk-version", str(API), "--target-sdk-version", "33",
         "--version-code", "2", "--version-name", "1.1", res_zip], env=env)

    say("packing...")
    with zipfile.ZipFile(base, "a") as z:  # (the library stored, not deflated: it is mapped from the apk as it is, page-aligned by zipalign)
        z.write(os.path.join(work, "apk", "classes.dex"), "classes.dex", zipfile.ZIP_DEFLATED)
        z.write(so, "lib/arm64-v8a/libwmmt.so", zipfile.ZIP_STORED)
        if embed:
            say("putting the game's data inside (%s)..." % embed)
            skip_dirs = {"save", "save-run"}
            n = 0
            for dp, dn, fn in os.walk(embed):
                dn[:] = [d for d in dn if d not in skip_dirs]
                for f in fn:
                    if f.endswith((".wmv", ".log", ".part")):
                        continue
                    full = os.path.join(dp, f)
                    z.write(full, "assets/wmmt/" + os.path.relpath(full, embed).replace(os.sep, "/"), zipfile.ZIP_DEFLATED, compresslevel=1)
                    n += 1
                    if n % 500 == 0:
                        say("  %d files..." % n)
    size = os.path.getsize(base)
    if size >= 4 * 1024 ** 3 - 1:
        raise SystemExit("the APK would be %.1f GB; Android's signature format allows under 4 GB" % (size / 1024 ** 3))
    aligned = os.path.join(work, "aligned.apk")
    if os.path.exists(aligned):
        os.remove(aligned)
    if bt is None:  # (no zipalign in Termux: apksigner 35 and later page-aligns the libraries itself when it signs)
        shutil.copyfile(base, aligned)
    else:
        run([tool(bt, "zipalign"), "-f", "-P", "16", "4", base, aligned], env=env)
    keystore = keystore or os.path.join(work, "debug.keystore")
    if not os.path.isfile(keystore):
        os.makedirs(os.path.dirname(keystore), exist_ok=True)
        run([tool(os.path.join(jdk, "bin"), "keytool"), "-genkeypair", "-keystore", keystore, "-storepass", STORE_PASS, "-keypass", STORE_PASS, "-alias", "super3dx",
             "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000", "-dname", "CN=Super3DX debug"], env=env)
    os.makedirs(os.path.dirname(os.path.abspath(out_apk)), exist_ok=True)
    run([tool(bt, "apksigner"), "sign", "--v4-signing-enabled", "false", "--ks", keystore, "--ks-pass", "pass:" + STORE_PASS, "--key-pass", "pass:" + STORE_PASS, "--out", out_apk, aligned], env=env)
    say("built %s (%.0f MB)" % (out_apk, os.path.getsize(out_apk) / 1e6))
    return out_apk


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sdk", default=os.environ.get("ANDROID_HOME"))
    ap.add_argument("--jdk", default=os.environ.get("JAVA_HOME"))
    ap.add_argument("--out", required=True)
    ap.add_argument("--pkg")
    ap.add_argument("--label")
    ap.add_argument("--embed")
    ap.add_argument("--novk", action="store_true")
    ap.add_argument("--keystore")
    ap.add_argument("--android-jar", help="Termux only: the Android platform's android.jar (downloaded to .super3dx/android/ when not given)")
    a = ap.parse_args()
    if TERMUX and not a.jdk:
        a.jdk = os.environ.get("JAVA_HOME") or os.path.join(os.environ["PREFIX"], "lib", "jvm", "java-17-openjdk")
    if not a.jdk or (not a.sdk and not TERMUX):
        raise SystemExit("give --sdk (or set ANDROID_HOME) and --jdk (or set JAVA_HOME)")
    build(a.sdk, a.jdk, a.out, a.pkg, a.label, a.embed, not a.novk, a.keystore, android_jar=a.android_jar)


if __name__ == "__main__":
    main()
