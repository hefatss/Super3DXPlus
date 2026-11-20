# Architecture

## The idea

The arcade game's car physics and race logic are **statically recompiled** from its i386 `main` to C
(`tools/recomp`), so they behave bit for bit as in the arcade. Everything around them (rendering, menus, HUD, sound,
input, saves) is ours, written from reverse engineering. This project keeps that split and makes it explicit, so the same
game runs on several targets.

```
            +--------------------------------------------------------------+
 your copy  | tools/ (Python): read `main` and data/ once, write tables,    |
 of the     |   the recompiled physics (phys_rc.c) and the converted assets  |
 game  ---> |   (data/), per build of the game                              |
            +------------------------------+-------------------------------+
                                           |
            +------------------------------v-------------------------------+
            | src/  platform-neutral C: physics glue (car.c), race and menu |
            |   logic, HUD, render, sound mixing. Talks to the platform     |
            |   through gl.h (GLES2), sound.h and app.h                     |
            +------------------------------+-------------------------------+
                                           |
            +------------------------------v-------------------------------+
            | platform/<target>/  window + GL context, audio output, input, |
            |   storage paths, the main loop:  android/                    |
            +--------------------------------------------------------------+
```

## Builds of the game

The executable differs between regions, so addresses differ. Three mechanisms keep the code build-independent:

1. **Python tools** wrap every hard-coded arcade address in `A(...)` (`tools/wmmtaddr.py`). It looks the address up in
   `tools/addrs/<build>.json`, chosen by the MD5 of the `main` being read (`$WMMT_MAIN`); an unknown build gets the Japan
   addresses unchanged. New tool code must follow this.
2. **C code** names game state as `RCA(98028e4)` and recompiled functions as `f_898317c` (Japan names). The generated
   `src/phys/rc_addr.h` (`tools/recomp/mkrcaddr.py`) maps them to the current build's addresses and also derives the
   memory layout (image, heap, stack) from the image bounds.
3. **The tables** are made by `tools/mkaddrmap.py` from the two executables (shared named symbols as anchors, checked
   against the bytes; PLT stubs matched by imported function name). That is needed only once per new build; the table is
   a list of numbers and is what is committed, not the executables.

Only **one build's generated files** live in a tree at a time (`src/*db.h`, `src/phys/*`, `tools/*.json`... all
git-ignored).

## Where the platform line is

`src/` still has iOS and EGL conditionals from Re3DX+ (`__APPLE__`, `WMMT_EGL`, `WMMT_VK`); Android's code lives
in `platform/android/` (entry point, the Vulkan-under-GLES layer `vkgl.c`, AAudio, the Java shell):

- video: a GLES2 context, or a Vulkan one, at the game's resolution;
- audio: AAudio, with the music decoded whole (`platform/sdl2/bgm.c`, built with pthreads in place of SDL's);
- input: the game pad through the Java shell, turned into **actions** (steer, throttle, brake, shift, menu
  navigate/confirm/back);
- storage: where profiles and settings go (the app's external files folder).

That interface is deliberately about a dozen functions. If it grows past what the Android backend needs, it is
too big.

## Conventions

- Generated or game-derived files are never committed (see `.gitignore`, `NOTICE.md`).
- Long scripts have time limits, logs, and stop at the first failure with a message; they end by saying DONE or FAILED.
- A claim that something works rests on output that was read, not on an exit code.
