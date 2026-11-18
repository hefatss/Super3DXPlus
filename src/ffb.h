/* Force feedback for a steering wheel (docs/INPUT.md, "Force feedback"): what the player's car does, as the forces a wheel's motor should give.
 *
 * The port has no copy of the cabinet's wheel protocol (the arcade's I/O board is not part of what was recompiled), so the forces are made here
 * from the car's own sim and handed to the platform's force feedback consumer (on the PC build, SDL haptic through
 * platform/sdl2/wheel.c; Android has no wheel backend and keeps only its pad rumble). The effects:
 *   - steering force: the pull of the front tyres (the car's lateral g), lighter as the tyres pass their grip, and towards the way the car is
 *     going in a slide (counter-steer);
 *   - centering spring and damping, both growing with speed (nothing while parked);
 *   - road effects: the taps and shakes the pad's vibration (haptic.h) makes, as a vibration of the wheel.
 * Settings and the chosen device are saved in <save dir>/ffb. The platform lists the devices it finds and plays ffb_get()'s forces once a frame. */
#ifndef FFB_H
#define FFB_H

#include "car.h"

/* the settings: tenths 0..10 except the switches (0 / 1) and the device (0 none, else the platform's list + 1) */
enum { FFB_ON, FFB_DEVICE, FFB_INVERT, FFB_OVERALL, FFB_STEER, FFB_CENTER, FFB_DAMP, FFB_ROAD, FFB_CURVE, FFB_MIN, FFB_COUNT };
/* FFB_CURVE lifts the small steering forces (0 as they are, 10 the strongest: force ^ (1 - 0.05 n)); FFB_MIN gives any steering force at least n * 2 % of the
 * motor (a wheel whose driver ignores the faintest forces, the Thrustmaster T300RS). Both 0 by default: a wheel that works is felt as before. */
int *ffb_val(int id);

/* the platform's devices (every joystick it found; haptic says the device can play forces): names are copied */
void ffb_set_devices(int n, const char *const names[], const int haptic[]);
int ffb_device_count(void);
const char *ffb_device_name(int i);
int ffb_device_haptic(int i);
/* the device the settings choose: its name, NULL for none or while it is not plugged in */
const char *ffb_selected_name(void);
/* the platform's words on the selected device (what it plays, or why not), for the screen */
void ffb_set_status(const char *text);
const char *ffb_status(void);

/* what the wheel is to do now: each -1..1 or 0..1 */
typedef struct ffb_out_s {
  float force;     /* the steering's pull, + to the right (after the strength, the invert and the overall) */
  float spring;     /* the centering spring's strength */
  float damper;     /* the damper's strength */
  float vib, vib_hz; /* the vibration's strength and frequency */
  float gain;       /* the device's overall gain (the OVERALL setting) */
  int on;           /* the effects run (a device is chosen and FORCE FEEDBACK is on) */
} ffb_out;

/* one arcade frame of the player's car, after car_update; running as for haptic_step (which must have run first) */
void ffb_step(const car_state *c, int running);
/* the forces now (the test's included, which runs anywhere); polls the settings, so call it once a frame */
void ffb_get(ffb_out *o);
/* the forces' settings back to their defaults (the device stays) */
void ffb_defaults(void);
/* a short sweep from side to side and a shake, to feel that the wheel answers (the screen's TEST row) */
void ffb_test(void);

void ffb_load(const char *dir);
void ffb_save(const char *dir);
int ffb_dirty(void);

#endif
