#include "bind.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SDL keycodes of the defaults (SDLK_*; no SDL here): the arrows are SDLK_SCANCODE_MASK | the scancode */
enum {
  K_LEFT = 0x40000050, K_RIGHT = 0x4000004F, K_UP = 0x40000052, K_DOWN = 0x40000051,
  K_ENTER = 13, K_SPACE = 32, K_BACKSPACE = 8, K_TAB = 9, K_X = 'x', K_Z = 'z', K_C = 'c', K_R = 'r', K_T = 't', K_P = 'p', K_L = 'l'
};

static const char *const MENU_NAMES[BM_COUNT] = { "LEFT", "RIGHT", "UP", "DOWN",
                                                  "OK (GAS)", "BACK (BRAKE)", "VIEW" };
static const char *const RACE_NAMES[BR_COUNT] = { "STEER LEFT", "STEER RIGHT", "GAS", "BRAKE", "SHIFT UP", "SHIFT DOWN", "CAMERA",
                                                  "AT / MT", "PAUSE", "GEAR 1", "GEAR 2", "GEAR 3", "GEAR 4", "GEAR 5", "GEAR 6", "HEADLIGHT FLASH" };

static bind_slot B[BIND_CONTEXTS][BR_COUNT][BIND_SLOTS];
static const char *(*key_namer)(int code);
static bind_axis AX[BA_COUNT];
static int dirty;

int bind_actions(int ctx) { return ctx == BIND_MENU ? BM_COUNT : BR_COUNT; }

const char *bind_action_name(int ctx, int action) { return ctx == BIND_MENU ? MENU_NAMES[action] : RACE_NAMES[action]; }

const bind_slot *bind_get(int ctx, int action, int slot) { return &B[ctx][action][slot]; }

static int slot_kind(int slot) { return slot < BIND_KEYS ? BIND_KEY : BIND_PAD; }

int bind_lookup(int ctx, int kind, int code) {
  for (int a = 0; a < bind_actions(ctx); ++a)
    for (int s = 0; s < BIND_SLOTS; ++s)
      if (B[ctx][a][s].kind == kind && B[ctx][a][s].code == code) return a;
  return -1;
}

/* how many inputs of a kind (keys, pad buttons) the action has */
static int count(int ctx, int action, int kind) {
  int n = 0;
  for (int s = 0; s < BIND_SLOTS; ++s) n += B[ctx][action][s].kind == kind;
  return n;
}

int bind_set(int ctx, int action, int slot, int kind, int code) {
  if (ctx < 0 || ctx >= BIND_CONTEXTS || action < 0 || action >= bind_actions(ctx) || slot < 0 || slot >= BIND_SLOTS) return 0;
  if (kind != slot_kind(slot)) return 0;
  bind_slot old = B[ctx][action][slot];
  if (old.kind == kind && old.code == code) return 1;
  /* the input does one action in a context: where it was, it goes (a menu action that would be left with nothing gets this slot's old input) */
  for (int a = 0; a < bind_actions(ctx); ++a)
    for (int s = 0; s < BIND_SLOTS; ++s) {
      if (a == action && s == slot) continue;
      if (B[ctx][a][s].kind != kind || B[ctx][a][s].code != code) continue;
      if (ctx == BIND_MENU && count(ctx, a, kind) == 1) { /* (a menu action keeps one key and one pad button: the menus work from either) */
        if (old.kind == BIND_NONE || a == action) return 0;
        B[ctx][a][s] = old;
      } else {
        B[ctx][a][s] = (bind_slot){ BIND_NONE, 0 };
      }
    }
  B[ctx][action][slot] = (bind_slot){ kind, code };
  dirty = 1;
  return 1;
}

int bind_clear(int ctx, int action, int slot) {
  if (B[ctx][action][slot].kind == BIND_NONE) return 1;
  if (ctx == BIND_MENU && count(ctx, action, slot_kind(slot)) <= 1) return 0;
  B[ctx][action][slot] = (bind_slot){ BIND_NONE, 0 };
  dirty = 1;
  return 1;
}

static void put(int ctx, int action, int slot, int kind, int code) { B[ctx][action][slot] = (bind_slot){ kind, code }; }

void bind_context_defaults(int ctx) {
  memset(B[ctx], 0, sizeof B[ctx]);
  const int PK = BIND_KEYS; /* the first pad slot */
  if (ctx == BIND_MENU) {
    put(ctx, BM_LEFT, 0, BIND_KEY, K_LEFT), put(ctx, BM_LEFT, PK, BIND_PAD, BP_DLEFT), put(ctx, BM_LEFT, PK + 1, BIND_PAD, BP_STICK_LEFT);
    put(ctx, BM_RIGHT, 0, BIND_KEY, K_RIGHT), put(ctx, BM_RIGHT, PK, BIND_PAD, BP_DRIGHT), put(ctx, BM_RIGHT, PK + 1, BIND_PAD, BP_STICK_RIGHT);
    put(ctx, BM_UP, 0, BIND_KEY, K_UP), put(ctx, BM_UP, PK, BIND_PAD, BP_DUP), put(ctx, BM_UP, PK + 1, BIND_PAD, BP_STICK_UP);
    put(ctx, BM_DOWN, 0, BIND_KEY, K_DOWN), put(ctx, BM_DOWN, PK, BIND_PAD, BP_DDOWN), put(ctx, BM_DOWN, PK + 1, BIND_PAD, BP_STICK_DOWN);
    put(ctx, BM_OK, 0, BIND_KEY, K_ENTER), put(ctx, BM_OK, 1, BIND_KEY, K_SPACE);
    put(ctx, BM_OK, PK, BIND_PAD, BP_A), put(ctx, BM_OK, PK + 1, BIND_PAD, BP_START);
    put(ctx, BM_BACK, 0, BIND_KEY, K_BACKSPACE), put(ctx, BM_BACK, 1, BIND_KEY, K_X);
    put(ctx, BM_BACK, PK, BIND_PAD, BP_B), put(ctx, BM_BACK, PK + 1, BIND_PAD, BP_LT);
    put(ctx, BM_VIEW, 0, BIND_KEY, K_TAB), put(ctx, BM_VIEW, 1, BIND_KEY, K_C);
    put(ctx, BM_VIEW, PK, BIND_PAD, BP_Y), put(ctx, BM_VIEW, PK + 1, BIND_PAD, BP_BACK);
  } else {
    put(ctx, BR_LEFT, 0, BIND_KEY, K_LEFT), put(ctx, BR_LEFT, PK, BIND_PAD, BP_DLEFT); /* (the stick, the triggers: axes) */
    put(ctx, BR_RIGHT, 0, BIND_KEY, K_RIGHT), put(ctx, BR_RIGHT, PK, BIND_PAD, BP_DRIGHT);
    put(ctx, BR_GAS, 0, BIND_KEY, K_UP), put(ctx, BR_GAS, PK, BIND_PAD, BP_A);
    put(ctx, BR_BRAKE, 0, BIND_KEY, K_DOWN), put(ctx, BR_BRAKE, PK, BIND_PAD, BP_B);
    put(ctx, BR_SHIFT_UP, 0, BIND_KEY, K_X);
    put(ctx, BR_SHIFT_UP, PK, BIND_PAD, BP_RB), put(ctx, BR_SHIFT_UP, PK + 1, BIND_PAD, BP_DUP);
    put(ctx, BR_SHIFT_DOWN, 0, BIND_KEY, K_Z);
    put(ctx, BR_SHIFT_DOWN, PK, BIND_PAD, BP_LB), put(ctx, BR_SHIFT_DOWN, PK + 1, BIND_PAD, BP_DDOWN);
    put(ctx, BR_CAMERA, 0, BIND_KEY, K_C), put(ctx, BR_CAMERA, 1, BIND_KEY, K_TAB);
    put(ctx, BR_CAMERA, PK, BIND_PAD, BP_Y), put(ctx, BR_CAMERA, PK + 1, BIND_PAD, BP_BACK);
    put(ctx, BR_TRANS, 0, BIND_KEY, K_T);
    put(ctx, BR_PAUSE, 0, BIND_KEY, K_P), put(ctx, BR_PAUSE, PK, BIND_PAD, BP_START);
    put(ctx, BR_FLASH, 0, BIND_KEY, K_L), put(ctx, BR_FLASH, PK, BIND_PAD, BP_X);
  }
  dirty = 1;
}

/* the GEARS list's own defaults: the shifts as they were, the six gears unbound */
void bind_gears_defaults(void) {
  bind_slot keep[BR_COUNT][BIND_SLOTS];
  memcpy(keep, B[BIND_RACE], sizeof keep);
  bind_context_defaults(BIND_RACE);
  static const int GEARS[8] = { BR_SHIFT_UP, BR_SHIFT_DOWN, BR_GEAR_1, BR_GEAR_2, BR_GEAR_3, BR_GEAR_4, BR_GEAR_5, BR_GEAR_6 };
  bind_slot own[8][BIND_SLOTS];
  for (int g = 0; g < 8; ++g) memcpy(own[g], B[BIND_RACE][GEARS[g]], sizeof own[g]);
  memcpy(B[BIND_RACE], keep, sizeof keep);
  for (int g = 0; g < 8; ++g) {
    for (int s = 0; s < BIND_SLOTS; ++s) B[BIND_RACE][GEARS[g]][s] = (bind_slot){ BIND_NONE, 0 };
    for (int s = 0; s < BIND_SLOTS; ++s) /* (an input the defaults give a gear action leaves any other race action it was on) */
      if (own[g][s].kind != BIND_NONE) bind_set(BIND_RACE, GEARS[g], s, own[g][s].kind, own[g][s].code);
  }
  dirty = 1;
}

void bind_axes_defaults(void) {
  AX[BA_STEER] = (bind_axis){ AXIS_LEFTX, 0, 10, 100 };
  AX[BA_GAS] = (bind_axis){ AXIS_TRIGR, 0, 5, 100 };
  AX[BA_BRAKE] = (bind_axis){ AXIS_TRIGL, 0, 5, 100 };
  dirty = 1;
}

void bind_defaults(void) {
  for (int c = 0; c < BIND_CONTEXTS; ++c) bind_context_defaults(c);
  bind_axes_defaults();
  dirty = 0;
}

const char *bind_axis_action_name(int action) {
  static const char *const N[BA_COUNT] = { "STEERING", "GAS", "BRAKE" };
  return N[action];
}

const bind_axis *bind_axis_get(int action) { return &AX[action]; }

void bind_axis_set(int action, int axis, int invert, int dead) {
  if (action < 0 || action >= BA_COUNT || axis < AXIS_NONE || axis >= AXIS_COUNT) return;
  dead = dead < 0 ? 0 : dead > 60 ? 60 : dead;
  AX[action] = (bind_axis){ axis, invert != 0, dead, AX[action].sens > 0 ? AX[action].sens : 100 };
  dirty = 1;
}

void bind_axis_set_sens(int action, int sens) {
  if (action < 0 || action >= BA_COUNT) return;
  AX[action].sens = sens < 25 ? 25 : sens > 300 ? 300 : sens;
  dirty = 1;
}

const char *bind_axis_name(int axis) {
  static const char *const N[AXIS_PAD] = { "LEFT STICK X", "LEFT STICK Y", "RIGHT STICK X", "RIGHT STICK Y", "LEFT TRIGGER", "RIGHT TRIGGER" };
  static char w[AXIS_WHEELS][16];
  if (axis >= AXIS_W0 && axis < AXIS_COUNT) {
    const int i = axis - AXIS_W0, dev = i / AXIS_PER_DEV;
    if (dev == 0) snprintf(w[i], sizeof w[0], "WHEEL AXIS %d", i % AXIS_PER_DEV + 1);
    else snprintf(w[i], sizeof w[0], "DEV%d AXIS %d", dev + 1, i % AXIS_PER_DEV + 1);
    return w[i];
  }
  return axis >= 0 && axis < AXIS_PAD ? N[axis] : "--";
}

float bind_axis_value(int action, const float raw[AXIS_COUNT]) {
  const bind_axis *a = &AX[action];
  if (a->axis < 0) return 0.0f;
  float v = raw[a->axis] * (a->invert ? -1.0f : 1.0f), d = (float)a->dead * 0.01f;
  if (a->axis >= AXIS_W0 && action != BA_STEER) v = 0.5f * (v + 1.0f); /* a wheel's pedal: the whole throw, from one end (rest) to the other */
  float m = v < 0 ? -v : v;
  m = m <= d ? 0.0f : (m - d) / (1.0f - d);
  if (m > 1.0f) m = 1.0f;
  /* the sensitivity is the response curve: 100 straight; above, the middle of the throw gives more (m^(100/sens), a root); below, less (a power).
   * The full throw is always the full value. */
  if (a->sens > 0 && a->sens != 100 && m > 0.0f) m = powf(m, 100.0f / (float)a->sens);
  if (action == BA_STEER) return v < 0 ? -m : m;
  return v > 0 ? m : 0.0f; /* the pedals: the pulled half (a trigger rests at 0 and goes to 1) */
}

void bind_set_key_namer(const char *(*namer)(int code)) { key_namer = namer; }

const char *bind_name(const bind_slot *s) {
  static char buf[4][40];
  static int n;
  char *out = buf[n++ & 3];
  if (s->kind == BIND_NONE) return "--";
  if (s->kind == BIND_KEY) {
    const char *k = key_namer ? key_namer(s->code) : NULL;
    if (!k || !*k) snprintf(out, 40, "KEY %d", s->code);
    else {
      snprintf(out, 40, "%s", k);
      for (char *c = out; *c; ++c) *c = (char)toupper((unsigned char)*c);
    }
    return out;
  }
  static const char *const PAD[BP_W0] = { "A", "B", "X", "Y", "BACK", "START", "L STICK", "R STICK", "LB", "RB", "D-PAD UP",
                                             "D-PAD DOWN", "D-PAD LEFT", "D-PAD RIGHT", "STICK LEFT", "STICK RIGHT", "STICK UP",
                                             "STICK DOWN", "RT", "LT" };
  if (s->code >= BP_W0 && s->code < BP_COUNT) {
    const int i = s->code - BP_W0, dev = i / BP_PER_DEV;
    if (dev == 0) snprintf(out, 40, "WHEEL BTN %d", i % BP_PER_DEV + 1);
    else snprintf(out, 40, "DEV%d BTN %d", dev + 1, i % BP_PER_DEV + 1);
    return out;
  }
  if (s->code >= 0 && s->code < BP_W0) return PAD[s->code];
  snprintf(out, 40, "PAD %d", s->code);
  return out;
}

static void path_of(char *out, size_t n, const char *dir) { snprintf(out, n, "%s/bindings", dir); }

void bind_save(const char *dir) {
  if (!dir || !*dir) return;
  char path[1100];
  path_of(path, sizeof path, dir);
  FILE *f = fopen(path, "w");
  if (!f) return;
  fprintf(f, "bindings 1\n");
  for (int a = 0; a < BA_COUNT; ++a) fprintf(f, "x %d %d %d %d %d\n", a, AX[a].axis, AX[a].invert, AX[a].dead, AX[a].sens);
  for (int c = 0; c < BIND_CONTEXTS; ++c)
    for (int a = 0; a < bind_actions(c); ++a)
      for (int s = 0; s < BIND_SLOTS; ++s)
        if (B[c][a][s].kind != BIND_NONE) fprintf(f, "%d %d %d %d %d\n", c, c == BIND_RACE && a >= BR_RESET_FILE ? a + 1 : a, s, B[c][a][s].kind, B[c][a][s].code);
  fclose(f);
  dirty = 0;
}

void bind_load(const char *dir) {
  bind_defaults();
  if (!dir || !*dir) return;
  char path[1100];
  path_of(path, sizeof path, dir);
  FILE *f = fopen(path, "r");
  if (!f) return;
  int version = 0;
  if (fscanf(f, "bindings %d", &version) != 1 || version != 1) {
    fclose(f);
    return;
  }
  memset(B, 0, sizeof B); /* (what the file lacks is unbound) */
  int c, a, s, kind, code;
  char line[128];
  fgets(line, sizeof line, f); /* (the rest of the header's line) */
  while (fgets(line, sizeof line, f)) {
    if (line[0] == 'x') { /* an axis: x <action> <axis> <invert> <dead> */
      int axis, invert, dead, sens = 100; /* (a file from before the sensitivity has four numbers) */
      int got = sscanf(line, "x %d %d %d %d %d", &a, &axis, &invert, &dead, &sens);
      if (got >= 4) bind_axis_set(a, axis, invert, dead), bind_axis_set_sens(a, got == 5 ? sens : 100);
    } else if (sscanf(line, "%d %d %d %d %d", &c, &a, &s, &kind, &code) == 5) {
      if (c == BIND_RACE && a == BR_RESET_FILE) continue; /* (the reset car control is gone) */
      if (c == BIND_RACE && a > BR_RESET_FILE) --a;
      if (c >= 0 && c < BIND_CONTEXTS && a >= 0 && a < bind_actions(c) && s >= 0 && s < BIND_SLOTS && kind == slot_kind(s))
        B[c][a][s] = (bind_slot){ kind, code };
    }
  }
  fclose(f);
  /* a file from before the headlight flash has none: its defaults, where the keys are free (the file leaves what it lacks unbound) */
  if (count(BIND_RACE, BR_FLASH, BIND_KEY) + count(BIND_RACE, BR_FLASH, BIND_PAD) == 0) {
    if (bind_lookup(BIND_RACE, BIND_KEY, K_L) < 0) B[BIND_RACE][BR_FLASH][0] = (bind_slot){ BIND_KEY, K_L };
    if (bind_lookup(BIND_RACE, BIND_PAD, BP_X) < 0) B[BIND_RACE][BR_FLASH][BIND_KEYS] = (bind_slot){ BIND_PAD, BP_X };
  }
  /* the menus must stay usable from the keyboard and from a pad: an action the file left without keys, or without pad buttons, has its defaults for them */
  static bind_slot dflt[BR_COUNT][BIND_SLOTS];
  static bind_slot keep[BR_COUNT][BIND_SLOTS];
  memcpy(keep, B[BIND_MENU], sizeof keep);
  bind_context_defaults(BIND_MENU);
  memcpy(dflt, B[BIND_MENU], sizeof dflt);
  memcpy(B[BIND_MENU], keep, sizeof keep);
  for (int m = 0; m < BM_COUNT; ++m)
    for (int kind = BIND_KEY; kind <= BIND_PAD; ++kind)
      if (count(BIND_MENU, m, kind) == 0)
        for (int s2 = 0; s2 < BIND_SLOTS; ++s2)
          if (dflt[m][s2].kind == kind) B[BIND_MENU][m][s2] = dflt[m][s2];
  dirty = 0;
}

int bind_dirty(void) { return dirty; }
