# Menu input: keyboard, pad, mouse

The arcade's menus were driven by its controls: the **steering** (left, right), the **gas** (decide), the **brake** (back), the
**shifter** (up, down, and characters of a name) and the **view button** (pages, the music). The port keeps the touch screen
and the mouse working (the menus began as a touch interface), and puts those controls beside them. A keyboard and a pad press the
same seven actions (`src/menu.h`):

| Action | Arcade control | Keyboard | Pad |
|---|---|---|---|
| `MENU_ACT_LEFT`, `RIGHT` | steering | Left, Right | d-pad, left stick |
| `MENU_ACT_UP`, `DOWN` | shifter | Up, Down | d-pad, left stick |
| `MENU_ACT_OK` | gas | Enter, Space, Z | A, right trigger, Start |
| `MENU_ACT_BACK` | brake | Backspace, X | B, left trigger |
| `MENU_ACT_VIEW` | view button | Tab, C | Y, Back |

In a race the same keys drive the car (`app_key`): Left/Right steer, Up/Down accelerate and brake (pad: A / right trigger gas,
B / left trigger brake, the stick or d-pad steer), Z/X (pad bumpers) shift down and up, C (Y) the camera, R (X) reset, T the
transmission, P (Start) pause. Esc quits. Which meaning a control has is decided by `app_racing()`; the table is `CTL` in
`platform/sdl2/main_sdl.c`, which also keeps the d-pad auto-repeat and the stick and trigger thresholds.

## How it is wired

`app_menu_action(act)` (`app.c`) hands the action to `menu_action` (`menu.c`) wherever a menu, the pause menu or a result is up.
Screens read **intents**, not coordinates. Each `step()` builds a `menu_in` (`menu.h`) from what was pressed:

| Field | Meaning | Controls | Pointer (touch, mouse) |
|---|---|---|---|
| `dx`, `dy` | left / right, up / down | steering, shifter | a tap beside the selection, on an arrow |
| `ok` | decide what is selected | gas | a tap on the selected item |
| `select` | an item chosen outright | (dx / dy walk to it) | a tap on it |
| `tab` | a chip, tab, page or value chosen outright | (dx / dy walk to it) | a tap on it |
| `back`, `view` | brake, view button | brake, view | the bottom bar's back key; a screen's own key (TRAFFIC) |
| `aux` | a key of the screen's own (METER / BGM, the save screen's "i") | none yet | a tap on it |
| `any` | any press at all (screens that wait to be dismissed) | any | any tap |

`menu_pointer` (`menu.c`, above `step()`) is the only place that knows where things are on the screen: it turns a tap into those
fields for the screen that is up (a tap on an unselected item is `select`, on the selected item `ok`, and so on). **Every
screen reads `menu_in` only**, so each works with no pointer at all, and the pointer is an adapter on top. The few leftovers are
not input to a screen: the pause menu keeps its own pointer and control presses (`menu_pause`: the race is still running behind
it) and makes a `menu_in` of them the same way, and the result and ending screens wait for `m->tap`, which is "any press".

The pointer's hit tests (`tap_dir`, `st_tap`, `on_card`, `have_on`, `tg_*_hit`, ...) are kept as they were; they are only called
from the adapter.

The test player drives the same actions by name: `MT3DX_SCRIPT="25:@LEFT;25:@OK+;..."` (`@LEFT @RIGHT @UP @DOWN @OK @BACK @VIEW @PAUSE`, @PAUSE the race's pause key;
a step waits for its screen, `+` repeats it until the screen changes, screen `0` is any menu screen until the race).

## What the controls do, screen by screen

| Screen | Left / right | Up / down | OK | View | Back |
|---|---|---|---|---|---|
| Attract, results, bonus, level-up, stars, effects | | | start / leave | | |
| Save data | NEW GAME / LOAD GAME / DELETE | the focused save | the focused save | the OPTION screen | the title |
| Name | the cursor on the grid, or the tabs | the cursor's row; a fourth row is the page tabs and END | puts the character in, takes the tab or END | next page | erases, then the saves |
| Maker | the selection | | decide | the OPTION screen | back |
| Car, colour | the selection | | decide | | back |
| Car setting | the power / handling balance | the shifter: MT / AT | on to the next screen | STOCK / FULL TUNE | back |
| Pre-race tune check | the balance | the shifter | DONE | | the screen before |
| Card settings (METER / BGM) | the choice | the row | DONE | | back (to the card check) |
| Card check | | up: car customization (dress-up) | go on | METER / BGM | back |
| Save-this-car | YES / NO | | decide | | back |
| Game mode, mode, course, story level / rival, 10 out run | the selection | story level: the chapters page | decide | course: traffic on / off; story level: UNLOCK ALL CHAPTERS | back |
| Story tuning | power / handling | | decide | | |
| Time zone and music | morning / night | | decide | the next music | back |
| Garage | the car shown | | take it | the bin (twice) | back |
| Dress-up (kind, maker, aero, part, wheel, vinyl) | the place | | decide | wheel, vinyl: the next colour | back |
| Title (YES / NO) | the card | | decide | | |
| Versus lobby | the button | the button (the keypad: a row) | press it | | back |
| Versus area vote | the card | | vote | | |
| OPTION | the row's value (the switches: left ON, right OFF) | the row | flips a switch; TOUCH LAYOUT | | back |
| Key layout | moves the group | moves the group | done | next group | done |
| Pause menu | YES / NO | | decide | | resume |

The focus the controls draw is the arcade's own where there is one: its cursor ring on the name grid, the lit chip, the selected
disc or card. The port's own screens draw a bright band (the save screen), a marker (the card settings, ">") or a lit key (the
versus lobby, OPTION's TOUCH LAYOUT key); the OPTION screen brightens its row.

## Not covered

The key layout's size and RESET ALL keys (touch keys mean nothing on a cabinet), and dragging a group (the pointer's drag); the
OPTION screen itself is Android's, and is to be replaced by a launcher or settings menu. Nothing else is mouse-only. The arcade's
control glyphs on the bottom bar come once the flow is settled.

## Testing without hands

- `MT3DX_SCRIPT` presses actions by name, by screen (above), or taps `x,y` (the touch screen's 1024 x 768): the whole front end from
  the save screen to a race was driven by actions on both builds, and by taps through the save, car, setting and card screens.
- `MT3DX_VPAD="<s>:<button>;..."` attaches an SDL virtual game controller and presses A B X Y START BACK LB RB UP DOWN LEFT RIGHT
  and the triggers RT, LT at those seconds, through the same code a real pad uses. The d-pad, A and Y drove the save, name and
  car screens that way. A real pad has not been tried yet (SDL's game controller layer maps most of them).

## The window's shape

The game's layout is 4:3 (1024 x 768). The SDL player draws into a framebuffer the size of the window and uses the app's own
wide-screen support (`app_resize` -> `app_wide`, the same as Android's): a window wider than 4:3 shows the layout in its middle with
the sides filled, the race's keys on the real edges. The window can be resized while playing (the player tells the app between
frames); one narrower than 4:3 is made 4:3 (windowed), and in fullscreen on a tall display the 4:3 picture sits at the bottom.

The mouse is mapped to the layout's px the way a touch is (x from `-edge` to `1024 + edge`, `to_layout_x` in `main_sdl.c`). Tested at
1024x768, 1280x720, 1720x720, 1600x500, 640x480, 800x600 (800x640 made 4:3): the menus, the race and the HUD at each, and
`MT3DX_CLICK="<s>:<x>,<y>;..."` / `MT3DX_RESIZE="<s>:<w>x<h>;..."` check the mapping and a live resize (window pixels, seconds).

## The race's look (HUD style)

`hud_set_style` (`hud.h`) picks how the race screen is laid out:

- **`HUD_STYLE_ARCADE`** (the SDL player's default; `--hud arcade`): the cabinet's screen as it was. Each console sits at its own place on
  the arcade's 640x480 screen at the arcade's size (1.6x on the 1024x768 layout): till goal, TOTAL TIME, the route map with TIME, the
  shift gate, the meters at (560, 228), the plate flush bottom right, the rival's box and ADVANTAGE top right (with the challenger
  pill under it, as in a story race on the cabinet). No touch keys, no fps counter, no AT/MT key, and a story race shows TOTAL TIME only
  (no section time). Checked against a recording of the cabinet (`media/`) in a time attack, a story race and the 10 out run, and in
  wide windows (what is on the left sits on the window's left edge, what is on the right on its right).
- **`HUD_STYLE_TOUCH`** (the default of the library, and so of Android and iOS; the SDL player's `--hud touch`): the iPad's race screen as it
  was kept, smaller consoles around the touch keys. It is unchanged, for the mobile build.

Set it before `app_wide` / `app_resize` (`hud_wide` lays the consoles out); the lifts `hud_lift` and `hud_lift_parts` are the touch style's
and do nothing in the arcade's.

### The scanline bars (the Pi test)

The nameplate's and the route map's backgrounds carry the arcade's scanline bars, a row of the texture to a row of the 640x480 screen.
The arcade style at a 640x480 window (and 720x480) lays every sprite on whole pixels, so the bars come out as on the cabinet; scaled
to any other size they smear (a 1024x768 window measures 64 % of the rows alternating, against 100 % at 640x480).
A time attack run at those sizes, shot and measured against the cabinet's own frame shows it.

## Key bindings

The keyboard and pad no longer have a fixed table: `src/bind.c` holds what each input does, in two lists, the menus' (LEFT, RIGHT, UP, DOWN,
OK (gas), BACK (brake), VIEW) and the race's (steer left and right, gas, brake, shift up and down, camera, reset car, AT / MT, pause). Each
action has two keyboard slots and three pad slots. The platform only reports which key or pad input went down or up (`app_input`: the
SDL keycode, or a `BP_*` code for a pad button, a stick direction or a trigger); the bindings turn it into the action. OPTION > KEY
BINDINGS edits them: up and down the rows, left and right the cells, OK waits for an input for the cell (Esc cancels, Delete empties
it), the view button switches the list, the last row puts the list's defaults back, back leaves. An input does one action in a list
(binding it moves it from where it was), and a menu action keeps at least one input. Saved with the options (`<save dir>/bindings`).
Axes are not bindable yet (the stick's four directions and the triggers are, as buttons).

Defaults (what the port was tested with): menus: arrows, D-pad and left stick move; Enter, Space, A, Start and RT are OK; Backspace, X, B and
LT are back; Tab, C, Y and Back are the view button. Race: Left / Right steer; Up gas, Down brake (A / RT and B / LT); X and RB shift up,
Z and LB shift down (the D-pad and stick up and down too); C, Tab, Y and Back the camera; R and pad X reset; T AT / MT; P and Start pause.
Esc always quits (it cancels while the binding screen waits). Z is no longer an OK in the menus (it shifts down in a race).

The OPTION screen shows the touch screen's own settings (steering, gyro sensitivity, slider pedals, TOUCH LAYOUT) only with the touch HUD
(`--hud touch`, `hud_style`); with the arcade HUD it has KEY BINDINGS and the graphics. The row the controls are on has an orange bar at its
left and a line under it (the keys are lit).

TOUCH LAYOUT: drag a group of keys or a gauge to move it (a finger, or the d-pad moves the chosen one 8 px; the view key picks the next), the - and + buttons size it (keys 60-160%, gauges 50-200%). It is saved in the options file (`keys` for the groups, `hudel` for the seven gauges). On a PC, `--hud touch` shows the same screen (`MT3DX_GOTO=43`).

Test hooks: `MT3DX_KEYS="<s>:<key name>;..."` presses and releases that keyboard key (SDL names: Return, Left, Delete, Escape, a) at
those seconds, through the same path as a real key.

## Analog axes and the input readout

A pad's axes are bindable now. `src/bind.c` keeps, for the three analog actions (STEERING, GAS, BRAKE), the axis that gives it (the pad's six: left
and right stick X and Y, the triggers), whether it is inverted, and a dead zone; the defaults are the left stick's X, the right trigger
and the left trigger, dead zones 10 / 5 / 5 %. The platform sends every axis motion (`app_axis`, -1..1; triggers 0..1) and the bindings
turn them into the car's steering (analog: no ramping), gas and brake (the digital inputs, keys and buttons, still work beside them and
a pressed one wins). OPTION > KEY BINDINGS has a third list, AXES: OK on the axis cell waits for a stick pushed or a trigger pulled
(Esc cancels, Delete takes the axis away), OK on INVERT flips it, OK on DEAD ZONE steps it, and the last column shows what the action gives
at the moment. The stick's and the triggers' digital directions are still the menus' (up, down, left, right, OK, back); the race's defaults
no longer use them (they are axes there).

**The readout** (temporary, for setting the axes up): over the race, bars for STEER (from the middle: orange right, blue left), GAS and BRAKE
as the car is given them, with their numbers, and the six raw axes in a line above. The touch layout's old bar over the steering keys, for
every HUD style, and for the pedals too. It is switched in OPTION > HUD OPTIONS (INPUT READOUT, on by default for now) or with F3 in the SDL player, and saved with the options.

Test hook: `MT3DX_VPAD="<s>:AX<n>=<v>"` sets the virtual pad's axis n (SDL's order: 0 left X, 1 left Y, 2 right X, 3 right Y, 4 left
trigger, 5 right trigger) to v (-1..1) and holds it (SDL scales a trigger's raw -1..1 to 0..1: a 0.4 reads 0.7).

## HUD options

OPTION > HUD OPTIONS (next to KEY BINDINGS, under CONTROLS) has three switches, saved with the options (a `hud <readout> <fps>` line of the
options file; the back mirror stays on its old line): **INPUT READOUT** (the steering, gas and brake bars over the race), **FPS COUNTER** (the
mobile HUD's frame rate counter in the arcade HUD, top middle; the touch HUD always shows its own) and **BACK MIRROR**, which moved here from
the graphics list. Left is ON, right OFF, OK flips, back leaves.

## Button steering speed

KEY BINDINGS > RACE, under STEER LEFT and RIGHT: **BUTTON STEERING SPEED**, 1..10 (a `btnsteer` line of the options file). It only affects steering
from buttons (keys, the D-pad): the push builds up at 0.6 x the value and releases at 1.6 x it, in full locks a second, before the steering filter
(an analog stick or the tilt is untouched). 10, the default, goes straight to full lock; lower is gentler. Left and right change it, OK steps up.

## Vibration

The pad's two rumble motors follow the player's car in a running race (nothing in the menus, the countdown or the pause):

- **Wall hits**: a heavy pulse on both motors when the car hits a wall (the arcade's own camera-shake request together with the hit's change of
  velocity decide its strength), and a lighter buzz for as long as the arcade throws scrape sparks.
- **Expansion joints**: a short, sharp tap on the light motor, twice a joint (the front axle, then the rear), harder with speed. The arcade
  asks the same shake for its road strips (`SAreaData::CheckGap`, `CheckZebra`, `CheckGuardRail`, in `CMachine::Set_SE_Effect_Reactive`).
- **Tyre slip**: a constant rumble on the heavy motor that grows with the body's slip angle (from about 3 degrees, full at 25) and with speed.

GAME OPTIONS > VIBRATION has OVERALL, WALL HITS, EXPANSION JOINTS and TIRE SLIP, each in tenths (the defaults: overall 80 %, the slip 50 %, the others 100 %); they are saved as
the `rumble4` line of `option`. The code is `src/haptic.c`; `MT3DX_FXLOG=1` logs its pulses.

## Steering wheels and force feedback

**Untested on real hardware** (the author has no wheel): the code is written against SDL's documented API and checked with a virtual wheel
(`MT3DX_VWHEEL`) for the device list, raw axes, buttons and saved settings, but no force has been played on a motor yet. Reports from wheel
owners are what it needs; see "Testing a wheel" below.

**The arcade's own protocol is not used.** The cabinet's wheel is driven by its I/O board, which is not part of what was recompiled, so
there is nothing to pass through. The forces are made from the player's car (`src/ffb.c`) and played through SDL's haptic API
(`platform/sdl2/wheel.c`), which covers wheels that have a force feedback driver (DirectInput on Windows, evdev on Linux, so also the Pi).
XInput-only devices and Android are not covered (Android keeps its pad rumble).

**Using one.** Plug the wheel in before starting, then SETTINGS > CONTROLS > KEY BINDINGS > **WHEEL** (the fourth list; View switches lists):

- **DEVICE**: left and right choose among the joysticks found (plain pads are left out; a device is listed even without force feedback,
  it then only steers). The line under the list says what the device reports: its axes and buttons and which force types it can play.
  The choice is saved by name, so it comes back when the wheel is plugged in again.
- **FORCE FEEDBACK** (on / off), **INVERT FORCE** (switch it if the wheel pulls away from straight ahead; the direction convention of
  drivers differs and could not be checked), **OVERALL** (the device's gain), **STEERING FORCE**, **CENTERING**, **DAMPING**, **ROAD EFFECTS**
  (tenths, 0-100 %), **STEERING RESPONSE** and **MINIMUM FORCE** (both 0 by default; below), **TEST** (a sweep from side to side and a shake, 2.5 s) and RESET TO DEFAULT.
- **A wheel that feels almost nothing in a race although TEST is strong** (the Thrustmaster T300RS): TEST pushes hard (85 % of the motor), but the steering
  force in a race is the tyres' pull, which on a wide road is small (0.3 g is about a tenth of the motor at the default settings), and some drivers ignore forces
  that faint. Raise STEERING FORCE and OVERALL, then **STEERING RESPONSE** (makes small pulls larger: force to the power 1 - 0.05 n, so 100 % is the 0.5th
  power) and **MINIMUM FORCE** (any pull gets at least n x 2 % of the motor). Start with STEERING RESPONSE 60 % and MINIMUM FORCE 30 %. Saved as the `curve` line of `ffb`.
- Saved as `<save dir>/ffb`. Defaults are mild (overall 60 %).

**Steering and pedals** come in as the wheel's raw joystick axes (the chosen wheel is read as a joystick, not as a game controller, because
SDL has no layout for most wheels). They are bindable on the **AXES** list as WHEEL AXIS 1-8 (the first eight axes the device reports): OK
on the AXIS cell, then turn the wheel or press the pedal (a pedal is taken when it moves far from where it rests). Use INVERT if a pedal
reads the wrong way round. A wheel's pedal is read over its whole throw (rest to floor). Wheel buttons are bindable on the MENUS and RACE
lists as WHEEL BTN 1-32 (PAD cells); the first hat is the D-pad. Set the wheel's rotation (e.g. 900 degrees) in its driver, and the
steering response with the AXES list's dead zone and SENS. (and the steering filter).

**The forces** (`ffb_step`), from the car's sim each arcade frame, all fading to nothing below walking speed:

- steering force: against the car's lateral g (the pull towards straight ahead), lighter as the front tyres pass their grip, plus a pull
  towards the way the car is going in a slide (counter-steer);
- centering spring and damping, growing with speed;
- road effects: the pad vibration's pulses (expansion joints, wall hits and scrapes, tyre slip; `haptic.c`) as one vibration of the wheel.
  The VIBRATION settings (controller rumble) scale these too.

All of it is a first guess at what feels right. The numbers to tune are at the top of `ffb_step`.

**Testing a wheel.** `MT3DX_LOG=1` prints the wheel axes and buttons as they arrive, and the player prints one `wheel:` line with what it
opened and which force types it created (stderr). If the wheel is listed but the line says no force feedback, the driver or OS permissions
are the cause (on Linux the `/dev/input/event*` node must be writable by the player's user). Useful in a report: the wheel's model, OS, the
`wheel:` line, which forces were felt, and whether INVERT FORCE was needed.

Each time a wheel is opened the player also writes **`wheel-diag.txt` in the save folder**: SDL's version and joystick driver settings, every
joystick with its vendor and product ids, the force feedback devices SDL lists (on Windows DirectInput's: none listed means the wheel's driver
does not offer forces to DirectInput, so check its own software for a DirectInput or compatibility setting), and what each way of opening the
wheel's forces said. If SDL's own link from the joystick to its force device fails, the player tries the force device with the same name
before giving up. When forces are not available the settings screen says so and names that file; attach it to a report.

**Forces that stop after a while** (some drivers drop a running effect, the Thrustmaster T300RS did): twice a second the player asks the wheel whether
each force still plays. One that was playing and has stopped is started again; if that happens three times in ten seconds, or the wheel refuses a
force update, the wheel is opened again by itself, as turning the device off and on in the settings does (at most five times a minute, then it is
left alone). Each of these is a line with the time in `wheel-diag.txt`, so a report shows what the driver did and when.

## Window mode (the PC player)

GAME OPTIONS > GRAPHICS > WINDOW MODE switches between a window and borderless fullscreen; F11 or Alt+Enter does the same at any time. The choice is
the `window` line of the `option` file, so the next start opens the same way; `--fullscreen` on the command line starts fullscreen whatever
was saved. The row is not shown on the Raspberry Pi (it always runs fullscreen) or in a CRT run.

**Gears list and other devices.** KEY BINDINGS > GEARS has SHIFT UP, SHIFT DOWN and GEAR 1 to GEAR 6 (an H shifter's positions, bound to any key, pad or wheel button). A gear press puts the lever in that gear at once; shift up and down step from where it is, so both can be bound together. Releasing an H position does nothing (no neutral). Only the manual gearbox uses them. Any game device that is not a game controller (a separate shifter, button box, pedals) is read as well as the wheel chosen for forces: the chosen wheel's inputs are WHEEL BTN n / WHEEL AXIS n, the others DEV2, DEV3, DEV4 (BTN 1-64, AXIS 1-12 each), numbered in the order of the device list.

## Headlight flash

In a versus race the interrupt switch's second job: HEADLIGHT FLASH (RACE list, L and the pad's X by default) flashes your headlights three times (off and on, 0.2 s a blink); holding it does not repeat, the next three need another press. The other machines see it too. It does nothing outside a versus race.
