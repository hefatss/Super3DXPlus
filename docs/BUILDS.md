# Known builds of the arcade game

The tools are written against the Japan 3DX+ `main` and read any other build through a table of translated addresses
(`tools/addrs/<name>.json`, made by `tools/mkaddrmap.py` from the two executables; the build is recognised by the
MD5 of `main`). A build with no table is treated as the Japan one, and its addresses are used as they are.

| Name | `main` MD5 | Notes |
|---|---|---|
| Japan 3DX+ (v386) | `22b9bbcc5a0cefd42a4e83b2dd892c98` | what Re3DX+ was made against |
| Export 3DX+ | `aac96862372ee774a14096792fc40c13` | English; table `tools/addrs/export.json` |

## Japan against Export (3DX+)

The data trees are almost the same; only region folders differ (`sprite` / `sprite-us`, `anime` / `anime-us`, `movie` /
`movie-us`, `cutin/jp` / `cutin/us`) and `config.lua` (the `JAMMA` flag). The `main` executables are different builds of
one code base: the same symbols, but every address moves, so nothing in the tools may hard-code one without going
through `A()` (`tools/wmmtaddr.py`), and nothing in the C code without `RCA()` or the generated `f_` aliases
(`src/phys/rc_addr.h`, made by `tools/recomp/mkrcaddr.py`).

Content differences (from TCRF, confirmed against the data):

- The three **Gemballa** cars (3.8 RS, RSR, Avalanche) are Japan only. The export data ships their folders empty
  (`gem_964_38RS`, `gem_964_RSR`, `gem_997_AVA`) but the executable still lists them: the tools keep their table
  entries (so car ids stay in step with the binary) and write placeholders where data is missing. They must never be
  offered in a car list.
- **Story rivals** are substituted in Export: Gemballa 3.8 RS becomes a Nissan Fairlady Z (Z33) (36 of the 100
  episodes), Avalanche a Subaru Alcyone SVX (3), RSR a Mazda Eunos Cosmo (1), and in one episode the 3.8 RS becomes a Mazda
  RX-8 (SE3P) (not listed by TCRF).
- A few cars are named differently: ROADSTER is MX-5, Mazdaspeed Atenza is Mazdaspeed6, and the Aristo and Celsior taxi
  variants read "Taxi".
- Toyota Aristo and Celsior show a "Japanese model" disclaimer in Export.
- Cards of the two regions are not interchangeable.

## What has been checked

- The generators (`python tools/pipeline.py gen <dump> --phys`) run to the end on both builds; the physics recompile gives 668
  functions and no problems on both.
- 40 distinct car types on three courses (`test/physics_sweep.c`): identical output on both builds.
- The first race of every story episode against its rivals (`test/story_rival_race.c`): 61 identical, 39 differ only by
  the substituted rival car, none differ with the same rivals.

The asset converters (`python tools/pipeline.py convert <dump>`) on the Export dump (first full pass, 2026-11-03):
- Passing, and byte-identical in size to the Japan output: the Tokyo, Osaka, Hakone, Nagoya and Fukuoka maps (day and
  night), their arrows and skies, lights, neon, the starter car, the minimap, the meters, wheels, traffic (4 files), and the
  sound effects (302 files).
- Passing, different from Japan as expected: the cars (208 files against 214: the six Gemballa packs are absent),
  characters (50 against 51), stickers (52 against 55), the HUD (English labels), the attract movies (English soundtrack),
  and `phys.img` (built from each build's own executable).
- Needed changes, all made: the region art lives in `sprite-us`, `anime-us` and `movie-us` in the Export dump (the plain
  folders are empty), so the converters read a view of the dump (`python tools/pipeline.py convert <dump>` builds it); some sprites exist
  only as `.tga` there (`tools/hudpack.py` falls back to them); three scripts hard-coded the Japan folder; the car packer
  skips the cars whose folders are empty.
- The **generator** (not only the converter) reads region text too: `storycut.py` reads the story's dialogue from
  `cutin/jp`, which is empty in the export dump (the files are in `cutin/us`). It used to write an empty table and
  say "done" (0 files, 0 rows; the Japan build has 401 rows), so the export game would have had no in-race dialogue.
  Both the generator and the converter now read the dump through `tools/pipeline.py` (arcade_view), and `storycut.py` stops with an
  error when it finds nothing. After the fix the export build gives 111 files, 2315 rows, 2105 texts. The lesson: a tool
  that reports success on an empty input is a bug; check counts, not exit codes.
- The 10-in-a-row banner is built differently in the export build: `%d racer` with the round's number, where Japan has
  `%säººç›®` and a table of full-width numbers. `tools/menupack.py` handles both, and the addresses that automatic mapping
  cannot find are in `tools/addrs/export.overrides.json`, each with the reason.
- With those, every step of `python tools/pipeline.py convert <dump>` passes on the Export dump (2026-11-03). The last, the menu, gives a
  `menu.bin` of 380,416,803 bytes (13,137 sprites, 338 animations) against Japan's 379,329,680 (13,284 sprites, 338
  animations): fewer sprites, more data, as the English text sprites are larger. Sizes and counts were compared
  file by file, not just exit codes.
- The shipped address table (`tools/addrs/export.json`) is made by `tools/mkaddrmap.py` from the two executables plus
  `export.overrides.json` (hand-checked entries with their evidence). Bytes decide first; cross-references find what the
  bytes cannot place; disagreements are listed for a person. Re-run it, and commit the result, whenever either input changes:
  a stale table in the repo is what failed the menu step twice.
- The Export build runs (native Windows player, RTX 4090, 60 fps) and, walked through its menus with scripted taps and screenshots
  (`MT3DX_TAP`, `MT3DX_SHOT`), shows correct English screens: the title, the save screen, the name entry (Upper case grid), the five
  makers, car and colour select (swatches, logo, drive type), the car setting, the card check (1/9) and "SAVE THIS CAR?".
- Walked further with `MT3DX_SCRIPT` (taps by screen): mode select, time attack (personal best / official machine), course select,
  time zone select, the story's level and rival select, a time attack race, a story race (English dialogue and rival boxes) and a
  10 out run race all show English text.
- Known English gaps: `menu_p_t_tuning_title_aeropart_template` does not exist in the export table (the dress-up part title); the
  story rival select's episode tabs show a stray glyph beside "Story" (the port swaps in the Japanese ç¬¬/è©± glyph sprites there);
  the tuning screens' "STEP" labels keep the full-width digits of the Japan wording; the result screens, garage and dress-up
  screens are not walked, nor are versus and ghost battle (they need the network).

## Text language

The arcade holds both languages in many tables (a title's Japanese text is at +0x14 of its 0x1c byte entry, the English at +0x18)
and its region picks. The tools follow the build: `tools/addrs/<name>.json` carries `"lang"` (from `"_lang"` in the overrides file;
`en` for the export build, `jp` otherwise), `wmmtaddr.lang()` returns it, and what the port writes itself is chosen by it:
`titledb.py` (the title column and so the glyph set: 88 characters in English against 1,573), `menupack.py` `LANG_TEXT` (the default
ranking name and title, the card's horsepower unit "HP", the class suffix " Class "), `hudpack.py` (the plate's "HP/" cut from the
atlas, the Japanese ç´š slot left empty, because the export dump's per-sprite PNGs for them are Japanese placeholders),
`storysel.py` ("Start ramp"), `weartexts.py` ("Equipped !", "Reset to default wheels!") and `cardb.py` (the taxis' colour names).
The Japan build's output is byte-identical to before for all of them.

## Sprite ids differ between the builds

A sprite id is a place in `clImageResourceManager`'s table in `main`, which is sorted by picture name. The two builds have nearly the
same pictures (Japan 8,091 names, 22 of them its own; Export 22 of its own), but the inserts and removals move almost every id: only 5
of 8,091 are the same, and most are 2 to 17 higher in Export. The ids that come from the arcade's own data (`.anm` records,
tables read out of `main`) are always the build's own and right. The ones written by hand in this port were read off the Japan build,
and in the Export build they pointed at other pictures (a swatch placeholder that never got its swatch, the save screen without its
header, a course digit that was a card picture).

The fix is by picture name, with nothing added to the repository but the names of the pictures the code uses (`tools/jpids.json`):

- C: `JID(2239)` is the id this build gives the picture the Japan build has at 2239. `src/spriteids.h` is generated per build by
  `tools/sprids.py` (from `main`) and defines each `JID_<n>`; the result is a constant, so static tables can hold it. In the Japan
  build every `JID_n` is `n`, and the assembly of `menu.c` and `hud.c` was checked identical before and after. `PIC_<name>` gives the
  id of a picture by name (for new code).
- Python: `J(2239)` or `J([...])` in the converters (`menupack.py`, `steff.py`, `storysel.py`); a picture the build lacks is left out
  with a note (Japan's Honda swatches). The id collections `menupack.py` builds were checked identical for Japan.
- `jpids.json` (kept in the repo) is the Japan build's ids; rewrite it from a Japan dump when a new Japan id is written into the code. Do not write a
  new literal id: use `PIC_<name>` / `J` with the name, or the generated data.

## Makers and cars the build does not have

The export `main` still lists the Gemballa cars (kept so that ids and pack numbers stay the Japan build's) but its data folders are
empty and its select screen has five makers. `tools/cardb.py` leaves a leading maker out of the menu list when none of its cars has
data (`CARDB_MAKERS` 5, `CARDB_MAKER0` 1: the port's maker p is the arcade's p + 1), while still numbering its packs and colour names.
The maker row's tap boxes are centred on the screen from the maker count (`MAKER_X0`, `menu.c`).

## Pitfalls found

- The Android flow of Re3DX+ never created `data/cutin/jp/*.txt` (only its iOS script copied them) and its README does
  not list them. Without them every story race start aborts (`rc: indirect call 0` in `Cut::clCutIn::Init`).
  `python tools/pipeline.py convert <dump>` copies them.
- `adb push <folder>` crashed `adb` on the machine this was made on (`std::bad_alloc`) and a half-pushed data set
  runs without sound or car models: `platform/android/deploy.ps1` pushes a tarball and checks file counts.
- `test/rival_test.c` (from Re3DX+) puts its `SERIES` setting into the wrong field, so it runs series 0, which never
  ends. Use `test/story_rival_race.c` for story races.

## Menu fixes (Nov 3)

- **Impreza wheels.** The GC8's, GDB's and GDBF's stock wheel textures (`gc8_wheel01_mid`, `gdb_wheel01_mid`, `gdbf_wheel01_mid`) are stored
  with red and blue exchanged in both dumps (they decode blue; swapped they are the gold of the real wheels). `tools/stage/pack.py` swaps them
  (`RB_SWAPPED`); only the `sub_GC8`, `sub_GC8_RA`, `sub_GDB`, `sub_GDB_specC` and `sub_GDBF` packs change (the aero sets borrow the stock
  pack's textures).
- **The port's text.** `hud_ui_text` (the OPTION screen, key layout, versus lobby, the DONE and METER / BGM keys, TRAFFIC, ...) draws the game's
  own font 3 now, packed into `hud.bin` (`tools/hudpack.py` FONT_ID: its ASCII glyphs and a measures sprite) instead of the old 5x7 pixel
  font; a `hud.bin` from before falls back to the 5x7 one. Each capital is as tall as the old font's seven rows.
- **Time attack flow.** The Personal Best / Official Machine select (`MENU_MODE`) is skipped: both race the same (`official` only reached a log
  line), so the game mode select goes straight to the course select, and back from it returns to the game mode select. The screen's code is
  still there, unreachable.
- **Card check.** The top bar's "(1/9)" is cut from the arcade's picture (`menupack.py` `drop_page_number`): the port has the first page only.
- **Labels.** The maker select's hidden cars key reads "Special Vehicles"; the pre-race tune check has no "TUNE CHECK" title and no
  "TAP THE CAR" hint over DONE.

## Car setting flow

The car setting screen (power / handling balance, STOCK / FULL TUNE, AT / MT) comes up where the arcade has it: right before the race, after
the day / night select (the pre-race check, with DONE). A car already in the save goes from its card check straight to the game mode select,
and back from the game mode select to its card; only a brand new car has the setting first (to choose STOCK or FULL TUNE), then its card and
KEEP, and so sees it twice. The METER / BGM key and the dress-up (car customization) key moved from the setting to the card check; both are
left off a new car's card (its card is made first), and the card check's controls are OK (go on), the view button (METER / BGM) and up
(customization).

## Fixes of Nov 4

- **Rival cars the car select lacks.** `rival_look` (app.c) gave up on a rival whose ECarType has no model in the car select, so
  Export's Tatsuya Shima (a Nissan Z33, `nis_Z33_Shima`, ECarType 1, the Gemballa's type in Japan) never showed in the 10-in-a-row
  (level 1's tenth rival, and later levels). It uses the rival's own model whenever one is packed now; the tyres of a type the car select
  lacks are left at zero with 5 spokes (guarded in the story path as well). Not yet seen in a race (he is the tenth): to test with the user.
- **Gold wheels of the rivals.** Like the Imprezas', the characar `tire*.igb` wheel textures `gtu_low_col1`, `te37_low_col1`, `rg2_low_col2`,
  `ce28_low_col1` and `eigh_low_col2` are stored red/blue exchanged; they are in `RB_SWAPPED` now and 11 (Export) / 13 (Japan) chara packs
  were repacked. Not touched, because they can
  really be blue or orange: `eigh_low_col4` (Z31 Akio), `57f_low_col1` (the FD3S shop demo car).
- **Aero kits painted.** An aero pack that carries its own body textures (FC3S aeroC with its `.tga` names, a Gemballa's kits) was never
  painted; the kits drew in the model's default colour. `wpk_paint` + `wpk_share_sync` now run on the aero scene in the menus, the race and
  the versus rivals.
- **Dress-up colours with a pad.** In the wheel and sticker screens up / down (the shifter) step the colour back / forward like the view button.
- **Name screen.** The ring sits over the cell (`NAME_RING_DX/DY`); the Export build opens on the upper case page, Japan on katakana.
- **Back from the game select** goes to the card, then (the arcade's flow) the garage and the save: a KEEP-yes had set the card's back
  target to the colour select, which skipped the pages after it.
- **Texture filtering.** `<save dir>/graphics` (text, read before the programs are built): `aniso <1..16>` and `lodbias <-3..3>`; env
  `WMMT_ANISO`, `WMMT_LODBIAS`. The SDL player and Android default to 16x and -1.0 (a mip level sharper: the road's lines stay crisp further
  out); the iPad build keeps 0. The bias goes through a `TEXB()` macro the map's fragment shaders sample with (render.c); cars' and UI
  shaders are untouched. Restart to apply.
- **Test script.** A `MT3DX_SCRIPT` with `{NAME}` types the name with the katakana page on Japan (its name screen drops the first inputs, so the script
  starts with two that cancel).

### The same model twice in one save

A save can hold up to `MENU_DUPS` (8) cards of one model. Every per-car array in `menu_state` (tune, parts, story, 10-in-a-row, class,
versus, titles ...) is indexed `[maker][slot]` with `slot = car + MENU_GARAGE_CARS * dup` (`menu_own.dup`, `menu_slot(m)`,
`menu_own_slot(o)`). `menu_slot` is the loaded card's slot (`m->dup`, set by `menu_own_load` and the garage), or, while a new car is
being chosen (the maker, car and colour selects, the setting of a card not yet registered) the model's first free dup, so a second car
of an owned model is a new card with stock parts; `own_register` makes it. Only with all 8 taken does choosing the model again recolour
its first card, as before.

Save format (`save_dir/garage<n>`): a card past the first of its model has " #<dup>" after the label on all of its lines (`own`, `cstory`,
`cclass`, `cten` ..., and the per-model `tune`, `sttune`, `dress`, `wear`, `wheel`, `neon`, `vinyl`, written for such a card only).
Every older save is dup 0 and loads unchanged (checked: all the existing saves of both builds load and write back line for line).
An older build reading a newer file skips the "#n" cards (it does not know the label).

Safety: the file is written to `garage<n>.tmp`, flushed and checked, then moved over the old one; the file replaced is kept as
`garage<n>.bak` (once per run, so it is what the run loaded); a write that would hold fewer cars than the file it replaces keeps that
file as `garage<n>.<time>.fewer`; a `.tmp` left by a cut write is used at load if the garage file is missing. `MT3DX_RESAVE=1` writes
the garage right after the load (a round-trip test).

## Input, ranking and minimaps (Nov 4, later)

- **Held pedals.** The pause (`hud_init`) let go of every key and pad axis, and a press made while the race was not taking input (the
  pause, a beaten 10-in-a-row rival's still) was dropped, with no event for a pedal still held: the gas had to be lifted and pressed
  again. `app_input` / `app_key` track the real held keys (`kb_phys`) and the axes are kept in `menu.axis_raw`; every race frame the HUD
  takes both. Checked with a virtual pad holding the right trigger through a pause, and through a 10-in-a-row rival's handover (the
  arcade pilot steering: `WMMT_PILOT=1`): the car keeps accelerating. The sim itself does not ignore a pedal already down at a handover.
- **Time attack ranking.** `save_dir/ranking` (the machine's, every save together): the 30 fastest runs of each course slot, with name,
  car, horsepower and handling grade. A goal adds its run; the attract loop's ranking screen shows them first (name, time, car, HP,
  handling), the fresh machine's 9'59"999 rows after; the result's "Ranking n/100", TOTAL BEST TIME and the console's RECORD TIME come
  from it (YOUR RECORD stays the save's own best, `records<n>`, which already saved). A kept run shows no class or title (they are not
  kept with it). The export build's default rows had no digits or car names (`3352 + i` and the cars' ids were the Japan build's): fixed
  with `JID`. The handling badges D, G, DG, HG of the ranking are in `menupack.py`'s EXTRA now (menu.bin regenerated). Test hooks:
  `WMMT_TA_GOAL=<km>` (the goal that close; its time is not the real one), `WMMT_TA_FAKE_MS=<ms>` (the time that goes to the records).
- **Minimaps of the other cities.** `pipeline.py convert` only made Tokyo's `navi.bin`; `navi_oosaka`, `navi_hakone`, `navi_nagoya` and
  `navi_fukuoka.bin` are made now (a local dev script (not kept here): `navipack.py <data> <main> <out> <City>Navi.igb navi_<city>.bin`) and are
  in both builds' data folders; the race HUD's map shows the road and the route in Osaka, Nagoya, Fukuoka and Hakone. `pipeline.py convert`
  should make them too.

## Fixes of Nov 4 (night)

- **Prologue.** The prologue has two parts. The English build shows the first (English) and skips the second (the 20XX text in Japanese); Japan shows both. (`WMMT_LANG_EN` is defined as 0 in the Japan build: test it with `#if`, not `#ifdef`.)
- **Back from the HP screen** (the pre-race check) went to the card: the card-back handler ran before the pre-race one; it ignores a check now.
  Back flow time attack: HP, time zone, course, game mode, card.
- **Music.** The select music plays through the time zone's choice and the HP screen and stops as the race loads (it stopped on the HP
  screen). The card, garage and have-a-card screens put the entry music back, so back from the mode select is back to the car's music.
- **After a time attack** the result goes to the course select (it went to the time zone select).
- **Records.** The save's own options had TRAFFIC on (`traffic 1`), and a run with the traffic on kept no record, so nothing was ever saved.
  Every finished run counts now: personal best, car best, the machine's ranking, RECORD TIME and YOUR RECORD.

## Volume mixer, skipping the boot, title screen (Nov 4, night)

- **VOLUME** (OPTION > VOLUME >, `MENU_VOLUME`): overall, car sounds (engine, turbo, tyres, wall, crashes, backfire), music and sound effects
  (the menus' and the race console's), each in tenths; left / right (a tap on the bar sets the level). Kept in `option` as
  `volume <overall> <car> <music> <effects>`. `snd_set_volume` (sound.c) scales the voices by kind; the music's level is
  `app_bgm_level` times overall times music, so the platforms need nothing else. Checked by the mixer's peak: overall 5 gave 62 % of the
  peak, car 0 left only the console's sounds, car 0 and effects 0 gave silence.
- **Skipping the boot.** A key, button or click during the maker logos and the prologue goes to the title (after the first 10 frames).
- **The first "new title" screen** (`MENU_SHOGO` phase 0) showed the rewrite's placeholder ãº in the export build: its frames hold the
  28 px font's place (`menu_f_28_1_font`) where Japan's hold the 40 px one. `shogo_font` takes whichever the frame has, hides both, and
  scales the text to the place.

## Bottom bar (arcade HUD)

With the arcade HUD style (the SDL player's default, `--hud arcade`) the menus' bottom bar is blank: the left / right arrows, the hidden
cars, ALL PARTS, TRAFFIC and UNLOCK ALL CHAPTERS keys stay in the arcade style too; DONE (card settings, the pre-race check) and the "TAP ..."
hints are drawn and tapped only with `--hud touch` (`bar_keys()` in menu.c, `hud_style()` in hud.c). The top bar's buttons (back, gear,
bin, wheel, METER / BGM) and the name screen's page tabs stay. The keyboard and pad controls are unchanged (the view button still turns
TRAFFIC and ALL PARTS on and off, with nothing on screen saying so for now).

## 10-in-a-row sound and Shima (Nov 4, evening)

- **Loops** now crossfade in at the white flash of the next rival's cutscene (`G.ten_bgm_round`, set when the beaten rival's still ends), not
  as the rival is beaten.
- **Car sound** (`snd_car_mute`): off from the race's start, back on a little after the 2 of the countdown (2.65 s left); off with each
  beaten rival's still and on again the same way for the next; after the last rival it stays off until the menus. Ramped over a few
  blocks. The menus' effects and the console's sounds are not the car's. (`MT3DX_LOG` prints each change.)
- **Effects cut off / the missing roar.** The one-shot pool had 4 voices taken in turn, so the rival intro's `approach2` / `approach3` (3.5 s)
  was cut by the next four sounds (turning3_cutin, the 2 and 1 of the countdown). 16 voices, a free one first. The roar is `approach3`
  (SE 0x4e, the table's volume 2.0), played when the rival being introduced is the tenth (`beaten == 9`, 0x8910a4b), as the port does.
- **Shima in the export build's 10-in-a-row.** The sim's table of the rivals' car types (`GetTenFightCarType`, 0x9778fc0, by character) still
  gives Shima's character the Gemballa 964 (type 1), whose car data the export build lacks: the sim never made the rival, and a race
  that has no rival at the front ends at once (the finish in front of the player). `tendb.py` takes the type the story gives the same
  character (the Z33's, 18) and `car_start` writes it into that table for the race's rivals. Not checked against a real run to his race
  (the sim's order of rivals is baked into its image; an edit of the config file does not change it).

## Android: the shader table, the pad, the touch controls (the Retroid Pocket 5 check)

- **`tools/vkprogs` must follow the shaders.** The Vulkan build (`platform/android/vkgl.c`) has no GLSL compiler: every program the game
  compiles is looked up by a hash of its source in a table made from `tools/vkprogs/*.txt` (`tools/vkglsl.py`). A program that is not in it is
  not drawn, and the phone writes its source to the save folder as `<key>.txt` (the world was simply missing after the shaders had changed).
  After a shader change: play the new build on the phone through the menus and a race, pull the `<16 hex>.txt` files from
  `/sdcard/Android/data/com.wmmt.port/files/wmmt/` into `tools/vkprogs/`, delete the superseded ones, rebuild. (A desktop build made with
  `-DWMMT_DUMP_PROGS` and `WMMT_DUMP_PROGS=<dir>` dumps them too, but with the desktop's lod bias, so the keys of the programs using `TEXB` differ.)
- **`glCopyTexSubImage2D` does not exist in vkgl**: `render_screen_env` (the cars' environment map: the screen) blits the frame, scaled down,
  into a small framebuffer's texture there (`senv_blit`).
- **The game pad** (the Retroid's own controls, a Bluetooth pad): `MainActivity` turns key and joystick events into bind.h's `BP_*` codes and
  `AXIS_*` values (`nativePadButton`, `nativePadAxis`), `main_android.c` feeds them to `app_input` / `app_axis` as the SDL player does
  (the sticks' and triggers' directions as buttons, a held button repeating for the menus). The bindings screen rebinds them.
- **The touch controls** (`controls.txt` in the save folder, `touch_controls auto|on|off`): `auto` (the default) hides them (the arcade HUD
  style, `hud_set_style`) once a pad is used and shows them again at the next touch; START + SELECT held for a second steps through
  auto / on / off (a toast says which).
- **Installing over a build with another signing key** fails (`INSTALL_FAILED_UPDATE_INCOMPATIBLE`, the debug keystore is made again with each
  a fresh build folder): uninstall deletes `/sdcard/Android/data/com.wmmt.port` with the 2 GB of data, so move it aside first
  (`adb shell mv .../files/wmmt /sdcard/wmmt_backup`, same volume: instant) and move it back after the install and a first launch.
- A phone that has gone to sleep shows a black screen and the game gets no surface (`dumpsys power | grep Wakefulness`).

## look.txt (the cars' paint and glass)

`<save dir>/look.txt`, written with its defaults on first run and read again whenever it changes: the environment map's gain and contrast,
the windows' alpha and reflection, the specular, and the menus' lighting (see `render.h` render_look). `race_*` is for the race (the car reflects
the frame), `menu_*` for the menus' cars. `WMMT_LOOK=<file>` reads another file (tests).

- **Android's music** is bgm.c's too (`platform/sdl2/bgm.c` with pthreads in place of SDL's): the `.ogg` tracks decoded whole and wrapped sample-accurately in
  the AAudio callback, the 10-in-a-row's next loop crossfaded in. (A `MediaPlayer` loop of the `.m4a` leaves a gap at each wrap; the Java
  `playBgm`/`bgmNext` are no longer called.)

- **The Export build on Android**: `PKG=com.wmmt.port.export LABEL="WMMT3DX Export" bash platform/android/build.sh` from the Export tree (its own generated
  tables) makes an APK that installs beside the Japan one, with a data folder of its own
  (`/sdcard/Android/data/com.wmmt.port.export/files/wmmt`); `am start -n com.wmmt.port.export/com.wmmt.port.MainActivity`. Checked on the Retroid Pocket 5:
  English menus, the car select, a time attack race. The shader table (`tools/vkprogs`) is shared by both builds.

## Headlight flares, tuned parts, the shift light, the attract movies (Nov 5)

- **Headlight flares** (`clCarLampHalo::update`, A(0x8626990)). Each car has a halo at every `HL_NULL_R1..R4` / `HL_NULL_L1..L4` transform of its
  body model (`tools/halodb.py` -> `src/halodb.h`, by pack and body, the story rivals' own cars after the packs; the two cars
  `nsCarLamp::CheckHaloExceptByCarType` names have none). A halo draws `effect/car_frontlight.igb`'s `head02` (a horizontal streak) and `head03`
  (a soft glow) as camera-facing quads 15 units across, put 5 units from the lamp toward the camera, scaled by the distance
  (`min(d / 30, 6)` and `min(d / 20, 10)`) and faded by the cosine between the car's forward axis and the way to the camera (hidden past 90
  degrees: only a camera in front sees them). `head01`, the star, is handed a matrix whose w is 0 (`makeRotationRadians` leaves it so) and
  never shows in the arcade either. The image is `lights.wpk`'s last texture (`pack_lights.py`, rgb x alpha baked: the arcade blends
  SRC_ALPHA, ONE). Visible with the car's lamps (SInfoCarDisp +0x146: the movies' cars always, so story cut scenes show them),
  `WMMT_NO_HALO` turns them off. Found by running the arcade's `update` under the emulator with made-up cameras
  (a local dev script (not kept here) in the dev folder, not kept here).
- **Tuned parts.** `clCarConstructParam` shows `MUFFLER2` for `isTunedMufflerChange` and `ROLLBAR` for `isTunedRollCage`: `IsTuned(car, power,
  handling, part)` is a level against a table (`tools/partdb.py` -> `src/partdb.h`): the exhaust from power level 2 (every car), the roll cage from
  handling level 8 (not the car whose `BNGCarID` is 0x21). The levels are the card's tune (`menu_tune`: the FULL TUNE's balance, else the story's). The
  cars' packs have `MUFFLER1/2` and `ROLLBAR` as chunk groups of their own (`pack.MUFFLER`, `pack.ROLLBAR`). Own car in a race and the dress-up
  screens. `<save dir>/graphics`: `exhaust 0` and `rollcage 0` leave them off. The Celsior (and its taxi) are the only cars with more: its body has `EMBLEM1/2` (gold from handling level 5), `GLASS1/2/3` (smoked from 7, with the Mach sticker from 10) and two sets of rear lamps (clear tail lamps from 2); `partdb.py` reads those levels too, `vip 0` in `graphics` leaves them off. The Aristo has none of them. Test: `WMMT_TUNE_LV="24 24"` with a Celsior.
- **The shift light.** `yt::CMachine::GetRevLamp` is `CEngine::GetRevRate() > 0.86` (A(0x943905c)); `clStandardConsole::update` hands it to
  `clMeterConsole` (+0x29a for the For Racing meter, +0x29c for SPECIAL), and `drawMeter` shows `racing_rev_on_*` in place of `racing_rev_off_*`, or
  `sp2_rev_on_*` where the SPECIAL meter shows nothing. `meterdb.py` records both states (`METER_LAMP_ON` / `METER_LAMP_OFF` parts) and `hud.c`
  draws the one that fits `car->rev_rate`, and it blinks: `CSeq::Get2DInfoMeter` (A(0x89827c1)) passes the lamp on 3 frames of every 5 (the arcade frame counter mod 5 of 2, 3, 4). (The R35 SpecV's own rev ramp, `drawRevRampR35SpecV`, is not done.) Test: `WMMT_METER=1`.
- **The attract movies.** Android plays MP4 (`data/intro.mp4`, `playdemo.mp4`, `tutorial.mp4`, made by `tools/pipeline.py` convert, step movies). About 125 MB for the three.

## Special (custom) body colours

Each car has ten special colours (`BodyCustomColorTexture`, packed after the normal paints; names/swatches from the arcade's
`get_car_color_name_icon`; `cardb_car.custom`). In the arcade a card wins one per 60 plays (its renewals, the first ten, in order; the R35 has none).
In the port: the card screen (reached from the garage) has a COLOR key; it opens the colour select with the car's own colour plus the
special colours the card has won - all ten for a car made with ALL PARTS, one per 60 plays (`card_plays`) for STOCK. The pick is saved per card
(`cplays <plays> <colour> <label>` in the garage file) and is the colour the car races in.

## One card, a car select of named cars

START takes the one card (save 1; the saves screen stays in the code for several cards later) and opens the car select: the card's cars,
five to a page and sorted by last used, each on a tile (car in its parts, its name, horsepower), the last tile a new car's. Each car has its own name
(`cname` lines in garage1, asked after a new car's setting; a car from before them takes the old player name), and `cused` its last drive. The player name of
the rankings and the card is the driven car's. Saves in slots 2 to 4 are not offered any more. In an `MT3DX_SCRIPT`, `{NAME}` types a car's name.

## Main menu, settings, a new car's start

After START the main menu: PLAY (the car select), SETTINGS (GAME the options screen, CONTROLS the key bindings, SOUND the volume mixer, each back to it), EXIT
(the player closes; run by systemd, as the Pi's service is, it shuts the console down: `sudo -n systemctl poweroff`, MT3DX_NO_POWEROFF skips it). They ride
on the versus select's three discs (`sel_menu` in menu.c), drawn by the port until their art is baked into menu.bin. A new kind of car, its colour chosen, asks
STOCK or FULL TUNED: that registers it (FULL TUNED: all parts and the full tune, balanced, which the pre-race setting can move), then its name and its card;
the ALL PARTS key, the setting screen and the keep question are gone from that path. Test scripts: `25:@OK+;37:@OK+;` is START and PLAY, `37:@OK;` STOCK.

## menu_extra.bin (the port's own menu art, in seconds)

`menu.bin` takes 25 to 45 minutes to regenerate, so new art for the port's own screens goes in a second, optional sprite file, `menu_extra.bin`,
beside it (`tools/extrapack.py`; `python tools/pipeline.py convert <dump> --only extras` builds it). The player
(`src/anm.c` `extra_load`) reads it after `menu.bin`: the same sprite table, no animations; a sprite whose id `menu.bin` has takes its place, the
others are added. Without the file or a sprite the screens draw their own discs and text. It holds the selectors' eight discs (ids 110000+: PLAY,
SETTINGS, EXIT, GAME, CONTROLS, SOUND, STOCK, FULL TUNED), their top bar titles (110100+, placeholder wording, the arcade's 512 x 64 title layout) and
"Game options" (replacing option text 10). Copy it next to `menu.bin` on Android too.

### menu_extra.bin's animations, the card check's pages and the bottom bar's key glyphs

`menu_extra.bin` also carries arcade animations (after its sprites: a table of the sprite ids they use, then the animations as menu.bin's; `src/anm.c`
`extra_anims`, `tools/extrapack.py` `EXTRA_ANIMS`). Their sprites that `menu.bin` lacks come with them; the ones it has are used by id. Two features
stand on it, each in seconds' rebuild (a local dev script (not kept here)) rather than the 45 minutes of `menu.bin`:

- **the bottom bar's key glyphs** (`menu_g_bottombar_*`: the wheel turning, the pedals pressed, the view key, each with its label), drawn over the bar
  by the screens that have them (`menu.c` `draw_bottom`, `bottom_kind` chosen per screen at the top of `menu_draw`);
- **the card check's nine pages** (`src/cardpages.h`, "Card data confirmed (n/9)"): the registration (1), story (2), ghost versus battle (3) and its
  record (4), 10 out run (5), versus (6), time attack (7) and the two settings pages (8: transmission, retire, meter; 9: BGM volume, sound source,
  the password). The view key steps through them, the gas confirms; on 8 and 9 up/down chooses the row and left/right the choice (transmission,
  meter, music volume and sound source are the port's own; retire and the password are shown at rest). Figures the port does not keep (ghost
  matches, conquered areas, the driving style, the time attack's cars) show as an empty card's. Their text is the arcade's own fonts 1, 2 and 5 as
  ASCII glyphs (`EXF_BASE`, `ex_text`). New JID(...) ids written into `cardpages.h` need `jpids.json` updated (merge with the old
  `jpids.json`: it alone drops the converters' computed ids).

`tools/stage/pack_lights.py` bakes the street, building and bridge lights' and the goal's glows as rgb * alpha, opaque, so that 565 with a dither
carries their soft edges (a 4444 texture banded them in rings).
