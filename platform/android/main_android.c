/* The Android side of the port (main.m's, for a phone like the Xiaomi Mi 6): MainActivity.java
 * hands over the SurfaceView's surface, touches, the gravity and the lifecycle; the game runs
 * on a thread of its own here with an OpenGL ES 3 context (4x MSAA) on that surface, the
 * loading thread with a context sharing it, and the sound through AAudio. Built with -DWMMT_VK
 * the game's GL goes through vkgl (Vulkan) instead: a swapchain on the surface, the loading
 * thread vkgl's second. */
#ifdef WMMT_VK
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <aaudio/AAudio.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <jni.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app.h"
#include "netdiag.h"
#include "bgm.h"
#include "bind.h"
#include "gl.h"
#include "hud.h"
#include "sound.h"

/* the attract movies (tools/pipeline.py convert, step movies), indexed as app_movie() returns them */
static const char *const MOVIES[3] = { "intro.mp4", "playdemo.mp4", "tutorial.mp4" };

#define TOUCH_MAX 256
typedef struct { int id, phase; float x, y; } touch_ev;
/* a game pad's input from MainActivity: a button (a BP_* code, down or up) or an axis (AXIS_*: the sticks -1..1, the triggers 0..1) */
#define PAD_MAX 128
typedef struct { int axis; int code; float v; } pad_ev;

static struct {
  JavaVM *vm;
  jobject activity;
  jmethodID show_movie, play_bgm, bgm_next, bgm_level, fail, exit_app, rumble;
  int exit_sent;
  char data_dir[512], save_dir[512];
  pthread_mutex_t mx;
  pthread_cond_t cv;
  ANativeWindow *window; /* the surface to draw on (NULL while there is none) */
  int window_changed, paused, started;
  touch_ev touches[TOUCH_MAX];
  int n_touch;
  int press_start, movie_done_ev, memory_warning;
  pad_ev pads[PAD_MAX];
  int n_pad;
  int touch_ui, touch_ui_new; /* the on-screen touch controls (MainActivity's choice: on, off, or off while a pad is used) */
  float tilt;
  int tilt_new, wants_tilt;
  /* the game thread's own */
#ifdef WMMT_VK
  int vk_up, have_surface;
  VkSurfaceKHR vk_surface;
#endif
  EGLDisplay dpy;
  EGLConfig config;
  EGLContext ctx, load_ctx;
  EGLSurface surface;
  int fb_w, fb_h;
  float touch_k, touch_edge;
  int movie; /* shown, -1 none */
  const char *bgm;
  AAudioStream *audio;
} A = { .movie = -1, .touch_ui = 1 };

static double now_s(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + t.tv_nsec * 1e-9;
}

/* ---- sound: sound.c's mix, 44.1 kHz stereo int16, low latency ---- */

static aaudio_data_callback_result_t audio_cb(AAudioStream *s, void *user, void *data, int32_t frames) {
  snd_mix((short *)data, frames);
  bgm_mix((short *)data, frames); /* (the music: bgm.c, decoded whole and looped sample-accurately) */
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static void audio_error(AAudioStream *s, void *user, aaudio_result_t err);

static void audio_open(void) {
  AAudioStreamBuilder *b;
  if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) return;
  AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
  AAudioStreamBuilder_setChannelCount(b, 2);
  AAudioStreamBuilder_setSampleRate(b, SND_RATE);
  AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_setUsage(b, AAUDIO_USAGE_GAME);
  AAudioStreamBuilder_setDataCallback(b, audio_cb, NULL);
  AAudioStreamBuilder_setErrorCallback(b, audio_error, NULL);
  if (AAudioStreamBuilder_openStream(b, &A.audio) == AAUDIO_OK) AAudioStream_requestStart(A.audio);
  else A.audio = NULL;
  AAudioStreamBuilder_delete(b);
}

/* the output went away (headphones in or out): a new stream, from a thread of its own as
 * AAudio asks */
static void *audio_reopen(void *arg) {
  if (A.audio) AAudioStream_close(A.audio);
  A.audio = NULL;
  audio_open();
  return NULL;
}

static void audio_error(AAudioStream *s, void *user, aaudio_result_t err) {
  if (err != AAUDIO_ERROR_DISCONNECTED) return;
  pthread_t t;
  if (pthread_create(&t, NULL, audio_reopen, NULL) == 0) pthread_detach(t);
}

/* ---- calls up to MainActivity (the game thread is attached to the VM) ---- */

static JNIEnv *env_here(void) {
  JNIEnv *env = NULL;
  if ((*A.vm)->GetEnv(A.vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) (*A.vm)->AttachCurrentThread(A.vm, &env, NULL);
  return env;
}

static void call_str(jmethodID m, const char *s) {
  JNIEnv *env = env_here();
  jstring js = s ? (*env)->NewStringUTF(env, s) : NULL;
  (*env)->CallVoidMethod(env, A.activity, m, js);
  if (js) (*env)->DeleteLocalRef(env, js);
}

/* the movie the attract loop wants and the BGM (main.m syncMovie) */
static void sync_movie(void) {
  int want = app_movie();
  if (want != A.movie) {
    A.movie = want;
    call_str(A.show_movie, want >= 0 ? MOVIES[want] : NULL);
  }
  const char *track = app_bgm();
  if (track != A.bgm) {
    A.bgm = track;
    bgm_play(track);
  }
  bgm_next(app_bgm_next()); /* (the 10-in-a-row's next loop, crossfaded in) */
  /* (the story ending's fade out) */
  static float level = 1.0f;
  float l = app_bgm_level();
  if (l != level) {
    level = l;
    bgm_level(l);
  }
}

/* ---- GL ---- */

#ifdef WMMT_VK
/* Vulkan (vkgl): the pipelines it made last time from save_dir; the programs it has no SPIR-V
 * for written there too, for the next build */
static int egl_init(void) {
  static const char *const ext[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME };
  if (vkgl_create(ext, 2) != 0) return -1;
  char path[600];
  snprintf(path, sizeof path, "%s/vk_pipelines.bin", A.save_dir);
  vkgl_pipeline_cache_file(path);
  setenv("VKGL_MISSING_DIR", A.save_dir, 1);
  A.vk_up = 1;
  A.load_ctx = (EGLContext)1; /* (a loading thread: vkgl's second) */
  return 0;
}

static void *loader(void *arg) {
  vkgl_thread_loader();
  app_loader();
  return NULL;
}

/* the surface MainActivity has now (under A.mx): its swapchain, or none */
static void take_window(void) {
  A.window_changed = 0;
  if (A.have_surface) {
    vkgl_release_surface(); /* (the surface with it) */
    A.vk_surface = VK_NULL_HANDLE;
    A.have_surface = 0;
  }
  if (!A.window) return;
  VkAndroidSurfaceCreateInfoKHR ci = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
  ci.window = A.window;
  if (vkCreateAndroidSurfaceKHR(vkgl_instance(), &ci, NULL, &A.vk_surface) != VK_SUCCESS ||
      vkgl_surface(A.vk_surface, ANativeWindow_getWidth(A.window), ANativeWindow_getHeight(A.window)) != 0) {
    fprintf(stderr, "no Vulkan surface\n");
    return;
  }
  A.have_surface = 1;
  vkgl_screen_size(&A.fb_w, &A.fb_h);
  fprintf(stderr, "surface %dx%d (Vulkan)\n", A.fb_w, A.fb_h);
}
#define SURFACE_UP A.have_surface
#else
#define SURFACE_UP (A.surface != EGL_NO_SURFACE)

static int egl_init(void) {
  A.dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (!eglInitialize(A.dpy, NULL, NULL)) return -1;
  /* 8-bit colour, depth 24 + stencil 8 (the cars' shadows), 4x MSAA where there is (an
   * Adreno resolves it on chip, nearly free) */
  for (int samples = 4; samples >= 0; samples -= 4) {
    const EGLint attr[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 0,
                            EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
                            EGL_SAMPLE_BUFFERS, samples ? 1 : 0, EGL_SAMPLES, samples, EGL_NONE };
    EGLint n = 0;
    if (eglChooseConfig(A.dpy, attr, &A.config, 1, &n) && n == 1) {
      fprintf(stderr, "EGL config: %d samples\n", samples);
      break;
    }
    if (!samples) return -1;
  }
  const EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  A.ctx = eglCreateContext(A.dpy, A.config, EGL_NO_CONTEXT, ca);
  if (A.ctx == EGL_NO_CONTEXT) return -1;
  A.load_ctx = eglCreateContext(A.dpy, A.config, A.ctx, ca);
  return 0;
}

static void *loader(void *arg) {
  const EGLint pa[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
  EGLSurface pb = eglCreatePbufferSurface(A.dpy, A.config, pa);
  if (!eglMakeCurrent(A.dpy, pb, pb, A.load_ctx)) {
    fprintf(stderr, "loading thread: no context\n");
    return NULL;
  }
  app_loader();
  return NULL;
}

/* the surface MainActivity has now (under A.mx): made current, or none */
static void take_window(void) {
  if (A.surface != EGL_NO_SURFACE) {
    eglMakeCurrent(A.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(A.dpy, A.surface);
    A.surface = EGL_NO_SURFACE;
  }
  A.window_changed = 0;
  if (!A.window) return;
  A.surface = eglCreateWindowSurface(A.dpy, A.config, A.window, NULL);
  if (A.surface == EGL_NO_SURFACE || !eglMakeCurrent(A.dpy, A.surface, A.surface, A.ctx)) {
    fprintf(stderr, "no window surface: 0x%x\n", eglGetError());
    A.surface = EGL_NO_SURFACE;
    return;
  }
  eglQuerySurface(A.dpy, A.surface, EGL_WIDTH, &A.fb_w);
  eglQuerySurface(A.dpy, A.surface, EGL_HEIGHT, &A.fb_h);
  eglSwapInterval(A.dpy, 1);
  fprintf(stderr, "surface %dx%d\n", A.fb_w, A.fb_h);
}
#endif

/* The 60 FPS LOCK: the game's clock is 60 Hz, and a 90 or 120 Hz panel that cannot hold a frame per refresh falls to half of it (45 on a 90 Hz
 * one). ANativeWindow_setFrameRate (Android 11) asks the system to run the screen at 60 Hz for this window, and 0 hands the choice back. Looked
 * up at run time: older systems do not have it, and the lock then does nothing. */
static void frame_lock_apply(void) {
  static int applied = -1;
  static ANativeWindow *win;
  int want = app_frame_lock();
  if (!A.window || (want == applied && win == A.window)) return;
  typedef int32_t (*set_rate_fn)(ANativeWindow *, float, int8_t);
  static set_rate_fn set_rate;
  static int looked;
  if (!looked) {
    looked = 1;
    void *lib = dlopen("libandroid.so", RTLD_NOW);
    set_rate = (set_rate_fn)dlsym(lib ? lib : RTLD_DEFAULT, "ANativeWindow_setFrameRate");
    if (!set_rate) fprintf(stderr, "frame rate: this Android has no ANativeWindow_setFrameRate, the 60 FPS LOCK does nothing\n");
  }
  applied = want, win = A.window;
  if (!set_rate) return;
  int32_t r = set_rate(A.window, want ? 60.0f : 0.0f, want ? 1 /* fixed source */ : 0 /* default */);
  fprintf(stderr, "frame rate: %s (setFrameRate %d)\n", want ? "asked the system for 60 Hz" : "left to the system", r);
}

/* The other half of the lock. A 120 Hz panel is a multiple of 60, so the system may well keep it at 120 whatever the window asks (the Thor does), and
 * then the game would run a frame on every refresh with only every second one holding a step: uneven, and twice the drawing. With the lock on and a
 * display of a whole multiple k of 60 Hz, one frame is drawn per k refreshes instead, each on its own step. The refreshes are counted from the
 * Choreographer's callbacks (their time is the refresh's own, whatever the clock says): the loop waits for the k-th after the last frame began, so
 * the frame it then draws is shown at the refresh after it, for k refreshes. Looked up at run time (Android 7 or later; none: no waiting). */
typedef void (*vsync_cb)(long frame_ns, void *data);
static void *(*choreo_get)(void);
static void (*choreo_post)(void *, vsync_cb, void *);
static void *choreo;
static long vsync_ns;
static int vsync_got;
static void on_vsync(long ns, void *data) {
  (void)data;
  vsync_ns = ns, vsync_got = 1;
}

/* the next refresh's time, waiting for it (0: none came within 60 ms, the surface is gone or the app is in the background) */
static long next_vsync(void) {
  vsync_got = 0;
  choreo_post(choreo, on_vsync, NULL);
  for (int tries = 0; tries < 3 && !vsync_got; ++tries) ALooper_pollOnce(30, NULL, NULL, NULL);
  return vsync_got ? vsync_ns : 0;
}

static void frame_lock_wait(void) {
  static int state; /* 0 not tried, 1 working, -1 not available */
  static double period; /* a refresh, ns (0: not measured since the lock went on or the surface changed) */
  static int k;
  static long begun; /* the refresh the last frame began at */
  static int was_on, off;
  int on = app_frame_lock();
  if (!on || !A.window) {
    period = 0, was_on = 0, k = 0;
    return;
  }
  if (state == 0) {
    void *lib = dlopen("libandroid.so", RTLD_NOW);
    choreo_get = lib ? (void *(*)(void))dlsym(lib, "AChoreographer_getInstance") : NULL;
    choreo_post = lib ? (void (*)(void *, vsync_cb, void *))dlsym(lib, "AChoreographer_postFrameCallback") : NULL;
    if (choreo_get && choreo_post) {
      ALooper_prepare(0);
      choreo = choreo_get();
    }
    state = choreo ? 1 : -1;
    fprintf(stderr, "frame rate: %s\n", state == 1 ? "counting refreshes through the Choreographer" : "no Choreographer, the lock only asks the system");
  }
  if (state != 1) return;
  if (!was_on) was_on = 1, period = 0; /* (measured again: the panel may have changed its rate for the lock) */
  if (period == 0) { /* the refresh period, from 30 of them (a quarter of a second at 120 Hz) */
    long first = next_vsync(), last = first;
    int n = 0;
    while (first && n < 30) {
      long t = next_vsync();
      if (!t) break;
      last = t, ++n;
    }
    if (!first || n < 10) return;
    period = (double)(last - first) / n;
    double hz = 1e9 / period;
    k = (int)(hz / 60.0 + 0.5);
    if (k < 2 || fabs(hz / (60.0 * k) - 1.0) > 0.05) k = 1; /* (60 Hz, or one that is no multiple: 90 Hz has no even split) */
    if (k > 1) fprintf(stderr, "frame rate: the display runs at %.1f Hz: the lock draws a frame every %d refreshes\n", hz, k);
    else fprintf(stderr, "frame rate: the display runs at %.1f Hz, no whole multiple of 60 above 1: the lock waits for no refresh\n", hz);
    begun = last;
  }
  if (k < 2) return;
  /* Wait for the refresh nearest the next game step's time, a 60th of a second after the last one's: that is k refreshes after the last frame, and one
   * fewer now and then when the panel is a little under k * 60 Hz (the Thor's 120 Hz is 118.4), which keeps the game at 60 steps a second, as the host's
   * is, in place of 59.2 and a step added every second or so to keep up. A late frame starts the clock again from where it is. */
  begun += 1000000000L / 60;
  long want = begun, at = 0, prev = 0;
  for (int tries = 0; tries < 12; ++tries) {
    at = next_vsync();
    /* (two refreshes in a row tell the display's rate as it is now: the system may have changed it since it was measured) */
    if (at && prev) {
      if (fabs((double)(at - prev) / period - 1.0) <= 0.25) off = 0;
      else if (++off >= 4) { /* (a late callback now and then is only the phone being busy: four in a row is a new rate) */
        period = 0, off = 0;
        break;
      }
    }
    prev = at;
    if (!at || at >= want - (long)(period * 0.5)) break;
  }
  if (at && at > begun + (long)(period * 2.5)) begun = at; /* (late by more than two refreshes) */
}

static int start_game(void) {
  fprintf(stderr, "start_game\n");
  app_save_dir(A.save_dir);
  /* the game's 1024x768 layout, 768 high on screen; a phone's 16:9 is wider (main.m) */
  A.touch_k = 768.0f / (float)A.fb_h;
  A.touch_edge = ((float)A.fb_w * A.touch_k - 1024.0f) * 0.5f;
  if (A.touch_edge < 1.0f) A.touch_edge = 0;
  app_console();
  app_frame_lock_row(1);
  /* the keys a little smaller than the iPhone's 1.3, under the meters' plate and the shift gate */
  app_wide(A.touch_edge, A.touch_edge > 0 ? 1.2f : 1.0f);
  if (app_init(A.data_dir, A.fb_w, A.fb_h, 1024, 768) != 0) {
    fprintf(stderr, "%s\n", app_error());
    call_str(A.fail, app_error());
    return -1;
  }
  bgm_init(A.data_dir);
  audio_open();
  pthread_t t;
  if (A.load_ctx != EGL_NO_CONTEXT && pthread_create(&t, NULL, loader, NULL) == 0) pthread_detach(t);
  else fprintf(stderr, "no loading thread: the menu frames load\n");
  return 0;
}

/* ---- the game pad (the Retroid Pocket's own controls, a Bluetooth one): what the SDL player does with SDL's controller events, bind.h's BP_* codes
 * for the buttons and the sticks' and triggers' directions, the analog axes for steering, gas and brake; a held one repeats for the menus ---- */
static double PAD_NEXT[BP_COUNT];
static int PAD_HELD[BP_COUNT];

static void pad_input(int code, int down) {
  if (code < 0 || code >= BP_COUNT || PAD_HELD[code] == down) return;
  PAD_HELD[code] = down;
  if (down) PAD_NEXT[code] = now_s() + 0.4;
  app_input(BIND_PAD, code, down, 0);
}

static void pad_axis_input(int code, float v, float on, float off) { pad_input(code, PAD_HELD[code] ? v > off : v > on); }

static void pad_events(const pad_ev *e, int n) {
  for (int i = 0; i < n; ++i) {
    if (e[i].axis < 0) {
      pad_input(e[i].code, e[i].v > 0.5f);
      continue;
    }
    float v = e[i].v;
    app_axis(e[i].axis, v);
    switch (e[i].axis) {
    case AXIS_LEFTX:
      pad_axis_input(BP_STICK_LEFT, -v, 0.43f, 0.24f);
      pad_axis_input(BP_STICK_RIGHT, v, 0.43f, 0.24f);
      break;
    case AXIS_LEFTY:
      pad_axis_input(BP_STICK_UP, -v, 0.43f, 0.24f);
      pad_axis_input(BP_STICK_DOWN, v, 0.43f, 0.24f);
      break;
    case AXIS_TRIGR: pad_axis_input(BP_RT, v, 0.5f, 0.25f); break;
    case AXIS_TRIGL: pad_axis_input(BP_LT, v, 0.5f, 0.25f); break;
    }
  }
  double t = now_s();
  for (int c = 0; c < BP_COUNT; ++c)
    if (PAD_HELD[c] && t >= PAD_NEXT[c]) {
      app_input(BIND_PAD, c, 1, 1); /* (held: the menus move on, the race ignores it) */
      PAD_NEXT[c] = t + 0.12;
    }
}

/* the pad's rumble (haptic.h through app_rumble, as the SDL player's pad_rumble_frame): sent to MainActivity each frame it changes and again every 80 ms
 * while it is on (each lasts 120 ms there, so a frozen game lets it die); 0 once when it ends */
static void pad_rumble_frame(void) {
  static float sent_lo, sent_hi;
  static double sent_at;
  float lo, hi;
  app_rumble(&lo, &hi);
  double now = now_s();
  int on = lo > 0.0f || hi > 0.0f, was = sent_lo > 0.0f || sent_hi > 0.0f;
  int changed = fabsf(lo - sent_lo) > 0.03f || fabsf(hi - sent_hi) > 0.03f || on != was;
  if (!changed && !(on && now - sent_at >= 0.08)) return;
  JNIEnv *env = env_here();
  (*env)->CallVoidMethod(env, A.activity, A.rumble, (jfloat)lo, (jfloat)hi);
  sent_lo = lo, sent_hi = hi, sent_at = now;
}

static void *game(void *arg) {
  env_here();
  if (egl_init() != 0) {
    call_str(A.fail, "no OpenGL ES 3 on this phone");
    return NULL;
  }
  double last = 0;
  int failed = 0, away = 0;
  for (;;) {
    touch_ev tv[TOUCH_MAX];
    int nt, press, done, warn, tilt_new;
    float tilt;
    pthread_mutex_lock(&A.mx);
    while (A.paused || (!A.window && !A.window_changed)) {
      if (A.started && !away && !failed) {
        app_suspend(1); /* the loading thread stops too */
#ifdef WMMT_VK
        vkgl_pipeline_cache_save();
#endif
        if (A.audio) AAudioStream_requestPause(A.audio);
        away = 1;
      }
      last = 0;
      pthread_cond_wait(&A.cv, &A.mx);
    }
    if (A.window_changed) take_window();
    nt = A.n_touch;
    memcpy(tv, A.touches, nt * sizeof *tv);
    A.n_touch = 0;
    pad_ev pv[PAD_MAX];
    int np = A.n_pad, touch_ui = A.touch_ui, touch_ui_new = A.touch_ui_new;
    memcpy(pv, A.pads, np * sizeof *pv);
    A.n_pad = 0;
    A.touch_ui_new = 0;
    press = A.press_start, done = A.movie_done_ev, warn = A.memory_warning;
    A.press_start = A.movie_done_ev = A.memory_warning = 0;
    tilt = A.tilt, tilt_new = A.tilt_new;
    A.tilt_new = 0;
    pthread_mutex_unlock(&A.mx);
    if (!SURFACE_UP) continue;
    if (!A.started) {
      A.started = 1;
      failed = start_game() != 0;
    }
    if (failed) {
      usleep(100000);
      continue;
    }
    if (away) {
      app_suspend(0);
      if (A.audio) AAudioStream_requestStart(A.audio);
      away = 0;
    }
    if (warn) app_memory_warning();
    if (done) {
      A.movie = -1;
      app_movie_done();
    }
    if (press) app_press_start();
    if (touch_ui_new) hud_set_style(touch_ui ? HUD_STYLE_TOUCH : HUD_STYLE_ARCADE);
    pad_events(pv, np);
    for (int i = 0; i < nt; ++i)
      app_touch((void *)(intptr_t)(tv[i].id + 1), tv[i].phase, tv[i].x * A.touch_k - A.touch_edge, tv[i].y * A.touch_k);
    A.wants_tilt = app_wants_tilt();
    if (tilt_new) app_tilt(tilt);
    double now = now_s();
    float dt = last > 0 ? (float)(now - last) : 1.0f / 30.0f;
    last = now;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (getenv("MT3DX_AUTODRIVE") && app_racing()) { /* tests: the accelerator held in a race, only below MT3DX_AUTODRIVE_KMH */
      app_key(APP_KEY_ACCEL, !getenv("MT3DX_AUTODRIVE_KMH") || app_speed_kmh() < (float)atof(getenv("MT3DX_AUTODRIVE_KMH")));
    }
    app_frame(dt);
    frame_lock_apply();
    sync_movie();
    pad_rumble_frame();
    if (app_exit_requested() && !A.exit_sent) { /* the main menu's EXIT: MainActivity closes the app */
      A.exit_sent = 1;
      (*env_here())->CallVoidMethod(env_here(), A.activity, A.exit_app);
    }
    /* depth and stencil need not go out to memory */
    static PFNGLDISCARDFRAMEBUFFEREXTPROC discard_fb;
    if (!discard_fb) discard_fb = (PFNGLDISCARDFRAMEBUFFEREXTPROC)eglGetProcAddress("glDiscardFramebufferEXT");
    const GLenum discard[] = { GL_DEPTH_EXT, GL_STENCIL_EXT };
    if (discard_fb) discard_fb(GL_FRAMEBUFFER, 2, discard);
    double p0 = now_s();
#ifdef WMMT_VK
    /* (the swapchain made again for a new size or rotation) */
    if (vkgl_present() != 0 && A.window) {
      pthread_mutex_lock(&A.mx);
      A.window_changed = 1;
      pthread_mutex_unlock(&A.mx);
    }
#else
    eglSwapBuffers(A.dpy, A.surface);
#endif
    app_frame_done((now_s() - p0) * 1000.0);
    frame_lock_wait();
  }
  return NULL;
}

/* ---- JNI ---- */

#define JFN(name) Java_com_wmmt_port_MainActivity_##name

JNIEXPORT void JNICALL JFN(nativeStart)(JNIEnv *env, jclass cls, jobject activity, jstring data, jstring save) {
  if (A.activity) return; /* a second onCreate (the activity remade): the game goes on */
  (*env)->GetJavaVM(env, &A.vm);
  A.activity = (*env)->NewGlobalRef(env, activity);
  A.show_movie = (*env)->GetMethodID(env, cls, "showMovie", "(Ljava/lang/String;)V");
  A.play_bgm = (*env)->GetMethodID(env, cls, "playBgm", "(Ljava/lang/String;)V");
  A.bgm_next = (*env)->GetMethodID(env, cls, "bgmNext", "(Ljava/lang/String;)V");
  A.bgm_level = (*env)->GetMethodID(env, cls, "bgmLevel", "(F)V");
  A.fail = (*env)->GetMethodID(env, cls, "fail", "(Ljava/lang/String;)V");
  A.exit_app = (*env)->GetMethodID(env, cls, "exitApp", "()V");
  A.rumble = (*env)->GetMethodID(env, cls, "rumble", "(FF)V");
  const char *d = (*env)->GetStringUTFChars(env, data, NULL), *s = (*env)->GetStringUTFChars(env, save, NULL);
  snprintf(A.data_dir, sizeof A.data_dir, "%s", d);
  snprintf(A.save_dir, sizeof A.save_dir, "%s", s);
  (*env)->ReleaseStringUTFChars(env, data, d);
  (*env)->ReleaseStringUTFChars(env, save, s);
  char log[600];
  snprintf(log, sizeof log, "%s/log.txt", A.save_dir);
  log_scrub_file(log); /* (the last run's log: no public address, room code or user name stays in it) */
  freopen(log, "w", stderr);
  setvbuf(stderr, NULL, _IONBF, 0);
  fprintf(stderr, "start (android)\n");
  { /* tests: <save>/test.env, KEY=VALUE lines set as environment variables (MT3DX_AUTODRIVE, WMMT_PILOT, ... as docs/TESTING.md) */
    char tp[600], ln[200];
    snprintf(tp, sizeof tp, "%s/test.env", A.save_dir);
    FILE *f = fopen(tp, "r");
    while (f && fgets(ln, sizeof ln, f)) {
      char *e = strchr(ln, '=');
      ln[strcspn(ln, "\r\n")] = 0;
      if (e && ln[0] != '#') *e = 0, setenv(ln, e + 1, 1), fprintf(stderr, "test env %s=%s\n", ln, e + 1);
    }
    if (f) fclose(f);
  }
  pthread_mutex_init(&A.mx, NULL);
  pthread_cond_init(&A.cv, NULL);
  pthread_t t;
  pthread_attr_t at;
  pthread_attr_init(&at);
  pthread_attr_setstacksize(&at, 8 << 20); /* the recompiled physics has deep frames */
  pthread_create(&t, &at, game, NULL);
  pthread_detach(t);
}

JNIEXPORT void JNICALL JFN(nativeSurface)(JNIEnv *env, jclass cls, jobject surface, jint w, jint h) {
  ANativeWindow *win = surface ? ANativeWindow_fromSurface(env, surface) : NULL;
  fprintf(stderr, "nativeSurface %p %dx%d\n", (void *)win, (int)w, (int)h);
  pthread_mutex_lock(&A.mx);
  if (A.window) ANativeWindow_release(A.window);
  A.window = win;
  A.window_changed = 1;
  pthread_cond_broadcast(&A.cv);
  pthread_mutex_unlock(&A.mx);
  /* the old surface must be let go of before surfaceDestroyed returns */
  if (!surface) {
    for (int i = 0; i < 200; ++i) {
      pthread_mutex_lock(&A.mx);
      int pending = A.window_changed && A.started;
      pthread_mutex_unlock(&A.mx);
      if (!pending) break;
      usleep(5000);
    }
  }
}

JNIEXPORT void JNICALL JFN(nativeTouch)(JNIEnv *env, jclass cls, jint id, jint phase, jfloat x, jfloat y) {
  pthread_mutex_lock(&A.mx);
  if (A.n_touch < TOUCH_MAX) A.touches[A.n_touch++] = (touch_ev){ id, phase, x, y };
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativePadButton)(JNIEnv *env, jclass cls, jint code, jboolean down) {
  pthread_mutex_lock(&A.mx);
  if (A.n_pad < PAD_MAX) A.pads[A.n_pad++] = (pad_ev){ -1, code, down ? 1.0f : 0.0f };
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativePadAxis)(JNIEnv *env, jclass cls, jint axis, jfloat v) {
  pthread_mutex_lock(&A.mx);
  if (A.n_pad < PAD_MAX) A.pads[A.n_pad++] = (pad_ev){ axis, 0, v };
  pthread_mutex_unlock(&A.mx);
}

/* the on-screen touch controls on or off (the race's keys and bars; the touch OPTION rows with them) */
JNIEXPORT void JNICALL JFN(nativeTouchUi)(JNIEnv *env, jclass cls, jboolean on) {
  pthread_mutex_lock(&A.mx);
  A.touch_ui = on ? 1 : 0;
  A.touch_ui_new = 1;
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativePause)(JNIEnv *env, jclass cls, jboolean p) {
  pthread_mutex_lock(&A.mx);
  A.paused = p;
  pthread_cond_broadcast(&A.cv);
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativeTilt)(JNIEnv *env, jclass cls, jfloat r) {
  pthread_mutex_lock(&A.mx);
  A.tilt = r;
  A.tilt_new = 1;
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT jboolean JNICALL JFN(nativeWantsTilt)(JNIEnv *env, jclass cls) { return A.wants_tilt ? JNI_TRUE : JNI_FALSE; }

JNIEXPORT void JNICALL JFN(nativeMovieDone)(JNIEnv *env, jclass cls) {
  pthread_mutex_lock(&A.mx);
  A.movie_done_ev = 1;
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativePressStart)(JNIEnv *env, jclass cls) {
  pthread_mutex_lock(&A.mx);
  A.press_start = 1;
  pthread_mutex_unlock(&A.mx);
}

JNIEXPORT void JNICALL JFN(nativeMemoryWarning)(JNIEnv *env, jclass cls) {
  pthread_mutex_lock(&A.mx);
  A.memory_warning = 1;
  pthread_mutex_unlock(&A.mx);
}
