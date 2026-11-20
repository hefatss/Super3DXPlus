# Building the whole game on an Android device

This builds the game from your own game files **entirely on the Android device**, with no PC: the data (about an hour), then the app (a few
minutes), then installing it. It uses [Termux](https://termux.dev), a terminal for Android.

If you have a PC, `setup-android.bat` / `./setup-android.sh` (see the README) is easier. This page is for when you do not.

**What was tested:** the whole of this page, start to finish, for the Export version on a Retroid Pocket 5 (Android, Snapdragon, 64-bit ARM) with
Termux 0.118 (Python 3.14): a fresh checkout, the data (72 minutes), the app (3 minutes), install, then the game itself: the menus, and a drive
through C1 inward in the morning (including the tunnel exit at about 10.9 km from the goal, where the ceiling beams and the sky show). The data
matches a PC's, apart from re-encoded sound and video files (no visible or audible difference) and one comment in a generated file. The Japan
version uses the same steps but was not tried on a device. The app was installed from the Files app, as step 7 describes.

## What you need

- An Android 9+ phone or handheld with a 64-bit ARM processor and Vulkan (nearly everything made since 2018).
- About **20 GB of free storage** (the game folder is 5-6 GB, the converted data 3 GB, the app 1 GB, and the installed app another 3 GB).
- Your own dump of the arcade game: a folder with `main` and `data/` in it (the same one the PC setup asks for). Put it anywhere in the phone's
  storage, for example `Download/WMMT3DX`. It must be on the phone itself.
- A charger and a few hours of patience. A handheld takes roughly **an hour and a quarter** for the data. Keep it plugged in.

## 1. Install Termux

Install Termux from **F-Droid** or from its **GitHub releases page** (https://github.com/termux/termux-app/releases, the `arm64-v8a` file). Do not
use the Google Play version: it is not the maintained one.

Open it, then give it access to the phone's storage (tap Allow) and update it:

```sh
termux-setup-storage
pkg update -y
```

Everything below is typed into Termux. Long builds can be stopped by Android's battery saving: in Android's settings, set Termux's battery use to
"Unrestricted" (or "Don't optimize"), keep the screen on, and run `termux-wake-lock` first (it keeps the processor awake while Termux works).

## 2. Install the tools

Install these one at a time (one failing package then does not stop the rest):

```sh
for p in python python-numpy python-pillow libc++ ffmpeg clang binutils git make cmake ninja pkg-config \
         aapt2 apksigner d8 openjdk-17 vulkan-headers shaderc glslang otf-noto-sans-cjk ttf-dejavu; do
  pkg install -y $p
done
pip install capstone unicorn
```

Do **not** install `vulkan-loader-android`: Termux has two Vulkan loaders that conflict, and the build uses the phone's own.

## 3. Get the game's code

```sh
cd ~
git clone https://github.com/smolderyguest/Super3DXPlus
cd Super3DXPlus
```

## 4. Build the data (about an hour)

Replace the path with where your game folder is (Termux sees the phone's storage as `/sdcard`). Keep the quotes.

```sh
termux-wake-lock
python build.py data "/sdcard/Download/WMMT3DX"
```

It prints each step as it goes. When it ends with `CONVERSION DONE` the data is in `~/Super3DXPlus/data`. If it stops with an error, the
message says which step; fix the cause and run the same command again, or run only that step:
`python tools/pipeline.py convert "/sdcard/Download/WMMT3DX" --only <step>` (`python tools/pipeline.py steps` lists them).

## 5. Build the app (a few minutes)

Export version (the English one):

```sh
mkdir -p .super3dx/android
python platform/android/build_apk.py --out /sdcard/Download/Super3DXPlus-Export-Android.apk \
  --pkg com.wmmt.port.export --label "Super3DX+ (Export)" --embed data --keystore .super3dx/android/debug.keystore
```

Japan version: use `--pkg com.wmmt.port --label "Super3DX+ (Japan)"` and a different `--out` name. Both versions can be installed together.

The first run downloads the Android platform file (63 MB; by running it you accept Google's Android SDK licence terms, the link is printed) and
compiles the game: the arcade physics takes two minutes, the rest about one. The result is one file of about 1 GB with the data inside.

## 6. Keep the signing key

The app is signed with a key made on the first build: `~/Super3DXPlus/.super3dx/android/debug.keystore`. Android only installs an update over an
app signed with **the same key**, so copy it somewhere safe:

```sh
cp .super3dx/android/debug.keystore /sdcard/Download/super3dx-debug.keystore
```

Updating later with a different key means uninstalling first, which deletes the game's saves.

## 7. Install it

Open the phone's file manager (Files), go to `Download` and tap `Super3DXPlus-Export-Android.apk`. Then:

1. If Android asks which app should open it, choose **Package installer** and **Just once**. Do **not** choose Termux: it offers itself for APK
   files, and it cannot install them.
2. The first time, Android warns that unknown apps are a risk (or asks you to allow "install unknown apps" for the file manager): allow it
   (**Continue**).
3. Tap **Install**, then **Open**.

The first start copies the game's data (a minute or two; the screen stays white or shows the intro movie meanwhile).

Once it is installed you can delete the APK, and the `data` folder and the game folder if you need the space (you need the data again only to
rebuild).

## Updating

```sh
cd ~/Super3DXPlus
git pull
```

If `CHANGELOG.md` says the data changed (a fix in the converters), run step 4 again, then step 5. If only the game changed, step 5 is enough.
Use the same signing key (step 6) so the new app installs over the old one and keeps your saves.

## Problems

- **`no font found` / `infopage.py: no CJK font found`:** `pkg install otf-noto-sans-cjk ttf-dejavu`, then run the failed step again.
- **The build just stops, or Termux closes:** Android killed it to save battery. See the note in step 1, then run the same command again: finished
  steps are kept (`python tools/pipeline.py convert ... --only <step>` for the rest).
- **`aapt2` / `d8` / `apksigner` not found:** step 2 did not finish for that package; run `pkg install <name>` and look at what it says.
- **"App not installed" when installing:** the phone already has this app signed with another key (for example one built on a PC). Uninstall it
  first (its saves go with it) or build with the same key.
- **The app closes at once:** run `python tools/pipeline.py steps` and check that step 4 finished with `CONVERSION DONE`; a partly built `data`
  folder gives a broken app. Open an issue with the output of `logcat` if you can get it, or the text of the error.
