/* The bindable controls: which keyboard keys and pad buttons do the game's actions (docs/INPUT.md).
 *
 * Two sets of actions, each its own context: the MENUS' (the arcade's steering, shifter, gas, brake and view button as they move,
 * decide and go back in a menu) and the RACE's (steer, gas, brake, shift, camera, transmission, pause). An action has up to
 * BIND_SLOTS inputs: BIND_KEYS keyboard keys, then BIND_PADS pad buttons. Keyboard codes are SDL keycodes (the platform's), pad codes
 * are BP_* below (the platform turns its buttons, the stick's directions and the triggers into them); axes come later. Saved in
 * <save dir>/bindings; the defaults are the ones the port has been tested with (arrows, Enter, Backspace ... and the pad's A, B, Y). */
#ifndef BIND_H
#define BIND_H

enum { BIND_NONE, BIND_KEY, BIND_PAD };
enum { BIND_MENU, BIND_RACE, BIND_CONTEXTS };

/* the menus' actions (in menu.h's MENU_ACT_* order) and the race's */
enum { BM_LEFT, BM_RIGHT, BM_UP, BM_DOWN, BM_OK, BM_BACK, BM_VIEW, BM_COUNT };
enum { BR_LEFT, BR_RIGHT, BR_GAS, BR_BRAKE, BR_SHIFT_UP, BR_SHIFT_DOWN, BR_CAMERA, BR_TRANS, BR_PAUSE,
       /* the six positions of an H shifter (the GEARS list): the press puts the lever in that gear at once, beside shift up and down */
       BR_GEAR_1, BR_GEAR_2, BR_GEAR_3, BR_GEAR_4, BR_GEAR_5, BR_GEAR_6,
       BR_FLASH /* the interrupt switch's second job in a versus race: the headlights flash */, BR_COUNT }; /* (the file's numbers keep the removed reset car's place: BR_RESET_FILE) */
#define BR_RESET_FILE 7

/* pad inputs: buttons, the left stick's four directions and the two triggers (as buttons, past a threshold) */
enum { BP_A, BP_B, BP_X, BP_Y, BP_BACK, BP_START, BP_LSTICK, BP_RSTICK, BP_LB, BP_RB, BP_DUP, BP_DDOWN, BP_DLEFT, BP_DRIGHT,
       BP_STICK_LEFT, BP_STICK_RIGHT, BP_STICK_UP, BP_STICK_DOWN, BP_RT, BP_LT,
       /* the buttons of the steering wheel and the other game devices (shifters, button boxes, pedals): WHEEL_DEVS devices of BP_PER_DEV
        * buttons each, the chosen wheel's first (the others in the order SDL lists them) */
       BP_W0, BP_WHEEL_LAST = BP_W0 + 255, BP_COUNT };
#define WHEEL_DEVS 4
#define BP_PER_DEV 64
#define BP_WHEELS (WHEEL_DEVS * BP_PER_DEV)

/* two keys the binding screen itself takes (SDL's keycodes): leave the capture, clear the slot */
#define BK_ESCAPE 27
#define BK_DELETE 127

#define BIND_KEYS 2
#define BIND_PADS 2
#define BIND_SLOTS (BIND_KEYS + BIND_PADS)
typedef struct { int kind, code; } bind_slot;

int bind_actions(int ctx);
const char *bind_action_name(int ctx, int action);
const bind_slot *bind_get(int ctx, int action, int slot);
/* the action a key or button does in the context, -1 for none */
int bind_lookup(int ctx, int kind, int code);
/* puts an input in a slot (the one it did before, in the context, lets go of it); the slot's kind must be the input's */
int bind_set(int ctx, int action, int slot, int kind, int code);
/* empties a slot; not the last key or the last pad button of a menu action (the menus must stay usable from either): returns 0 then */
int bind_clear(int ctx, int action, int slot);
void bind_defaults(void);
void bind_context_defaults(int ctx);
/* the GEARS list: shift up and down as they were by default, the six gears unbound */
void bind_gears_defaults(void);
/* the slot's input as the screen shows it ("ENTER", "A", "STICK LEFT"; "--" for none) */
const char *bind_name(const bind_slot *s);
/* the platform's name of a keyboard code (SDL_GetKeyName); without one, the code */
void bind_set_key_namer(const char *(*namer)(int code));
void bind_load(const char *dir);
void bind_save(const char *dir);
/* whether the bindings changed since they were loaded or last saved */
int bind_dirty(void);


/* ---- the analog controls: which axis steers, which is the gas and which the brake (docs/INPUT.md). The axes are the pad's six (SDL's
 * game controller axes: the sticks -1..1, the triggers 0..1); an action takes one, inverted or not, with a dead zone (percent). The
 * digital inputs above still work beside them (a pressed key wins over the axis). ---- */
enum { BA_STEER, BA_GAS, BA_BRAKE, BA_COUNT };
enum { AXIS_NONE = -1, AXIS_LEFTX, AXIS_LEFTY, AXIS_RIGHTX, AXIS_RIGHTY, AXIS_TRIGL, AXIS_TRIGR,
       /* the axes of the wheel and the other game devices, AXIS_PER_DEV each (the joystick layer's, -1..1: a pedal rests at one end and goes to the other) */
       AXIS_W0, AXIS_W7 = AXIS_W0 + 47, AXIS_COUNT };
#define AXIS_PER_DEV 12
#define AXIS_WHEELS (WHEEL_DEVS * AXIS_PER_DEV)
#define AXIS_PAD 6 /* the game controller's axes (the first of them) */
typedef struct { int axis, invert, dead, sens; } bind_axis; /* dead: percent of the throw ignored about rest; sens: percent, the value's gain after it (100 as it is) */

const char *bind_axis_action_name(int action);
const bind_axis *bind_axis_get(int action);
void bind_axis_set(int action, int axis, int invert, int dead);
void bind_axis_set_sens(int action, int sens); /* 25..300 percent */
const char *bind_axis_name(int axis);
void bind_axes_defaults(void);
/* the action's value from the raw axes: steer -1..1 (+ right), gas and brake 0..1 (a stick's half, a trigger's pull) */
float bind_axis_value(int action, const float raw[AXIS_COUNT]);

#endif
