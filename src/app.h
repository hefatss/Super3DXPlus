/* Platform-independent game loop; main.m forwards lifecycle, touches and frames. */
#ifndef APP_H
#define APP_H

/* data_dir holds c1.wpk, c1.wcl and fd3s.wpk. Returns 0 on success. */
/* Before app_init: a writable folder for the player's files (records, garage, the last time
 * zone, perf.log) when data_dir is read-only, as the app bundle of an .ipa install. */
void app_save_dir(const char *dir);
/* Before app_init on a screen wider than 4:3 (an iPhone's 3:2): the 1024x768 layout in its
 * middle, edge layout px more on each side; the race's touch keys key_k times as big. It also
 * turns on the iPhone's cuts. Touches then come in the layout's px (x from -edge). */
void app_wide(float edge, float key_k);
/* Before app_wide on a fast device (Android): no iPhone cuts though the screen is wide, the
 * maps drawn further. */
void app_console(void);
/* A desktop window's size: before app_init and again whenever the window changes (between frames). The framebuffer is that size;
 * a window wider than 4:3 is app_wide's screen (the layout in its middle, the sides filled), one taller than 4:3 must be given
 * as the 4:3 part the game draws in (the platform keeps it at the window's bottom). */
void app_resize(int fb_width, int fb_height);
int app_init(const char *data_dir, int fb_width, int fb_height, float screen_w, float screen_h);
void app_touch(void *id, int phase, float x, float y);
void app_frame(float dt);
/* Keys of a keyboard or a pad's buttons, for the race (the menus take touches: the platform moves a cursor and taps). */
enum { APP_KEY_LEFT, APP_KEY_RIGHT, APP_KEY_ACCEL, APP_KEY_BRAKE, APP_KEY_SHIFT_UP, APP_KEY_SHIFT_DOWN,
       APP_KEY_CAMERA, APP_KEY_RESET, APP_KEY_TRANS, APP_KEY_PAUSE,
       APP_KEY_GEAR_1, APP_KEY_GEAR_2, APP_KEY_GEAR_3, APP_KEY_GEAR_4, APP_KEY_GEAR_5, APP_KEY_GEAR_6 /* an H shifter's positions */,
       APP_KEY_FLASH /* held: the headlights flash (a versus race) */ };
/* down: pressed (1) or let go (0). The one-press keys (shifts, camera, reset, transmission, pause) act on the press. */
void app_key(int key, int down);
/* A control of the menus (menu.h's MENU_ACT_*): the steering, the shifter, the gas, the brake, the view button, from a keyboard
 * or a pad (docs/INPUT.md). Taken wherever a menu, the pause menu or a result is up; the race itself takes app_key. */
void app_menu_action(int act);
/* A keyboard key or pad button pressed or let go (bind.h: kind BIND_KEY with the platform's keycode, BIND_PAD with a BP_* code; repeat: the
 * key repeat of a held one): the bindings turn it into the race's action (app_key) or the menus' (app_menu_action). */
void app_input(int kind, int code, int down, int repeat);
/* A pad axis moved (bind.h AXIS_*; v -1..1, a trigger 0..1): the steering, the gas and the brake take theirs through the bindings. */
void app_axis(int axis, float v);
/* The input readout (steering, gas and brake bars, the raw axes) over the race: on, off or (-1) switched. */
void app_readout(int on);
void app_phys(int on); /* the physics panel on, off or (-1) flipped (F4) */
void app_tune(int on); /* the steering filter's tuning panel on, off or (-1) flipped (F5) */
/* The binding screen is waiting for an input: the platform gives it the escape key too, not its own meaning. */
int app_bind_capturing(void);
/* The race is on screen and takes the keys (not a menu, the pause menu or a result). */
int app_racing(void);
/* the window mode (the PC player): 0 windowed, 1 fullscreen; app_window_row(1) gives the game options their WINDOW MODE row */
int app_fullscreen(void);
void app_set_fullscreen(int on);
void app_toggle_fullscreen(void);
void app_window_row(int on);
/* the Windows player's network options rows (ALLOW THROUGH FIREWALL, OPEN LOG FOLDER): app_firewall_row(1) gives the screen them */
void app_firewall_row(int on);
/* 1 while the race on screen is drawn with smooth motion (the picture blended between the physics' steps: a display that is not 60 Hz, a versus): the
 * platform should then show a frame every refresh of the display, not hold itself to 60 a second. */
int app_smooth(void);
/* The screen's refresh in Hz where the platform knows it (the PC player, which shows its menus at 60 on a faster screen): smooth motion is chosen by it */
void app_display_hz(float hz);
/* 1: smooth motion in every race whatever the option says (the player cannot show the game at 60 on this screen) */
void app_force_smooth(int on);
/* Android's 60 FPS LOCK (graphics screen): app_frame_lock_row(1) gives it the row; app_frame_lock() is 1 while it is on, the platform's to ask the system for a 60 Hz screen */
void app_frame_lock_row(int on);
int app_frame_lock(void);
/* the network options screen: the action it asks of the platform, once (1 allow through the firewall, 2 open the log folder, 3 the diagnostics text to the clipboard), and that text */
int app_net_act(void);
const char *app_net_diag(void);
/* the save folder (the log's) */
const char *app_save_folder(void);
/* the BLACK LEVEL option (0 off .. 10): the PC player pulls the picture's blacks down by it (a display or driver that lifts them) */
int app_black_level(void);
/* the controller rumble now, 0..1 each: the heavy motor and the light (zero outside a running race; haptic.h) */
void app_rumble(float *low, float *high);
/* the steering wheel's forces now (ffb.h; the platform plays them on the chosen device; zero outside a running race, the test's anywhere) */
struct ffb_out_s;
void app_ffb(struct ffb_out_s *out);
float app_speed_kmh(void); /* the player's car's speed, signed */
/* The iPad's turn about the screen's normal from level (radians, clockwise as the player
 * sees it positive), for steering by tilt (the OPTION screen's GYRO); main.m each frame. */
void app_tilt(float radians);
/* Steering by tilt is on (main.m reads the gravity only then). */
int app_wants_tilt(void);
/* The start button (a tap on the attract movie): leaves the attract loop if it is up. */
void app_press_start(void);
/* The menu screen now (menu.h's MENU_* in order), -1 in a race: for logs and tests. */
int app_screen(void);
/* EXIT on the main menu: the player closes (the console it runs on, started by systemd, shuts down) */
int app_exit_requested(void);
/* the app's end: a versus game (or race) it is in is left, the others told */
void app_vs_leave(void);
/* The attract movie to show now (0 atruct, 1 playdemo, 2 tutorial; see main.m), or -1. */
int app_movie(void);
/* The movie shown for app_movie() ended, or its file is missing. */
void app_movie_done(void);
/* The menu or race BGM that should play: a file name in data_dir/sound/bgm (without
 * .m4a), or NULL for none (the attract loop has its own sound). */
const char *app_bgm(void);
/* The track to queue after the one playing (the 10-in-a-row's next loop), NULL for none: bgm_next */
const char *app_bgm_next(void);
/* the BGM's level over its own (1; the story ending's fade out): the platform code (platform/ folders) */
float app_bgm_level(void);
/* Loading on a thread of its own, so the menus keep their frames: after app_init main.m
 * runs app_loader on a new thread with a GL context sharing the app's current there; it
 * never returns. Without it the menu frames load in their spare time. */
void app_loader(void);
/* main.m after each frame's presentRenderbuffer, with the time it waited there: the loading
 * thread runs till the next frame (app_loader) */
void app_frame_done(double present_ms);
/* The app goes to the background (1) or comes back (0): the loading thread stops while it
 * is away, where iOS allows no GL. */
void app_suspend(int away);
/* iOS is short of memory: free what can be loaded again. */
void app_memory_warning(void);
const char *app_error(void);

#endif
