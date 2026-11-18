package com.wmmt.port;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.os.CombinedVibration;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.os.Handler;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;

import java.io.File;
import java.io.IOException;
import java.io.FileWriter;
import java.io.BufferedReader;
import java.io.FileReader;

/* The Android side of the port, as main.m is the iOS one: the game draws on a SurfaceView
 * from its own thread (src/main_android.c), the attract movies play in a VideoView over it,
 * the BGM in a MediaPlayer (both decode the AAC/H.264 in hardware); touches, the gravity for
 * steering by tilt and the lifecycle go down to the native side. */
public class MainActivity extends Activity implements SurfaceHolder.Callback, SensorEventListener {
  static { System.loadLibrary("wmmt"); }

  static native void nativeStart(MainActivity a, String dataDir, String saveDir);
  static native void nativeSurface(Surface s, int w, int h);
  static native void nativeTouch(int id, int phase, float x, float y);
  static native void nativePause(boolean p);
  static native void nativePadButton(int code, boolean down);
  static native void nativePadAxis(int axis, float v);
  static native void nativeTouchUi(boolean on);
  static native void nativeTilt(float radians);
  static native boolean nativeWantsTilt();
  static native void nativeMovieDone();
  static native void nativePressStart();
  static native void nativeMemoryWarning();

  private FrameLayout root;
  private SurfaceView game;
  private VideoView movie;
  private MediaPlayer bgm;
  private String bgmName, bgmNextName;      /* the track playing, and the one queued to follow it (bgmNext) */
  private MediaPlayer bgmNextPlayer;
  private float bgmLevel = 1.0f;
  private int xfadeStep = -1;               /* the crossfade's tick, -1 none */
  private String dataDir;
  private SensorManager sensors;
  private Sensor gravity;

  @Override protected void onCreate(Bundle b) {
    super.onCreate(b);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON | WindowManager.LayoutParams.FLAG_FULLSCREEN);
    if (android.os.Build.VERSION.SDK_INT >= 28)
      getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
    root = new FrameLayout(this);
    setContentView(root);
    hideBars();
    /* the data as adb pushes it: /sdcard/Android/data/com.wmmt.port/files/wmmt (needs no
     * storage permission); the player's files next to it. An apk built with the data in it (build.sh EMBED=<data>)
     * copies it there on its first start, then the game starts as with pushed data. */
    File ext = getExternalFilesDir(null);
    File data = new File(ext, "wmmt");
    data.mkdirs();
    dataDir = data.getAbsolutePath();
    sensors = (SensorManager) getSystemService(SENSOR_SERVICE);
    gravity = sensors.getDefaultSensor(Sensor.TYPE_GRAVITY);
    if (needsInstall(data)) installData(data);
    else startGame();
  }

  /* ---- the game's data inside the apk (assets/wmmt): copied to the data folder once per install of the apk ---- */

  private boolean started, resumed;
  private TextView status;
  private static final String DATA_MARK = ".data-installed";

  private String apkStamp() {
    try { return "apk " + getPackageManager().getPackageInfo(getPackageName(), 0).lastUpdateTime; } catch (Exception e) { return "apk"; }
  }

  private boolean needsInstall(File data) {
    try {
      String[] top = getAssets().list("wmmt");
      if (top == null || top.length == 0) return false; /* (an apk without the data: it is pushed) */
    } catch (IOException e) { return false; }
    File mark = new File(data, DATA_MARK);
    try (BufferedReader r = new BufferedReader(new FileReader(mark))) {
      return !apkStamp().equals(r.readLine());
    } catch (Exception e) { return true; }
  }

  private void listAssets(String path, java.util.ArrayList<String> out) throws IOException {
    String[] l = getAssets().list(path);
    if (l == null || l.length == 0) { out.add(path); return; }
    for (String n : l) listAssets(path + "/" + n, out);
  }

  private void installData(final File data) {
    root.setBackgroundColor(0xFF000000);
    status = new TextView(this);
    status.setTextColor(0xFFFFFFFF);
    status.setTextSize(22);
    status.setGravity(android.view.Gravity.CENTER);
    status.setText("Preparing the game data...");
    root.addView(status, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
    new Thread(() -> {
      try {
        java.util.ArrayList<String> files = new java.util.ArrayList<>();
        listAssets("wmmt", files);
        long last = 0;
        byte[] buf = new byte[1 << 20];
        for (int i = 0; i < files.size(); ++i) {
          String path = files.get(i);
          File dest = new File(data, path.substring("wmmt/".length()));
          dest.getParentFile().mkdirs();
          File tmp = new File(dest.getPath() + ".part");
          try (java.io.InputStream in = getAssets().open(path); java.io.FileOutputStream out = new java.io.FileOutputStream(tmp)) {
            long need = in.available();
            if (new android.os.StatFs(data.getAbsolutePath()).getAvailableBytes() < need + (64L << 20))
              throw new IOException("not enough free space (" + (need >> 20) + " MB more needed for " + dest.getName() + ")");
            for (int n; (n = in.read(buf)) > 0; ) {
              out.write(buf, 0, n);
              long now = System.currentTimeMillis();
              if (now - last > 400) {
                last = now;
                final String s = "Installing the game data (first start only)...\n" + (i * 100 / files.size()) + "%\n" + dest.getName();
                runOnUiThread(() -> status.setText(s));
              }
            }
          }
          if (dest.exists()) dest.delete();
          if (!tmp.renameTo(dest)) throw new IOException("could not write " + dest.getName());
        }
        try (FileWriter w = new FileWriter(new File(data, DATA_MARK))) { w.write(apkStamp() + "\n"); }
        runOnUiThread(() -> { root.removeView(status); startGame(); });
      } catch (final Exception e) {
        runOnUiThread(() -> status.setText("The game data could not be installed:\n" + e.getMessage() + "\n\nFree some space and start the game again."));
      }
    }).start();
  }

  private void startGame() {
    game = new SurfaceView(this);
    game.getHolder().addCallback(this);
    game.setOnTouchListener((v, e) -> { touch(e); return true; });
    root.addView(game, 0);
    nativeStart(this, dataDir, dataDir);
    started = true;
    loadTouchMode();
    applyTouchUi();
    if (resumed) {
      nativePause(false);
      if (gravity != null) sensors.registerListener(this, gravity, SensorManager.SENSOR_DELAY_GAME);
    }
  }

  /* ---- the game pad (the Retroid Pocket's own controls, a Bluetooth pad): the buttons and axes go down as bind.h's BP_* codes and AXIS_* ---- */

  private static final int BP_A = 0, BP_B = 1, BP_X = 2, BP_Y = 3, BP_BACK = 4, BP_START = 5, BP_LSTICK = 6, BP_RSTICK = 7, BP_LB = 8, BP_RB = 9,
      BP_DUP = 10, BP_DDOWN = 11, BP_DLEFT = 12, BP_DRIGHT = 13, BP_RT = 18, BP_LT = 19;
  private static final int AXIS_LEFTX = 0, AXIS_LEFTY = 1, AXIS_RIGHTX = 2, AXIS_RIGHTY = 3, AXIS_TRIGL = 4, AXIS_TRIGR = 5;

  private static int padCode(int key) {
    switch (key) {
      case KeyEvent.KEYCODE_BUTTON_A: return BP_A;
      case KeyEvent.KEYCODE_BUTTON_B: return BP_B;
      case KeyEvent.KEYCODE_BUTTON_X: return BP_X;
      case KeyEvent.KEYCODE_BUTTON_Y: return BP_Y;
      case KeyEvent.KEYCODE_BUTTON_SELECT: return BP_BACK;
      case KeyEvent.KEYCODE_BUTTON_START: return BP_START;
      case KeyEvent.KEYCODE_BUTTON_THUMBL: return BP_LSTICK;
      case KeyEvent.KEYCODE_BUTTON_THUMBR: return BP_RSTICK;
      case KeyEvent.KEYCODE_BUTTON_L1: return BP_LB;
      case KeyEvent.KEYCODE_BUTTON_R1: return BP_RB;
      case KeyEvent.KEYCODE_BUTTON_L2: return BP_LT;
      case KeyEvent.KEYCODE_BUTTON_R2: return BP_RT;
      case KeyEvent.KEYCODE_DPAD_UP: return BP_DUP;
      case KeyEvent.KEYCODE_DPAD_DOWN: return BP_DDOWN;
      case KeyEvent.KEYCODE_DPAD_LEFT: return BP_DLEFT;
      case KeyEvent.KEYCODE_DPAD_RIGHT: return BP_DRIGHT;
      default: return -1;
    }
  }

  private static boolean isPad(InputEvent e) {
    int s = e.getSource();
    return (s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
        || (s & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
  }

  private boolean startDown, selectDown;
  private final Handler handler = new Handler();
  private final Runnable chord = () -> cycleTouchMode();

  @Override public boolean dispatchKeyEvent(KeyEvent e) {
    int key = e.getKeyCode();
    int code = padCode(key);
    if (!started || code < 0 || !isPad(e)) return super.dispatchKeyEvent(e);
    int a = e.getAction();
    if (a == KeyEvent.ACTION_DOWN && e.getRepeatCount() > 0) return true; /* (the game repeats the menus' keys itself) */
    boolean down = a == KeyEvent.ACTION_DOWN;
    usedPad();
    /* START and SELECT held together for a second: the touch controls on, off, or automatic */
    if (key == KeyEvent.KEYCODE_BUTTON_START) startDown = down;
    if (key == KeyEvent.KEYCODE_BUTTON_SELECT) selectDown = down;
    handler.removeCallbacks(chord);
    if (startDown && selectDown) handler.postDelayed(chord, 1000);
    nativePadButton(code, down);
    return true;
  }

  private float hatX, hatY;
  private final float[] axes = new float[6];

  /* Android batches a pad's motion events and hands over the latest values with the earlier ones as "history": a D-pad that is a hat (the AYN Thor's)
   * pressed and let go inside one batch looked like nothing happened, and a shift on it was lost. Every sample is taken in order, the history first. */
  private static float sample(MotionEvent e, int axis, int h) {
    return h < 0 ? e.getAxisValue(axis) : e.getHistoricalAxisValue(axis, h);
  }

  @Override public boolean dispatchGenericMotionEvent(MotionEvent e) {
    if (!isPad(e) || e.getActionMasked() != MotionEvent.ACTION_MOVE) return super.dispatchGenericMotionEvent(e);
    for (int h = 0; h <= e.getHistorySize(); ++h) padSample(e, h == e.getHistorySize() ? -1 : h);
    return true;
  }

  private void padSample(MotionEvent e, int h) {
    sendAxis(AXIS_LEFTX, sample(e, MotionEvent.AXIS_X, h));
    sendAxis(AXIS_LEFTY, sample(e, MotionEvent.AXIS_Y, h));
    sendAxis(AXIS_RIGHTX, sample(e, MotionEvent.AXIS_Z, h));
    sendAxis(AXIS_RIGHTY, sample(e, MotionEvent.AXIS_RZ, h));
    sendAxis(AXIS_TRIGL, Math.max(sample(e, MotionEvent.AXIS_LTRIGGER, h), sample(e, MotionEvent.AXIS_BRAKE, h)));
    sendAxis(AXIS_TRIGR, Math.max(sample(e, MotionEvent.AXIS_RTRIGGER, h), sample(e, MotionEvent.AXIS_GAS, h)));
    /* a d-pad that is a hat */
    float hx = sample(e, MotionEvent.AXIS_HAT_X, h), hy = sample(e, MotionEvent.AXIS_HAT_Y, h);
    if (hx != hatX) {
      if (hatX < 0) nativePadButton(BP_DLEFT, false);
      if (hatX > 0) nativePadButton(BP_DRIGHT, false);
      if (hx < 0) nativePadButton(BP_DLEFT, true);
      if (hx > 0) nativePadButton(BP_DRIGHT, true);
      hatX = hx;
      usedPad();
    }
    if (hy != hatY) {
      if (hatY < 0) nativePadButton(BP_DUP, false);
      if (hatY > 0) nativePadButton(BP_DDOWN, false);
      if (hy < 0) nativePadButton(BP_DUP, true);
      if (hy > 0) nativePadButton(BP_DDOWN, true);
      hatY = hy;
      usedPad();
    }
  }

  private void sendAxis(int axis, float v) {
    if (Math.abs(v - axes[axis]) < 0.004f) return;
    if (Math.abs(v) > 0.2f) usedPad();
    axes[axis] = v;
    nativePadAxis(axis, v);
  }

  /* ---- the on-screen touch controls: automatic (the default: hidden while a pad is in use, back at the next touch), on, off; kept in the
   * save folder's controls.txt ("touch_controls auto|on|off"), and START + SELECT held for a second steps through the three ---- */

  private static final int TOUCH_AUTO = 0, TOUCH_ON = 1, TOUCH_OFF = 2;
  private int touchMode = TOUCH_AUTO;
  private boolean padActive;

  private File controlsFile() { return new File(dataDir, "controls.txt"); }

  private void loadTouchMode() {
    File f = controlsFile();
    if (!f.exists()) { saveTouchMode(); return; }
    try (BufferedReader r = new BufferedReader(new FileReader(f))) {
      for (String line; (line = r.readLine()) != null; ) {
        line = line.trim();
        if (line.startsWith("touch_controls")) {
          String v = line.substring("touch_controls".length()).trim().toLowerCase();
          touchMode = v.startsWith("on") ? TOUCH_ON : v.startsWith("off") ? TOUCH_OFF : TOUCH_AUTO;
        }
      }
    } catch (Exception ex) {}
  }

  private void saveTouchMode() {
    try (FileWriter w = new FileWriter(controlsFile())) {
      w.write("# the on-screen touch controls: auto (hidden while a game pad is in use, shown again by a touch), on, off.\n"
          + "# In the game, START and SELECT held together for a second change it.\n"
          + "touch_controls " + (touchMode == TOUCH_ON ? "on" : touchMode == TOUCH_OFF ? "off" : "auto") + "\n");
    } catch (Exception ex) {}
  }

  private void applyTouchUi() {
    nativeTouchUi(touchMode == TOUCH_ON || (touchMode == TOUCH_AUTO && !padActive));
  }

  private void usedPad() {
    if (padActive) return;
    padActive = true;
    if (touchMode == TOUCH_AUTO) applyTouchUi();
  }

  private void cycleTouchMode() {
    touchMode = (touchMode + 1) % 3;
    saveTouchMode();
    applyTouchUi();
    final String s = "Touch controls: " + (touchMode == TOUCH_ON ? "on" : touchMode == TOUCH_OFF ? "off" : "auto (hidden while a pad is used)");
    runOnUiThread(() -> Toast.makeText(this, s, Toast.LENGTH_SHORT).show());
  }


  private void hideBars() {
    getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN
        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
  }

  @Override public void onWindowFocusChanged(boolean f) {
    super.onWindowFocusChanged(f);
    if (f) hideBars();
  }

  private void touch(MotionEvent e) {
    int a = e.getActionMasked();
    if (a == MotionEvent.ACTION_DOWN && padActive && touchMode == TOUCH_AUTO) {
      padActive = false;
      applyTouchUi();
    }
    if (a == MotionEvent.ACTION_MOVE) {
      for (int i = 0; i < e.getPointerCount(); ++i) nativeTouch(e.getPointerId(i), 1, e.getX(i), e.getY(i));
      return;
    }
    int phase = a == MotionEvent.ACTION_DOWN || a == MotionEvent.ACTION_POINTER_DOWN ? 0
        : a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_POINTER_UP ? 2 : a == MotionEvent.ACTION_CANCEL ? 3 : -1;
    if (phase < 0) return;
    if (phase == 3) {
      for (int i = 0; i < e.getPointerCount(); ++i) nativeTouch(e.getPointerId(i), 2, e.getX(i), e.getY(i));
      return;
    }
    int i = e.getActionIndex();
    nativeTouch(e.getPointerId(i), phase, e.getX(i), e.getY(i));
  }

  @Override public void surfaceCreated(SurfaceHolder h) {}
  @Override public void surfaceChanged(SurfaceHolder h, int fmt, int w, int ht) { nativeSurface(h.getSurface(), w, ht); }
  @Override public void surfaceDestroyed(SurfaceHolder h) { nativeSurface(null, 0, 0); }

  @Override protected void onPause() {
    super.onPause();
    resumed = false;
    if (!started) return;
    nativePause(true);
    sensors.unregisterListener(this);
    if (bgm != null) bgm.pause();
    if (bgmNextPlayer != null && xfadeStep >= 0) bgmNextPlayer.pause();
    if (movie != null) movie.pause();
  }

  @Override protected void onResume() {
    super.onResume();
    hideBars();
    resumed = true;
    if (!started) return;
    nativePause(false);
    if (gravity != null) sensors.registerListener(this, gravity, SensorManager.SENSOR_DELAY_GAME);
    if (bgm != null) bgm.start();
    if (bgmNextPlayer != null && xfadeStep >= 0) bgmNextPlayer.start();
    if (movie != null) movie.start();
  }

  @Override public void onTrimMemory(int level) {
    super.onTrimMemory(level);
    if (started && level >= TRIM_MEMORY_RUNNING_LOW) nativeMemoryWarning();
  }

  /* the phone turned like a wheel (main.m's tilt): gravity's angle in the screen's plane from
   * its down, clockwise positive; nothing when it lies flat. In landscape the screen's right is
   * the sensor's -y (rotation 90) or +y (270) and its down the sensor's +x or -x. */
  @Override public void onSensorChanged(SensorEvent e) {
    if (!started || !nativeWantsTilt()) return;
    float gx = e.values[0], gy = e.values[1];
    int rot = getWindowManager().getDefaultDisplay().getRotation();
    float right = rot == Surface.ROTATION_270 ? gy : -gy, down = rot == Surface.ROTATION_270 ? -gx : gx;
    float g2 = right * right + down * down;
    nativeTilt(g2 < 0.04f * 96.2f ? 0.0f : (float) Math.atan2(-right, down));
  }
  @Override public void onAccuracyChanged(Sensor s, int a) {}

  /* ---- called by the game's thread ---- */

  private void dropMovie(VideoView v) {
    if (movie != v || v == null) return;
    v.stopPlayback();
    root.removeView(v);
    movie = null;
  }

  /* the attract movie (data dir's intro.mp4 etc.), or none for null */
  void showMovie(final String name) {
    runOnUiThread(() -> {
      if (movie != null) {
        movie.stopPlayback();
        root.removeView(movie);
        movie = null;
      }
      if (name == null) return;
      File f = new File(dataDir, name);
      if (!f.exists()) {
        nativeMovieDone();
        return;
      }
      movie = new VideoView(this);
      movie.setZOrderMediaOverlay(true);
      movie.setVideoPath(f.getAbsolutePath());
      /* a finished movie is taken off the screen at once: a VideoView keeps its last frame up (the intro's is white), and the game, which only asks
       * for a movie to be shown or gone when the attract loop changes, would never take it down (the AYN Thor: white screen for good) */
      final VideoView mine = movie;
      movie.setOnCompletionListener(mp -> { dropMovie(mine); nativeMovieDone(); });
      movie.setOnErrorListener((mp, what, extra) -> { dropMovie(mine); nativeMovieDone(); return true; });
      /* a tap on the movie is the arcade's "press the gas" */
      movie.setOnTouchListener((v, e) -> {
        if (e.getActionMasked() == MotionEvent.ACTION_DOWN) nativePressStart();
        return true;
      });
      FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
          FrameLayout.LayoutParams.MATCH_PARENT, android.view.Gravity.CENTER);
      /* the intro's picture is letterboxed in its 4:3 (640x326 at y 76): on a wider screen it
       * goes the screen's width, its black bands off the top and bottom; playdemo and tutorial
       * fill their 4:3 and stay so */
      int w = root.getWidth(), h = root.getHeight();
      if (name.equals("intro.mp4") && w * 3 > h * 4) {
        lp.width = w;
        lp.height = w * 3 / 4;
      }
      root.addView(movie, lp);
      movie.start();
    });
  }

  /* data dir's sound/bgm/<name>.m4a over and over, or nothing for null */
  void playBgm(final String name) {
    runOnUiThread(() -> {
      dropBgmNext();
      bgmName = name;
      if (bgm != null) {
        bgm.release();
        bgm = null;
      }
      if (name == null) return;
      File f = new File(dataDir, "sound/bgm/" + name + ".m4a");
      if (!f.exists()) return;
      try {
        bgm = new MediaPlayer();
        bgm.setDataSource(f.getAbsolutePath());
        bgm.setLooping(true);
        bgm.setVolume(bgmLevel, bgmLevel);
        bgm.prepare();
        bgm.start();
      } catch (Exception ex) {
        bgm = null;
      }
    });
  }

  /* the main menu's EXIT: the activity closed and its task removed, and the process ended a moment after (the game thread runs on in it otherwise,
   * and a second start would find the game "going on" without a window) */
  void exitApp() {
    runOnUiThread(() -> {
      finishAndRemoveTask();
      new Handler().postDelayed(() -> android.os.Process.killProcess(android.os.Process.myPid()), 400);
    });
  }

  /* the pad's rumble (haptic.c: low = the strong motor, high = the weak one, 0..1; the game thread repeats it every 80 ms while it is on, so each
   * call lasts 120 ms and 0 stops it): the game pads' own vibrators (a pad with two motors gets one each, the Retroid's built-in controller
   * too, as Android lists it), or the phone's vibrator when no pad has one */
  void rumble(final float lo, final float hi) {
    try {
      boolean on = lo > 0.0f || hi > 0.0f;
      boolean any = false;
      if (Build.VERSION.SDK_INT >= 31) {
        for (int id : InputDevice.getDeviceIds()) {
          InputDevice d = InputDevice.getDevice(id);
          if (d == null) continue;
          int s = d.getSources();
          if ((s & InputDevice.SOURCE_GAMEPAD) != InputDevice.SOURCE_GAMEPAD && (s & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK) continue;
          VibratorManager vm = d.getVibratorManager();
          int[] ids = vm.getVibratorIds();
          if (ids.length == 0) continue;
          any = true;
          if (!on) {
            vm.cancel();
            continue;
          }
          CombinedVibration.ParallelCombination pc = CombinedVibration.startParallel();
          int n = 0;
          for (int i = 0; i < ids.length; ++i) {
            float v = ids.length > 1 ? (i == 0 ? lo : hi) : Math.max(lo, hi);
            if (v <= 0.0f) continue;
            pc.addVibrator(ids[i], VibrationEffect.createOneShot(120, Math.max(1, Math.min(255, Math.round(v * 255.0f)))));
            ++n;
          }
          if (n > 0) vm.vibrate(pc.combine());
        }
      }
      if (any) return;
      Vibrator vib = (Vibrator) getSystemService(VIBRATOR_SERVICE);
      if (vib == null || !vib.hasVibrator()) return;
      if (!on) {
        vib.cancel();
      } else if (Build.VERSION.SDK_INT >= 26) {
        float v = Math.max(lo, hi);
        vib.vibrate(vib.hasAmplitudeControl() ? VibrationEffect.createOneShot(120, Math.max(1, Math.min(255, Math.round(v * 255.0f))))
                                              : VibrationEffect.createOneShot(120, VibrationEffect.DEFAULT_AMPLITUDE));
      }
    } catch (Exception e) {
      /* (a pad that refuses: no rumble) */
    }
  }

  /* the BGM's level (the story ending's fade out; a new track starts at full) */
  void bgmLevel(final float level) {
    runOnUiThread(() -> {
      bgmLevel = level;
      if (bgm != null && xfadeStep < 0) bgm.setVolume(level, level);
    });
  }

  /* the 10-in-a-row's next loop (app_bgm_next, as the SDL player's bgm_next): the track to switch to at once, prepared ahead and crossfaded in over
   * 0.6 s from its start (the one playing fading out); null, or the one playing, drops the queue */
  private static final int XFADE_TICKS = 20, XFADE_MS = 30;

  private void dropBgmNext() {
    xfadeStep = -1;
    handler.removeCallbacks(xfadeTick);
    if (bgmNextPlayer != null) {
      try { bgmNextPlayer.release(); } catch (Exception ex) {}
      bgmNextPlayer = null;
    }
    bgmNextName = null;
    if (bgm != null) bgm.setVolume(bgmLevel, bgmLevel);
  }

  private final Runnable xfadeTick = new Runnable() {
    @Override public void run() {
      if (bgmNextPlayer == null || xfadeStep < 0) return;
      float k = (float) (++xfadeStep) / XFADE_TICKS;
      if (k >= 1.0f) {
        if (bgm != null) bgm.release();
        bgm = bgmNextPlayer;
        bgmName = bgmNextName;
        bgmNextPlayer = null;
        bgmNextName = null;
        xfadeStep = -1;
        bgm.setVolume(bgmLevel, bgmLevel);
        return;
      }
      if (bgm != null) bgm.setVolume(bgmLevel * (1.0f - k), bgmLevel * (1.0f - k));
      bgmNextPlayer.setVolume(bgmLevel * k, bgmLevel * k);
      handler.postDelayed(this, XFADE_MS);
    }
  };

  void bgmNext(final String name) {
    runOnUiThread(() -> {
      if (name == null || name.equals(bgmName)) {
        if (bgmNextName != null) dropBgmNext();
        return;
      }
      if (name.equals(bgmNextName)) return;
      dropBgmNext();
      File f = new File(dataDir, "sound/bgm/" + name + ".m4a");
      if (!f.exists()) return;
      final MediaPlayer p = new MediaPlayer();
      try {
        p.setDataSource(f.getAbsolutePath());
        p.setLooping(true);
        p.setVolume(0.0f, 0.0f);
        p.setOnPreparedListener(mp -> {
          if (mp != bgmNextPlayer) { mp.release(); return; }
          mp.start();
          xfadeStep = 0;
          handler.post(xfadeTick);
        });
        bgmNextPlayer = p;
        bgmNextName = name;
        p.prepareAsync();
      } catch (Exception ex) {
        try { p.release(); } catch (Exception e2) {}
        bgmNextPlayer = null;
        bgmNextName = null;
      }
    });
  }

  void fail(final String msg) {
    runOnUiThread(() -> new android.app.AlertDialog.Builder(this).setTitle("WMMT3DX").setMessage(msg)
        .setPositiveButton("OK", null).show());
  }
}
