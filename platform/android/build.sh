#!/bin/bash
# Build the Android app: build/android/super3dx-android.apk (arm64-v8a, Android 9+, Vulkan through src's GLES
# calls translated by platform/android/vkgl.c, AAudio sound). No Gradle: the SDK's build-tools and the NDK.
#
#   ANDROID_HOME   the Android SDK (build-tools >= 35 for zipalign -P, a platform, an NDK in ndk/)
#   JAVA_HOME      a JDK (17 works)
#   VK=0           OpenGL ES 3 instead of Vulkan
#
# Needs the generated sources (python build.py gensrc <your game folder>) and tools/vkprogs. It does not install
# or push anything: platform/android/deploy.ps1 (Windows, adb on USB) does.
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?set ANDROID_HOME to the Android SDK}"
: "${JAVA_HOME:?set JAVA_HOME to a JDK}"
SDK="$ANDROID_HOME"
BT="$SDK/build-tools/$(ls "$SDK/build-tools" | sort -V | tail -1)"
PLAT="$SDK/platforms/$(ls "$SDK/platforms" | sort -V | tail -1)"
NDK="$SDK/ndk/$(ls "$SDK/ndk" | sort -V | tail -1)"
HOST=$(ls "$NDK/toolchains/llvm/prebuilt" | head -1)
API=28
CC="$NDK/toolchains/llvm/prebuilt/$HOST/bin/aarch64-linux-android$API-clang"
case "$(basename "$BT")" in
  [0-9]|[0-9].*|[12][0-9].*|3[0-4].*) echo "build-tools $(basename "$BT") is too old: zipalign -P needs 35 or newer" >&2; exit 1 ;;
esac
for f in src/phys/phys_rc.c src/phys/rc_addr.h src/cardb.h; do
  [ -f "$f" ] || { echo "missing $f: run python build.py gensrc <your game folder> first" >&2; exit 1; }
done
OUT=build/android
rm -rf "$OUT/apk" "$OUT/classes" && mkdir -p "$OUT/apk/lib/arm64-v8a" "$OUT/classes" "$OUT/obj" "$OUT/res"

# the programs' SPIR-V for Vulkan (tools/vkglsl.py) from their sources (tools/vkprogs: a new one the game meets
# is written to the phone's save folder, <key>.txt, to put there), by the NDK's glslc unless there is a glslangValidator
VK="${VK:-1}"
if [ "$VK" = 1 ]; then
  GLSLC="$NDK/shader-tools/$HOST/glslc"
  if ! command -v glslangValidator >/dev/null 2>&1 && [ -x "$GLSLC" ]; then export VKGLSL_CC="$GLSLC"; fi
  if [ ! -f "$OUT/vkgl_spv.c" ] || [ -n "$(find tools/vkprogs tools/vkglsl.py -newer "$OUT/vkgl_spv.c" | head -1)" ]; then
    echo "the programs to SPIR-V..."
    python3 tools/vkglsl.py table tools/vkprogs "$OUT/vkgl_spv.c"
  fi
  GFX="-DWMMT_VK platform/android/vkgl.c $OUT/vkgl_spv.c"
  GFX_LIBS="-lvulkan"
else
  GFX=""
  GFX_LIBS="-lEGL -lGLESv2"
fi

# the native library
if [ ! -f "$OUT/obj/phys_rc.o" ] || [ src/phys/phys_rc.c -nt "$OUT/obj/phys_rc.o" ]; then
  echo "compiling the arcade physics (takes a minute)..."
  timeout 900 $CC -O2 -fPIC -ffp-contract=off -DRC_FPT=double -w -Isrc/phys -c src/phys/phys_rc.c -o "$OUT/obj/phys_rc.o"
fi
$CC -O2 -fPIC -shared -std=gnu99 -Wall -Wno-unused-function -DRC_FPT=double -DWMMT_EGL $GFX \
  -mcpu=cortex-a73 -Isrc -Iplatform/android -Iplatform/sdl2 -Ithird_party/stb \
  platform/android/main_android.c platform/sdl2/bgm.c src/app.c src/render.c src/hud.c src/car.c src/collide.c src/sound.c src/anm.c src/menu.c src/bind.c \
  src/lights.c src/spark.c src/haptic.c src/ffb.c src/net.c src/vs_host.c src/nat.c src/title.c \
  src/phys/rc_runtime.c "$OUT/obj/phys_rc.o" \
  -o "$OUT/apk/lib/arm64-v8a/libwmmt.so" \
  -landroid $GFX_LIBS -laaudio -llog -lz -lm -Wl,-z,max-page-size=16384

# the Java side
"$JAVA_HOME/bin/javac" -source 8 -target 8 -nowarn -classpath "$PLAT/android.jar" -d "$OUT/classes" \
  platform/android/java/com/wmmt/port/MainActivity.java
"$BT/d8" --min-api $API --lib "$PLAT/android.jar" --output "$OUT/apk" $(find "$OUT/classes" -name '*.class')

# the manifest and icon, then the apk, aligned and signed with a key of its own. The icon is a plain
# placeholder unless data/icon/Icon-76@2x.png exists (put your own artwork there)
if [ ! -f data/icon/Icon-76@2x.png ]; then
  mkdir -p data/icon
  python3 tools/placeholder_icon.py data/icon/Icon-76@2x.png
fi
mkdir -p "$OUT/res/mipmap"
cp data/icon/Icon-76@2x.png "$OUT/res/mipmap/icon.png"
"$BT/aapt2" compile --dir "$OUT/res" -o "$OUT/res.zip"
# PKG=<id> (and LABEL=<name>): another application id, so a second build (the Export one) installs beside this one with a data folder of its own
MANIFEST=platform/android/AndroidManifest.xml
if [ -n "${PKG:-}" ]; then
  MANIFEST="$OUT/AndroidManifest.xml"
  sed "s/package=\"com.wmmt.port\"/package=\"$PKG\"/; s/android:name=\".MainActivity\"/android:name=\"com.wmmt.port.MainActivity\"/; s/android:label=\"[^\"]*\"/android:label=\"${LABEL:-WMMT3DX}\"/" \
    platform/android/AndroidManifest.xml > "$MANIFEST"
fi
"$BT/aapt2" link -o "$OUT/base.apk" -I "$PLAT/android.jar" --manifest "$MANIFEST" \
  --min-sdk-version $API --target-sdk-version 33 --version-code 1 --version-name 1.0 "$OUT/res.zip"
# (the library stored, not deflated, and page-aligned: it is mapped from the apk as it is)
(cd "$OUT/apk" && zip -q ../base.apk classes.dex && zip -q -0 -r ../base.apk lib)
# EMBED=<converted data folder>: the data goes into the apk (assets/wmmt, deflated; the SDL player's .wmv movies and any saves left out), and the app
# copies it to its data folder on its first start (MainActivity.installData): one file to carry to a phone, no adb. The apk grows to ~1 GB for the
# English data (it must stay under 4 GB: the v2 signature has no zip64).
if [ -n "${EMBED:-}" ]; then
  echo "embedding $EMBED..."
  rm -rf "$OUT/apkdata" && mkdir -p "$OUT/apkdata/assets"
  ln -s "$(cd "$EMBED" && pwd)" "$OUT/apkdata/assets/wmmt"
  (cd "$OUT/apkdata" && zip -q -1 -r ../base.apk assets -x '*.wmv' -x 'assets/wmmt/save/*' -x 'assets/wmmt/save-run/*' -x '*.log' -x '*.part')
fi
"$BT/zipalign" -f -P 16 4 "$OUT/base.apk" "$OUT/aligned.apk"
KS="$OUT/debug.keystore"
[ -f "$KS" ] || "$JAVA_HOME/bin/keytool" -genkeypair -keystore "$KS" -storepass super3dxdebug -keypass super3dxdebug \
  -alias super3dx -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Super3DX+ debug" >/dev/null 2>&1
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:super3dxdebug --key-pass pass:super3dxdebug --out "$OUT/super3dx-android.apk" "$OUT/aligned.apk"
echo "built $(pwd)/$OUT/super3dx-android.apk"
