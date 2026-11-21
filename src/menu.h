/* The arcade's front end, played from its own .anm animations: the attract loop (movie,
 * title screen, time attack ranking, maker logos), the car select (maker, car on its
 * turntable carousel, body colour), time attack mode (Personal Best / Official Machine),
 * course select, the time zone and BGM select, and the race's title page. */
#ifndef MENU_H
#define MENU_H

#define MENU_VS_PLAYERS 4
#define MENU_VS_FOUND 4
/* the dress-up GET screens a race can bring (a 10-in-a-row run: a part a rival beaten) */
#define MENU_DRESS_DUE 16

/* What a screen reads of the input in one step. Everything that drives the menus makes it: the controls (menu_action) directly, the
 * touch screen and the mouse through the screen's pointer adapter (menu.c menu_pointer: a position on an item becomes
 * "select it", a position on the selected item "OK"). A screen that reads this has no coordinates in it. */
typedef struct {
  int dx, dy;  /* the steering and the shifter: -1, 0, 1 (the bottom keys of the touch screen are dx too) */
  int select;  /* an item chosen outright (a pointer on it): its index, else -1 */
  int tab;     /* a chip, tab or page chosen outright (a pointer on it): its index, else -1 */
  int ok;      /* decide the selected item */
  int back, view;
  int aux;     /* a key of the screen's own (the setting's METER / BGM, the save's "i"): the pointer's, until the controls have one */
  int any;     /* any press at all: the screens that wait to be dismissed */
} menu_in;

/* The race's pause menu: the arcade's continue dialog (clSeqContinueThread: "Continue?" over
 * the YES and NO cards of the time zone select's spin box) in a box over the race. */
typedef struct {
  int open;        /* frames since it opened, 0 = closed */
  int sel;         /* the card at rest: 0 YES (back to the race), 1 NO (back to the menus) */
  int slide;       /* frames left of the slide over to the other card */
  int card_t;      /* frames since the card came to rest */
  int go;          /* the slide ends in deciding the other card (it was tapped) */
  int deciding;    /* frames left of the decided card, then of the fade out (YES) or to white (NO) */
  int act_dx, act_ok, act_back; /* the controls pressed since the last step (menu_action) */
  int tap;                      /* the pointer's tap since the last step, and where (the pause's adapter turns it into menu_in) */
  float tap_x, tap_y;
} menu_pause;

/* clTaResult's figures (its clArg, 0x97fda04), filled by the app as the race ends at the goal */
#define MENU_SECTIONS 7
typedef struct {
  float course_record, car_record, personal; /* s: the course's record, the car's, the player's (after this run) */
  int new_course, new_car, new_personal;     /* this run set them */
  float section[MENU_SECTIONS];              /* s each section took (0 for those the course lacks) */
  float total;                               /* s */
  int top_speed;                             /* km/h */
  int rank;                                  /* the run's place in the machine's ranking (shown up to 100) */
} menu_result;

#define MENU_GARAGE_MAKERS 7 /* CARDB_ALL: the makers and the hidden cars */
/* the port's save slots (MENU_SAVE): what the screen shows of each, filled by the app */
#define MENU_SAVES 4
typedef struct {
  int used;                  /* the save has data */
  int story, medals, cars;   /* story episodes won, 10-in-a-row gold medals (its cars' together), cars */
  long long when;            /* last saved (time_t) */
  short name[5];             /* the player's name (namedb.h's characters, -1 past its end) */
} menu_save_info;
/* A save's cars, each one of the arcade's cards: the car (src/cardb.h's maker and car) and
 * its colour, when it was registered (time_t) and how far it has been driven (m). Its tune,
 * parts, story and 10-in-a-row results are menu_state's per car arrays. */
#define MENU_OWN_MAX 72
typedef struct {
  signed char maker, car, color;
  long long born;
  double odo;
  signed char dup; /* which card of its model (0 the first: a save may hold MENU_DUPS of one model) */
  short name[5];   /* the car's own name (namedb.h's characters, -1 past its end), asked for when the car is registered */
  long long used;  /* when it was last driven (time_t): the car select lists the cars by it, last used first */
  double odo0;     /* the distance (m) when the game started or the card was made: THIS TIME on the card is odo - odo0 (not saved) */
} menu_own;
#define MENU_GARAGE_CARS 12  /* CARDB_MAX_CARS */
/* The per-car arrays below are indexed by a card's slot: its model's car plus MENU_GARAGE_CARS times its dup (menu_slot, menu_own_slot) */
#define MENU_DUPS 8
#define MENU_GARAGE_SLOTS (MENU_GARAGE_CARS * MENU_DUPS)

/* The time attack ranking, the machine's (every save's runs): per course (the time attack slot, 0..14) the best runs, fastest first,
 * for the attract loop's ranking screen, the result's place and the console's RECORD TIME. */
#define MENU_RANK_COURSES 15
#define MENU_RANK_KEEP 30
typedef struct {
  int ms;
  short name[5];            /* namedb.h's characters, -1 past its end */
  signed char maker, car;   /* src/cardb.h */
  short hp;
  signed char grade;        /* the handling grade (car_tune_grade) */
  signed char cls;          /* the car's class when it ran (GetClass 0 N .. 46 SSS) */
} menu_rank_row;

typedef struct {
  int screen;      /* MENU_MOVIE ... MENU_PRE */
  int t;           /* 60 Hz frames since the screen (or its current move) started */
  int clock;       /* 60 Hz frames since the menus first ran: the backgrounds' and glyphs' own motion (kept across screens) */
  int sel, from;   /* selected item; item a move started from (-1 = idle) */
  /* the course select's strip (clThread_6_2_CourseSelect's spin box): the selected card's
   * place on screen, and the strip's shift in cards before the move (it slides 88 px a card) */
  int course_pos, course_scroll_from;
  int deciding;    /* frames left of the "decided" animation, then the race starts */
  int tap;         /* pending tap on the screen */
  float tap_x, tap_y;
  int key;         /* pending left/right key: -1, +1 (0 = none) */
  int key_held;    /* key under the finger, for drawing */
  void *back_touch; /* the finger on the back key: it goes back when let go on the key */
  int game;        /* the game mode select's choice: MENU_GAME_STORY, _TEN, _TA, _VS */
  /* the 10-in-a-row (mseqTen): the course select's choice (its card, the course in menu
   * order: src/tendb.h), the level select's (0..9, 10 the 32-man extend) and the levels it
   * offers (11 once every level has gold); the medal of each course (clEffTenCourseType)
   * and level as the card keeps it (getTenResult: 0 none, 1 bronze, 2 silver, 3 gold; the
   * app keeps them, data_dir/garage); the level select's faces: the level they show and the
   * frames since they popped in */
  int ten_course, ten_level, ten_levels;
  int mip;      /* the textures' sharpness (HUD OPTIONS): 0 soft .. 3 sharpest (the mip bias 0, -1, -2, -3), 2 by default */
  int rum[4];   /* the controller vibration (MENU_RUMBLE), each in tenths 0..10: overall, wall hits, expansion joints, tyre slip */
  int cam_shake; /* the camera shake in tenths 0..10 (CAMERA SHAKE in the game options): 10 is the arcade's own, 0 a steady view */
  int rum_row;  /* the vibration screen's row */
  int disp_row; /* the display options screen's row (MENU_DISPLAY, PC) */
  int gfx_row;  /* the graphics screen's row (MENU_GRAPHICS) */
  int smooth_on; /* SMOOTH MOTION (graphics screen, saved, on by default): a race is drawn between the physics' steps where the display needs it (app.c want_smooth) */
  int fps_row, lock60; /* Android's 60 FPS LOCK row in the graphics screen (fps_row, set by the app), and the switch (saved): asks the system for a 60 Hz screen */
  int net_row, net_ping, net_log, net_act; /* the network options screen (MENU_NETWORK): its row, PING ON THE HUD, DETAILED NET LOG (both saved), and the action it asks of the platform */
  char net_lines[6][72]; /* its connection lines, filled by the app */
  int ten_beep; /* the 10 out run's beep as the rival's line is near (HUD OPTIONS), on by default */
  int ten_set; /* the 10 out run's soundtrack chosen on the BGM select: 0..4 (app.c's TEN_LOOP rows), 5 none */
  signed char ten_result[10][11];
  int ten_face_lv, ten_face_t;
  int story_sel;   /* the story select of the race ahead (the app sets it; src/storypre.h), -1 none */
  /* The story's progress as the arcade's card keeps it (clV386CardData): st_div counts the
   * divisions cleared (getStoryClearDivCount: the loop is st_div / 5, the division st_div % 5,
   * 20 episodes each, four levels of five), st_bits the division's cleared episodes. The app
   * keeps them (data_dir/garage); menu_story_clear moves them on. */
  int st_div, st_bits;
  int st_level;    /* the level select's choice */
  int story_pick;  /* the rival select's choice: its story index (src/rivaldb.h's select), -1 none */
  /* the story's poem after a won select (MENU_POEM, seqStPoem; menu_story_points sets it): its
   * row of menu.c's POEMS, -1 none; the screen's frame */
  int poem, poem_t;
  /* the story's ending after episode 100 (seqRaceEnding; menu_story_points sets it): 1 due
   * (after the poem), 2 the app plays it (the select 111's realtime movie with the staff roll
   * over it, ending_t its frame, ending_skip a tap); menu_ending_done after */
  int ending, ending_t, ending_skip;
  /* the bonuses after the ending (MENU_BONUS, mseqStory): per car, the card's flags (1 the
   * story clear bonus given: +0x279, 2 the undefeated one: +0x186, 4 the second: +0x278, 8 a
   * story race lost this loop: hasStoryLose +0x187); the ones due (the same bits 1, 2, 4), the
   * one up and its frame */
  unsigned char st_bonus_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* The card's settings (MENU_CARDSET, the card check's cardcheck_7 and _8, from the car
   * setting): its meter (clV386CardData +0x26c: 0 Normal, 1 For Racing, 2 SPECIAL, which the
   * second undefeated bonus unlocks) and BGM sound source (0 3/3DX/3DX+, 1 Maximum Tune & 2:
   * the clear bonus, 2 Wangan Midnight R: the undefeated bonus), per car */
  signed char meter_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], sound_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int bonus_due, bonus_kind, bonus_t;
  int story_n;     /* and its place in the loop, 20 * division + 5 * level + episode */
  int course, official; /* the choice, once a race is ready */
  int demo;        /* rounds of the attract loop so far: picks the movie and the ranked course */
  int flash;       /* frames left of the white flash over the mode select */
  int bars;        /* the top and bottom bars slide in while t < 11 */
  int lock;        /* frames the arcade ignores input after a move */
  int back;        /* pending tap on the back button */
  int leaving;     /* the back button on the maker select: fading to white, then the title screen */
  int maker, car, color; /* the car select's choice (src/cardb.h; maker CARDB_HIDDEN: the hidden cars) */
  /* the maker select's "Special Vehicles" key (the hidden cars) pressed; the maker under the cursor
   * when it was (where the maker select comes back to) */
  int hidden, hidden_go, hidden_from;
  /* the colour select's ALL PARTS key (a new kind of car's, in the hidden cars key's place):
   * on, the card is made with every part and tune it can have (own_register: the dress-up
   * level 63, the story's tune full) */
  int all_parts;
  float rot, rot_speed;  /* the car carousel: angle (radians) and direction of its turn */
  float spin;      /* the body colour turntable's heading, degrees */
  int bgm;         /* the BGM select's line: 0 RANDOM, 1..20 the songs, 21 NO BGM */
  int song;        /* the race's song (1..20), or -1 for none, once a race is ready */
  int music;       /* the menus play the entry music (0) until the mode select, then the select music (1) */
  int zone;        /* the time zone select's card, the race's map: 0 morning, 1 night */
  int zones;       /* the time zones there are maps for (the app sets it; 1: morning only) */
  int slide;       /* frames left of the cards' slide over to the other time zone */
  int card_t;      /* frames since the card came to rest (its ring flashes once) */
  int ready;       /* the app sets it once the chosen map is loaded: the title page waits for it */
  int loading;     /* the app counts the frames the car select's cars on show (or the race title's course) have been loading */
  int load_shown, load_out; /* the race title's NOW LOADING: the frames it showed, then those of its fade once done */
  float acc;
  menu_pause pause;
  int finish;      /* frames since the race ended (menu_finish_open), 0 = none */
  int finish_wait; /* frames the race runs on before FINISH or TIME UP shows */
  int finish_goal; /* it was the goal (FINISH), not the time running out (TIME UP) */
  int finish_out;  /* frames since the result was left (the fade to black), 0 = still up */
  int pre_quiet; /* MENU_PRE between a story select's races: no title page (back_from_race) */
  int finish_story; /* a story race's end (menu_finish_story): 1 WIN, 2 LOSE, 3 on to the next race,
                    * 4 the 10-in-a-row's (menu_finish_ten), 0 the time attack's */
  int finish_streak; /* the episodes won in a row, with this win (the WIN result's "N WINS" from 2 on) */
  /* the mileage tally at the end of a session (MENU_MILAGE, menu_layout_milage): the card, the km it drove this time and its total before, the frames since it began */
  int milage_card, milage_t, milage_race; /* milage_race: it came after a single player race (back to the menus after it), not from the title */
  long milage_this, milage_total0;
  /* on to the next race: how the race fades out (src/storymovie.h's STORYRACE_FADE: 1 black,
   * 2 white, 0 none), and so the screen while the next one loads and what it fades in from
   * (pre_fade; the app takes it at the start) */
  int finish_fade, pre_fade;
  menu_result result;
  /* The car setting screen's choices: per car of the car select (src/cardb.h's makers and
   * cars), stock or full tune, and the full tune's balance as its power (the handling has
   * the rest of CAR_TUNE_TOTAL); and the gearbox, AT (0) or MT (1). menu.c sets
   * garage_changed when one changes; the app keeps them (data_dir/garage) and clears it. */
  signed char tune_full[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], tune_power[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int trans_full; /* the starting point chosen (FULL TUNED), kept for the transmission select that follows */
  int manual;
  /* The port's OPTION screen (MENU_OPTION, the gear key on the maker select): steering by
   * the touch keys (0) or by tilting the iPad (1), the tilt's sensitivity (1..10) and the
   * bridge reflection on the car (on 1), the FSR sharpening (on 1), the cars' baked ambient
   * occlusion (on 1) and the back mirror (on 1); kept with the garage too. */
  int gyro, gyro_sens, over_on, sharp_on, pedal_slide, ao_on, bm_on;
  /* the HUD options (MENU_HUDOPT): the input readout over the race, the fps counter in the arcade HUD (the touch HUD always has it); the back
   * mirror (bm_on) is among them; hud_row the row the controls are on */
  int readout_on, fps_on, hud_row;
  int phys_on; /* the physics panel (hud.c draw_phys), the HUD options' fifth row; F4 flips it */
  int filter_on, filter_smooth, filter_counter, btn_steer; /* (the two in half steps, 0..20) */ /* the steering filter (car.h car_set_filter): off = direct, the AXES list's last row and the race's tuning panel (F5) set them */
  /* the volume mixer (MENU_VOLUME, OPTION > VOLUME): overall, the car's, the music's and the effects', each 0..10 (tenths); vol_row the row on */
  int vol[6], vol_row; /* (rows 4, 5: the engine and the turbine against each other, 5 = as the arcade, 0..10 = 0..200%) */
  /* and on Android (hq_rows, set by the app with render_hq on), the GRAPHICS rows in two
   * columns with its own: the AI-upscaled ASTC maps, the bloom, the screen-space ambient
   * occlusion, the wet road, the paint's reflection and the anti-aliasing (4x 1, 2x 0, from
   * the next start) */
  int ctl_touch; /* the last input was a touch (menu_touch) and not a pad or key (menu_action): the CONTROLS screen shows the touch controls */
  int hq_rows, hd_tex, bloom_on, ssao_on, wet_on, paint_on, aa4, milage_on; /* (paint_on: the paint's clear coat, now the REFLECTION row's, over_on) */
  /* and on an iPad (res_row, set by the app), the screen's resolution: 1024x768 (0) or the
   * Retina iPads' 2048x1536 (1), from the next start (main.m reads it from the options) */
  int res_row, hires, res_phone;
  int black_lv; /* the PC player's BLACK LEVEL row (0 off .. 10): the picture's blacks pulled down, for a display or driver that lifts them */
  int fw_row; /* the Windows player's rows in the network options: the firewall and the log folder (app_firewall_row) */
  int win_row, fullscreen; /* the PC player's WINDOW MODE row (win_row), and the mode: 0 windowed, 1 fullscreen */
  /* the gear key pressed; the maker under the cursor meanwhile; the screen it was pressed on
   * (the maker select or the SAVE DATA screen) */
  int option, opt_sel, opt_from;
  /* The testers' hidden switch: on the OPTION screen a finger held 3 s on the
   * CONTROLS band steps it off -> episodes 1-20 -> 21-40 -> .. -> 81-100 -> the second loop's
   * 1-20 .. 81-100 -> off. On (story_test = the tested st_div + 1), the story's level select
   * shows that division with every episode open, and its races change no progress (no clear,
   * no tuning points). Kept with the options; test_hold the frames held, test_note the note's. */
  int story_test, test_hold, test_note;
  /* The pre-race tune check: the car setting shown again right before a race
   * loads (a story episode chosen, the time attack's or 10-in-a-row's time zone, the versus
   * kind before its lobby), DONE on to it; tune_confirm the screen it came from (0 none) */
  int tune_confirm;
  /* the time attack's traffic: the course select's TRAFFIC key, off as the
   * arcade's (its hidden button set SConfigPlay +0xac); kept with the options */
  int ta_traffic;
  /* an owned car's colour before the colour select's choice went onto it (-1 none): its card's
   * back key gives it back (only the card's tap keeps the new one) */
  int card_color_was;
  /* The OPTION screen's KEY LAYOUT (MENU_LAYOUT): the race's touch keys by group
   * (hud.h HUD_KEYS_*) their size (percent, 60..160) and move (layout px), kept with the
   * options; the editor's chosen group, the finger dragging it (lay_drag) from where (lay_x, y
   * less the group's move), and a button of its panel tapped (lay_tap, 1 + LAY_*) */
  int key_k[5], key_dx[5], key_dy[5]; /* (HUD_KEY_GROUPS) */
  /* and the consoles' (hud.h HUD_EL_*, 7 of them): size percent (50..200) and move; lay_sel counts the key groups first (0..2), then these (3..9) */
  int el_k[7], el_dx[7], el_dy[7];
  int lay_sel, lay_drag, lay_tap;
  void *lay_touch;
  float lay_x, lay_y;
  /* The dress-up menu the game mode select's wheel key opens (MENU_TG_SELECT, the arcade's
   * seqTgDressUpPartsSelect: there only once a car has won every part): the key pressed, the
   * menu open (its change screens can go back, the wear comes back to it), the kind of part
   * last chosen (its place is where the menu opens, -1 none) */
  int dress, dress_free, dress_kind;
  /* the save's car view's delete key pressed (erase), once to arm it, again to delete */
  int erase, erase_armed;
  int garage_changed;
  /* The port's SAVE DATA screen (MENU_SAVE, between START and the maker select; the
   * arcade's card check stands there): NEW GAME or LOAD GAME lit, the save of a NEW GAME
   * tapped once (it must be tapped again to start over in it, 0 none). The app keeps the
   * save in use (save_slot, 1..MENU_SAVES, 0 none yet), fills save[] when save_scan is set
   * and takes save_pick (the save chosen, 1..; save_pick_new: started over) once set;
   * save_skip (a test) has START go straight to the maker select. */
  int save_new, save_confirm, save_slot, save_scan, save_pick, save_pick_new, save_skip;
  /* DELETE lit (a tapped save then tapped again is deleted: save_erase, 1..MENU_SAVES, which
   * the app takes) */
  int save_del, save_erase;
  menu_save_info save[MENU_SAVES];
  /* The save's cars (n_own of them) and the one driven (own_sel, -1 none yet): st_div,
   * st_bits and ten_result are its, kept per car in st_div_car, st_bits_car and ten_car
   * (menu_own_store puts them back, menu_own_load takes a car's). HAVE (the arcade's "do
   * you have a card?", clSeqCardHaveSelectThread) offers the save's cars (GARAGE, the port's
   * look at them, then CARD, the card's first page, clSeqCardDisp1) or a new car (MAKER ...
   * COLOR, then CARD for the car registered); have_sel the card at rest (0 YES, 1 NO),
   * own_view the car GARAGE shows, card_back the screen CARD goes back to. */
  menu_own own[MENU_OWN_MAX];
  int n_own, own_sel, own_view, have_sel, card_back;
  int card_page, card_row; /* the card check's page shown (0 .. 8) and the row on, on its settings pages */
  int grid_sel; /* the car select's tile (a car's place, or n_own: a new car's) */
  int sel_age;        /* frames since the port's own selector opened (the fade in: t is reset by each move) */
  int sel_menu;       /* what MENU_VS_SEL's three discs are: 0 the versus kinds, 1 the main menu (PLAY, SETTINGS, EXIT), 2 the settings (GAME, CONTROLS, SOUND), 3 a new car's STOCK / FULL TUNED */
  int trans_edit;     /* the transmission select opened from the Car / Settings discs (a car already made): it goes back there, not on to a card */
  int carmenu;        /* the dress-up or the METER / BGM screen opened from the Car / Settings discs: leaving goes back to them */
  int settings_direct; /* the controls or sound screen opened straight from the settings selector: back goes to it, not to the OPTION screen */
  int exit_req;       /* EXIT chosen: the player closes (or the console shuts down) */
  int name_car;  /* the NAME screen names a car just registered (not the player), then its keep question */
  int save_auto; /* the one card taken without the save screen: the screen after it follows once the app has loaded it */
  menu_rank_row rank_row[MENU_RANK_COURSES][MENU_RANK_KEEP];
  unsigned char rank_n[MENU_RANK_COURSES];
  int dup; /* the dup of the card loaded (menu_own_load); menu_slot gives a new car's free one while one is chosen */
  /* The player's name (the card's five characters, namedb.h's, -1 past its end), asked for
   * by NAME (clSeqCardNameSelectThread) as soon as a NEW GAME starts: the grid's page
   * (0 katakana .. 3 small letters), the key last tapped (its place, -1 none) and the frames
   * left of its pop, of the character last put in. */
  short name[5];
  short new_name[5]; /* a new car's name, kept here from the NAME screen to its registering (m->name stays the car driven's, whose card it is stored on) */
  int new_named;
  int name_page, name_key, name_pop, name_char_pop;
  int st_div_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], st_bits_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  signed char ten_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS][10][11];
  /* each car's class (the card's +0x172: 0 N, C9..C1, B9..B1, A9..A1, S9..S1, SS9..SS1, 46
   * SSS; menu_class_update) */
  signed char class_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* The story's own tune per car, as the arcade's card keeps it (clCarStyle getTunePower,
   * getTuneHandling, 0..24 each) and the story points gauge (card+0x178, 0..19 units of 50
   * points). A full gauge (menu_story_points) sets st_tune_due: the tuning screen
   * (MENU_ST_TUNE, seqStTune) comes up on the way back from the race. */
  signed char st_power[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], st_handling[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  signed char st_gauge[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* dress-up (the arcade's ghost battle parts, get_tb_level_parts): each car's dress-up level
   * (0..63, the parts it has won) and the aero set it wears (0 stock, 1..3 A, B, C) */
  signed char dress_lv[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], aero[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* the special colours (the arcade's custom colours, car.lua's BodyCustomColorTexture): the races a card has been through, and the colour
   * it wears (0 its own, n the nth special one). A card gets one more special colour for every 60 plays (the arcade's card renews when its 60
   * overhaul plays run out, and each of the first ten renewals hands one out: mseqFlowRenewal), up to ten; ALL PARTS has all ten. cc_mode:
   * the colour select is on them (the card's COLOR key), cc_back the card screen's way back. */
  short card_plays[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  signed char ccolor[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int cc_mode, cc_back;
  /* and the parts it wears, as the card keeps them for clCarConstructParam: the aero mirror
   * (card+0xc: 0 stock, 1), the wing (card+7: 0 stock, 1..3, 4 none) and the bonnet (card+6:
   * 0 stock, 1..3); render_params' mirror, wing, bonnet */
  signed char mirror[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], wing[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  signed char bonnet[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* and its wheel (card+0x10): 0 its own, else the dress-up wheel's emTireModelID + 1 */
  unsigned char wheel[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* and its under neon (card+0x1a, clCarConstructParam::setNeon): 0 none, 1..6 the straight
   * neons (green, blue, purple, red, yellow, dark purple), 7..12 the spot neons */
  signed char neon[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* and its sticker (vinyl: car.lua's VinylTexture entry, 0 none; 1..10 line 1 in its colours,
   * 11.. line 2, 21.. fire 1, 31.. fire 2, 41 namco, 42 pac-man, 43 works, 48.. line 3,
   * 58.. fire 3, 68.. tribal) */
  unsigned char vinyl[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int st_tune_due;
  /* the episode (story_n) lost in a row and how often (clWorkSpace +0x24, the rivals' ease) */
  int st_lost_n, st_lost;
  /* the car setting's STOCK / FULL TUNE fixed: a car picked from the save's (its card's mode
   * kept); a new card chooses */
  int tune_locked;
  /* a car of a kind not in the save, its colour chosen: the setting (its mode first) comes
   * before its card, and the card is made when the setting goes on */
  int new_card;
  /* HAVE asking whether to keep the new car in the save (after its card) */
  int keep_ask;
  /* the class-up screen (seqLvUp, MENU_LVUP), the first on the way back from a race whose
   * card's class rose (menu_lvup_check): the class before and after, the frame its loop
   * started (0 till a tap or the intro's end) */
  int lvup_due, lvup_from, lvup_to, lvup_loop;
  /* The titles (称号, src/title.h): each car's (the card's +0x170: 0 not set yet, 湾岸の新人),
   * its story wins in a row (getStorySuccessiveVictoryCount) and its versus wins; the games
   * played since the app started (clPlayerWork +0x24, the continues). title_judge's new
   * titles for the race just run (title_n of them) wait for the title select (seqTitleSelect,
   * MENU_SHOGO), after the class-up on the way back from the race: GET (all of them), the
   * spin box (YES take it, NO keep the one the card has) for each (title_k), then REWRITE
   * if one was taken (title_taken); title_f the phase's frame, title_dec the frames since the
   * spin box was decided (0 not yet), title_out the frames of the phase's fade out (0 none). */
  short title_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], st_streak_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int vs_wins_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  /* The versus stars (the card's +0x180, clRaceThread::finiJoin) and versus count (+0x17c) of
   * each car; the stars screen after a versus (MENU_VS_STAR, clSeqJoStarGetThread): the stars
   * before the race and the race's change (menu_vs_stars; the screen only for one), its frame */
  int vs_stars_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS], vs_count_car[MENU_GARAGE_MAKERS][MENU_GARAGE_SLOTS];
  int star_from, star_delta, star_t, star_skip;
  float star_spin;
  int plays;
  int title_new[3], title_n, title_k, title_phase, title_taken, title_f, title_dec, title_out;
  /* the tuning work (seqStTuneEff, MENU_ST_EFF) after the tuning screen: the row (steff.h:
   * the top bar's title, the part's picture, the install anime, the car's pose, the lines),
   * power tuned (else handling), T (power + handling after), the horsepower before and
   * after, a step that ends a stage (T 20, 30, 34: the complete picture), the tuner of
   * that picture; the frame the work's main part started (the text box on), the frame the
   * complete picture came on, the 10 s (frames left), the frames left after a tap, the
   * car's turn (degrees) and whether it is the new one (the pose's second set) */
  struct {
    int title, parts, anim, pose, text, power, total, hp0, hp1, milestone, chara;
    int main_t, complete_t, timer, quit, after;
    float angle;
  } eff;
  /* dress-up levels won since the last GET screens (menu_dress_win queues them, the way back
   * from the race shows them: MENU_TG_GET each, MENU_TG_AERO after an aero set's,
   * MENU_TG_PART after a bonnet's, wing's or mirror's, MENU_TG_WEAR after either) */
  int dress_due[MENU_DRESS_DUE], n_dress_due;
  /* the GET screen (clSeqTgPartsGetAeroThread) and the aero set's change
   * (clSeqTgChangePartsAeroThread): the level and part shown, the car turning on the GET
   * screen (degrees), the change screen's choices (the aero set of each, -1 Refuse to
   * change), how many, the newest, the move under way (+1, -1 or 0, and its frame), each
   * choice's model's depth offset (0 near .. -400); the bonnet's, wing's and mirror's
   * change (clSeqTgChangeParts*Thread): the layout's places (the choices and the parts not
   * won yet) and the frame the rest loop started */
  struct {
    int lv, kind, num, name, image, gem;
    float angle;
    int opt[8], n, newest, move, move_t, decided;
    float z[8];
    int slots, rest_t;
    /* the part worn (clSeqTgWearPartsAeroThread, after the change): the value worn before
     * and the one chosen (the part's number), the same one kept (wk_s +0x42: no banner),
     * the yield's state (0 the old part, 1 white, 2 the new one, 3..5 the banner) and its
     * timer, the run's 8 s (frames left), the car's turn (degrees about y) */
    int old, chosen, same, state, timer, left;
    float ry;
    /* the white flash (menu_p_whiteout): the first frame of its run (-1 none) and when */
    int white_from, white_at;
    /* the wheel's change (clSeqTgChangePartsWheelThread): the places (menu_state.wheel's
     * values: stock, the one worn, then the maker's won), the maker, the colour chosen, the
     * cursor's place on the bar's window and the bar's scroll (pixels, and -1, +1 or 0 while
     * it slides), when the arrows' and colour cursor's loops started; the wheel worn's
     * (clSeqTgWearPartsWheelThread) jack: the car's lift, the body's over the tyres, its
     * bounce (phase, size, offset) and the COMPLETE's first frame */
    int wl[16], maker, col, win, scroll_dir, reset_t;
    float scroll;
    float lift, body, phase, amp, bounce;
    int complete_t;
  } tg;
  /* the race result's (clRaceResultConsole point_calc) gauge: shown at all, its points
   * before the race (card+0x178 * 50), those the race added (to at most 1000), the tuning
   * message once full (menu_p_finish_result_st_get_*), the advantage (m, > 0 ahead) */
  int st_show, st_prev, st_gain, st_get;
  float st_gap;
  /* the tuning screen (clSeqStTuneThread::clMenuLayout): the values with the step's +1
   * (b8, bc), its gauges' cells, the choice (c4: 1 power) and whether it is fixed (c5), the
   * choice the screen opened on, when the choice last changed (changeTune), each cell's
   * state (0 empty, 1 lit, 2 off, 3 on, 4 the step) and the frame it took it */
  struct {
    int power, handling, total, cells, choice, fixed, choice0, power0, handling0, changed;
    unsigned char state[2][24];
    int since[2][24];
  } tune;
  /* The versus (MENU_VS, the port's own screen, from the game mode select's versus disc and
   * the versus select's three, MENU_VS_SEL): over the Wi-Fi's LAN, a device
   * hosts and the others join the one they find; over a server, the screen types its address.
   * The screen asks the app to connect, host, join, start (the host) and which ramp and
   * distance; the app fills in what it found and the lobby each frame. */
  struct {
    int kind;            /* MENU_VS_LAN, MENU_VS_NET */
    char ip[16]; /* the address typed, or a 15 digit room code */
    int connect;         /* the screen's ask: 1 connect (the server's: to ip; else host), -1 disconnect (the app clears it) */
    int join;            /* the screen's ask: join found game join - 1 (the app clears it) */
    int hosting;         /* the app's: this device hosts the lobby */
    int n_found;         /* the app's: the games nearby (LAN) */
    char found[MENU_VS_FOUND][24];
    int start;           /* the host's START (the app clears it) */
    int state;           /* the app's: MENU_VS_OFF .. MENU_VS_RACE */
    int n, me, host;
    char name[MENU_VS_PLAYERS][17], car[MENU_VS_PLAYERS][32];
    short glyph[MENU_VS_PLAYERS][5]; /* the names' own characters (namedb.h's, -1 past the end; none from an older build) */
    unsigned char mismatch[MENU_VS_PLAYERS]; /* a player on another build (app.c VS_BUILD): no start with one */
    int ready, auto_ready;  /* this player is ready for the next round (a click each round; the host's START waits for every other player), and clicks it by itself (AUTO READY, saved) */
    unsigned char ready_of[MENU_VS_PLAYERS]; /* each player's, from its status */
    int racing[MENU_VS_PLAYERS];
    int ramp, km;        /* the host's choice (the others: the last race's) */
    int last_place, last_n; /* the last race's place of how many (0 none) */
    int picker;             /* the lobby place that picks the race's song: the host first, then the last race's winner */
    int song_set;           /* the sound source the race's song plays from (1 + 0..2; 0 the car's own) */
    int timeup;             /* the last race ended by the clock (TIME UP first, then the places) */
    int held;            /* the button under the finger (1 + its number), 0 none */
    int touch;
    int focus;           /* the button the controls are on (1 + its number), 0 none yet */
    char note[64];      /* a line from the app */
    char invite[24], addr[40], natnote[64]; /* the app's, hosting online: the room code (grouped), the address it stands for, how the router took it */
    /* the area vote (MENU_VS_AREA, the arcade's clseqJoCourseSelectThread): the app opens it
     * (area_go) when the host's START opens the vote, fills in the others' cards from their
     * status (menu_vs_status) and the host's result (area_final); the screen keeps this
     * player's card (0 omakase, 1..8 the areas by menu order), its place, the strip's scroll,
     * the 20 s and the vote */
    int area_go;
    int item, pos, item_from, move_t;
    float scroll, scroll_from;
    int timer, voted, vote, vote_t; /* voted: 0 not yet, 1 an area, 2 omakase's random one */
    int area_final, final_t;        /* the host's card, -1 none yet */
    int other[MENU_VS_PLAYERS];     /* each player's card */
    int other_vote[MENU_VS_PLAYERS]; /* -1 not in the vote, 0 choosing, 1 voted, 2 voted by omakase */
    /* the time zone vote after it (MENU_BGM with day set, the arcade's seqJoinDayTimeSelect
     * over seqStdDayTimeSelect's screen): the app opens it (menu_vs_day_open) after the area's
     * result, fills in the others' zones and the host's result (day_final); the screen keeps
     * this player's card (zone), its 20 s and its vote; the BGM stays each player's own */
    int day, day_timer, day_voted, day_t; /* day_voted 2: omakase (left to the others) */
    int tuning, tuned, tune_timer; /* (tune_timer: frames left on the HP screen) */
    /* the HP screen after the zone's result (the arcade sets the power per course): on it; done with it */
    int day_final;                  /* the host's zone, -1 none yet */
    int other_day[MENU_VS_PLAYERS];  /* -1 not in the vote, 0 choosing, 1 voted, 2 omakase */
    int other_zone[MENU_VS_PLAYERS];
    /* the race's loading (MENU_VS's LOADING): the app's, this machine's race is in (its status
     * says so) and it waits for the others' (wait_others: the screen says so) */
    int loaded, wait_others;
    /* the cars flying in before the race (MENU_VS_MATCH, the arcade's
     * clRaceThread::JoMatchingInformation): the app opens it (menu_vs_match) with the race's
     * machines (its GO's, in PCB order: car, colour, tune, the name's glyphs) once their cars
     * are in, before the race's loading page (the cars first, then the loading);
     * menu_vs_match_back on to that page once it is over (match_played) */
    int match_t, match_n, match_me, match_played;
    struct {
      int maker, car, color, power, handling;
      short glyph[5];
      int title; /* (0 not sent) */
      int aero, mirror, wing, bonnet, wheel, neon, vinyl; /* its dress-up (menu_car's) */
      int ccolor; /* the special colour its card wears (0 its own) */
    } match[MENU_VS_PLAYERS];
  } vs;
  /* Controller and keyboard input (menu_action; docs/INPUT.md): the presses since the last step, which step() makes menu_in of.
   * focus: the row of the list screens (the save slots, the option rows, the card settings' row), name_x / name_y the cursor on the
   * name grid. */
  int act_ok, act_up, act_down, act_view, act_left, act_right;
  int focus, name_x, name_y, name_ctl;
  /* The key binding screen (MENU_BINDINGS, bind.h): the list shown (BIND_MENU / BIND_RACE), the cell (row; the last row resets the list)
   * and whether it waits for an input, and a note under the list. */
  int bind_ctx, bind_row, bind_col, bind_wait, bind_wait_t; /* bind_wait_t: the frames the wait has lasted (it gives up after 8 s) */
  char bind_note[72];
  float axis_raw[56]; /* the pad's and a wheel's axes as they are (bind.h AXIS_COUNT; the binding screen's AXES list shows them live) */
  float axis_base[56]; /* the axes as they were when the wait for one began (a wheel's pedal rests at the end of its throw) */
  int axis_cand[8], n_axis_cand, axis_choose, axis_pick; /* the wait for an axis: those that moved in its 5 s, then the choice between them */
} menu_state;
enum { MENU_VS_OFF, MENU_VS_CONNECTING, MENU_VS_LOBBY, MENU_VS_LOADING, MENU_VS_RACE };
enum { MENU_VS_LAN, MENU_VS_NET, MENU_VS_KINDS };
#define MENU_VS_RAMPS 22

/* The attract loop (mseqDemo) runs MOVIE, TITLE, RANKING, LOGO, PROLOGUE and round again; a tap
 * anywhere in it is START (the gas pedal), which flashes white into SAVE (the port's save
 * slots), then HAVE (a car of the save, through GARAGE, or a new one, through the car
 * select), each ending on CARD. The back button steps back through MAKER ... BGM, MAKER to
 * HAVE (or SAVE without cars), and from SAVE fades to white and back to the title screen. SETTING, the car's tune and gearbox, follows the colour select,
 * then GAME, the arcade's game mode select (story, 10 out run, time attack): the time attack
 * goes on to MODE, COURSE and BGM, the story to ST_LEVEL (seqStEnemyLevelSelect: the level,
 * C, B, A or A+) and ST_RIVAL (clThread_5_2_EnemySelect: one of its five episodes), then PRE.
 * The 10-in-a-row goes on to TEN_COURSE (clThread_7_1_CourseSelect) and TEN_LEVEL
 * (clSeqTenLevelSelectThread), then PRE.
 * PRE, the race's title page, stays up while the course loads. */
enum { MENU_MOVIE, MENU_TITLE, MENU_RANKING, MENU_LOGO, MENU_PROLOGUE, MENU_START,
       MENU_MAKER, MENU_CAR, MENU_COLOR, MENU_SETTING, MENU_GAME, MENU_MODE, MENU_COURSE, MENU_BGM,
       MENU_ST_LEVEL, MENU_ST_RIVAL, MENU_PRE, MENU_ST_TUNE, MENU_TEN_COURSE, MENU_TEN_LEVEL,
       MENU_TG_GET, MENU_TG_AERO, MENU_TG_PART, MENU_TG_WEAR, MENU_OPTION, MENU_SAVE, MENU_HAVE, MENU_GARAGE,
       MENU_CARD, MENU_LVUP, MENU_ST_EFF, MENU_TG_WHEEL, MENU_TG_SELECT, MENU_TG_MAKER, MENU_NAME,
       MENU_TG_VINYL, MENU_VS, MENU_VS_SEL, MENU_VS_AREA, MENU_VS_MATCH, MENU_SHOGO, MENU_VS_STAR, MENU_INFO, MENU_LAYOUT, MENU_POEM, MENU_BONUS, MENU_CARDSET, MENU_BINDINGS, MENU_HUDOPT, MENU_VOLUME, MENU_PORTINFO, MENU_RUMBLE, MENU_MILAGE, MENU_DISPLAY, MENU_NETWORK, MENU_GRAPHICS };
enum { MENU_GAME_STORY, MENU_GAME_TEN, MENU_GAME_TA, MENU_GAME_VS };

/* The cars the car select shows in 3D this frame (the carousel, or the colour turntable),
 * with the arcade's menu camera. Units and axes are the race's (0.1 m, +y up). */
#define MENU_MAX_CARS 12
/* aero: the dress-up set (0 stock), mirror, wing, bonnet its parts (render_params'), wheel
 * its wheel (menu_state.wheel's), lift its body over its tyres (render_params.wheel_drop), neon
 * its under neon (clMenuCar's: lights_draw_neon 3.1 under the car's matrix); cp:
 * drawn as the dress-up screens' clSpriteAnimeCPModel, its own camera on the axis through
 * screen point cp_x, cp_y (640 x 480) */
typedef struct {
  int pack, paint, aero, cp;
  float cp_x, cp_y;
  float model[16];
  int mirror, wing, bonnet, wheel;
  int tuned; /* the player's own car in a dress-up screen: it shows its tune's exhaust and roll cage (src/partdb.h) */
  int tune_set, tune_pw, tune_hd; /* (tune_set: the tune levels it shows, not the car on hand's: the car select's tiles) */
  float lift;
  /* its under neon (menu_state.neon's) */
  int neon;
  /* its sticker (menu_state.vinyl's) */
  int vinyl;
  /* its aura (clMenuCar::setAura): 0 none, else the colour + 1 (green, purple, red, blue) and its level 0..4 (nsMenuCheck::checkAura) */
  int aura, aura_level;
  /* its wheels turned (the versus' cars flying in): each corner's rotation (a GL matrix's
   * columns, render_params.wheels'), at its own place */
  int turned;
  float tyre[4][16];
} menu_car;
typedef struct {
  int n;
  menu_car cars[MENU_MAX_CARS];
  float eye[3], at[3];
  float fov_x; /* radians, on the arcade's 4:3 screen */
  int env;     /* the screen's clMenu3dEnvironment (data/menu/menu_<env>.lua's lights) */
  /* the wheel GET screen's turning wheel (menu_state.wheel's value, 0 none; its model
   * menu_wheel_get's) at matrix wheel_model */
  int get_wheel;
  float wheel_model[16];
} menu_scene;

/* Needs anm_load() first. Starts the attract loop. */
void menu_init(menu_state *m);
/* A phone's 16:9, edge layout px past the 4:3 on each side: the courses' photos behind the
 * screens go its full width; menu_photo_bg: one was drawn by the last menu_draw. */
void menu_wide(float edge);
int menu_photo_bg(void);
/* A new game's progress: every car stock and without parts, no story or 10-in-a-row results
 * (the OPTION screen's choices stay). */
void menu_garage_reset(menu_state *m);
/* The driven car's story and 10-in-a-row results back to its per car arrays; car k's
 * (own[k]) into st_div, st_bits and ten_result, k the car driven. */
void menu_own_store(menu_state *m);
void menu_own_sort(menu_state *m); /* the cars by last used (the car select's order) */
void menu_own_load(menu_state *m, int k);
/* GetClass (0x857bb0c) of car (maker, car) by its story and 10-in-a-row results. */
int menu_class(const menu_state *m, int maker, int slot);
/* clRaceThread::fini: each of the save's cars' class up to GetClass's (it never goes down);
 * the driven car's results first back to its arrays (menu_own_store). */
void menu_class_update(menu_state *m);
/* After a race's results: the class-up screen due if the car's class rose (the class kept
 * raised to it). */
void menu_lvup_check(menu_state *m);
/* After a race's results: the new titles (title_judge's, 0 for none) offered on the way back */
void menu_title_due(menu_state *m, const int titles[3]);
/* the chosen car's title (1 湾岸の新人 when it has none yet) */
int menu_title(const menu_state *m);
/* The save's car of this maker and car (-1 none). */
/* a finished run into the course's ranking: its place (1 first), 0 not kept; the time of the fastest, 0 none */
int menu_rank_add(menu_state *m, int course, const menu_rank_row *row);
int menu_rank_best(const menu_state *m, int course);
int menu_own_find(const menu_state *m, int maker, int car, int dup);
/* the first dup of a model no card of the save has, -1 when all MENU_DUPS are taken */
int menu_free_dup(const menu_state *m, int maker, int car);
/* the array slot of the car on hand (m->maker, m->car; a new car in the choosing: its free dup), and of a card */
int menu_slot(const menu_state *m);
/* the special colour the current car wears (0 its own, n the nth), and how many its card has (plays / 60 up to the car's ten) */
int menu_ccolor(const menu_state *m);
int menu_cc_unlocked(const menu_state *m, int mk, int sl);
/* the mip bias the HUD OPTIONS' TEXTURES setting means (0 soft, -1, -2, -3 sharper) */
float menu_mip_bias(const menu_state *m);
/* a race ended for the current car: one more play */
void menu_card_play(menu_state *m);
static inline int menu_own_slot(const menu_own *o) { return o->car + MENU_GARAGE_CARS * o->dup; }
/* Back to the mode select (after a race). */
void menu_open(menu_state *m);
/* The chosen car's tune as car_spec has it: power and handling, 0 and 0 when stock. */
void menu_tune(const menu_state *m, int *power, int *handling);
/* The chosen car's story tune (the story races run on it). */
void menu_story_tune(const menu_state *m, int *power, int *handling);
/* clRaceResultConsole::point_calc's story points for the race just run (before
 * menu_story_clear: the episode's earlier clear counts): moves the chosen car's gauge on,
 * a full one to the tuning screen. Returns the units (50 points each) earned. */
int menu_story_points(menu_state *m, int won);
/* The story race's loop (0 the first) and its episode's losses so far, for the rivals */
int menu_story_loop(const menu_state *m);
int app_best_ms(int course); /* the app's: the player's best time on a course (ms, 0 none), for the card check's time attack page */
int menu_story_losses(const menu_state *m);
/* The episode's end: a WIN clears its losses, a LOSE counts one */
void menu_story_lost(menu_state *m, int won);
/* The 10-in-a-row's race: its clEffTenCourseType (0..9), EStage and level. */
int menu_ten_course(const menu_state *m);
int menu_ten_stage(const menu_state *m);
/* Open the pause menu over the race. */
void menu_pause_open(menu_state *m);
int menu_exit_requested(const menu_state *m);
/* Advance it by dt seconds: MENU_PAUSED while it is up, MENU_RESUME once YES is decided
 * and the box has faded, MENU_LEAVE once NO is decided and the screen is white. Then the
 * menus are back on the race's last menu, the time zone and BGM select, coming out of
 * the white. */
enum { MENU_PAUSED, MENU_RESUME, MENU_LEAVE };
int menu_pause_update(menu_state *m, float dt);
/* Draws it over the race (needs no 3D). */
void menu_pause_draw(const menu_state *m);
/* The end of a time attack over the race (car_race.over): the music stops, the race runs on
 * a moment, then FINISH and the result (goal = 1, with res), left with a tap, or TIME UP,
 * both with the result music. menu_finish_update gives MENU_PAUSED meanwhile, then
 * MENU_LEAVE once the screen is black, the menus coming back out of it on the time zone and
 * BGM select. Taps go to menu_touch meanwhile. */
void menu_finish_open(menu_state *m, int goal, const menu_result *res);
int menu_finish_update(menu_state *m, float dt);
void menu_finish_draw(const menu_state *m);
/* Each race frame from the moment the race stops running (goal or time up, before
 * menu_finish_open): gets the end's sprites up, one a frame, so they show without a hitch.
 * Draws (nothing to see), so it goes with the frame's drawing. */
/* goal: 0 TIME UP, 1 FINISH, 2 a story race, 3 a 10-in-a-row (what menu_finish_ten names) */
void menu_finish_warm(int goal);
/* after menu_finish_open, for a story race: WIN or LOSE (clRaceResultConsole_story) in place of
 * the time attack's result */
/* a story race's end: WIN or LOSE; next: won, and the select runs another race (no WIN yet:
 * its fade, then the next race), fade the race's STORYRACE_FADE */
void menu_finish_story(menu_state *m, int won, int next, float gap, int fade);
/* the story select's race `race` (0 first): its own music (clEffStoryType::getBgm) as the song */
void menu_story_song(menu_state *m, int race);
/* after menu_finish_open, for a 10-in-a-row: its end (clTenResult) in place of the time
 * attack's, from the rivals beaten, the total time and the medal (CTenSeq::GetResult 1..3,
 * counted when all were beaten) */
void menu_finish_ten(menu_state *m, int beaten, float time, int medal);
/* after menu_finish_open, for an online versus: WIN or LOSE (the story's) with the place of
 * how many; back to MENU_VS after */
void menu_finish_vs(menu_state *m, int place, int n, int timeup);
/* a versus' stars for the place (1 the first) of n machines (clRaceThread::finiJoin's table by
 * the stars held: none lost up to 2000); the car's stars and count updated, the change kept
 * for the stars screen (shown on the way back to the lobby when not 0); the change */
int menu_vs_stars(menu_state *m, int place, int n);
/* the stars screen (MENU_VS_STAR) for star_from and star_delta, then the lobby */
void menu_vs_star_open(menu_state *m);
/* The story ending (clEndingConsole): MENU_ENDING_LEN frames; the movie's band at frame t (0
 * the whole screen, 1 the bottom 640 x 192, 2 the top: clSubScreen), the frame the skip goes
 * to, the staff roll's draw over the movie at m->ending_t, and back to the menus after it */
#define MENU_ENDING_LEN 9780
#define MENU_ENDING_SKIP 9420
int menu_ending_band(int t);
void menu_ending_draw(const menu_state *m);
void menu_ending_done(menu_state *m);
/* the ending's song level (its fade out from MENU_ENDING_SKIP over 120 frames) */
float menu_bgm_level(const menu_state *m);
/* the chosen car's meter for the race (hud_set_meter), its unlock checked */
int menu_meter(const menu_state *m);
/* the host closed the game under a versus race: back to MENU_VS, off, with `note` on it */
void menu_vs_closed(menu_state *m, const char *note);
/* The area vote's status byte (the lobby's HELLO, which the servers pass on unread): 0 the
 * lobby, 1 racing, MENU_VS_CHOOSING + card while choosing, MENU_VS_VOTED + card voted,
 * MENU_VS_OMAKASE + card voted by omakase, MENU_VS_FINAL + card the host's result */
/* (in the lobby: 0 not ready, MENU_VS_READY ready) */
enum { MENU_VS_READY = 12 };
enum { MENU_VS_CHOOSING = 2, MENU_VS_VOTED = 16, MENU_VS_OMAKASE = 32, MENU_VS_FINAL = 48, MENU_VS_CARDS = 9 };
/* then the time zone vote's (+ zone, 0 morning, 1 night): MENU_VS_DAY choosing, MENU_VS_DAY_VOTED
 * voted, MENU_VS_DAY_FINAL the host's result, MENU_VS_DAY_OMAKASE left to the others; and
 * MENU_VS_LOADED: the race loaded, waiting for the others' (1 while loading and racing) */
enum { MENU_VS_DAY = 64, MENU_VS_DAY_VOTED = 66, MENU_VS_DAY_FINAL = 68, MENU_VS_LOADED = 70, MENU_VS_DAY_OMAKASE = 72,
       MENU_VS_TUNE = 74 /* the HP screen, after the zone: + the zone */, MENU_VS_TUNED = 76 /* done there, waiting for the others: + the zone */ };
/* a status of the time zone vote's (the host's tells the others the vote is on) */
#define MENU_VS_IN_DAY(st) (((st) >= MENU_VS_DAY && (st) < MENU_VS_LOADED) || ((st) >= MENU_VS_DAY_OMAKASE && (st) < MENU_VS_DAY_OMAKASE + 2))
int menu_vs_status(const menu_state *m);
/* the HP screen after the course and the zone are decided */
void menu_vs_tune_open(menu_state *m);
void menu_vs_tune_close(menu_state *m);
/* the area's result in: the time zone vote (MENU_BGM, vs.day) */
void menu_vs_day_open(menu_state *m);
/* MENU_VS_MATCH: open it (from MENU_VS's loading, m->vs.match filled in), whether it is over,
 * and back to the loading page after it */
void menu_vs_match(menu_state *m);
int menu_vs_match_done(const menu_state *m);
void menu_vs_match_back(menu_state *m);
/* the arcade's omakase: a random card (1..8), Hakone at half the odds */
int menu_vs_random_card(void);
/* the host's song for everyone's race (the GO's): the BGM line and the song as day_decide's */
/* the picker's song (menu_vs_pick: the sound source in bits 5, 6, the line + 2 below) for the race */
void menu_vs_song(menu_state *m, int pick);
int menu_vs_pick(const menu_state *m);
/* the card name's glyphs (namedb's, -1 past its end) of an ASCII name (vs_name's: a save
 * with no name sends PLAYER n, which is nP) */
void menu_name_glyphs(short g[5], const char *ascii);
/* the vote's card (1..8) to the arcade's area (0 C1 .. 7 FUKUOKA: areaIDFromMenuOrder) */
int menu_vs_card_area(int card);
/* a status as the lobby has it without the host's area card (menu_vs_status), the card in *card (-1 none) */
int menu_vs_status_plain(int st, int *card);
/* the race's GO: from the votes (or the lobby) to the versus' NOW LOADING */
void menu_vs_loading(menu_state *m);
/* the join ramp's name for the versus screen (0..MENU_VS_RAMPS-1) */
const char *menu_vs_ramp_name(int ramp);
/* the player's name in capitals for the online lobby (kana as romaji) */
void menu_name_ascii(const menu_state *m, char *out, int n);
/* clRaceThread::finiStory on a won episode (story_n): clears it, and the division's last
 * (its 20th, A+'s fifth) moves the story on to the next division. */
void menu_story_clear(menu_state *m, int n);
/* Battles won with the chosen car (a story race won, each 10-in-a-row rival beaten): its
 * dress-up level goes up one each (to 63), and an aero set won comes up after the race. */
void menu_dress_win(menu_state *m, int wins);
/* A dress-up wheel's pack (menu_state.wheel: emTireModelID + 1): its wheel_list.lua name and
 * texture set, data/wheels/<name>_<set>.wpk; NULL for none. */
const char *menu_wheel_pack(int wheel, int *set);
/* its GET screen model's pack name (wheels/<name>.wpk), NULL if none */
const char *menu_wheel_get(int wheel);
/* The dress-up wheels the screen may show next (up to max, the nearest first): the GET
 * screen's won one and the one worn, the change screen's places either side and the place's
 * colours, the wheel worn. The app keeps them loaded ahead. */
int menu_wheel_next(const menu_state *m, int *out, int max);
/* Taps on the items and the left/right keys (1024x768 points); with the pause menu open,
 * on its cards. */
void menu_touch(menu_state *m, void *id, int phase, float x, float y);
/* The arcade's controls as the menus use them (docs/INPUT.md): the steering left and right, the shifter up and down, the gas
 * (OK: decide what is selected), the brake (BACK) and the view button (VIEW: pages, music, the other chips). A keyboard or a
 * pad (platform/*) presses them; the touch screen and the mouse keep working beside them. */
enum { MENU_ACT_LEFT, MENU_ACT_RIGHT, MENU_ACT_UP, MENU_ACT_DOWN, MENU_ACT_OK, MENU_ACT_BACK, MENU_ACT_VIEW };
void menu_action(menu_state *m, int act);
/* The key binding screen is waiting for an input (a key or pad button), or wants this one (Delete clears the cell): the platform's
 * input goes to menu_bind_capture first (app_input), and the escape key cancels instead of quitting. */
int menu_bind_capturing(const menu_state *m);
int menu_bind_wants(const menu_state *m, int kind, int code);
void menu_bind_capture(menu_state *m, int kind, int code);
/* An axis moved (bind.h AXIS_*, -1..1): the binding screen's wait for an axis takes it past a good push; returns 1 then. */
int menu_axis_capture(menu_state *m, int axis, float v);
/* The start button: leaves the attract loop (not while it boots, menu_booting). */
void menu_start(menu_state *m);
/* The maker logos and the prologue the app boots into, before the first attract movie: no
 * tap skips them, so the course and the car models have that time to load (app.c). */
int menu_booting(const menu_state *m);
/* The attract movie that should be on screen now (0 atruct, 1 playdemo, 2 tutorial), or -1. */
int menu_movie(const menu_state *m);
/* The movie ended (or is missing): on to the title screen. */
void menu_movie_done(menu_state *m);
/* The BGM to play: a file name in data_dir/sound/bgm (without .m4a), or NULL for none.
 * racing: the race's song rather than the menus' music. */
const char *menu_bgm(const menu_state *m, int racing);
/* Advance by dt seconds; returns 1 once a race has been chosen (m->course, m->zone,
 * m->song) and its title page has shown (with m->ready set). */
int menu_update(menu_state *m, float dt);
/* The 3D layer of the current screen; returns 0 when it has none. */
int menu_scene3d(const menu_state *m, menu_scene *s);
/* Draws the screen; draw3d (if set) is called where the 3D cars go, between the
 * backgrounds and the overlays. */
/* mini3d (if set): the course selects' 3D map of the route (clMiniatureCourse) over the
 * background, given the menu and the screen's fade (1 none, 0 black) */
void menu_draw(const menu_state *m, float screen_w, float screen_h, void (*draw3d)(void *), void *ctx,
               int (*mini3d)(void *, float));

#endif
