#include "ffb.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "haptic.h"

#define MAXDEV 16

static struct {
  int v[FFB_COUNT];
  int saved[FFB_COUNT];
  char name[128];                 /* the chosen device's name (kept while it is unplugged) */
  int n_dev, dev_haptic[MAXDEV];
  char dev_name[MAXDEV][128];
  int last_idx;                   /* v[FFB_DEVICE] as last resolved to a name */
  char status[160];
  float sat;                      /* the steering force, smoothed */
  float vib, vib_hz;              /* the road effects, smoothed from haptic.c */
  float speed;                    /* |speed| factor, 0..1 */
  int running;
  int test;                       /* frames of the test left */
  int test_total;
  int loaded;
} F = { .v = { 1, 0, 0, 6, 7, 3, 3, 5, 0, 0 }, .last_idx = -1 };

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

int *ffb_val(int id) { return &F.v[id >= 0 && id < FFB_COUNT ? id : 0]; }

/* the settings' device number against the list: after a change in the menu the name follows; after the list changes the number follows the name */
static void resolve(void) {
  int idx = F.v[FFB_DEVICE];
  if (idx < 0 || idx > F.n_dev) idx = F.v[FFB_DEVICE] = 0;
  if (idx != F.last_idx) { /* the menu (or a load) chose */
    if (idx > 0) snprintf(F.name, sizeof F.name, "%s", F.dev_name[idx - 1]);
    else if (F.last_idx >= 0) F.name[0] = 0; /* (chosen "none") */
    F.last_idx = idx;
  }
}

/* the chosen name's place in the list (it may have moved, or been unplugged: then the number is 0 but the name stays for its return) */
static void find_name(void) {
  int found = 0;
  for (int i = 0; i < F.n_dev && F.name[0]; ++i)
    if (!strcmp(F.dev_name[i], F.name)) {
      found = i + 1;
      break;
    }
  F.v[FFB_DEVICE] = F.last_idx = found;
}

void ffb_set_devices(int n, const char *const names[], const int haptic[]) {
  resolve(); /* (a choice made in the menu keeps its name before the list changes) */
  F.n_dev = n < 0 ? 0 : n > MAXDEV ? MAXDEV : n;
  for (int i = 0; i < F.n_dev; ++i) {
    snprintf(F.dev_name[i], sizeof F.dev_name[i], "%s", names[i] ? names[i] : "?");
    F.dev_haptic[i] = haptic ? haptic[i] : 0;
  }
  find_name();
}

int ffb_device_count(void) { return F.n_dev; }
const char *ffb_device_name(int i) { return i >= 0 && i < F.n_dev ? F.dev_name[i] : ""; }
int ffb_device_haptic(int i) { return i >= 0 && i < F.n_dev ? F.dev_haptic[i] : 0; }

const char *ffb_selected_name(void) {
  resolve();
  return F.v[FFB_DEVICE] > 0 ? F.dev_name[F.v[FFB_DEVICE] - 1] : NULL;
}

void ffb_set_status(const char *text) { snprintf(F.status, sizeof F.status, "%s", text ? text : ""); }
const char *ffb_status(void) { return F.status; }

void ffb_step(const car_state *c, int running) {
  F.running = running;
  if (!running) {
    F.sat = F.vib = F.speed = 0.0f;
    return;
  }
  const float v = fabsf(c->speed);
  F.speed = clampf(v / 8.0f, 0.0f, 1.0f); /* nothing while parked, all of it from about 30 km/h */
  /* the tyres' pull: the car's lateral g (+ right) is the way the road pushes the car; the wheel is pulled the other way, towards straight ahead */
  const float g = clampf(c->phys.g_lat, -1.5f, 1.5f);
  float sat = -g / 1.0f;
  /* past the grip (the front tyres' slip angle) the wheel goes light */
  const float over = clampf((fabsf(c->phys.tyre_angle) - 0.10f) / 0.25f, 0.0f, 1.0f);
  sat *= 1.0f - 0.6f * over;
  /* in a slide the wheel is pulled to where the car is going: the body's slip angle, + when it goes to the right of its heading */
  const float sa = c->phys.slip_body;
  const float drift = clampf(sa / 0.5f, -1.0f, 1.0f) * 0.6f * clampf((fabsf(sa) - 0.03f) / 0.10f, 0.0f, 1.0f);
  float target = clampf((0.8f * sat + drift) * F.speed, -1.0f, 1.0f);
  F.sat += (target - F.sat) * 0.35f;
  /* the road: what the pad's rumble would give the two motors, as one vibration of the wheel (the heavy one slow, the light one quick) */
  float lo, hi;
  haptic_get(&lo, &hi);
  const float vib = fmaxf(lo, hi);
  F.vib_hz = hi > lo ? 70.0f : 28.0f;
  F.vib += (vib - F.vib) * 0.6f;
}

void ffb_defaults(void) {
  static const int D[FFB_COUNT] = { 1, 0, 0, 6, 7, 3, 3, 5, 0, 0 };
  for (int i = 0; i < FFB_COUNT; ++i)
    if (i != FFB_DEVICE) F.v[i] = D[i];
}

void ffb_test(void) { F.test = F.test_total = 150; }

void ffb_get(ffb_out *o) {
  resolve();
  memset(o, 0, sizeof *o);
  o->on = F.v[FFB_ON] && F.v[FFB_DEVICE] > 0;
  o->gain = clampf((float)F.v[FFB_OVERALL] / 10.0f, 0.0f, 1.0f);
  if (!o->on) {
    F.test = 0;
    return;
  }
  if (F.test > 0) { /* 2.5 s at 60 frames: the sweep side to side, then the shake */
    const int t = F.test_total - F.test--;
    if (t < 100) o->force = 0.85f * sinf((float)t * (6.2831853f / 50.0f));
    else o->vib = 0.7f, o->vib_hz = 40.0f;
    o->spring = 0.0f;
    return;
  }
  if (!F.running) return;
  const float inv = F.v[FFB_INVERT] ? -1.0f : 1.0f;
  /* the steering force: the response curve first (a small pull of the tyres made larger), then the strength, then the faintest force the driver lets through */
  float s = F.sat;
  if (F.v[FFB_CURVE] > 0 && s != 0.0f) s = copysignf(powf(fabsf(s), 1.0f - 0.05f * (float)F.v[FFB_CURVE]), s);
  float force = clampf(s * (float)F.v[FFB_STEER] / 10.0f, -1.0f, 1.0f);
  if (F.v[FFB_MIN] > 0 && fabsf(force) > 0.004f) {
    const float lo = 0.02f * (float)F.v[FFB_MIN];
    force = copysignf(lo + (1.0f - lo) * fabsf(force), force);
  }
  o->force = clampf(force * inv, -1.0f, 1.0f);
  o->spring = clampf((float)F.v[FFB_CENTER] / 10.0f * F.speed, 0.0f, 1.0f);
  o->damper = clampf((float)F.v[FFB_DAMP] / 10.0f * (0.3f + 0.7f * F.speed), 0.0f, 1.0f);
  o->vib = clampf(F.vib * (float)F.v[FFB_ROAD] / 10.0f, 0.0f, 1.0f);
  o->vib_hz = F.vib_hz;
}

static void path_of(char *out, size_t n, const char *dir) { snprintf(out, n, "%s/ffb", dir); }

void ffb_load(const char *dir) {
  if (!dir || !*dir) return;
  char path[1100], line[256];
  path_of(path, sizeof path, dir);
  FILE *f = fopen(path, "r");
  if (!f) {
    memcpy(F.saved, F.v, sizeof F.saved);
    return;
  }
  while (fgets(line, sizeof line, f)) {
    int v[FFB_COUNT];
    if (!strncmp(line, "device ", 7)) {
      size_t n = strcspn(line + 7, "\r\n");
      if (n >= sizeof F.name) n = sizeof F.name - 1;
      memcpy(F.name, line + 7, n);
      F.name[n] = 0;
    } else if (sscanf(line, "values %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]) == 7) {
      F.v[FFB_ON] = v[0] != 0, F.v[FFB_INVERT] = v[1] != 0;
      for (int i = 0; i < 5; ++i) F.v[FFB_OVERALL + i] = (int)clampf((float)v[2 + i], 0.0f, 10.0f);
    } else if (sscanf(line, "curve %d %d", &v[0], &v[1]) == 2) { /* (after "values", so a file from before keeps 0 and 0) */
      F.v[FFB_CURVE] = (int)clampf((float)v[0], 0.0f, 10.0f), F.v[FFB_MIN] = (int)clampf((float)v[1], 0.0f, 10.0f);
    }
  }
  fclose(f);
  find_name(); /* (or when the platform lists its devices, if it has not yet) */
  memcpy(F.saved, F.v, sizeof F.saved);
  F.loaded = 1;
}

/* the part of the settings that is saved: the device is its name */
static int differs(void) {
  for (int i = 0; i < FFB_COUNT; ++i)
    if (i != FFB_DEVICE && F.v[i] != F.saved[i]) return 1;
  resolve();
  return F.v[FFB_DEVICE] != F.saved[FFB_DEVICE];
}

int ffb_dirty(void) { return differs(); }

void ffb_save(const char *dir) {
  if (!dir || !*dir) return;
  resolve();
  char path[1100];
  path_of(path, sizeof path, dir);
  FILE *f = fopen(path, "w");
  if (!f) return;
  fprintf(f, "ffb 1\ndevice %s\nvalues %d %d %d %d %d %d %d\n", F.name, F.v[FFB_ON], F.v[FFB_INVERT], F.v[FFB_OVERALL], F.v[FFB_STEER], F.v[FFB_CENTER],
          F.v[FFB_DAMP], F.v[FFB_ROAD]);
  fprintf(f, "curve %d %d\n", F.v[FFB_CURVE], F.v[FFB_MIN]);
  fclose(f);
  memcpy(F.saved, F.v, sizeof F.saved);
}
