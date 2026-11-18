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
- The turbo's whine was far too loud against the engine ("a jet engine", issue 6): measured on a recording of the arcade's own tuned Evo IX it sits about 19 dB under the engine, here it sat 9 to 11 dB under. Its level is now the arcade's; the whine's pitch was already right
- Volume mixer: ENGINE and TURBO rows (0 to 200%, 100% = the arcade's balance) to set the engine against the turbo's whine
- Fixed the course select's run light (the glow riding the route on the 3D Tokyo map) drawing as an offset white box when the data is built on Windows (issue 8): the converter compared path names with the wrong slashes there, so the glow's texture was left out. Run `python build.py gensrc` again (it rebuilds `data/minicourse.wpk`), then the setup or `build.py exe`
- Fixed tuned cars missing the exhaust, roll cage, lowering and other tuned parts in the colours that belong to another trim (the Lancer Evos' RS whites, the Impreza GC8's RA colours, the R34 Nür, the 964's RS and others): the tune table only knew each car's first colour's model, the others were left at "never". Every colour's own model is used now (`python build.py gensrc`, then rebuild the player)
- Wheel force feedback that stopped after a minute or two (a Thrustmaster T300RS, until the device was switched off and on in the settings): the player now notices a force the driver dropped or a refused update, starts it again or opens the wheel again by itself, and logs it in `wheel-diag.txt`
- Setup failing with "CERTIFICATE_VERIFY_FAILED" while downloading its tools (issue 10; an antivirus or proxy that inspects HTTPS): when Python's own download fails, setup tries Windows' `curl` and PowerShell, which use the system's certificates; a tool file saved by hand in the `.super3dx` folder (under the name setup prints, or the SDL2 zip under its download-page name) is used instead of downloading
- Android: a shift (or any press) on a D-pad that is a hat axis, as the AYN Thor's, was sometimes lost: Android hands a pad's motion events over in batches with only the latest value on top, so a quick press and release inside one batch looked like nothing. The earlier values in each batch are now read in order
- Wheel force keepalive (issue 2, a T300RS whose force still went away with nothing wrong reported): every half second the steering force is sent again and the forces are started again, and a steering force of exactly nothing is sent as the smallest force; changes to and from nothing are logged in `wheel-diag.txt`
- Wheel settings STEERING RESPONSE and MINIMUM FORCE (off by default): for a wheel that gives strong forces in TEST but almost none in a race (a Thrustmaster T300RS), they make the small cornering pulls larger and lift forces the driver would ignore; see `docs/INPUT.md`
- The Nissan R35 no longer offers body parts it does not have in the arcade (issue 9): in its dress-up the aero set, bonnet, wing, mirror and sticker places and the special colour select cannot be opened (the wheels and the neon stay), none is won, and ones an earlier save holds are not drawn, so there is no black hole where a bonnet was (the save itself is not changed)
- Exhaust flames follow the tuned exhaust's pipes (the Savanna RX-7's single tuned pipe, and other cars)
- Arcade-style race HUD alongside the touch HUD, with a debug options menu
- Texture sharpness and graphics options (HD textures, wet road, paint reflections, anti-aliasing)
- Minimaps for Osaka, Hakone, Nagoya and Fukuoka

## Platforms and builds

- Export (English) build support and an English language layer
- Raspberry Pi 5 support with composite CRT output (and optional HDMI)
- Windows, Linux and updated Android players
- Windowed / fullscreen mode (F11 or Alt+Enter), remembered between starts
- `build.py` and `tools/pipeline.py` build driver: Python 3.9+ and a C compiler, no WSL
- Guided setup (`setup.bat` / `setup.sh`): installs what it needs, finds and checks your game folder, builds, and explains failures in plain language
- Updates only redo what changed: a new version usually rebuilds just the game program in minutes, keeping your data and saves
- `setup-pi`: builds and installs the game on a Raspberry Pi over the network, with the boot service and composite/HDMI output
- `setup-android`: builds one self-contained Android app (any OS) and can install it over USB
