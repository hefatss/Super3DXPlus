# Changelog

What Super3DX+ adds or changes compared with [Re3DX+](NOTICE.md). Longer explanations are in `docs/`.

## Input and menus

- Controller-first menus: full pad and keyboard control on every screen, with rebindable keys, pad buttons and analog axes
- Controller input filtering, deadzone, response and button-steering speed tuning
- Controller vibration (wall hits, expansion joints, tyre slip) with adjustable strength
- Steering wheel support (untested on hardware): wheel axes and buttons are bindable, and force feedback (steering pull, centering, damping, road effects) through SDL haptic, with a WHEEL list in KEY BINDINGS for the device and gains
- Rebuilt main menu flow: PLAY / SETTINGS / EXIT, then car selection, then Solo / Versus / Car settings
- Stock, Full Tune and Hell's Tuning starting points (Hell's Tuning begins the story on Ch. 101)
- Restored AT / MT transmission select screen
- Special color selection for cars
- Restored suspension lowering for tuned cars
- Volume mixer
- Option to skip the intro cutscenes, and restored missing intro cutscenes
- Information screen

## Game modes

- Fixed versus play over LAN or online, with restored route choice at junctions, last-stretch calls, placings and disconnect handling
- Music selection by the host and by each race's winner in versus
- Time attack rankings with car class and level, kept across sessions
- Restored 10 Outrun's special OST and in-game functionality
- Music track and time zone selection in 10 Outrun
- 10 Outrun rival line animation and rival marker
- Restored junction-choice walls and the "Choose a direction" animation

## Graphics and HUD

- Fixed many missing animations (rival markers, etc.)
- Restored original reflection handling, applied to rival cars
- Tire smoke, headlight flares and the missing shift light
- Fixed the reflective buildings of the morning Tokyo maps (New Belt Line, C1, Yokohane) showing checkered, see-through facades with things moving behind them: the reflection program added the gloss map's alpha to the base alpha, which went above 1 and, blended into a floating-point target, subtracted what was behind. The alpha is clamped; both map vertex shaders also declare the vertex position invariant, so surfaces at nearly the same depth keep their order on every GPU
- Fixed a solid white wall at the exit of the C1 tunnels (about 10.8 km into C1 inward): the data build dropped the alpha of the tunnel mouths' faint white light mask (nondxt_sk_nk_lighteff01, alpha 17 at most) as if it were a sky dome, so it drew opaque. Rebuild the Tokyo packs (python tools/pipeline.py convert <arcade data> --only tokyo-day) to get the fix
- The whole game can be built on an Android device with no PC (data and app, in Termux): `docs/ANDROID_ON_DEVICE.md`. `build_apk.py` uses Termux's own tools there; the converters find Termux's font packages; the Khronos EGL headers are bundled
- Windows: a racing wheel on its maker's driver (a Fanatec) says "forces not available" and its axes and buttons cannot be bound: SDL's RawInput and Windows.Gaming.Input drivers took it first and have no forces. The wheel now comes through DirectInput; the reason SDL gives for a refused force device is shown in the settings (issue 2)
- Exhaust flames follow the tuned exhaust's pipes (the Savanna RX-7's single tuned pipe, and other cars)
- Arcade-style race HUD alongside the touch HUD, with a debug options menu
- Texture sharpness and graphics options (HD textures, wet road, paint reflections, anti-aliasing)
- Minimaps for Osaka, Hakone, Nagoya and Fukuoka

## Platforms and builds

- Export (English) build support and an English language layer
- Updated Android player (64-bit ARM, Android 9+, Vulkan or GLES3 with `--novk`)
- `build.py` and `tools/pipeline.py` build driver: Python 3.9+ and a JDK, no WSL
- Guided setup (`setup-android.bat` / `setup-android.sh`): installs what it needs, finds and checks your game folder, converts the data, builds the APK, and explains failures in plain language
- Updates only redo what changed: a new version usually rebuilds just the app in minutes, keeping your converted data and saves
- `setup-android`: builds one self-contained Android app (any OS) and can install it over USB
