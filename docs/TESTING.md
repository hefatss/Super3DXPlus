# Testing

## Test hooks of the player

Set as environment variables (they do nothing unset, and silence the sound unless `MT3DX_AUDIO` is set):

| Variable | What it does |
|---|---|
| `MT3DX_LOG=1` | log to stderr as the player runs (frame lines, the versus' events) |
| `MT3DX_AUTOSTART=n` | press START each second from n seconds on |
| `MT3DX_GOTO=n` | after START go straight to menu screen n (the `MENU_` enum in `src/menu.h`; 36 is the versus' lobby) |
| `MT3DX_SCRIPT="screen:step;..."` | steps by screen: `@OK`, `@DOWN`, `@LEFT`..., or a tap `x,y`; a `+` repeats until the screen changes (`platform/sdl2/main_sdl.c`) |
| `MT3DX_SHOT=s,s` `MT3DX_SHOT_DIR=dir` | the frame as `shot_<s>.ppm` at those seconds |
| `MT3DX_AUTOQUIT=s` | leave after s seconds |
| `MT3DX_BLACK=n` | the PC player's BLACK LEVEL (0..10) for this run, whatever the option says |
| `MT3DX_SHAKE=n` | the CAMERA SHAKE (0..10) for this run, whatever the option says |
| `MT3DX_SRGB_KEEP=1` | (Windows) leave the driver's `GL_FRAMEBUFFER_SRGB` as it is, to compare pictures with and without the game switching it off |
| `--angle` (or `MT3DX_NOGL=1`) | (Windows) use the bundled ANGLE instead of the driver's OpenGL ES: the log says "window made through EGL". `MT3DX_NOGL=1` or `2` | (Windows) skip the first window attempt to test the fallback: 1 goes on to the bundled ANGLE, 2 also hides it to see the "no OpenGL ES" message box |
| `--native-gl` (or `MT3DX_NATIVE_GL=1`, or a `native-gl.txt` beside the exe) | (Windows) keep the driver's own OpenGL ES on an AMD card, which starts on ANGLE otherwise (a build with `WMMT_NO_AUTO_ANGLE=1` never switches) |
| `MT3DX_INTERP=0\|1` | force smooth motion off or on for this run (it is otherwise on for a screen that is not 60 Hz and in every versus race, unless GAME OPTIONS > GRAPHICS > SMOOTH MOTION is off) |
| `MT3DX_INTERP_LOG=n` | log n frames of a race from its 120th: the frame's length, the time it moved the picture by, the steps run, the blend, the drawn car's and camera's place and the eye-to-car distance |
| `MT3DX_FRAMELOG=n` | (PC player) every race frame's length and where it went (the app's frame, the swap, the hold), n frames from the 600th; `MT3DX_HITCH=<ms>` logs each frame that took longer |
| `MT3DX_GPUTIME=1` | twice a second in a race a glFinish and how long the GPU took to catch up, with the speed and the radial blur; `MT3DX_FINISH=1` waits for the swap (it made things worse) |
| `MT3DX_LEAVE_AFTER=n` | leave a race for the menus after n seconds, as the pause menu's YES: several races in one run with `MT3DX_SCRIPT="0:@OK+;0:@OK+;..."` |
| `MT3DX_SHOT_BURST=<s>:<n>` | save n frames in a row from second s as `burst_<k>.ppm` (how motion looks frame to frame) |
| `MT3DX_CAMTOGGLE=<n>` | presses the camera key at the race's frame n (with `MT3DX_INTERP_LOG` to see the view change frame by frame) |
| `MT3DX_FAKE_AMD=1` | (Windows) act as if the card were an AMD one: the player starts again on ANGLE, and the new log (the old one is `log-prev.txt`) says "ANGLE chosen automatically" |
| `MT3DX_NOREADY=1` | a `MT3DX_VSAUTO=join` player does not click READY |
| `MT3DX_CALLTEST=km` | show the "Remaining km" callout (3 or 5) at a race's start and again every 4 s |
| `MT3DX_AUTODRIVE=1` `MT3DX_AUTODRIVE_KMH=n` | hold the accelerator in a race (below n km/h) |
| `WMMT_PILOT=1` `WMMT_PILOT_BIAS=x` `WMMT_PILOT_LOG=1` | the arcade's own pilot steers (with `MT3DX_AUTODRIVE`); BIAS adds a steady steering offset (about -0.05 or 0.05) to hug a lane and clear the centre dividers; LOG prints the pilot's numbers every 2 s |
| Android: `test.env` in the save folder | `KEY=VALUE` lines set as environment variables at start (`MT3DX_AUTODRIVE`, `MT3DX_AUTODRIVE_KMH`, `WMMT_PILOT*`); the menus are driven with `adb shell input gamepad keyevent`. Delete the file after a test: it makes the car drive itself |
| `MT3DX_VSAUTO=host[:n]\|join` | host a LAN game and start it once n machines are in, or join the first found |
| `MT3DX_VSKM=n` | a versus race of n km (short, to test the flow) |
| `MT3DX_NETLOG=1`, `MT3DX_NET_LAG=ms` | the netcode's measurements; an artificial lag on received packets |

## Versus on one PC

`tools/test/vs2.ps1` starts two or three players on this PC (a host and clients that drive themselves), logs and screenshots in
`build/test-runs/<name>-<role>`:

```powershell
powershell -File tools\test\vs2.ps1 -Players 2 -Km 1 -Quit 150 -Shots '60,90'
```

Per-player variables go in `-Host_` / `-Client` / `-Client2` (`'MT3DX_AUTODRIVE_KMH=120;MT3DX_AUTOQUIT=60'` to have one slow down or leave).
`-Players 1 -Expect 2` waits for a second machine (another PC or a Pi) to join. Do not rebuild the player while a test runs (the file is in use).

## The C tests (`test/`)

Desktop programs that exercise the physics and the story's start without a window: `physics_sweep.c` (every car on three courses),
`story_start_test.c`, `story_rival_race.c`, `rival_test.c`, `ten_test.c`, `vs_test.c`, and `hudshot.c` / `menushot.c` (screenshots through a stub GL).
They are built by hand against `src/` (see the head of each file) and need the generated sources.
