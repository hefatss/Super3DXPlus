# Super3DX+

This is an accessibility-focused native port of *Wangan Midnight Maximum Tune 3DX+* built primarily for controllers, with a console-style frontend. It is based on the static recompilation work of [Re3DX+](https://github.com/Allannn-sudo). This branch builds the game for **Android** (64-bit ARM, Android 9+, Vulkan or GLES3 with `--novk`). It supports both the Japan and Export (English) versions of the game.

## Copyright disclaimer

This repository does **NOT** contain any copyrighted content from the original game or from Bandai Namco Entertainment, Inc. or its affiliates, and nothing generated from it. It contains only free, open-source (MIT) tools for building the conversion from **your own legally obtained game data**. See [Quick start](#quick-start).

*Wangan Midnight Maximum Tune* is a trademark of Bandai Namco Entertainment, Inc. This is an UNOFFICIAL, strictly NON-COMMERCIAL fan project and is not affiliated with them in any way, shape or form, written or implied.

**Where can I get the APK?**

You can't. Any pre-built binaries you see online are unauthorized distributions of copyrighted content, and are not affiliated with this project or its developers. Running unsigned executables from strangers is a bad idea anyway.

## Quick start

**You need your own copy of the game:** a folder with a file called `main` and a folder called `data` inside (from the Japan or Export version of 3DX+; `docs/BUILDS.md` lists the versions known to work). Nothing from the game is included or downloaded. You also need a PC with Python 3.9+ and a JDK 17+, about 15 GB of free disk space (plus about 3 GB for the Android SDK and NDK if you don't have them already), an internet connection for the first run, and an hour or more of waiting.

1. On this page, click the green **Code** button, then **Download ZIP**, and unzip it somewhere (not inside a protected folder like Program Files).
2. Double-click **`setup-android.bat`** (or run `./setup-android.sh` on Linux/macOS).
3. Follow the prompts. It installs what it needs (the Python packages, the Android SDK and NDK, the JDK), asks you to pick your game folder, converts the data, and builds one self-contained APK.
4. When it says it's done, it tells you where the APK is (`Super3DXPlus-Export-Android.apk` or `Super3DXPlus-Japan-Android.apk`). Copy that one file to a phone (Cable, cloud drive, ...), open it there, and allow "install unknown apps" when asked. If the phone is connected by USB with USB debugging on, the setup can install it for you.

The converted data (about 3 GB) goes inside the APK, so the app is self-contained: the first start copies it out, then plays. If you would rather build it on the device itself, with no PC, see `docs/ANDROID_ON_DEVICE.md`.

**Updating to a new version** (no full rebuild needed)

1. Get the new code: in a git checkout, `git pull` (the setup also offers to do it for you); otherwise download the new ZIP and unzip it **over** the old folder, replacing files when asked. Your finished APKs (`Super3DXPlus-*`) and the tools the setup downloaded (`.super3dx`) are not in the ZIP, so they stay.
2. Run **`setup-android.bat`** (or `./setup-android.sh`) again and give it the same game folder. It recognises your earlier build and tells you what it will do:
   - **Most updates (game code, menus, fixes):** it only rebuilds the APK, in a few minutes. Your converted data is not touched.
   - **When the data converters changed** (the changelog says so): it converts the data again (about an hour) and rebuilds the APK.
3. Install the new APK over the old one. Your saves stay, as long as you keep the signing key in `.super3dx/android/debug.keystore`: future updates from this PC install over the app only if it is signed with the same key.

**If something goes wrong:** the setup stops with a plain-language explanation of what probably happened and what to do, and saves a full log in `build/logs/` (named `setup-<date>.txt`). If you are stuck, open an issue and attach that file.

**To build both the Japan and Export versions,** run the setup again and pick the other game folder. `docs/BUILDS.md` lists the versions known to work, and `docs/INPUT.md` covers controls and remapping. To build the entire thing on an Android phone with no PC at all, in Termux, see `docs/ANDROID_ON_DEVICE.md`.

## Layout

```
build.py        build driver (Android; runs the data pipeline, then the APK build)
src/            the game (platform-neutral C)
platform/       app code: android/ (the NDK/toolchain build, MainActivity.java, the Vulkan-or-GLES3 layer)
tools/          data pipeline (Python): converters, generators, packers, per-build address tables
third_party/    stb, zlib, Khronos GLES2 headers (each with its own notice)
docs/           building, testing, versus, input, builds, architecture, on-device Android
```

## NOTE - Wheel / FFB support

Basic FFB/wheel support is implemented in `src/ffb.c` (the forces are made from the car's own sim). The SDL2 wheel backend that plays them on a PC wheel is **not** part of the Android build.

## What makes this "native"?

This is not an emulator wrapper. The arcade's car physics are statically recompiled from the game's own executable, and everything else (rendering, menus, sound, netcode) is a ground-up reconstruction. The game code runs on bare metal and "just works" without emulator setup, and most of the work goes into quality-of-life improvements for all sorts of control schemes.

## What's different from Re3DX+

Re3DX+ was touch-only, Japan-only, and basic in functionality. Super3DX+ adds:

- Controller support, with vibration and input filtering
- Support for both Export and Japan versions
- Menus and flow reworked for a non-arcade setting, as authentic as possible
- A rebuilt car save/load system, like a modern Banapass-style garage
- The original race UI restored, with touch controls optional
- Restored features: 10 Outrun music, story-mode visual upgrades (roll cages, exhaust, Gatchan's Celsior parts), body color customization, and more
- Many bug fixes, such as broken texture conversions and missing tuning parts

This only scratches the surface; see `CHANGELOG.md`.

## Status

Time attack, story mode, 10 Outrun, the garage, dress-up and card screens, and versus for up to four machines (`docs/VERSUS.md`) work.

- **Not implemented:** ghost VS battle, the post-race mileage screen, the 10 Outrun post-race rewards / progress screen.
- **Known bugs:** menu slide/select animations aren't smooth; story rival headlight flare positions are inaccurate (no position data seems to be stored for them); gamepad detection for vibration is a little finicky.
- **Limitations:** the Android build is tested on Android 9 and newer, 64-bit ARM. Devices without Vulkan run on GLES3 (`--novk`).
- **Cautions:** story mode hasn't been played all the way through. The Japan build is tested much less than Export, since I can't read Japanese; if the text is messed up, please submit an issue. The Android setup script is new: if it fails, please open an issue with its log (`build/logs/`).
- **Maybe later:** rollback netcode for online versus (big undertaking, noticeable performance cost).

## Licence and credits

MIT (`LICENSE`), keeping Re3DX+'s copyright. Game content is not covered and not included. See `NOTICE.md`.

- **Super3DX+**: developed by [smolderyguest](https://github.com/smolderyguest).
- **Re3DX+** by [Allannn-sudo](https://github.com/Allannn-sudo): the original conversion effort, including the static recompilation of the physics and the first rewrite of rendering, menus, HUD, sound and data converters (MIT).
