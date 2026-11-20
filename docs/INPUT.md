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
the Android input code (`platform/android/main_android.c`), which also keeps the d-pad auto-repeat and the stick and trigger thresholds.

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
| OPTION | the row's value (the switches: left ON, right OFF) | the row | flips a switch; KEY LAYOUT | | back |
| Key layout | moves the group | moves the group | done | next group | done |
| Pause menu | YES / NO | | decide | | resume |

The focus the controls draw is the arcade's own where there is one: its cursor ring on the name grid, the lit chip, the selected
disc or card. The port's own screens draw a bright band (the save screen), a marker (the card settings, ">") or a lit key (the
versus lobby, OPTION's KEY LAYOUT key); the OPTION screen brightens its row.

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

The game's layout is 4:3 (1024 x 768). A given device's screen is mapped onto it through the app's own wide-screen support
(`app_resize` -> `app_wide`): a wide display shows the layout in its middle with the sides filled, the race's keys on the real
edges.

Taps and touches are mapped to the layout's px the way the menus expect (x from `-edge` to `1024 + edge`, `to_layout_x`).

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

### The scanline bars (the 640x480 test)

The nameplate's and the route map's backgrounds carry the arcade's scanline bars, a row of the texture to a row of the 640x480 screen.
The arcade style at a 640x480 window (and 720x480) lays every sprite on whole pixels, so the bars come out as on the cabinet; scaled
to any other size they smear (a 1024x768 window measures 64 % of the rows alternating, against 100 % at 640x480).

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

The OPTION screen shows the touch screen's own settings (steering, gyro sensitivity, slider pedals, KEY LAYOUT) only with the touch HUD
(`--hud touch`, `hud_style`); with the arcade HUD it has KEY BINDINGS and the graphics. The row the controls are on has an orange bar at its
left and a line under it (the keys are lit).

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

