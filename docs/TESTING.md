# Testing

## Test hooks of the player

Set as environment variables (they do nothing unset, and silence the sound unless `MT3DX_AUDIO` is set):

| Variable | What it does |
|---|---|
| `MT3DX_LOG=1` | log to stderr as the player runs (frame lines, the versus' events) |
| `MT3DX_AUTOSTART=n` | press START each second from n seconds on |
| `MT3DX_GOTO=n` | after START go straight to menu screen n (the `MENU_` enum in `src/menu.h`; 36 is the versus' lobby) |
| `MT3DX_SHOT=s,s` `MT3DX_SHOT_DIR=dir` | the frame as `shot_<s>.ppm` at those seconds |
| `MT3DX_AUTOQUIT=s` | leave after s seconds |
| `MT3DX_BLACK=n` | the PC player's BLACK LEVEL (0..10) for this run, whatever the option says |
| `MT3DX_FLASH=1` | a versus race with the headlight flash switch held from the start |
| `MT3DX_CALLTEST=km` | show the "Remaining km" callout (3 or 5) at a race's start and again every 4 s |
| `MT3DX_AUTODRIVE=1` `MT3DX_AUTODRIVE_KMH=n` | hold the accelerator in a race (below n km/h) |
| `WMMT_PILOT=1` `WMMT_PILOT_BIAS=x` `WMMT_PILOT_LOG=1` | the arcade's own pilot steers (with `MT3DX_AUTODRIVE`); BIAS adds a steady steering offset (about -0.05 or 0.05) to hug a lane and clear the centre dividers; LOG prints the pilot's numbers every 2 s |
| Android: `test.env` in the save folder | `KEY=VALUE` lines set as environment variables at start (`MT3DX_AUTODRIVE`, `MT3DX_AUTODRIVE_KMH`, `WMMT_PILOT*`); the menus are driven with `adb shell input gamepad keyevent`. Delete the file after a test: it makes the car drive itself |
| `MT3DX_VSAUTO=host[:n]\|join` | host a LAN game and start it once n machines are in, or join the first found |
| `MT3DX_VSKM=n` | a versus race of n km (short, to test the flow) |
| `MT3DX_NETLOG=1`, `MT3DX_NET_LAG=ms` | the netcode's measurements; an artificial lag on received packets |

`test.env` is how the hooks above are reached on the Android build: it sits in the app's save
folder (`/sdcard/Android/data/<pkg>/files/save`), one `KEY=VALUE` per line, and is read at start.
