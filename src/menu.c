#include "menu.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "anm.h"
#include "spriteids.h" /* JID(n): the Japan build's sprite id n, as this build has it (tools/sprids.py) */
#include "battle.h"
#include "car.h"
#include "cardb.h"
#include "gl.h"
#include "hud.h"
#include "bind.h"
#include "ffb.h"
#include "storypre.h"
#include "storysel.h"
#include "storybgm.h"
#include "tendb.h"
#include "sttune.h"
#include "steff.h"
#include "dressdb.h"
#include "namedb.h"
#include "mathx.h"
#include "sound.h"
#include "title.h"

/* Frame layout of the arcade's animations (found by stepping through them):
 * - menu_g_topbar_bar / menu_g_bottombar_bar: 0..159 idle, 160..170 slide out, 189..199 slide in.
 * - menu_layout_ta_carsel (clSeqTaModeSelectThread's clMenuSpinBoxStd, its defaults): 100
 *   frames an item k: k*100 + 0..59 idle (looped), k*100 + 60..69 decided (held), k*100 + 90..94
 *   the slide on to k + 1 (backwards going back). menu_layout_ta_carsel_ring: the ring's pulse
 *   k*100 + 0..59, once each time the selection comes to rest (then hidden).
 * - menu_layout_mode_sel (clSeqModeSelectThread's spin box with a card: story, ghost versus
 *   battle, 10 out run, time attack), menu_layout_mode_image (the mode's picture and line) and
 *   menu_layout_mode_ring: as menu_layout_ta_carsel.
 * - menu_layout_ta_course: 100 frames per course k: k*100 idle, k*100+61..75 decided,
 *   k*100+90..99 selection moves on to k+1.
 * - menu_layout_ta_course_arrow: k*100+10..49 the arrows' pulse for course k. */
static struct {
  int title, bg, line, logo, top, bottom, mode, mode_ring, course, arrow, course_map;
  int ten_timeup, ten_face, ten_x, ten_finish, ten_summary; /* clTenResult */
  int game, game_image, game_ring; /* the game mode select */
  int vs_sel, vs_sel_image, vs_sel_ring; /* the versus select */
  int vs_course, vs_icon, vs_icon_omakase, vs_icon_omakase2, vs_ex_player, timer, wait1, wait2, finish_vs; /* the area vote; the Please wait! screens; the versus' place at the goal */
  int shogo_bg, pre_shadow[3], pre_cars[3], pre_vs[3], plate, plate_ph; /* the cars flying in (MENU_VS_MATCH) */
  int maker[3], pre_bg, ranking_bg, ranking;
  int prologue[2];
  /* the car select */
  int bg_carsel, maker_layout, maker_car, car_info, car_arrow, car_new, mes_r35, mes_jzs161;
  int col_strip[7], col_arrow[7], col_name; /* strips for 4, 5 and 6 colours, one (the hidden taxis'), and the special colours' 10, 11 and 12 */
  int mes_maker, mes_car, mes_color;
  /* the car setting's wheel prompt */
  int set_handle;
  /* the time zone and BGM select */
  int day_cards, day_ring, day_sound, day_vs, day_vs_ring;
  /* the race's title page */
  int pre_ta, pre_sound;
  int pre_st, pre_sound_st; /* a story race's title */
  /* the story's level select (its layout, arrows, CLEARED marks, ring, the line under the
   * episodes, the episodes by level) and rival select (its layout, arrows, the chapter's and
   * start ramp's names, the episodes by level, the odd and even title glyphs) */
  int st_level, st_level_arrow, st_level_clear, st_level_ring, st_lv_back, st_lv[4];
  int st_rival, st_rival_arrow, st_rival_ramp, st_ep[4][2];
  /* the story's tuning screen: its layout, the tuner's lines, the background, the gauges'
   * cells (lit, the frames), the P and H markers */
  int st_tune, st_tune_serifu, bg_st_tune, gage, gage_waku, gage_waku2, ph;
  /* dress-up: the backgrounds, the bar without time, the GET screen's title and part, the aero
   * change's layouts by its number of choices */
  int tg_bg1, tg_bg3, top_notime, tg_get_title, tg_get_parts, tg_aero, tg_aero2, tg_aero3, tg_aero5;
  int tg_get_image, tg_get_wheel;
  /* the bonnet's, wing's (5 and 6 places) and mirror's change layouts and their arrows */
  int tg_bonnet, tg_bonnet_arrow, tg_wing5, tg_wing5_arrow, tg_wing6, tg_wing6_arrow, tg_mirror, tg_mirror_arrow;
  /* the neons' change layout and its arrows */
  int tg_neon, tg_neon_arrow;
  /* the stickers' change: the bar, the colour column and the arrows; the gem cars' bar */
  int tg_vinyl, tg_vinyl_col, tg_vinyl_arrow, tg_vinyl_gem;
  /* the part worn's title, its name's banner, the white flash */
  int tg_wear_title, tg_wear_logo, whiteout;
  /* the wheel's change: the bar and its arrows by maker (wheel_maker_to_cursor_anim,
   * wheel_maker_to_arrow_anim), the colour column's swatches, cursor, name logo and frame;
   * the level 63 part's COMPLETE */
  int tg_wheel_bar[5], tg_wheel_arrow[5], tg_wheel_col, tg_wheel_cursor, tg_wheel_logo, tg_wheel_waku, tg_complete;
  /* the dress-up menu's places (the gem cars' fewer) and the wheel maker select's
   * pictures and places */
  int tg_all[2], wmaker_bg, wmaker_sel;
  /* the card's first page: its frame and its fields */
  int card_waku, card_layout;
  int loading;
  int finish[2]; /* the race's end: TIME UP, FINISH */
  int win, lose;  /* a story race's end */
  /* its result's advantage row and tuning points: the gauge's frame, the gauge, the title / message */
  int st_adv, st_waku, st_bar, st_moji;
  /* clTaResult's rows: the course, the course's, the car type's and the player's best, the
   * sections, TOTAL TIME, the top speed, the ranking */
  int res_course, res_best[3], res_congrats, res_sections, res_sections2, res_total, res_speed, res_rank;
  /* the 10-in-a-row's course cards, level boxes, their ring, arrows and faces */
  int or_course, or_lv, or_lv_ring, or_lv_arrow, or_lv_face, pre_or;
  /* the class-up screen: its backgrounds and layout; the story's tuning work: its top bar,
   * text boxes (power, handling), complete pictures and install animes */
  int shogo_bg1, shogo_bg2, lvup;
  /* the title select: the back's letters, TITLE, the new titles' frames (one, two, three),
   * the rewrite and the spin box */
  int shogo_moji, shogo_title, shogo_n[3], shogo_rw, yn_rank;
  int eff_top, eff_result, eff_handle, eff_complete, eff_complete2, eff_install[13];
  /* the player's name: the grid's frame, the name's band, its lines (each, the next one's
   * blink), the page tabs and END */
  int name_select, name_back, name_use, name_line, name_tab[5];
  /* the versus stars: background, logo, car, title row, wins, the stars held; the stars won
   * (1..3), their carry (keta_a, keta_b by the ones left), the big stars' finish (1, 100,
   * 1000, 10000) and the stars lost (1, 2) */
  int star_bg, star_logo, star_car, star_title, star_wins, star_use;
  int star_add[3], star_keta_a[3], star_keta_b[3], star_finish[4], star_minus[2];
} ID;

/* the bottom bar's key glyphs (menu_extra.bin's animations, the arcade's menu_g_bottombar_*: the wheel, the pedals and the page key with
 * their own motion, each with its label) over the bar: the variant a screen has by the keys it takes */
enum { BK_NONE, BK_GD, BK_GC, BK_HS_GD, BK_HS_BB_GD, BK_BB_GD, BK_BP_GC, BK_BP_GC_VS, BK_BP_GC_VK_HS, BK_VP_GC_BK_HS, BK_COUNT };
static const char *const BK_NAME[BK_COUNT] = { NULL, "menu_g_bottombar_gd", "menu_g_bottombar_gc", "menu_g_bottombar_hs_gd", "menu_g_bottombar_hs_bb_gd",
  "menu_g_bottombar_bb_gd", "menu_g_bottombar_bp_gc", "menu_g_bottombar_bp_gc_vs", "menu_g_bottombar_bp_gc_vk_hs", "menu_g_bottombar_vp_gc_bk_hs" };
static int bottom_anim[BK_COUNT];
static int bottom_kind; /* the variant of the screen being drawn (menu_draw) */
static int bottom_clock; /* and the frames run since: the glyphs' own motion (0 .. 159 idle) once the bar is in */

static void draw_bottom(int bars, float fade) {
  anm_draw(ID.bottom, bars, fade);
  if (hud_style() != HUD_STYLE_TOUCH && bottom_kind > BK_NONE && bottom_kind < BK_COUNT && bottom_anim[bottom_kind] >= 0) anm_draw(bottom_anim[bottom_kind], bars ? bars : bottom_clock % 160, fade);
}

#define N_COURSES CAR_COURSES /* the course cards (car.h; car_course_id their course ids) */
#define MOVE_MODE 4   /* clMenuSpinBoxStd: 1/5 of the way a frame, at rest on the fifth */
#define DECIDE_MODE 10
#define MOVE_COURSE 10
#define DECIDE_COURSE 15
#define FADE 20
/* NOW LOADING from the VS title page's bottom right to the middle of the car select's carousel */
#define LOADING_DX -205.0f
#define LOADING_DY -205.0f
/* and on the race title (clSeqNowLoading::run: setAnime(pre_vs_loading, 4, 1, 20), its
 * pop-in, then held) to the bottom left, as far in from the edge as the arcade's is from the
 * right; it breathes while it waits and fades out once the loading is done */
#define LOADING_PRE_DX -409.0f
#define LOADING_POP 20
#define LOADING_OUT 15

/* mseqDemo: clSeqTitleThread runs 600 frames, clRanking 600 (its clArg), each maker logo of
 * seqAtructCi 240 */
#define TITLE_LEN 600
#define RANKING_LEN 600
#define LOGO_LEN 240
/* seqPrologue after the logos: atract_g_02 ("This game is a work of fiction..."), then with
 * the Japanese locale (0x9735384 == 1) atract_g_01, the story; each plays its 120 frames */
#define PROLOGUE_LEN 120
/* pressing start: white in, then the mode select shows through the fading white */
#define START_LEN 20
#define FLASH_LEN 30

/* ---- the title screen's clock (atract_p_title_time_waku) ----
 * Its digits are templates showing '89.01.23 23:45:67: year, month, day, hour, minute and
 * second, two each. The arcade puts its own clock in them; here the iPad's, with the
 * title's serial number digits (atract_p_title_sn_0..9). */
static const int CLOCK_TEMPLATE[12] = { JID(7415), JID(7416), JID(7617), JID(7618), JID(7619), JID(7620), JID(7406), JID(7410), JID(7411), JID(7412), JID(7413), JID(7414) };
#define T_TITLE_DIGIT JID(2594)

static void draw_title(int frame, float alpha) {
  time_t now = time(NULL);
  const struct tm *tm = localtime(&now);
  int v[6] = { tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec };
  anm_swap clock[12];
  for (int i = 0; i < 12; ++i) {
    clock[i].from = anm_sprite(CLOCK_TEMPLATE[i]);
    clock[i].to = anm_sprite(T_TITLE_DIGIT + (i & 1 ? v[i / 2] % 10 : v[i / 2] / 10));
  }
  anm_draw_ex(ID.title, frame, alpha, 0, 0, clock, 12);
}

/* ---- the time attack ranking (clRanking) ---- */

/* nsMenuCheck::taCourseIDFromMenuOrder; clRanking::run skips course ids 5, 7 and 11 */
static const int ORDER_TO_COURSE[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 10, 11, 12, 13 };
/* nsTaData::getBgSprite and getCourceNameSprite, by course id */
static const int COURSE_BG[15] = { JID(2863), JID(2866), JID(2869), JID(2865), JID(2870), JID(2873), JID(2880), JID(2880), JID(2357), JID(2355), JID(2885), JID(2885), JID(3930), JID(6244), JID(2359) };
static const int COURSE_NAME[15] = { JID(3931), JID(3932), JID(3933), JID(3934), JID(5609), JID(3936), JID(5610), JID(3937), JID(3944), JID(3939), JID(5611), JID(3941), JID(3942), JID(3943), JID(5612) };

/* ---- the course cards' distances (clThread_6_2_CourseSelect's constructor) ---- */

/* clEffTaCourseType::getTaDist by course id: the length in mm yt::TARaceFieldAry gives each
 * time attack stage (0x9778900); ids 5, 7 and 11 have no card */
static const float COURSE_LEN[15] = { 13771708.0f, 13815960.0f, 17464274.0f, 17643834.0f, 22491096.0f, 0, 15103232.0f, 0,
                                      14938780.0f, 18659480.0f, 15790000.0f, 0, 59798504.0f, 38178028.0f, 16810000.0f };
/* menu_p_course_no_N_template by menu order: tens, ones and tenths of a km (0x93e3a40) */
static const int KM_TEMPLATE[15][3] = {
  { JID(7031), JID(7032), JID(7033) }, { JID(7034), JID(7035), JID(7036) }, { JID(7037), JID(7038), JID(7039) }, { JID(7040), JID(7226), JID(7227) }, { JID(7228), JID(7229), JID(7230) },
  { JID(7231), JID(7232), JID(7233) }, { JID(7234), JID(7235), JID(7236) }, { JID(7237), JID(7238), JID(7239) }, { JID(7256), JID(7257), JID(7258) }, { JID(7240), JID(7241), JID(7242) },
  { JID(7259), JID(7260), JID(7261) }, { JID(7243), JID(7244), JID(7245) }, { JID(7246), JID(7247), JID(7248) }, { JID(7249), JID(7250), JID(7251) }, { JID(7252), JID(7253), JID(7254) },
};
#define KM_DIGIT JID(2239) /* menu_p_course_no_0..9 (0x93e3b00) */
static anm_swap km_swaps[15 * 3];
static int km_nswaps;

/* setImageReplace of each card's digits, with the arcade's float maths: a zero tens digit
 * is blanked */
static void km_init(void) {
  km_nswaps = 0;
  for (int order = 0; order < 15; ++order) {
    float len = COURSE_LEN[ORDER_TO_COURSE[order]];
    if (len <= 0) continue;
    int km = (int)(len * 1e-6f), tenths = (int)(len * 1e-5f);
    int digit[3] = { km / 10, km % 10, tenths % 10 };
    for (int k = 0; k < 3; ++k) {
      anm_swap *sw = &km_swaps[km_nswaps++];
      sw->from = anm_sprite(KM_TEMPLATE[order][k]);
      sw->to = k == 0 && digit[0] == 0 ? -1 : anm_sprite(KM_DIGIT + digit[k]);
    }
  }
}

/* A fresh machine's top ten: clTimeAttackRankingDataManager::load fills every course from
 * clRankingDefault::data_ (records 5, 4, 3, 2, 1, 0, 46, 45, 44, 43 in insertion order),
 * all ＧＵＥＳＴ, title 1 (湾岸の新人), class N, untuned (handling B) at 9'59"999.
 * Car name sprite = get_car_model_data(model)+0x2c, horsepower = GetCarData(car)+0x20. */
static const struct { int car_sprite, hp; } DEFAULT_ROWS[10] = {
  { JID(7627), 205 }, /* FC3S */
  { JID(7801), 260 }, /* UCF10 */
  { JID(7439), 240 }, /* CXD */
  { JID(7448), 280 }, /* Z33 */
  { JID(7636), 280 }, /* Z16A */
  { JID(7798), 280 }, /* JCESE */
  { JID(7952), 120 }, /* KZH100G */
  { JID(7905), 120 }, /* NZE121 */
  { JID(7907), 280 }, /* JZS161 */
  { JID(7801), 260 }, /* UCF10 */
};
#define DEFAULT_TIME_MS 599999
#define NAME_GLYPH5 106600 /* menupack.py NAME_GLYPH5_BASE: the name's characters, font 5 */

/* clRankingPlate::InitForHorsePowerDisp: the template sprites of menu_layout_ranking it replaces */
enum {
  T_WAKU = JID(7308), T_RANK10 = JID(3353), T_RANK1 = JID(7527), T_NAME = JID(2656),
  T_TIME = JID(7517), /* 7517..7523: M M ' S S " m m m digits */
  T_CAR = JID(7450), T_HP = JID(7524), /* 7524..7526 */
  T_HANDLING = JID(7481), T_CLASS = JID(7528), /* 7528..7530 */
  T_TITLE = JID(1408),
};
#define TEXT_GUEST 100000 /* menupack.py TEXTS */
#define TEXT_TITLE 100001

static struct {
  int digit[10]; /* menu_p_ranking_time0..9 */
  int waku, handling_b, class_n, name_waku, guest, title;
  int car[10];
  int t_waku, t_rank10, t_rank1, t_name, t_time[7], t_car, t_hp[3], t_handling, t_class[3], t_title;
  int bg_template, name_template, waku_template;
} R;

/* The arcade ranks a new course each round (seqRanking(round % 15)), skipping ids 5, 7 and
 * 11 (no card). */
static int course_for(int demo) {
  for (int i = 0; i < 15; ++i) {
    int id = ORDER_TO_COURSE[(demo + i) % 15];
    if (id != 5 && id != 7 && id != 11) return id;
  }
  return 0;
}

/* menu_p_ranking_time0..9 (the Japan build's 3352..3361) */
static const int RANK_DIGIT[10] = { JID(3352), JID(3353), JID(3354), JID(3355), JID(3356), JID(3357), JID(3358), JID(3359), JID(3360), JID(3361) };

static void ranking_init(void) {
  for (int i = 0; i < 10; ++i) {
    R.digit[i] = anm_sprite(RANK_DIGIT[i]);
    R.car[i] = anm_sprite(DEFAULT_ROWS[i].car_sprite);
  }
  R.waku = anm_sprite(JID(4062));
  R.handling_b = anm_sprite(JID(2718));
  R.class_n = anm_sprite(JID(3349));
  R.name_waku = anm_sprite(JID(0x1ba8)); /* title_waku_2: a course with over 99 records */
  R.guest = anm_sprite(TEXT_GUEST);
  R.title = anm_sprite(TEXT_TITLE);
  R.t_waku = anm_sprite(T_WAKU);
  R.t_rank10 = anm_sprite(T_RANK10);
  R.t_rank1 = anm_sprite(T_RANK1);
  R.t_name = anm_sprite(T_NAME);
  for (int i = 0; i < 7; ++i) R.t_time[i] = anm_sprite(T_TIME + i);
  R.t_car = anm_sprite(T_CAR);
  for (int i = 0; i < 3; ++i) {
    R.t_hp[i] = anm_sprite(T_HP + i);
    R.t_class[i] = anm_sprite(T_CLASS + i);
  }
  R.t_handling = anm_sprite(T_HANDLING);
  R.t_title = anm_sprite(T_TITLE);
  R.bg_template = anm_sprite(JID(0xb2f));
  R.name_template = anm_sprite(JID(0x1b9a));
  R.waku_template = anm_sprite(JID(0x1f45));
}

/* the handling grade badges of the plate (car_tune_grade's DG, D, B, G, HG; B is the untuned car's) */
static const int RANK_GRADE[5] = { JID(3145), JID(2719), JID(2718), JID(2720), JID(3146) };

/* the course's ranking: the runs kept (the machine's, fastest first) with the fresh ones' 9'59"999 rows behind them, to ten */
static int rank_course_key(int course) { return course >= 0 && course < MENU_RANK_COURSES ? course : 0; }

int menu_rank_best(const menu_state *m, int course) {
  course = rank_course_key(course);
  return m->rank_n[course] ? m->rank_row[course][0].ms : 0;
}

int menu_rank_add(menu_state *m, int course, const menu_rank_row *row) {
  course = rank_course_key(course);
  menu_rank_row *t = m->rank_row[course];
  int n = m->rank_n[course], at = 0;
  while (at < n && t[at].ms <= row->ms) at++; /* (a tie stays behind the older run) */
  if (at >= MENU_RANK_KEEP) return 0;
  if (n < MENU_RANK_KEEP) n++;
  for (int i = n - 1; i > at; --i) t[i] = t[i - 1];
  t[at] = *row;
  m->rank_n[course] = (unsigned char)n;
  return at + 1;
}

/* one plate of menu_layout_ranking, as InitForHorsePowerDisp fills it; row NULL: the fresh machine's */
static void draw_plate(int rank, int frame, float alpha, const menu_rank_row *row) {
  anm_swap sw[24];
  int n = 0, ms = row ? row->ms : DEFAULT_TIME_MS, hp = row ? row->hp : DEFAULT_ROWS[rank - 1].hp;
  sw[n++] = (anm_swap){ R.t_waku, R.waku };
  sw[n++] = (anm_swap){ R.t_rank10, rank > 9 ? R.digit[rank / 10 % 10] : -1 };
  sw[n++] = (anm_swap){ R.t_rank1, R.digit[rank % 10] };
  sw[n++] = (anm_swap){ R.t_name, row ? -1 : R.guest };
  const int t_digits[7] = { ms / 600000, ms % 600000 / 60000, ms % 60000 / 10000, ms % 10000 / 1000,
                            ms % 1000 / 100, ms % 100 / 10, ms % 10 };
  for (int i = 0; i < 7; ++i)
    sw[n++] = (anm_swap){ R.t_time[i], i == 0 && ms < 600000 ? -1 : R.digit[t_digits[i]] };
  sw[n++] = (anm_swap){ R.t_car, row ? anm_sprite(CARDB_CARS[row->maker][row->car].plate) : R.car[rank - 1] };
  sw[n++] = (anm_swap){ R.t_hp[0], hp > 99 ? R.digit[hp / 100 % 10] : -1 };
  sw[n++] = (anm_swap){ R.t_hp[1], hp > 9 ? R.digit[hp / 10 % 10] : -1 };
  sw[n++] = (anm_swap){ R.t_hp[2], R.digit[hp % 10] };
  sw[n++] = (anm_swap){ R.t_handling, row && row->grade >= 0 && row->grade < 5 ? anm_sprite(RANK_GRADE[(int)row->grade]) : R.handling_b };
  /* the class from the table at 0x93f2300: a fresh machine's row N; a kept run its class when it ran (the title is not kept with it) */
  /* the class (the table at 0x93f2300: a letter or two and the level's digit: C9 .. C1, B9 .. B1, A9 .. A1, S9 .. S1, SS9 .. SS1, SSS; N alone) */
  int cls = row && row->cls > 0 && row->cls <= 46 ? row->cls : 0, c0 = -1, c1 = -1, c2 = R.class_n;
  if (cls) {
    static const int LETTER[4] = { JID(3348), JID(3347), JID(3346), JID(3350) }; /* menu_p_ranking C, B, A, S */
    int s_ = anm_sprite(LETTER[3]), k9 = (cls - 1) % 9, g = (cls - 1) / 9;
    c2 = cls == 46 ? s_ : R.digit[9 - k9];
    c1 = g < 3 ? anm_sprite(LETTER[g]) : s_;
    c0 = cls >= 37 ? s_ : -1;
  }
  sw[n++] = (anm_swap){ R.t_class[0], c0 };
  sw[n++] = (anm_swap){ R.t_class[1], c1 };
  sw[n++] = (anm_swap){ R.t_class[2], c2 };
  sw[n++] = (anm_swap){ R.t_title, row ? -1 : R.title };
  /* plate k sits 36 px under plate k-1 (clSpriteAnime::setPosition) */
  anm_draw_ex(ID.ranking, frame, alpha, 0, 36.0f * (float)(rank - 1), sw, n);
  /* the player's name where GUEST would be */
  float r[4];
  if (row && anm_sprite(NAME_GLYPH5) >= 0 && anm_sprite_rect(ID.ranking, frame, R.t_name, r))
    for (int i = 0; i < 5 && row->name[i] >= 0; ++i) {
      int g = anm_sprite(NAME_GLYPH5 + row->name[i]);
      if (g < 0) continue;
      float src[4] = { 0, 0, anm_sprite_width(g), anm_sprite_height(g) };
      float x = r[0] + 19.0f * (float)i, y = r[1] + 36.0f * (float)(rank - 1);
      float dst[4] = { x, y, x + 0.7f * src[2], y + 0.7f * src[3] };
      anm_draw_part(g, src, dst, alpha);
    }
}

/* A phone's 16:9 (menu_wide): a course's photo behind a screen, menu_g_pre_bg, goes the
 * screen's width about its middle, its top and bottom off the screen, in place of app.c's
 * stretched edges (menu_photo_bg) */
static float WIDE_ZOOM = 1.0f;
static int photo_bg;

void menu_wide(float edge) { WIDE_ZOOM = (1024.0f + 2.0f * edge) / 1024.0f; }
int menu_photo_bg(void) { return photo_bg; }

static void draw_photo_bg(const anm_swap *bg, float fade) {
  float z = WIDE_ZOOM;
  anm_draw_xf(ID.pre_bg, 0, fade, 320.0f * (1.0f - z), 240.0f * (1.0f - z), z, bg, 1);
  photo_bg = 1;
}

/* a screen's back (menu_g_bg_image, the white one most screens have; the car select's, the
 * story tune's) stretched across a 16:9 screen's width, rows kept so its band stays behind the
 * cards, in place of app.c's stretched edges */
static void draw_back(int anim, int frame, float fade) {
  float z = WIDE_ZOOM;
  anm_draw_xy(anim, frame, fade, 320.0f * (1.0f - z), 0, z, 1, NULL, 0);
  if (z > 1.0f) photo_bg = 1;
}

static void draw_ranking(const menu_state *m) {
  int t = m->t;
  float fade = 1.0f;
  if (t < FADE) fade = (float)t / FADE;
  else if (t > RANKING_LEN) fade = 1.0f - (float)(t - RANKING_LEN) / FADE;
  int course = course_for(m->demo);
  anm_swap bg = { R.bg_template, anm_sprite(COURSE_BG[course]) };
  draw_photo_bg(&bg, fade);
  anm_swap head[2] = { { R.name_template, anm_sprite(COURSE_NAME[course]) }, { R.waku_template, R.name_waku } };
  anm_draw_ex(ID.ranking_bg, 0, fade, 0, 0, head, 2);
  /* clRanking::run starts plate k's slide-in (layout frames 11..49) at frame 13 + 2(k-1) */
  int key = rank_course_key(course);
  int real = 0;
  while (real < m->rank_n[key] && real < 10 && m->rank_row[key][real].ms <= DEFAULT_TIME_MS) real++;
  for (int k = 1; k <= 10; ++k) {
    int start = 13 + 2 * (k - 1);
    if (t < start) continue;
    int f = 11 + t - start;
    draw_plate(k, f < 49 ? f : 49, fade, k <= real ? &m->rank_row[key][k - 1] : NULL);
  }
}

/* ---- the car select (clSeqCardMakerSelectThread, clseqCardCarSelectThread,
 * clseqCardColorSelectThread) ----
 * - menu_layout_carsel_maker_ / menu_g_carsel_maker_car_image: 100 frames per maker m,
 *   idle from 100m (and 1 + 100m) looping 59 frames; 80 + 100m .. 83 moves on to m + 1 (the
 *   car image loops 80 + 100m .. 84 meanwhile), played backwards to move back. Input waits
 *   20 frames after a move.
 * - menu_g_carsel_col_0n (+ _arrow): the same for n colours, 6 frames per move.
 * - menu_g_carsel_car: the car's logo, drive and gears, by setImageReplace of templates. */
#define MAKER_MOVE 4
#define MAKER_LOCK 20
#define COLOR_MOVE 6
enum {
  T_CAR_LOGO = JID(7911), T_CAR_DRIVE = JID(7910), T_CAR_SPEED = JID(7683), T_BG_LOGO = JID(6958),
  T_SWATCH = JID(7650), /* 7650 + n - i holds colour i of n (the table at 0x93f7a80) */
  T_COL_FONT = JID(890), /* the colour name's place in menu_g_carsel_col_name */
};
/* maker buttons on menu_layout_carsel_maker_: 128 px sprites from x = 26, 92 apart, at y 286,
 * the button itself the middle 84 px */
/* the first maker's left edge (the row is centred on the 640 wide screen: 48 for Japan's six makers, 94 for the export
 * build's five, which has no Honda) */
#define MAKER_X0C (320.0f - (MAKER_DX * (float)(CARDB_MAKERS - 1) + MAKER_W) / 2.0f) /* (centred without the Special Vehicles button) */
#define MAKER_DX 92.0f
#define MAKER_W 84.0f
#define MAKER_Y0C 312.0f
#define MAKER_Y1C 390.0f
/* the Special Vehicles button: one more on the row's right end, as slot CARDB_MAKERS (it decides the hidden cars). The arcade's row is
 * drawn moved and, where its makers and the button do not fit the screen (the Japan build's six and Gemballa), smaller: a point x of
 * its layout goes to MAKER_A + MAKER_Z * x across, MAKER_B + MAKER_Z * y down, the whole row centred on the screen. */
#define HID_BTN 1
#define MAKER_SLOTS (CARDB_MAKERS + 1)
#define MAKER_Z (MAKER_DX * (float)CARDB_MAKERS + MAKER_W > 600.0f ? 600.0f / (MAKER_DX * (float)CARDB_MAKERS + MAKER_W) : 1.0f)
#define MAKER_A (320.0f - MAKER_Z * (320.0f + 0.5f * MAKER_DX))
#define MAKER_B (350.0f * (1.0f - MAKER_Z))
#define MAKER_X0 (MAKER_A + MAKER_Z * MAKER_X0C)
#define MAKER_STEP (MAKER_Z * MAKER_DX)
#define MAKER_BW (MAKER_Z * MAKER_W)
#define MAKER_Y0 (MAKER_B + MAKER_Z * MAKER_Y0C)
#define MAKER_Y1 (MAKER_B + MAKER_Z * MAKER_Y1C)
enum {
  T_MK_MARU = JID(0x1820), T_MK_WAKU = JID(0x1821), T_MK_LIGHT = JID(0x1d40), T_MK_LAST = JID(0x1a5f), T_MK_NAME = JID(0x1f55),
  T_MK_BACK = JID(0x181f), T_MK_ARROW = JID(0x1be), T_MK_ARROW2 = JID(0x383), T_QMARK = 123000 /* (tools/extrapack.py QMARK_ID) */
};
static void draw_maker_hidden(const menu_state *m, int f, int sel);
static void draw_maker_row(int f, const anm_swap *sw, int n);

/* The carousel of clseqCardCarSelectThread::yield, by maker (DB_XYZ tables at 0x9772fc0):
 * look-at height, centre x, eye x, field of view, radius, -, eye z (offsets on its constants) */
static const float CAROUSEL[7][7] = { /* (the arcade's makers: the port's are CARDB_MAKER0 fewer in front) */
  { -15.0f, 53.0f, -18.5f, -7.0f, -2.0f, -7.0f, -19.0f },
  { -13.5f, 45.0f, -18.5f, -10.5f, -11.0f, -14.5f, -19.0f },
  { -13.0f, 47.5f, -18.5f, -7.0f, -9.0f, -7.0f, -19.0f },
  { -17.5f, 56.25f, -28.5f, -1.5f, 5.0f, -7.0f, -22.0f },
  { -12.5f, 43.0f, -25.0f, -10.5f, -11.0f, -9.5f, -19.0f },
  { -11.5f, 48.25f, -13.0f, -10.0f, -10.75f, -28.0f, -12.25f },
  { -17.5f, 56.25f, -28.5f, 4.0f, 5.0f, -7.0f, -22.0f }, /* the hidden cars (the port's): TOYOTA's ring (its 12 sedans), wider for the vans (MITSUBISHI's 7 overlapped) */
};
#define TWO_PI 6.2831853f
#define DEG 0.017453292f

static void go(menu_state *m, int screen);
static int vs_hit(const menu_state *m, float x, float y);
enum { SELMENU_VS, SELMENU_MAIN, SELMENU_SETTINGS, SELMENU_TUNE, SELMENU_TRANS /* a new car's AT / MT (the arcade's Select transmission) */,
       SELMENU_MODE /* after the car's card: SOLO, VERSUS / BATTLE, CAR / SETTINGS */, SELMENU_CAR /* TRANSMISSION, DRESS-UP / PARTS, METER & BGM, DELETE CAR */,
       SELMENU_DEL /* DELETE CAR's question: KEEP or DELETE */, SELMENU_CUSTOM /* CUSTOMIZE: DRESS-UP / PARTS, PAINT COLOR */ };
#define SEL_TWO(m) ((m)->sel_menu == SELMENU_TUNE || (m)->sel_menu == SELMENU_TRANS || (m)->sel_menu == SELMENU_DEL || (m)->sel_menu == SELMENU_CUSTOM) /* (the selectors of two discs) */
static void selmenu_enter(menu_state *m, int kind, int sel);
static void tg_select_open(menu_state *m);
static void cardset_open(menu_state *m);
static void ccolor_open(menu_state *m);
static int tg_no_bodyparts(const menu_state *m);
static void tg_select_open(menu_state *m);
static void main_menu_enter(menu_state *m);
static void selmenu_decide(menu_state *m);


static int has(const char *s, const char *word) {
  for (; *s; ++s) {
    const char *a = s, *b = word;
    while (*b && *a == *b) a++, b++;
    if (!*b) return 1;
  }
  return 0;
}

/* the colour select on the special colours (the card's COLOR key): a copy of the car whose colours are its own first, then the ones its card has
 * won, each the car's colour with the special one's swatch, name and paint; chosen_car answers with it while cc_mode is on */
#define CC_PLAYS 60
static cardb_car CC_CAR;

static const cardb_car *chosen_car(const menu_state *m, int car) { return m->cc_mode && car == m->car ? &CC_CAR : &CARDB_CARS[m->maker][car]; }

int menu_cc_unlocked(const menu_state *m, int mk, int sl) {
  int n = CARDB_CARS[mk][sl % MENU_GARAGE_CARS].ncustom;
  if (has(CARDB_CARS[mk][sl % MENU_GARAGE_CARS].label, "[R35]")) return 0; /* (the R35 has no special colours in the arcade: the colour select only sounds, issue 9) */
  if (m->dress_lv[mk][sl] >= 63) return n;
  int k = m->card_plays[mk][sl] / CC_PLAYS;
  return k < n ? k : n;
}

/* car `car` of maker mk in its colour `color` wearing special colour cc (0 none): the colour as the screens show it */
static const cardb_color *color_of(int mk, int car, int color, int cc) {
  static cardb_color ring[4];
  static int at;
  const cardb_car *c = &CARDB_CARS[mk][car];
  cardb_color *r = &ring[at++ & 3];
  *r = c->colors[color];
  if (cc > 0 && cc <= c->ncustom) r->swatch = c->custom[cc - 1].swatch, r->name = c->custom[cc - 1].name, r->paint = c->custom[cc - 1].paint;
  return r;
}

static const cardb_color *cur_color(const menu_state *m) { return color_of(m->maker, m->car, m->color, m->ccolor[m->maker][menu_slot(m)]); }

int menu_ccolor(const menu_state *m) { return m->ccolor[m->maker][menu_slot(m)]; }

void menu_card_play(menu_state *m) {
  int mk = m->maker, sl = menu_slot(m);
  if (m->card_plays[mk][sl] < 30000) m->card_plays[mk][sl]++;
  m->garage_changed = 1;
}

static int ncars(const menu_state *m) { return CARDB_NCARS[m->maker]; }

/* the car info bubble (menu_g_carsel_car) and the background logo of `car` */
static int car_swaps(const cardb_car *c, anm_swap *sw) {
  sw[0] = (anm_swap){ anm_sprite(T_CAR_LOGO), anm_sprite(c->logo) };
  sw[1] = (anm_swap){ anm_sprite(T_CAR_DRIVE), anm_sprite(c->drive) };
  sw[2] = (anm_swap){ anm_sprite(T_CAR_SPEED), anm_sprite(c->speed) };
  return 3;
}

static void draw_bg_logo(const cardb_car *c, int frame, float alpha) {
  anm_swap sw = { anm_sprite(T_BG_LOGO), anm_sprite(c->bglogo) };
  anm_draw_ex(ID.logo, frame, alpha, 0, 0, &sw, 1);
}

static int strip_index(int n) { return n == 1 ? 3 : n <= 4 ? 0 : n == 5 ? 1 : n == 6 ? 2 : n <= 10 ? 4 : n == 11 ? 5 : 6; }
/* the swatches a strip has places for; a list of fewer than that (the special colours not all won) fills the first and hides the rest */
static int strip_slots(int n) {
  static const int S[7] = { 4, 5, 6, 1, 10, 11, 12 };
  return S[strip_index(n)];
}
/* the template number of colour i of n in the strip: counted down from n as the arcade's own lists are, from the strip's places for the special colours' */
static int strip_place(const menu_state *m, int n, int i) { return (m->cc_mode || n > 6 ? strip_slots(n) : n) - i; }

/* the colour strip's frame for the current state */
static int color_frame(const menu_state *m) {
  if (m->from >= 0) {
    int t = m->t < COLOR_MOVE ? m->t : COLOR_MOVE - 1;
    return m->from < m->sel ? 80 + 100 * m->from + t : 80 + 100 * m->sel + COLOR_MOVE - 1 - t;
  }
  return 100 * m->sel + m->t % 59;
}

/* Which colour is drawn at (x, y): its template's place in the strip's current frame. */
static int color_at(const menu_state *m, float x, float y) {
  const cardb_car *c = chosen_car(m, m->car);
  int strip = ID.col_strip[strip_index(c->ncolors)];
  for (int i = 0; i < c->ncolors; ++i) {
    float r[4];
    if (anm_sprite_rect(strip, color_frame(m), anm_sprite(T_SWATCH + strip_place(m, c->ncolors, i)), r) &&
        x >= r[0] && x < r[2] && y >= r[1] && y < r[3])
      return i;
  }
  return -1;
}

/* The game mode select's discs (seqModeSelect with a card: setMax 4, modes 0x38..0x3b):
 * story, the ghost versus battle (0x39, only with a card; the port's online versus in its
 * place), 10 out run, time attack */
#define GAME_SLOTS 3 /* (the versus battle moved to the mode select's discs: SELMENU_MODE; the three solo games are the arcade's card-less mode select) */
static const int GAME_AT[GAME_SLOTS] = { MENU_GAME_STORY, MENU_GAME_TEN, MENU_GAME_TA };

static int game_slot(int game) {
  for (int k = 0; k < GAME_SLOTS; ++k)
    if (GAME_AT[k] == game) return k;
  return 0;
}

static void car_select_enter(menu_state *m, int screen) {
  go(m, screen);
  /* the car screens have the entry music (mseqEntry), the mode select on the select music:
   * back from the mode select, the car's music comes back */
  if (screen <= MENU_SETTING) m->music = 0;
  /* (back from the hidden cars: the maker their key was pressed on) */
  if (screen == MENU_MAKER) m->sel = m->maker < CARDB_MAKERS ? m->maker : m->hidden_from;
  if (screen == MENU_CAR) {
    m->sel = m->car;
    m->rot = (float)m->sel * TWO_PI / (float)ncars(m);
    m->rot_speed = 0;
  }
  if (screen == MENU_COLOR) {
    m->sel = m->color;
    m->spin = -24.0f; /* car_common(true) */
  }
  if (screen == MENU_GAME) {
    m->sel = game_slot(m->game);
    m->music = 1;
  }
  if (screen == MENU_VS_SEL) {
    m->sel = m->vs.kind;
    m->music = 1;
  }
  if (screen == MENU_MODE) {
    m->sel = m->official;
    m->music = 1;
  }
  if (screen == MENU_COURSE) {
    m->sel = m->course;
    m->course_pos = m->sel <= 3 ? m->sel : m->sel >= 9 ? m->sel - 6 : 3;
  }
}

/* one step of the maker, car or colour select; returns 1 when the screen decides */
static int car_select_step(menu_state *m, const menu_in *in) {
  if (m->lock) m->lock--;
  int move = m->screen == MENU_MAKER ? MAKER_MOVE : COLOR_MOVE;
  if (m->from >= 0 && m->t >= move) { m->from = -1; m->t = 0; }
  if (m->screen == MENU_COLOR) m->spin -= 0.3f;
  int count = m->screen == MENU_MAKER ? MAKER_SLOTS : m->screen == MENU_CAR ? ncars(m) : chosen_car(m, m->car)->ncolors;
  if (m->screen == MENU_CAR && m->rot_speed != 0.0f) {
    /* the carousel turns 0.3 / n radians a frame until the selected car is in front */
    float target = (float)m->sel * TWO_PI / (float)count;
    m->rot += m->rot_speed * 0.3f / (float)count;
    if ((m->rot_speed > 0 && m->rot >= target) || (m->rot_speed < 0 && m->rot <= target)) {
      m->rot = target;
      m->rot_speed = 0;
    }
    return 0;
  }
  if (m->lock || m->from >= 0 || (!in->dx && in->select < 0 && !in->ok)) return 0;
  /* NOW LOADING: the cars on show are not in yet; no other until they are */
  if (m->loading && m->screen != MENU_MAKER) return 0;
  /* the item to be on: a step either way, an item chosen outright, or (OK) the selected one */
  int to = in->dx ? m->sel + in->dx : in->select >= 0 ? in->select : m->sel;
  if (to == m->sel && !in->dx) {
    snd_menu(SND_SELECT);
    return 1;
  }
  if (m->screen == MENU_CAR) {
    /* the carousel goes round: past the last car is the first */
    int d = to > m->sel ? 1 : -1;
    if (to >= count) { to = 0; m->rot -= TWO_PI; }
    if (to < 0) { to = count - 1; m->rot += TWO_PI; }
    snd_menu(SND_CURSOR);
    m->rot_speed = (float)d;
    m->sel = to;
    return 0;
  }
  if (to < 0 || to >= count) return 0;
  snd_menu(SND_CURSOR);
  /* a tap further along jumps there; the arcade's buttons only move one step */
  m->from = (to == m->sel + 1 || to == m->sel - 1) && to < CARDB_MAKERS && m->sel < CARDB_MAKERS ? m->sel : -1; /* (the arcade's animation has no move to the extra button) */
  m->sel = to;
  m->t = 0;
  m->lock = m->screen == MENU_MAKER ? MAKER_LOCK : COLOR_MOVE;
  return 0;
}

/* dress-up's screens: see menu_dress_win */
static int tg_due(menu_state *m);
#define TG_FADE 10
#define TG_GET_END -185.0f
#define TG_GET_SKIP (TG_FADE + 192) /* 3.2 s after the fade in */
#define TG_GET_HOLD 192           /* the picture GET screens' 3.2 s (0x93fba58) after the fade in */
#define TG_IMAGE_FROM 80          /* get_parts_image: frames 80..99, then held */
#define TG_IMAGE_LEN 20
#define TG_WHEEL_FROM 101         /* get_wheel: from frame 101, then held */
#define TG_WHEEL_START -60.0f     /* the wheel's turn: from -60 degrees, 0.3 a frame, to -120 */
#define TG_WHEEL_END -120.0f
/* seqTgPartsGet's GET threads by the part's kind: the wheels' (0), the bonnet's, wing's and
 * mirror's (2..4, clSeqTgPartsGetWingThread) and the car's (the rest, the aero thread) */
enum { TG_GET_CAR, TG_GET_WHEEL, TG_GET_PICTURE };
static int tg_get_type(const menu_state *m) {
  return m->tg.kind == 0 ? TG_GET_WHEEL : m->tg.kind >= 2 && m->tg.kind <= 4 ? TG_GET_PICTURE : TG_GET_CAR;
}
#define TG_MOVE 29
#define TG_DECIDE 9
enum {
  T_TG_TENS = JID(0x1efe), T_TG_ONES = JID(0x1efc), T_TG_KIND = JID(0x1f96), T_TG_PART = JID(0x1f7c),
  T_TG_PLATE = JID(0x1f48), S_TG_PLATE = JID(0x1bc8), T_TG_NAME = JID(0x1344), T_TG_NAME2 = JID(0x1ec2), T_TG_NEW = JID(0xb83),
  T_TG_DOT = JID(0x6b4), S_TG_STOCK = JID(0x1bc6), S_TG_AERO_GEM = JID(0xd2a), S_TG_REFUSE = JID(0x1e2e), S_TG_TOPBAR = JID(0x1ecf),
  T_TG_BG = JID(0x5af), S_TG_BG_BLACK = JID(0x111f), T_TG_IMAGE = JID(0xd3c), T_TG_WHEEL_NAME = JID(0x1e5f),
};

static int tg_place(const menu_state *m, int i) { return ((m->sel - i) % m->tg.n + m->tg.n) % m->tg.n; }

static int tg_layout(const menu_state *m) {
  return m->tg.n <= 2 ? ID.tg_aero2 : m->tg.n == 3 ? ID.tg_aero3 : m->tg.n == 4 ? ID.tg_aero : ID.tg_aero5;
}

/* choice i's frame of the layout */
static int tg_frame(const menu_state *m, int i) {
  if (m->tg.decided) {
    int t = m->t < TG_DECIDE - 1 ? m->t : TG_DECIDE - 1;
    /* the player shows a segment's first frame after one step: 91 on (90 is still the
     * move's end, which for the place right of the front is the front) */
    return 100 * tg_place(m, i) + 91 + t;
  }
  if (m->tg.move) {
    /* place r's slide (100r+60 .. 100r+88) takes it to place r+1: on from the place before
     * the move going right, back from the place after it going left */
    int r = ((m->tg.move > 0 ? m->from - i : m->sel - i) % m->tg.n + m->tg.n) % m->tg.n, t = m->tg.move_t;
    return 100 * r + 60 + (m->tg.move > 0 ? t : TG_MOVE - 1 - t);
  }
  return 100 * tg_place(m, i) + m->card_t % 60;
}

/* a dress-up screen's car: the parts the car wears */
static void tg_wear(const menu_state *m, menu_car *c) {
  c->tuned = 1;
  c->mirror = m->mirror[m->maker][menu_slot(m)];
  c->wing = m->wing[m->maker][menu_slot(m)];
  c->bonnet = m->bonnet[m->maker][menu_slot(m)];
  c->wheel = m->wheel[m->maker][menu_slot(m)];
  c->neon = m->neon[m->maker][menu_slot(m)];
  c->vinyl = m->vinyl[m->maker][menu_slot(m)];
}

/* the colours' set (src/dressdb.h DRESS_WHEEL_SETS) a wheel (menu_state.wheel's value) is
 * one of, and which colour it is there; NULL for the car's own */
static const dress_wheel_set *wheel_set(int wheel, int *col) {
  for (size_t i = 0; wheel > 0 && i < sizeof DRESS_WHEEL_SETS / sizeof DRESS_WHEEL_SETS[0]; ++i)
    for (int c = 0; c < DRESS_WHEEL_SETS[i].n; ++c)
      if (DRESS_WHEEL_SETS[i].model[c] == wheel - 1) {
        if (col) *col = c;
        return &DRESS_WHEEL_SETS[i];
      }
  return NULL;
}

/* the wheel change's place i in colour c: stock, the one worn (No change), else the won
 * one's colour (get_parts_data(0, wheel, colour, gem) + 8) */
static int tg_wheel_value(const menu_state *m, int i, int c) {
  if (i <= 1) return m->tg.wl[i];
  const dress_wheel_set *w = wheel_set(m->tg.wl[i], NULL);
  return w ? w->model[c < w->n ? c : 0] + 1 : m->tg.wl[i];
}

/* the bonnet's, wing's and mirror's change screens (clSeqTgChangePartsBonnet/Wing/MirrorThread,
 * their yield()s' 0x9776aa0, 0x9776b60, 0x9776bc0): by the part (0 bonnet, 1 wing, 2 mirror)
 * the car's turn about X, Y, Z (degrees) and place, and the eye; it looks at the origin, 60
 * degrees across */
static const float TG_PART_ROT[3][3] = { { 21.6f, 150.3f, 11.5f }, { -8.0f, 27.0f, -4.0f }, { 24.5f, 146.6f, 16.5f } };
static const float TG_PART_POS[3][3] = { { -3.3f, 4.4f, -935.5f }, { 0.5f, 0.5f, 11.5f }, { 2.0f, 0.5f, -3.5f } };
static const float TG_PART_EYE[3][3] = { { 0, 0, -1000.0f }, { 0, 3.0f, -50.0f }, { 0, -6.0f, -50.0f } };
static int tg_part(const menu_state *m) { return m->tg.kind == 2 ? 0 : m->tg.kind == 3 ? 1 : 2; }

/* the part worn (clSeqTgWearPartsAeroThread::yield's DB_XYZ 0x97ff560, by the kind: table
 * 0x9409f24): the car's turn about X, Y, Z (degrees) and place; the aero set's turns on 0.3
 * degrees a frame about y. The eye (0, 10, 50), the mirror's (0, 10, -50), at the origin,
 * 60 degrees across. */
static const float TG_WEAR_POSE[6][6] = {
  { 0, 0, 0, 0, 0, 0 },
  { 0, -6.0f, 0, 0, -7.5f, -3.0f },
  { 0.5f, -42.0f, 0, 3.8f, -5.7f, -0.5f },
  { 18.8f, -1591.5f, -8.5f, -5.2f, -10.0f, 0.5f },
  { -8.0f, 145.6f, -2.0f, -4.0f, -7.5f, 0 },
  { 0, -56.0f, 0, 0, -7.5f, -3.0f }, /* the sticker's and neons' (wk_s 5, 6, 8) */
};
/* TG_WEAR_POSE's row of a kind: the aero set's for the rest (the gem cars' gull wing) */
static int tg_wear_k(int kind) { return kind >= 1 && kind <= 4 ? kind : kind == 5 || kind == 6 || kind == 8 ? 5 : 1; }

/* igMatrix44f::makeRotationRadians(x, y, z) (degrees here) then the translation: turned about
 * the world's X, then Y, then Z; its rows (row vectors) are the model's axes */
static int tg_vinyl_value(const menu_state *m);
static mat4 tg_rotation(const float *pos, float x, float y, float z) {
  float cx = cosf(x * DEG), sx = sinf(x * DEG), cy = cosf(y * DEG), sy = sinf(y * DEG);
  float cz = cosf(z * DEG), sz = sinf(z * DEG);
  /* Rx Ry */
  const float a[3][3] = { { cy, 0, -sy }, { sx * sy, cx, sx * cy }, { cx * sy, -sx, cx * cy } };
  mat4 r = mat4_identity();
  for (int i = 0; i < 3; ++i) {
    r.m[4 * i + 0] = a[i][0] * cz - a[i][1] * sz;
    r.m[4 * i + 1] = a[i][0] * sz + a[i][1] * cz;
    r.m[4 * i + 2] = a[i][2];
  }
  r.m[12] = pos[0], r.m[13] = pos[1], r.m[14] = pos[2];
  return r;
}

static int match_scene3d(const menu_state *m, menu_scene *s);

/* ---- the car select (HAVE, the port's; a Banapassport's): the card's cars five to a page, each on a tile (its number, the car in its
 * parts, its name and horsepower), the tile after the last a new car's (while the card holds fewer than MENU_CARS_MAX) ---- */
#define REGISTER_TEXT 109000 /* menupack.py REGISTER_TEXT_BASE: the new car card's three lines */
#define GRID_X0 18.0f
#define GRID_W 116.0f
#define GRID_GAP 6.0f
#define GRID_Y 138.0f
#define GRID_H 250.0f
#define GRID_TAB_Y 100.0f
#define GRID_TAB_X 24.0f
#define GRID_TAB_W 76.0f
#define GRID_TAB_H 30.0f
#define GRID_TABS 6 /* the page tabs on view */
#define GRID_CAR_SCALE 5.6f
/* the tiles: place 0 a new car's, place k the car k - 1 (the cars, last used first, from the second tile on) */
static int grid_slots(const menu_state *m) { return m->n_own + 1; }
static int grid_tab0(const menu_state *m) { /* the first tab on view */
  int pages = (grid_slots(m) + 4) / 5, p0 = m->grid_sel / 5 - 2;
  return p0 > pages - GRID_TABS ? (pages - GRID_TABS < 0 ? 0 : pages - GRID_TABS) : (p0 < 0 ? 0 : p0);
}
static float grid_x(int col) { return GRID_X0 + (GRID_W + GRID_GAP) * (float)col; }

/* a card's car as the select's tile shows it: its parts and the tune's exhaust and roll cage (its own, not the car on hand's) */
static void own_wear(const menu_state *m, const menu_own *o, menu_car *c) {
  int mk = o->maker, sl = menu_own_slot(o);
  c->aero = m->aero[mk][sl];
  c->tuned = 1;
  c->mirror = m->mirror[mk][sl];
  c->wing = m->wing[mk][sl];
  c->bonnet = m->bonnet[mk][sl];
  c->wheel = m->wheel[mk][sl];
  c->neon = m->neon[mk][sl];
  c->vinyl = m->vinyl[mk][sl];
  int full = m->tune_full[mk][sl];
  c->tune_set = 1;
  c->tune_pw = full ? m->tune_power[mk][sl] : m->st_power[mk][sl];
  c->tune_hd = full ? CAR_TUNE_TOTAL - c->tune_pw : m->st_handling[mk][sl];
}

static void menu_aura_of(const menu_state *m, menu_car *c);
int menu_scene3d(const menu_state *m, menu_scene *s) {
  /* all of it: a car's aero and cp left unset made the car select's cars aero cars not in
   * (push86, nothing drawn) */
  memset(s, 0, sizeof *s);
  s->env = 4;
  /* the cars flying in (and while the race loads before them: their packs in) */
  if (m->screen == MENU_VS_MATCH || (m->screen == MENU_VS && m->vs.state == MENU_VS_LOADING && m->vs.match_n))
    return match_scene3d(m, s);
  if (m->screen == MENU_HAVE && !m->keep_ask) {
    /* the select's tiles: each car in its tile, as the dress-up lists place theirs (the screen point, a 3/4 view) */
    s->fov_x = 73.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 10.0f, s->eye[2] = 50.0f;
    s->at[0] = 0, s->at[1] = 3.0f, s->at[2] = 0;
    int page = m->grid_sel / 5;
    for (int i = 0; i < 5; ++i) {
      int k = page * 5 + i - 1;
      if (k < 0 || k >= m->n_own) continue;
      const menu_own *o = &m->own[k];
      const cardb_color *c = color_of(o->maker, o->car, o->color, m->ccolor[o->maker][menu_own_slot(o)]);
      const float pos[3] = { 0, 0, 0 };
      mat4 mm = tg_rotation(pos, 10.0f, -30.0f, -4.0f);
      for (int e = 0; e < 3; ++e) mm.m[e] *= GRID_CAR_SCALE, mm.m[4 + e] *= GRID_CAR_SCALE, mm.m[8 + e] *= GRID_CAR_SCALE;
      menu_car *mc = &s->cars[s->n];
      *mc = (menu_car){ c->pack, c->paint, 0, 1, grid_x(i) + GRID_W * 0.5f + 4.0f, GRID_Y + 58.0f, { 0 } };
      own_wear(m, o, mc);
      for (int e = 0; e < 16; ++e) mc->model[e] = mm.m[e];
      s->n++;
    }
    return 1;
  }
  if (m->screen == MENU_CAR) {
    const float *k = CAROUSEL[m->maker + CARDB_MAKER0];
    int n = ncars(m);
    /* makeProjection(fov + 60, -1, 4/3, 5, 20000); setInterest(eye, at) */
    s->fov_x = (k[3] + 60.0f) * DEG;
    s->eye[0] = k[2];
    s->eye[1] = 1.0f + 5.0f;
    s->eye[2] = -7.0f + k[6] + 121.0f;
    s->at[0] = 0;
    s->at[1] = k[0] + 9.5f;
    s->at[2] = 0;
    float r = 48.0f + k[4];
    for (int i = 0; i < n && i < MENU_MAX_CARS; ++i) {
      float j = (float)(n - i);
      float a = 2.61f + m->rot + TWO_PI * j / (float)n;
      float yaw = (-360.0f * j / (float)n - 61.1f - m->rot * 57.32484f) * DEG;
      float pos[3] = { cosf(a) * r - 15.0f + k[1], -7.0f, sinf(a) * r + 15.0f };
      mat4 mm = mat4_body(pos, yaw, 0, 0);
      const cardb_color *c = &CARDB_CARS[m->maker][i].colors[0];
      s->cars[s->n].pack = c->pack;
      s->cars[s->n].paint = c->paint;
      for (int e = 0; e < 16; ++e) s->cars[s->n].model[e] = mm.m[e];
      s->n++;
    }
    return 1;
  }
  if (m->screen == MENU_VS_STAR) {
    /* clSeqJoStarGetThread: the card's car turning about y at the origin, environment 0, the
     * default projection, setInterest((0, 11, 50), (0, 4, 0)) */
    s->env = 0;
    s->fov_x = 73.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 11.0f, s->eye[2] = 50.0f;
    s->at[0] = 0, s->at[1] = 4.0f, s->at[2] = 0;
    const float pos[3] = { 0, 0, 0 };
    mat4 mm = mat4_body(pos, m->star_spin * DEG, 0, 0);
    const cardb_color *c = cur_color(m);
    s->cars[0].pack = c->pack;
    s->cars[0].paint = c->paint;
    s->cars[0].aero = m->aero[m->maker][menu_slot(m)];
    tg_wear(m, &s->cars[0]);
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_GARAGE || m->screen == MENU_CARD || m->screen == MENU_MILAGE) {
    /* the turntable of the colour select, the car in its parts (the mileage tally's too) */
    s->fov_x = 73.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 10.0f, s->eye[2] = 50.0f;
    s->at[0] = 0, s->at[1] = 3.0f, s->at[2] = 0;
    const float pos[3] = { 1.6f, -0.8f, 0.8f };
    mat4 mm = mat4_body(pos, m->spin * DEG, 2.2f * DEG, 0);
    const cardb_color *c = cur_color(m);
    s->cars[0].pack = c->pack;
    s->cars[0].paint = c->paint;
    s->cars[0].aero = m->aero[m->maker][menu_slot(m)];
    tg_wear(m, &s->cars[0]);
    if (m->screen != MENU_GARAGE) menu_aura_of(m, &s->cars[0]);
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_COLOR || m->screen == MENU_SETTING || (m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_TRANS)) {
    /* clMenu3dEnvironment's default projection (73 degrees), the turntable of car_common
     * (the setting screen's too, with the colour chosen) */
    s->fov_x = 73.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 10.0f, s->eye[2] = 50.0f;
    s->at[0] = 0, s->at[1] = 3.0f, s->at[2] = 0;
    const float pos[3] = { 1.6f, -0.8f, 0.8f };
    mat4 mm = mat4_body(pos, m->spin * DEG, 2.2f * DEG, 0);
    const cardb_color *c = (m->screen == MENU_COLOR ? &chosen_car(m, m->car)->colors[m->sel] : cur_color(m));
    s->cars[0].pack = c->pack;
    s->cars[0].paint = c->paint;
    /* the setting's car in its parts, as the garage showed it (push116) */
    if (m->screen == MENU_SETTING || m->cc_mode) { /* (the card's special colours: its parts too) */
      s->cars[0].aero = m->aero[m->maker][menu_slot(m)];
      tg_wear(m, &s->cars[0]);
    }
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_ST_EFF) {
    /* clSeqStTuneEffThread: the card's car scaled 0.9, turned and placed by its pose (the
     * second set once rebuilt, turning), environment 8, makeProjection(20, -1, 4/3, 5,
     * 20000), setInterest((0, 28.6, -143), (0, 5, 0)) */
    const cardb_color *c = cur_color(m);
    const float *p = STEFF_POSE[m->eff.after][m->eff.pose & 7];
    s->env = 8;
    s->fov_x = 20.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 28.6f, s->eye[2] = -143.0f;
    s->at[0] = 0, s->at[1] = 5.0f, s->at[2] = 0;
    mat4 mm = tg_rotation(p, p[3], p[4] + m->eff.angle, p[5]);
    for (int e = 0; e < 3; ++e) mm.m[e] *= 0.9f, mm.m[4 + e] *= 0.9f, mm.m[8 + e] *= 0.9f;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_GET || m->screen == MENU_TG_AERO) {
    const cardb_color *c = cur_color(m);
    int worn = m->aero[m->maker][menu_slot(m)];
    /* clSeqTgPartsGetAeroThread's environment 0, clSeqTgChangePartsAeroThread's 3 */
    s->env = m->screen == MENU_TG_GET ? 0 : 3;
    s->fov_x = 60.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 10.0f, s->eye[2] = 50.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    if (m->screen == MENU_TG_GET && tg_get_type(m) == TG_GET_WHEEL) {
      /* clSeqTgPartsGetWheelThread::yield: the wheel (data/menu3d/wheel) turned the angle
       * about y at (0, 0, -660) (DB 0x97768c0, 0x97768d0), the camera 1 degree across from
       * (0, 0, 5) at the origin */
      const float pos[3] = { 0, 0, -660.0f };
      mat4 mm = tg_rotation(pos, 0, m->tg.angle, 0);
      s->fov_x = 1.0f * DEG;
      s->env = 14;
      s->eye[0] = s->eye[1] = 0, s->eye[2] = 5.0f;
      s->get_wheel = m->tg.num + 1;
      for (int e = 0; e < 16; ++e) s->wheel_model[e] = mm.m[e];
      return 1;
    }
    if (m->screen == MENU_TG_GET) {
      /* the car the card has, as it is (clMenuCar); the picture GET screens have none */
      if (tg_get_type(m) != TG_GET_CAR) return 0;
      const float pos[3] = { 0.5f, -5.0f, -9.5f };
      mat4 mm = mat4_body(pos, (m->tg.angle - 21.3f) * DEG, 0, 0);
      s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, worn, 0, 0, 0, { 0 } };
      for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
      tg_wear(m, &s->cars[0]);
      s->n = 1;
      return 1;
    }
    int lay = tg_layout(m);
    for (int i = 0; i < m->tg.n && s->n < MENU_MAX_CARS; ++i) {
      float r[4];
      if (!anm_sprite_rect(lay, tg_frame(m, i), anm_sprite(T_TG_DOT), r)) continue;
      int o = m->tg.opt[i];
      const float pos[3] = { 0, 20.0f, m->tg.z[i] };
      mat4 mm = tg_rotation(pos, 10.0f, -30.0f, -4.0f);
      /* the arcade turns X by 10 (the nose down), Y by -30, Z by -4; the dot's scale (its rect is the 2-pixel dot's) times the model's 3 */
      float k = 3.0f * (r[2] - r[0]) / 2.0f;
      for (int e = 0; e < 3; ++e) mm.m[e] *= k, mm.m[4 + e] *= k, mm.m[8 + e] *= k;
      s->cars[s->n] = (typeof(s->cars[0])){ c->pack, c->paint, o < 0 ? worn : o, 1, r[0], r[1], { 0 } };
      tg_wear(m, &s->cars[s->n]);
      for (int e = 0; e < 16; ++e) s->cars[s->n].model[e] = mm.m[e];
      s->n++;
    }
    return 1;
  }
  if (m->screen == MENU_TG_PART && (m->tg.kind == 6 || m->tg.kind == 8)) {
    /* clSeqTgChangePartsNeonThread::yield: the place's car (a clMenuCar a place, its neon
     * on) moved (0, 1.5, -10) (DB 0x9776d90), then turned (3.5, 153.5, 2) degrees (0x9776d80);
     * environment 5, makeProjection(60, -1, 4/3, 5, 20000), setInterest((0, 3, -50), origin) */
    const cardb_color *c = cur_color(m);
    const float zero[3] = { 0, 0, 0 };
    mat4 mm = tg_rotation(zero, 3.5f, 153.5f, 2.0f);
    for (int j = 0; j < 3; ++j) mm.m[12 + j] = 1.5f * mm.m[4 + j] - 10.0f * mm.m[8 + j];
    s->fov_x = 60.0f * DEG;
    s->env = 5;
    s->eye[0] = 0, s->eye[1] = 3.0f, s->eye[2] = -50.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->cars[0].neon = m->tg.opt[m->sel];
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_PART) {
    /* the card's car (clMenuCar) with the choice on, turned and placed */
    const cardb_color *c = cur_color(m);
    int k = tg_part(m), o = m->tg.opt[m->sel];
    const float *r = TG_PART_ROT[k];
    s->fov_x = 60.0f * DEG;
    s->env = k == 2 ? 13 : 5; /* clSeqTgChangePartsMirrorThread's, the bonnet's and wing's */
    for (int e = 0; e < 3; ++e) s->eye[e] = TG_PART_EYE[k][e], s->at[e] = 0;
    mat4 mm = tg_rotation(TG_PART_POS[k], r[0], r[1], r[2]);
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    if (k == 0) s->cars[0].bonnet = o;
    else if (k == 1) s->cars[0].wing = o;
    else s->cars[0].mirror = o;
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_SELECT) {
    /* clTgDressUpPartsSelect::yield: the card's car (clMenuCar) turned -45 degrees about y
     * at the origin; environment 0, setInterest((0, 7.5, 50), (0, -2.5, 0)) */
    const cardb_color *c = cur_color(m);
    const float pos[3] = { 0, 0, 0 };
    mat4 mm = tg_rotation(pos, 0, -45.0f, 0);
    s->env = 0;
    s->fov_x = 60.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 7.5f, s->eye[2] = 50.0f;
    s->at[0] = 0, s->at[1] = -2.5f, s->at[2] = 0;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_WHEEL) {
    /* clSeqTgChangePartsWheelThread::yield: the card's car with the place's wheel in the
     * colour (clMenuCarSep, one a place and colour) turned 120 degrees about y at (-17.5,
     * 2.5, 0); environment 7, makeProjection(73, -1, 4/3, 5, 20000), setInterest((0, 12,
     * -48), (0, 0, 0)) */
    const cardb_color *c = cur_color(m);
    const float pos[3] = { -17.5f, 2.5f, 0 };
    mat4 mm = tg_rotation(pos, 0, 120.0f, 0);
    s->env = 7;
    s->fov_x = 73.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 12.0f, s->eye[2] = -48.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->cars[0].wheel = tg_wheel_value(m, m->sel, m->tg.col);
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_VINYL) {
    /* clSeqTgChangePartsStickerThread::yield: the card's car (a clMenuCar with the sticker,
     * check_body: rebuilt as it changes) moved (0.5, 3.5, -8.5) (0x9776cf0), then turned (7,
     * 124, 16) degrees (0x9776ce0); environment 5, makeProjection(60, -1, 4/3, 5, 20000),
     * setInterest((0, 35.5, -50), origin) */
    const cardb_color *c = cur_color(m);
    const float zero[3] = { 0, 0, 0 };
    mat4 mm = tg_rotation(zero, 7.0f, 124.0f, 16.0f);
    for (int j = 0; j < 3; ++j) mm.m[12 + j] = 0.5f * mm.m[j] + 3.5f * mm.m[4 + j] - 8.5f * mm.m[8 + j];
    s->fov_x = 60.0f * DEG;
    s->env = 5;
    s->eye[0] = 0, s->eye[1] = 35.5f, s->eye[2] = -50.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->cars[0].vinyl = tg_vinyl_value(m);
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_WEAR && m->tg.kind == 0) {
    /* clSeqTgWearPartsWheelThread::yield: the car (clCarModelData's body and tyres) turned
     * (0, -37, 0) degrees (DB 0x9776e00) at (12.5 - 10, -11, -34.6) (0x9776e10), the tyres
     * lifted by the jack (+0x44), the body by that and its own (+0x54, the bounce +0x50); the
     * wheel worn before (state < 0) then the new one; environment 11, makeProjection(60, -1,
     * 4/3, 5, 20000), setInterest((0, 2.2, 14), (0, 0, 0)) */
    const cardb_color *c = cur_color(m);
    const float pos[3] = { 2.5f, -11.0f + m->tg.lift + m->tg.body + m->tg.bounce, -34.6f };
    mat4 mm = tg_rotation(pos, 0, -37.0f, 0);
    s->env = 11;
    s->fov_x = 60.0f * DEG;
    s->eye[0] = 0, s->eye[1] = 2.2f, s->eye[2] = 14.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    s->cars[0].wheel = m->tg.state < 0 ? m->tg.old : m->tg.chosen;
    s->cars[0].lift = m->tg.body + m->tg.bounce;
    s->n = 1;
    return 1;
  }
  if (m->screen == MENU_TG_WEAR) {
    /* the card's car (clMenuCar) as it is now; a bonnet's, wing's or mirror's shows the
     * one worn before (car[0]) until the white (state 2: car[1]) */
    const cardb_color *c = cur_color(m);
    int k = tg_wear_k(m->tg.kind);
    const float *p = TG_WEAR_POSE[k];
    s->fov_x = 60.0f * DEG;
    s->env = k == 4 ? 16 : 11; /* clSeqTgWearPartsAeroThread's: 16 for a mirror (wk_s 4), else 11 */
    s->eye[0] = 0, s->eye[1] = 10.0f, s->eye[2] = k == 4 ? -50.0f : 50.0f;
    s->at[0] = s->at[1] = s->at[2] = 0;
    mat4 mm = tg_rotation(p + 3, p[0], k == 1 ? m->tg.ry : p[1], p[2]);
    s->cars[0] = (typeof(s->cars[0])){ c->pack, c->paint, m->aero[m->maker][menu_slot(m)], 0, 0, 0, { 0 } };
    for (int e = 0; e < 16; ++e) s->cars[0].model[e] = mm.m[e];
    tg_wear(m, &s->cars[0]);
    if (m->tg.state <= 1) {
      if (k == 2) s->cars[0].bonnet = m->tg.old;
      else if (k == 3) s->cars[0].wing = m->tg.old;
      else if (k == 4) s->cars[0].mirror = m->tg.old;
      else if (m->tg.kind == 6 || m->tg.kind == 8) s->cars[0].neon = m->tg.old;
      else if (m->tg.kind == 5) s->cars[0].vinyl = m->tg.old;
    }
    s->n = 1;
    return 1;
  }
  return 0;
}

static void draw_car_select(const menu_state *m, int clock, void (*draw3d)(void *), void *ctx) {
  if (m->screen == MENU_MAKER) {
    draw_back(ID.bg, clock % 600, 1);
    anm_draw(ID.line, clock % anm_frames(ID.line), 1);
    int f, g;
    if (m->sel >= CARDB_MAKERS) { /* (the Special Vehicles button: the last maker's idle frame, set apart) */
      draw_maker_hidden(m, 100 * (CARDB_MAKERS - 1) + m->t % 59, 1);
      return;
    }
    if (m->from >= 0) {
      int t = m->t < MAKER_MOVE ? m->t : MAKER_MOVE - 1;
      int k = m->from < m->sel ? m->from : m->sel;
      f = m->from < m->sel ? 80 + 100 * k + t : 80 + 100 * k + MAKER_MOVE - 1 - t;
      g = m->from < m->sel ? 80 + 100 * k + m->t % 5 : 84 + 100 * k - t;
    } else {
      f = 100 * m->sel + m->t % 59;
      g = 1 + 100 * m->sel + m->t % 59;
    }
    draw_maker_row(f, NULL, 0);
    anm_draw(ID.maker_car, g, 1);
    if (HID_BTN) draw_maker_hidden(m, f, 0);
    return;
  }
  int car = m->screen == MENU_CAR ? m->sel : m->car;
  const cardb_car *c = chosen_car(m, car);
  draw_back(ID.bg_carsel, clock % 240, 1);
  draw_bg_logo(c, clock % 240, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  if (draw3d) draw3d(ctx);
  anm_swap sw[3 + 12 + 1];
  int n = car_swaps(c, sw);
  anm_draw_ex(ID.car_info, clock % 240, 1, 0, 0, sw, n);
  int r35 = has(c->label, "[R35]");
  if (m->screen == MENU_CAR) {
    /* the R35 and the KPGC10 were new on this version */
    if (r35 || has(c->label, "[KPGC10]")) anm_draw(ID.car_new, m->t % 60, 1);
    anm_draw(ID.car_arrow, clock % 60, 1);
  } else {
    int nc = c->ncolors, k = strip_index(nc);
    int places = m->cc_mode || nc > 6 ? strip_slots(nc) : nc;
    for (int i = 0; i < places && i < 12; ++i)
      sw[n + i] = (anm_swap){ anm_sprite(T_SWATCH + strip_place(m, nc, i)), i < nc ? anm_sprite(c->colors[i].swatch) : -1 };
    anm_draw_ex(ID.col_strip[k], color_frame(m), 1, 0, 0, sw + n, places);
    anm_draw(ID.col_arrow[k], color_frame(m), 1);
    /* the colour's name, right-aligned on its plate (clSpriteFont, datrums 5) */
    anm_swap none = { anm_sprite(T_COL_FONT), -1 };
    anm_draw_ex(ID.col_name, 0, 1, 0, 0, &none, 1);
    int name = anm_sprite(c->colors[m->sel].name);
    anm_draw_sprite(name, 624.0f - anm_sprite_width(name), 316.0f, 1);
  }
  if (r35) anm_draw(ID.mes_r35, m->t % 60, 1);
  if (has(c->label, "[JZS161]")) anm_draw(ID.mes_jzs161, m->t % 60, 1);
}

/* ---- the top bar (clMenuBarStd): the arcade draws its countdown (menu_g_timer_x) where the
 * bar has menu_p_time_dot_3 and hides dots 1 and 2. The port has no time limit in the menus,
 * so a big infinity sign (menupack.py INFINITY_ID) stands in for the seconds. */
enum { T_TIME_DOT1 = JID(0x771), T_TIME_DOT2 = JID(0x772), T_TIME_DOT3 = JID(0x773), T_INFINITY = 102000 };
#define INF_W 128 /* the sign's sprite, centred a little right of and below the dot */
#define INF_H 72
#define INF_DX 3
#define INF_DY 6

/* the bar without its seconds (a screen with a time limit draws them: draw_vs_area) */
static void draw_topbar_timed(int bars, float fade, const anm_swap *mes) {
  anm_swap sw[4] = { { anm_sprite(T_TIME_DOT1), -1 }, { anm_sprite(T_TIME_DOT2), -1 }, { anm_sprite(T_TIME_DOT3), -1 } };
  int n = 3;
  if (mes) sw[n++] = *mes;
  anm_draw_ex(ID.top, bars, fade, 0, 0, sw, n);
}

static void draw_topbar(int bars, float fade, const anm_swap *mes) {
  draw_topbar_timed(bars, fade, mes);
  float r[4];
  int inf = anm_sprite(T_INFINITY);
  if (inf >= 0 && anm_sprite_rect(ID.top, bars, anm_sprite(T_TIME_DOT3), r))
    anm_draw_sprite(inf, r[0] + INF_DX - INF_W / 2, r[1] + INF_DY - INF_H / 2, fade);
}

/* The story's races each play their own music (clEffStoryType::getBgm by the BGM sound
 * source, src/storybgm.h; clSeqStdDayTimeSelectThread sets the BGM line to it): the select's
 * race's line, the random one kept where it has none */
static int bgm_set(const menu_state *m);
void menu_story_song(menu_state *m, int race) {
  if (m->story_pick < 0 || m->story_pick > 111 || race < 0 || race > 1) return;
  int line = STORYBGM[m->story_pick][race][bgm_set(m)];
  if (line > 0) m->song = line;
}

/* ---- the time zone and BGM select (clSeqStdDayTimeSelectThread, between the course
 * select and the race) ----
 * - menu_layout_yn_card_1_gold: the morning (0) and night (1) cards of a clMenuSpinBoxStd,
 *   100 frames a card: from 100i a 60-frame idle loop, 100i+60..69 decided, 100i+90..94 the
 *   slide on to the next card. menu_layout_yn_ring: the ring's flash, 100i..159 once.
 *   The wheel turns the spin box: here the left/right keys or a tap on its arrows.
 * - menu_g_yn_day_sound: the BGM line. The view change button steps through the list
 *   (nsMenuCheck::getBgmSelectNext): here a tap on the line. The line's name takes the
 *   place of template 0xbaf (clSpriteFont, font 0) and the soundtrack's logo that of 0x1643
 *   (set 0's own, Maxi3). Without the night map the cards stay on morning and the keys step
 *   through the BGM too. */
#define DECIDE_DAY 10
#define SLIDE 5 /* frames 100i+90..94 */
#define BGM_LINES 22
#define BGM_TEXT 101000 /* menupack.py BGM_TEXT_BASE */
enum {
  T_MORNING = JID(0x1e68), T_NIGHT = JID(0x1cdc), T_RING = JID(0x3b5), T_RING_BLUE = JID(0xb94), T_ARROW = JID(446), T_ARROW2 = JID(899),
  T_TOPBAR_MES = JID(0x1ab5), T_BGM_FONT = JID(0xbaf),
};
/* The songs of set 0 (0x93dd780) by line, as files of data/sound/bgm (tools/media/prepare_bgm.sh encodes
 * the arcade's data/sound/bgm/<dir>/<name>.wav): the four of 3DX+, the one of 3DX, then
 * Maxi3's fifteen. Line 0 is RANDOM, line 21 NO BGM. */
static const char *const SONG_FILE[BGM_LINES] = {
  NULL,
  "wm3DX_P_race_01", "wm3DX_P_race_02", "wm3DX_P_race_03", "wm3DX_P_race_04", "wm3DX_race_01_mainD",
  "wm3_01", "wm3_02", "wm3_03", "wm3_04", "wm3_05", "wm3_06", "wm3_07", "wm3_08", "wm3_09", "wm3_10",
  "wm3_11", "wm3_12", "wm3_13", "wm3_14", "wm3_15",
  NULL,
};
/* The other two sets (the card's BGM sound source: nsMenuCheck's sets 1 and 2, getBgmSelectId's
 * ids at 0x93dd780 + 25 * 4 * set, getMusicTitle's files): Maximum Tune & 2's (maxi1, maxi2)
 * and Wangan Midnight R's; RANDOM first, NO BGM last. */
static const char *const SONG_FILE_MT[25] = {
  NULL, "wm2_01", "wm2_02", "wm2_03", "wm2_04", "wm2_05", "wm2_06", "wm2_07", "wm2_08", "wm2_09", "wm2_10", "wm2_11",
  "wm2_12", "wm2_13", "wm2_14", "wm2_15", "wm2_16", "wm2_17", "wm2_18", "wm2_19", "wm2_20", "wm2_21", "wm2_22",
  "wm2_23", NULL,
};
static const char *const SONG_FILE_R[14] = {
  NULL, "wmr_01", "wmr_02", "wmr_03", "wmr_04", "wmr_05", "wmr_06", "wmr_07", "wmr_08", "wmr_09", "wmr_10", "wmr_11",
  "wmr_12", NULL,
};
/* by set: its lines (nsMenuCheck's counts at 0x93dd760), files, names (menupack.py: BGM_TEXT +
 * 30 * set + line) and soundtrack logo (getBgmSelectIcon: 0x1643, 0x1641, 0x19ce) */
static const struct { int lines; const char *const *file; int logo; } BGM_SETS[3] = {
  { BGM_LINES, SONG_FILE, JID(0x1643) }, { 25, SONG_FILE_MT, JID(0x1641) }, { 14, SONG_FILE_R, JID(0x19ce) }, /* (the Japan build's sprite ids: JID maps them) */
};
static int bgm_set(const menu_state *m);
/* The 10 out run's own list (the levels 1..10 play their loops, not the card's songs): RANDOM, the five soundtracks (app.c's TEN_LOOP rows d, e, a, b, c;
 * their names are menu_extra.bin's EX_TENBGM + row, the first two Maxi 3's, the others Maxi 2's), NO BGM. The 32-man level keeps the card's list. */
#define EX_TENBGM 110200
#define TEN_BGM_LINES 7
static int ten_pick(const menu_state *m) { return m->game == MENU_GAME_TEN && m->ten_level >= 0 && m->ten_level < 10 && m->vs.state == MENU_VS_OFF; }
static int bgm_lines(const menu_state *m) { return ten_pick(m) ? TEN_BGM_LINES : BGM_SETS[bgm_set(m)].lines; }
/* the name and the logo (-1: none) of line `line` of the 10 out run's list */
static void ten_bgm_art(int line, int *name, int *logo) {
  *name = line == 0 ? anm_sprite(BGM_TEXT) : line == TEN_BGM_LINES - 1 ? anm_sprite(BGM_TEXT + BGM_LINES - 1) : anm_sprite(EX_TENBGM + line - 1);
  *logo = line == 0 || line == TEN_BGM_LINES - 1 ? -1 : anm_sprite(BGM_SETS[line <= 2 ? 0 : 1].logo);
}
static int bgm_set(const menu_state *m);
/* the sound source a song plays from: a versus' race has the picker's (the host's first, then the last race's winner's), else the car's own */
static int bgm_play_set(const menu_state *m) { return m->vs.state != MENU_VS_OFF && m->vs.song_set > 0 ? m->vs.song_set - 1 : bgm_set(m); }
static const char *bgm_file(const menu_state *m, int line) {
  int s = bgm_play_set(m);
  return line > 0 && line < BGM_SETS[s].lines ? BGM_SETS[s].file[line] : NULL;
}

/* mseqEntry plays emBgmType 0x5e through the car select, mseqPlay 0x5f from the mode select on */
#define MUSIC_ENTRY "wm3DX_P_sys_01_ent"
#define MUSIC_SELECT "wm3DX_P_sys_02_sel"
/* clRaceThread::result2 plays 0x6d (maxi3/sys_04, looped) with clTaResult or clTaTimeUp */
/* one array: main.m tells tracks apart by pointer, and the result music goes on from the race
 * into the menus */
static const char MUSIC_RESULT[] = "sys_04";
/* the story ending's (seqRaceEnding: getBgm of select 111, emBgmType 54 maxi3/sys_05), from
 * its frame 150 (clRaceThread::race resumeStream) till the skip's or 9420's fade out (120) */
static const char MUSIC_ENDING[] = "sys_05";

float menu_bgm_level(const menu_state *m) {
  if (m->ending != 2 || m->ending_t < MENU_ENDING_SKIP) return 1.0f;
  float v = 1.0f - (float)(m->ending_t - MENU_ENDING_SKIP) / 120.0f;
  return v < 0 ? 0.0f : v;
}

const char *menu_bgm(const menu_state *m, int racing) {
  /* result2 stops the race's song as the race ends; clRaceThread::run only after the select's
   * last period, the song plays on into the next */
  if (m->ending == 2) return racing && m->ending_t >= 150 && m->ending_t < MENU_ENDING_SKIP + 120 ? MUSIC_ENDING : NULL;
  if (racing && m->finish && m->finish_story != 3) return m->finish > m->finish_wait ? MUSIC_RESULT : NULL;
  if (racing || (m->screen == MENU_PRE && m->pre_quiet)) return m->song > 0 ? bgm_file(m, m->song) : NULL;
  /* the pre-race tune check keeps the music of the screen it came from: none after
   * the time attack's BGM choice (mseqTimeAttack stops it), else the select music on */
  if (m->screen == MENU_SETTING && m->tune_confirm)
    return m->music ? MUSIC_SELECT : MUSIC_ENTRY; /* (the time attack's too: it plays through this check and stops with the race's start) */
  if (m->screen < MENU_START) return NULL; /* the attract loop: its movies have their own sound */
  /* the music stops as the race loads (the pre-race check, the HP screen, comes before) */
  if (m->screen == MENU_PRE) return NULL;
  /* the result's music plays on through the screens after a race (mseqStory: no stream change
   * from result2's 0x6d till its sequence ends; the story's tuning, the dress-up parts won),
   * the select music back on the mode select; the car setting's on the dress-up menu it opens */
  if (!m->dress_free && (m->screen == MENU_ST_TUNE || m->screen == MENU_TG_GET || m->screen == MENU_TG_AERO || m->screen == MENU_TG_PART ||
      m->screen == MENU_TG_WHEEL || m->screen == MENU_TG_VINYL || m->screen == MENU_TG_WEAR || m->screen == MENU_LVUP || m->screen == MENU_SHOGO ||
      m->screen == MENU_ST_EFF || m->screen == MENU_MILAGE || m->screen == MENU_VS_STAR || m->screen == MENU_POEM || m->screen == MENU_BONUS))
    return MUSIC_RESULT;
  if (m->dress_free) return MUSIC_ENTRY;
  return m->music ? MUSIC_SELECT : MUSIC_ENTRY;
}

/* the cards' layout: a versus' its own (menu_layout_yn_day_vs: higher, the players' dots under) */
static int day_anim(const menu_state *m) { return m->vs.day && ID.day_vs >= 0 ? ID.day_vs : ID.day_cards; }

/* Whether the tap is on the chosen time zone's card (the decide tap), or with sprite set,
 * near that sprite of the spin box (its arrows) */
static int on_card(const menu_state *m, int sprite) {
  float r[4];
  int f = 100 * m->zone + m->card_t % 60;
  if (sprite) {
    if (!anm_sprite_rect(day_anim(m), f, anm_sprite(sprite), r)) return 0;
    return m->tap_x >= r[0] - 24.0f && m->tap_x < r[2] + 24.0f && m->tap_y >= r[1] - 24.0f && m->tap_y < r[3] + 24.0f;
  }
  if (!anm_sprite_rect(day_anim(m), f, anm_sprite(m->zone ? T_NIGHT : T_MORNING), r)) return 0;
  float cx = (r[0] + r[2]) * 0.5f, cy = (r[1] + r[3]) * 0.5f, rad = (r[2] - r[0]) * 0.5f;
  return (m->tap_x - cx) * (m->tap_x - cx) + (m->tap_y - cy) * (m->tap_y - cy) < rad * rad;
}

/* the card chosen: the BGM line's song with it */
static void day_decide(menu_state *m) {
  snd_menu(SND_SELECT);
  m->bgm = m->sel;
  if (ten_pick(m)) { /* the 10 out run's: one of its five loops' sets (RANDOM draws one), or none */
    m->ten_set = m->sel == 0 ? rand() % 5 : m->sel == TEN_BGM_LINES - 1 ? 5 : m->sel - 1;
    m->song = -1;
    return;
  }
  /* getBgmSelectId: RANDOM draws one of the songs (clMTRandom over lines 1..20) */
  int n = bgm_lines(m);
  m->song = m->sel == 0 ? 1 + rand() % (n - 2) : m->sel == n - 1 ? -1 : m->sel;
}

/* one step of the time zone and BGM select; returns 1 when it decides */
static int day_select_step(menu_state *m, const menu_in *in) {
  if (m->sel >= bgm_lines(m)) m->sel = 0; /* (a line of another sound source's longer list) */
  if (m->slide) {
    /* the other card comes to rest */
    if (--m->slide == 0) {
      m->zone = (m->zone + 1) % 2;
      m->card_t = 0;
    }
    return 0;
  }
  m->card_t++;
  /* the wheel: left or right turns to the other card (with one zone, they step the music) */
  int turn = m->zones > 1 && in->dx;
  if (turn) {
    snd_menu(SND_CURSOR);
    m->slide = SLIDE;
    return 0;
  }
  /* (a versus' BGM is the host's: the others have no line) */
  int bgm = !m->vs.day || m->vs.me == m->vs.picker;
  int d = m->zones > 1 || !bgm ? 0 : in->dx;
  if (bgm && in->view) d = 1; /* the view button: the next music, as the screen says */
  if (d) {
    snd_menu(SND_CURSOR);
    m->sel = (m->sel + d + bgm_lines(m)) % bgm_lines(m);
    return 0;
  }
  if (in->ok) {
    day_decide(m);
    return 1;
  }
  return 0;
}

/* The versus' time zone vote (seqJoinDayTimeSelect: the same screen, clArg's join set): the
 * card chosen sets this machine's vote (clNet::setJoinDayTime), 20 s (at 0 the card shown is
 * the vote); voted, the card holds decided till the host's result (getJoinDayTime: the most
 * voted, a tie at random), which the card turns to. The BGM is each player's. */
#define DAY_TIME (20 * 60)
static void vs_day_step(menu_state *m, const menu_in *in) {
  /* the vote lost its lobby (the host gone, the connection lost): back to the versus screen */
  if (m->vs.state != MENU_VS_LOBBY) {
    m->vs.day = 0;
    go(m, MENU_VS);
    return;
  }
  m->vs.day_t++;
  if (m->vs.day_final >= 0) {
    if (m->zone != m->vs.day_final && !m->slide) m->zone = m->vs.day_final, m->card_t = 0;
    if (!m->vs.day_voted) m->vs.day_voted = 1, m->vs.day_t = 0;
    m->vs.day_timer = 0; /* (decided: the bar's timer at its 0) */
    m->card_t++;
    return;
  }
  if (m->vs.day_timer > 0) {
    int was = (m->vs.day_timer + 59) / 60;
    m->vs.day_timer--;
    int now = (m->vs.day_timer + 59) / 60;
    if (now != was && now >= 1 && now <= 9) snd_menu(SND_TIME_COUNT);
    if (!m->vs.day_timer && !m->vs.day_voted && !m->slide) day_decide(m), m->vs.day_voted = 1, m->vs.day_t = 0;
  }
  if (m->vs.day_voted) {
    m->card_t++;
    return;
  }
  if (day_select_step(m, in)) m->vs.day_voted = 1, m->vs.day_t = 0;
}

/* The omakase key (the bottom bar's middle, HUD_MENU_HIDDEN's place) of the area vote and the
 * time zone vote: the choice left to the others (the area strip's omakase card
 * went unseen, the time zone vote had none). On the area vote it is the strip's card 0, which
 * the cursor walks to; on the time zone vote the card shown stays, the BGM line its song. */
static int vs_omakase_shown(const menu_state *m) {
  if (m->vs.state != MENU_VS_LOBBY) return 0;
  if (m->screen == MENU_VS_AREA) return !m->vs.voted && m->vs.area_final < 0;
  return m->screen == MENU_BGM && m->vs.day && !m->vs.day_voted && m->vs.day_final < 0 && !m->slide;
}

static void vs_omakase(menu_state *m) {
  if (m->screen == MENU_VS_AREA) {
    snd_menu(SND_SELECT);
    m->vs.voted = 2;
    m->vs.vote = 0;
    m->vs.vote_t = 0;
  } else {
    day_decide(m);
    m->vs.day_voted = 2;
    m->vs.day_t = 0;
  }
}

/* the key, or (voted so) its caption alone */
static void draw_vs_omakase(const menu_state *m, float screen_w, float screen_h) {
  int voted = m->screen == MENU_VS_AREA ? m->vs.voted == 2 && m->vs.area_final < 0
                                        : m->vs.day_voted == 2 && m->vs.day_final < 0;
  if (!voted && !vs_omakase_shown(m)) return;
  if (!voted) hud_draw_menu_hidden(screen_w, screen_h, m->key_held == HUD_MENU_HIDDEN);
  const char *s = voted ? "LEFT TO THE OTHERS" : "LEAVE IT TO THE OTHERS";
  float px = 2.4f;
  hud_ui_begin(screen_w, screen_h);
  hud_ui_text(s, 0.5f * (screen_w - 1024.0f) + 764.0f - 0.5f * hud_ui_text_width(s, px), 680.0f - 3.5f * px, px, 1.0f, voted ? 0.55f : 1.0f,
              voted ? 0.1f : 1.0f, 1);
  hud_ui_end();
}

void menu_vs_day_open(menu_state *m) {
  go(m, MENU_BGM);
  m->vs.day = 1;
  m->vs.day_timer = DAY_TIME;
  m->vs.day_voted = m->vs.day_t = 0;
  m->vs.day_final = -1;
  m->sel = m->bgm;
  m->slide = m->card_t = 0;
  if (m->zone >= m->zones) m->zone = 0;
}

static void vs_timer(int timer, int bars, float fade);

/* the versus' "Please wait!" (menu_layout_wait_1: with "Waiting on other player(s)", over the screen dimmed a little; _2: the words alone, over
 * black, the screens' last before the race's cutscene), t the frames it has been up */
static void draw_wait(int black, int t, float fade) {
  int a = black ? ID.wait2 : ID.wait1;
  if (!black) anm_fill(0, 0, 0, 0.35f * fade);
  if (a < 0) return;
  int n = anm_frames(a);
  anm_draw(a, n > 0 ? (t < n ? t : n - 1) : 0, fade);
}

/* enmUpdate's marks of the others' cards (0x8775884): menu_layout_yn_day_vs's template dots
 * menu_p_day_dot01..10 (1463..1472, hidden by the run), a card's n (1..3) taking those of its
 * row (0x93f06c0 morning, 0x93f06f0 night), each set to the player's menu_g_vs_ex_player
 * (frames 1..59 round, its icon menu_p_vs_4p_<n>: 0x93f0748). The dots' places are the
 * layout's (the templates have no pictures to pack, so not looked up): the chosen card's row
 * at y 324, the other's at 271; the chosen card's hop on its decided frames (60..68, 20.2 px
 * a frame down and back); none while the cards slide. */
#define DAY_MARK_SLOT0 JID(1463)
#define DAY_MARK_SLOTS 10
#define DAY_ICON JID(0x39e) /* menu_p_vs_4p's template (the area vote's A_ICON) */
static const signed char DAY_MARK[2][3][3] = { { { 1 }, { 6, 7 }, { 0, 1, 2 } }, { { 4 }, { 8, 9 }, { 3, 4, 5 } } };
static const float DAY_DOT[2][DAY_MARK_SLOTS][2] = {
  { { 119, 324 }, { 191, 324 }, { 263, 324 }, { 404, 271 }, { 476, 271 }, { 548, 271 }, { 155, 324 }, { 227, 324 }, { 440, 271 }, { 512, 271 } },
  { { 94.5f, 271 }, { 166.5f, 271 }, { 238.5f, 271 }, { 376, 324 }, { 448, 324 }, { 520, 324 }, { 130, 271 }, { 202.5f, 271 }, { 412, 324 }, { 484, 324 } },
};
static void day_marks(const menu_state *m, int f, int clock, float fade) {
  if (m->slide) return;
  int on[2][3], n[2] = { 0 }, state = f >= 100;
  for (int i = 0; i < m->vs.n && i < MENU_VS_PLAYERS; ++i) {
    if (i == m->vs.me || m->vs.other_day[i] < 0 || m->vs.other_day[i] == 2) continue;
    int z = m->zones > 1 ? m->vs.other_zone[i] & 1 : 0;
    if (n[z] < 3) on[z][n[z]++] = i;
  }
  int t = f % 100 - 60; /* the decided frames' hop */
  float hop = t >= 0 && t <= 8 ? 20.2f * (float)(t < 4 ? t + 1 : 9 - t) : 0.0f;
  for (int z = 0; z < 2; ++z)
    for (int k = 0; k < n[z]; ++k) {
      const float *d = DAY_DOT[state][DAY_MARK[z][n[z] - 1][k]];
      int i = on[z][k];
      anm_swap sw = { anm_sprite(DAY_ICON), anm_sprite(DAY_ICON + i) };
      /* still choosing: fainter */
      anm_draw_ex(ID.vs_ex_player, 1 + clock % 59, fade * (m->vs.other_day[i] ? 1.0f : 0.55f), d[0],
                  d[1] + (z == state ? hop : 0.0f), &sw, 1);
    }
}

static void draw_day_select0(const menu_state *m, int clock, float fade, int bars);
static void draw_day_select(const menu_state *m, int clock, float fade, int bars) {
  draw_day_select0(m, clock, fade, bars);
  /* (the time up: the vote waits on the others, dimmed; the timer at its 0) */
  if (m->vs.day && m->vs.day_timer == 0 && m->vs.day_final < 0) draw_wait(0, m->vs.day_t, fade);
}

static void draw_day_select0(const menu_state *m, int clock, float fade, int bars) {
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  int z = 100 * m->zone, f = z + m->card_t % 60;
  if (m->slide) f = z + 90 + SLIDE - m->slide;
  /* decided (a versus' vote: held so till the result) */
  int t = m->deciding ? DECIDE_DAY + FADE - m->deciding : m->vs.day && m->vs.day_voted && !m->slide ? m->vs.day_t : -1;
  if (t >= 0) f = z + 60 + (t < DECIDE_DAY ? t : DECIDE_DAY - 1);
  /* the spin box's templates: the cards and rings of this screen (its stPimpl); without
   * the night map the arrows that point the way to the other card are left out */
  int one = m->zones < 2;
  anm_swap cards[4 + DAY_MARK_SLOTS] = { { anm_sprite(T_MORNING), anm_sprite(JID(0x10a2)) }, { anm_sprite(T_NIGHT), anm_sprite(JID(0xb93)) },
                                         { anm_sprite(T_ARROW), one ? -1 : anm_sprite(T_ARROW) },
                                         { anm_sprite(T_ARROW2), one ? -1 : anm_sprite(T_ARROW2) } };
  for (int k = 0; k < DAY_MARK_SLOTS; ++k) cards[4 + k] = (anm_swap){ anm_sprite(DAY_MARK_SLOT0 + k), -1 };
  anm_swap rings[2] = { { anm_sprite(T_RING), anm_sprite(JID(0xb95)) }, { anm_sprite(T_RING_BLUE), anm_sprite(JID(0x1b0b)) } };
  anm_draw_ex(day_anim(m), f, fade, 0, 0, cards, 4 + DAY_MARK_SLOTS);
  if (m->vs.day) day_marks(m, f, clock, fade);
  int ring = m->vs.day && ID.day_vs_ring >= 0 ? ID.day_vs_ring : ID.day_ring;
  if (!m->slide) anm_draw_ex(ring, z + (m->card_t < 59 ? m->card_t : 59), fade, 0, 0, rings, 2);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(JID(0x105a)) };
  if (m->vs.day) {
    draw_topbar_timed(bars, fade, &title);
    vs_timer(m->vs.day_timer, bars, fade);
  } else {
    draw_topbar(bars, fade, &title);
  }
  draw_bottom(bars, fade);
  if (m->vs.day && m->vs.me != m->vs.picker) return; /* a versus' song is the picker's */
  int set = bgm_set(m);
  anm_swap name[2] = { { anm_sprite(T_BGM_FONT), anm_sprite(BGM_TEXT + 30 * set + m->sel) },
                       { anm_sprite(JID(0x1643)), anm_sprite(BGM_SETS[set].logo) } };
  int n = set ? 2 : 1;
  if (ten_pick(m)) { /* (the 10 out run's list: its soundtracks' names, and their logos but for RANDOM and NO BGM) */
    int nm, lg;
    ten_bgm_art(m->sel < TEN_BGM_LINES ? m->sel : 0, &nm, &lg);
    name[0].to = nm, name[1].to = lg;
    n = lg >= 0 ? 2 : 1;
  }
  anm_draw_ex(ID.day_sound, 0, fade, 0, 0, name, n);
}

/* ---- the car setting (clSeqCarSettingThread: in this build only its debug menus run it,
 * the art is all there), between the colour select and the mode select ----
 * Over the colour select's turntable: STOCK or FULL TUNE top left and AT or MT top right,
 * tapped, on menu_layout_setting's course band (a car picked from the save's keeps its
 * card's STOCK or FULL TUNE: that chip alone). With full tune the left/right keys move the
 * balance as the arcade's wheel does (left more power, right more handling, SE 0x19):
 * power and handling share CAR_TUNE_TOTAL points, neither under 10 (m_select_gage_max =
 * total - 10), the wheel prompt menu_g_setting_handle looping meanwhile. Below, where
 * menu_layout_setting has them, Power and Handling over their gauges (initGageAnime: 24
 * lights each, x from the table at 0x93f0780 less 34 past 30 points, y 266 and 304; lit by
 * ten in the s1, s2 and s3 colours), the horsepower (GetHorsePower) and the balance badges
 * DG..HG (menu_g_setting_setting's, the car's lit: GetHandling). A tap on the car goes on. */
typedef char garage_fits[CARDB_ALL <= MENU_GARAGE_MAKERS && CARDB_MAX_CARS <= MENU_GARAGE_CARS ? 1 : -1];
enum {
  T_SET_BAND = JID(3364), T_SET_POWER = JID(3363), T_SET_HANDLING = JID(5695), T_SET_LIGHT = JID(7315), /* s1..s3 */
  T_SET_EMPTY = JID(6270), T_SET_DIGIT = JID(5684), T_SET_HP = JID(4067), T_SET_COURSE_BAND = JID(6804), T_SET_TITLE = JID(6639),
};
/* the badges dark and lit, DG, D, B, G, HG (car_tune_grade order) */
static const int SET_BADGE[5][2] = { { JID(7084), JID(7085) }, { JID(6800), JID(6801) }, { JID(6798), JID(6799) }, { JID(6802), JID(6803) }, { JID(7086), JID(7087) } };
static const short GAGE_X[CAR_TUNE_MAX] = { 171, 190, 209, 228, 247, 266, 285, 304, 323, 342, 381, 400,
                                            419, 438, 457, 476, 495, 514, 533, 552, 591, 610, 629, 648 };
#define GAGE_DX (-34.0f) /* past 30 points */
#define SET_TEXT 101100  /* menupack.py SETTING_TEXT_BASE: STOCK, FULL TUNE, AT, MT, the hint */
#define SET_ROW_Y 101.0f /* the chips' top, on the course band (84..148) */
#define SET_CHIP_GAP 28.0f
#define SET_HP_X 268.0f
#define SET_BADGE_X 140.0f /* the first badge's centre (menu_layout_setting: 191.5, 95 apart) */
#define SET_BADGE_DX 90.0f
#define SET_MIN_POWER (CAR_TUNE_TOTAL - CAR_TUNE_MAX)
/* the car on the turntable, where a tap goes on */
#define SET_CAR_X0 150.0f
#define SET_CAR_X1 490.0f
#define SET_CAR_Y0 150.0f
#define SET_CAR_Y1 250.0f

/* the car's tune: the FULL TUNE's setting, else the story's steps (the card's tune, which
 * the arcade's every mode races on; a stock car's setting and card showed 280 hp for a car
 * the story had brought to 400) */
void menu_tune(const menu_state *m, int *power, int *handling) {
  int full = m->tune_full[m->maker][menu_slot(m)];
  *power = full ? m->tune_power[m->maker][menu_slot(m)] : m->st_power[m->maker][menu_slot(m)];
  *handling = full ? CAR_TUNE_TOTAL - *power : m->st_handling[m->maker][menu_slot(m)];
}

/* DONE in the bottom bar's middle, as the other screens' keys there (hud.c MENU_HIDDEN: 328, 652,
 * 368 x 56 in the 1024 x 768 layout): the card settings' and the tune check's */
static const float CS_DONE[4] = { 205, 407.5f, 230, 35 };
/* the bottom bar keys (the arrows, DONE, the hints) are the touch HUD's: the arcade HUD's bar is blank for now */
static int bar_keys(void) { return hud_style() == HUD_STYLE_TOUCH; }

/* ---- the pre-race tune check: the car setting (its gauges, the balance on the bottom bar's
 * keys, AT / MT) with DONE in the bottom bar's middle, before a race's title page; its back
 * key goes back to the screen it came from ---- */
static void tune_confirm_enter(menu_state *m, int from) {
  int music = m->music;
  car_select_enter(m, MENU_SETTING);
  m->music = music; /* (the music of the screen before goes on: menu_bgm) */
  m->tune_confirm = from;
}
static int tune_confirm_done_hit(const menu_state *m) {
  const float *d = CS_DONE;
  return bar_keys() && m->tap_x >= d[0] && m->tap_x < d[0] + d[2] && m->tap_y >= d[1] - 6 && m->tap_y < d[1] + d[3] + 6;
}
static void draw_tune_confirm(const menu_state *m, float screen_w, float screen_h) {
  const float *d = CS_DONE, k16 = 1.6f;
  if (m->vs.tuned) { /* (done: the others are still choosing) */
    const char *w = "WAITING FOR THE OTHERS";
    hud_ui_begin(screen_w, screen_h);
    hud_ui_text(w, 0.5f * screen_w - 0.5f * hud_ui_text_width(w, 3.5f), 427.0f * k16 - 3.5f * 3.5f, 3.5f, 1, 0.85f, 0.4f, 1);
    hud_ui_end();
    return;
  }
  if (!bar_keys()) return;
  hud_ui_begin(screen_w, screen_h);
  hud_ui_button(d[0] * k16, d[1] * k16, d[2] * k16, d[3] * k16, 0);
  hud_ui_text("DONE", (d[0] + 0.5f * d[2]) * k16 - 0.5f * hud_ui_text_width("DONE", 4.0f), (d[1] + 0.5f * d[3]) * k16 - 14.0f,
              4.0f, 1, 1, 1, 1);
  hud_ui_end();
}

/* a story-tuned car (STOCK chosen) whose earned tune can be split again: 20 steps or more */
static int st_rebalance_ok(const menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m);
  return !m->tune_full[mk][sl] && !m->new_card && m->st_power[mk][sl] + m->st_handling[mk][sl] >= 20;
}

/* the chips' left edges: AT, MT (top right of the course band) */
static void setting_chips(float x[2]) {
  x[1] = 616.0f - anm_sprite_width(anm_sprite(SET_TEXT + 3));
  x[0] = x[1] - SET_CHIP_GAP - anm_sprite_width(anm_sprite(SET_TEXT + 2));
}

/* one step of the car setting; returns 1 when it goes on */
/* ---- the card's settings (MENU_CARDSET; the arcade's card check, seqCardDisp8's
 * cardcheck_7 and seqCardDisp9's cardcheck_8, as the card goes in), from the car setting's
 * METER / BGM key: per car (the card's). Meter (+0x26c, clStandardConsole: 1 For Racing sets
 * +0x299, 2 SPECIAL +0x29b for drawMeter): Normal, For Racing, and SPECIAL once the second
 * undefeated bonus is in (hasMaxi2NoLosePoint or +0x278: cardcheck_7_sel_meter2). BGM sound
 * source (cardcheck_8: sound_sel_no / _2 / sound_sel by +0x279 and +0x186): 3/3DX/3DX+
 * always, Maximum Tune & 2 with the story clear bonus, Wangan Midnight R with the undefeated
 * bonus; the BGM select's set (nsMenuCheck's 0, 1, 2) follows it. Locked choices show dim
 * with LOCKED under them. The arcade's own words and logos (menu_p_card_7_*, _8_sound_*),
 * the rest the HUD's letters. The back key or DONE back to the car setting. ---- */
enum { S_CS_METER = JID(0xEED), S_CS_NORMAL = JID(0x11F8), S_CS_RACING = JID(0xC5A), S_CS_SPECIAL = JID(0xC5B), S_CS_SOUND = JID(0xEEE),
       S_CS_3DX = JID(0x158E), S_CS_MT = JID(0x158C), S_CS_R = JID(0x1589), S_CS_BAR = JID(0x8B7) };
static const int CS_SPRITE[2][3] = { { S_CS_NORMAL, S_CS_RACING, S_CS_SPECIAL }, { S_CS_3DX, S_CS_MT, S_CS_R } };
/* the sound row's logos lit (the chosen one) and dim (menu_p_card_8_sound_<set>_1 and _2), in CS_SPRITE's order: 3DX, MT, R */
static const int CS_SOUND_LIT[3] = { JID(0x158E), JID(0x158B), JID(0x1588) }, CS_SOUND_DIM[3] = { JID(0x158F), JID(0x158C), JID(0x1589) };
#define CS_ROW0 150.0f /* the rows' middles (640 x 480) */
#define CS_ROW1 290.0f
#define CS_X0 196.0f /* the choices' centres */
#define CS_DX 150.0f

/* whether choice k of row (0 meter, 1 sound) is open for the car (st_bonus_car's bits) */
static int cs_open(const menu_state *m, int row, int k) {
  (void)m, (void)row, (void)k;
  return 1; /* every meter and soundtrack is open from the start (the arcade's story bonuses unlocked them; st_bonus_car's bits 1, 2 and 4) */
}

static int bgm_set(const menu_state *m) {
  if (m->maker < 0 || m->car < 0) return 0;
  int s = m->sound_car[m->maker][menu_slot(m)];
  return s > 0 && s < 3 && cs_open(m, 1, s) ? s : 0;
}

int menu_meter(const menu_state *m) {
  if (m->maker < 0 || m->car < 0) return 0;
  int s = m->meter_car[m->maker][menu_slot(m)];
  return s > 0 && s < 3 && cs_open(m, 0, s) ? s : 0;
}

static void cardset_open(menu_state *m) {
  go(m, MENU_CARDSET);
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
  m->focus = 0; /* the meter row */
}

static void card_enter(menu_state *m, int back);

/* the top bar of one of the port's own screens (menu_extra.bin's titles 20 .. 26: key bindings, volume, meter and BGM, HUD options, information, debug options, vibration): 1 when drawn, else
 * the caller puts its words */
static int draw_title_bar(int bars, int k) {
  int s = anm_sprite(110100 + 20 + k);
  if (s < 0) return 0;
  anm_swap title = { anm_sprite(T_TOPBAR_MES), s };
  draw_topbar(bars, 1, &title);
  return 1;
}

/* one step of the card's settings: up / down the row, left / right the choice, OK or back done; the pointer chooses a place outright
 * (select = row * 3 + choice) */
static void cardset_step(menu_state *m, const menu_in *in) {
  if (in->back || in->ok) {
    snd_menu(SND_CANCEL);
    if (m->carmenu) { /* (opened from the Car / Settings discs) */
      m->carmenu = 0;
      selmenu_enter(m, SELMENU_CAR, 2);
      return;
    }
    card_enter(m, m->card_back); /* (opened from the card check) */
    return;
  }
  if (in->dy) m->focus = in->dy > 0;
  int row = m->focus, k = -1;
  signed char *v = row ? &m->sound_car[m->maker][menu_slot(m)] : &m->meter_car[m->maker][menu_slot(m)];
  if (in->select >= 0) row = in->select / 3, k = in->select % 3;
  else if (in->dx) k = *v + in->dx;
  if (k < 0 || k > 2) return;
  v = row ? &m->sound_car[m->maker][menu_slot(m)] : &m->meter_car[m->maker][menu_slot(m)];
  if (!cs_open(m, row, k)) {
    snd_menu(SND_CANCEL);
  } else if (*v != k) {
    snd_menu(SND_CURSOR);
    *v = (signed char)k;
    m->garage_changed = 1;
  }
}

static void draw_cardset(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  int now[2] = { menu_meter(m), bgm_set(m) }, title[2] = { S_CS_METER, S_CS_SOUND };
  for (int row = 0; row < 2; ++row) {
    float y = row ? CS_ROW1 : CS_ROW0;
    anm_draw_sprite(anm_sprite(T_SET_BAND), -192.0f, y - 50.0f, 0.5f);
    anm_draw_sprite(anm_sprite(title[row]), 24.0f, y - 64.0f, 1);
    for (int k = 0; k < 3; ++k) {
      int s = anm_sprite(row == 1 ? (now[row] == k ? CS_SOUND_LIT[k] : CS_SOUND_DIM[k]) : CS_SPRITE[row][k]);
      if (s < 0) s = anm_sprite(CS_SPRITE[row][k]);
      if (s < 0) continue;
      float w = anm_sprite_width(s), h = anm_sprite_height(s), sc = row ? 0.9f : 0.75f;
      float x = CS_X0 + CS_DX * (float)k, a = !cs_open(m, row, k) ? 0.18f : now[row] == k ? 1.0f : 0.4f;
      float src[4] = { 0, 0, w, h }, dst[4] = { x - 0.5f * w * sc, y - 0.5f * h * sc, x + 0.5f * w * sc, y + 0.5f * h * sc };
      anm_draw_part(s, src, dst, a);
    }
    anm_draw_sprite(anm_sprite(S_CS_BAR), 0, y + 42.0f, 1);
  }
  draw_bottom(bars, 1);
  int tb = draw_title_bar(bars, 2);
  /* the chosen one's frame, LOCKED under the locked ones, DONE, the title: the HUD's */
  float k16 = 1.6f;
  hud_ui_begin(screen_w, screen_h);
  for (int row = 0; row < 2; ++row) {
    float y = row ? CS_ROW1 : CS_ROW0;
    if (m->focus == row) hud_ui_text(">", 6.0f * k16, (y - 6.0f) * k16, 4.0f, 1.0f, 0.42f, 0.05f, 1); /* the row the keys move */
    for (int k = 0; k < 3; ++k) {
      float x = CS_X0 + CS_DX * (float)k;
      if (now[row] == k) {
        float x0 = (x - 64.0f) * k16, y0 = (y - 30.0f) * k16, w = 128.0f * k16, h = 60.0f * k16;
        hud_ui_rect(x0, y0, w, 3, 1.0f, 0.42f, 0.05f, 0.95f);
        hud_ui_rect(x0, y0 + h - 3, w, 3, 1.0f, 0.42f, 0.05f, 0.95f);
      }
      if (!cs_open(m, row, k)) {
        const char *s = "LOCKED";
        hud_ui_text(s, x * k16 - 0.5f * hud_ui_text_width(s, 2.5f), (y + 26.0f) * k16, 2.5f, 0.85f, 0.85f, 0.85f, 1);
      }
    }
  }
  const char *t = "CARD SETTINGS";
  if (!tb) hud_ui_text(t, 24.0f * k16, 40.0f * k16, 4.0f, 1, 1, 1, 1);
  const float *d = CS_DONE;
  if (bar_keys()) {
    hud_ui_button(d[0] * k16, d[1] * k16, d[2] * k16, d[3] * k16, 0);
    hud_ui_text("DONE", (d[0] + 0.5f * d[2]) * k16 - 0.5f * hud_ui_text_width("DONE", 4.0f), (d[1] + 0.5f * d[3]) * k16 - 14.0f,
                4.0f, 1, 1, 1, 1);
  }
  hud_ui_end();
}

/* the colour select on the card's special colours: its own colour first, then each one the card has won */
static void ccolor_open(menu_state *m) {
  int mk = m->maker, sl = menu_slot(m), n = menu_cc_unlocked(m, mk, sl);
  if (n <= 0) {
    snd_menu(SND_CANCEL);
    return;
  }
  const cardb_car *base = &CARDB_CARS[mk][m->car];
  CC_CAR = *base;
  CC_CAR.colors[0] = base->colors[m->color];
  for (int i = 0; i < n && i < 10; ++i) {
    CC_CAR.colors[1 + i] = base->colors[m->color];
    CC_CAR.colors[1 + i].swatch = base->custom[i].swatch, CC_CAR.colors[1 + i].name = base->custom[i].name, CC_CAR.colors[1 + i].paint = base->custom[i].paint;
  }
  CC_CAR.ncolors = 1 + n;
  snd_menu(SND_SELECT);
  m->cc_mode = 1;
  m->cc_back = m->card_back;
  car_select_enter(m, MENU_COLOR);
  m->sel = m->ccolor[mk][sl] <= n ? m->ccolor[mk][sl] : 0;
}

static int setting_step(menu_state *m, const menu_in *in) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), key = in->dx;
  m->spin -= 0.3f;
  if (key) {
    if (!m->tune_full[mk][sl]) {
      /* the tune the story earned split again (実戦調教), by clSeqCarSettingThread's
       * rules (0x877d278): the total kept, each of power and handling at least 10 (so a total
       * under 20 stays as it is), the wheel left a step more power; the story's next step goes
       * on top of the split (seqStTune) */
      int p = m->st_power[mk][sl], h = m->st_handling[mk][sl], t = p + h, np = p - key;
      if (!st_rebalance_ok(m) || np < 10 || t - np < 10 || np > CAR_TUNE_MAX || t - np > CAR_TUNE_MAX) return 0;
      snd_menu(SND_CURSOR);
      m->st_power[mk][sl] = (signed char)np;
      m->st_handling[mk][sl] = (signed char)(t - np);
      m->garage_changed = 1;
      return 0;
    }
    int p = m->tune_power[mk][sl] - key;
    if (p < SET_MIN_POWER || p > CAR_TUNE_MAX) return 0;
    snd_menu(SND_CURSOR);
    m->tune_power[mk][sl] = (signed char)p;
    m->garage_changed = 1;
    return 0;
  }
  if (in->dy) { /* the shifter: up the manual box, down the automatic */
    int want = in->dy < 0;
    if (m->manual != want) snd_menu(SND_CURSOR), m->manual = want, m->garage_changed = 1;
    return 0;
  }
  if (in->view) { /* the view button: AT or MT */
    snd_menu(SND_CURSOR);
    m->manual = !m->manual;
    m->garage_changed = 1;
    return 0;
  }
  if (in->ok) { /* OK (the gas): on to the next screen */
    snd_menu(SND_SELECT);
    return 1;
  }
  if (in->tab >= 2 && in->tab <= 3) { /* a button: 2 AT, 3 MT */
    int want = in->tab & 1;
    if (m->manual != want) {
      snd_menu(SND_CURSOR);
      m->manual = want;
      m->garage_changed = 1;
    }
  }
  return 0;
}

static void draw_setting(const menu_state *m, int clock, void (*draw3d)(void *), void *ctx, int bars) {
  const cardb_car *c = chosen_car(m, m->car);
  int power, handling;
  menu_tune(m, &power, &handling);
  int full = m->tune_full[m->maker][menu_slot(m)];
  draw_back(ID.bg_carsel, clock % 240, 1);
  draw_bg_logo(c, clock % 240, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  if (draw3d) draw3d(ctx);
  /* the choices on the course band, the chosen ones lit */
  anm_draw_sprite(anm_sprite(T_SET_COURSE_BAND), -1.7f, 84.3f, 0.5f);
  if (full || st_rebalance_ok(m)) anm_draw(ID.set_handle, clock % anm_frames(ID.set_handle), 1);
  /* the gauges */
  anm_draw_sprite(anm_sprite(T_SET_BAND), -192.0f, 246.0f, 0.5f);
  anm_draw_sprite(anm_sprite(T_SET_POWER), 7.0f, 256.0f, 1);
  anm_draw_sprite(anm_sprite(T_SET_HANDLING), 6.0f, 290.0f, 1);
  for (int row = 0; row < 2; ++row) {
    int n = row ? handling : power;
    float y = row ? 304.0f : 266.0f;
    for (int i = 0; i < CAR_TUNE_MAX; ++i) {
      float gx = (float)GAGE_X[i] + GAGE_DX - 32.0f, gy = y - 32.0f;
      anm_draw_sprite(anm_sprite(T_SET_EMPTY), gx, gy, 1);
      if (i < n) anm_draw_sprite(anm_sprite(T_SET_LIGHT + i / 10), gx, gy, 1);
    }
  }
  /* the horsepower (three digits and HP) between the choices, the left/right keys having
   * the place menu_layout_setting gives it */
  int hp = car_tune_hp(c->hp, power);
  for (int i = 0, div = 100; i < 3; ++i, div /= 10)
    anm_draw_sprite(anm_sprite(T_SET_DIGIT + hp / div % 10), SET_HP_X + 22.0f * (float)i, SET_ROW_Y - 18.0f, 1);
  anm_draw_sprite(anm_sprite(T_SET_HP), SET_HP_X + 70.0f, SET_ROW_Y + 2.0f, 1);
  /* the balance badges, the car's lit, drawn in from the keys */
  int grade = car_tune_grade(power, handling);
  for (int k = 0; k < 5; ++k)
    anm_draw_sprite(anm_sprite(SET_BADGE[k][k == grade]), SET_BADGE_X + SET_BADGE_DX * (float)k - 64.0f, 347.5f - 32.0f, 1);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(T_SET_TITLE) };
  if (m->vs.tuning) draw_topbar_timed(bars, 1, &title); /* (its own seconds: no infinity under them) */
  else draw_topbar(bars, 1, &title);
  if (m->vs.tuning) vs_timer(m->vs.tune_timer, bars, 1.0f); /* (the 15 seconds, in the top bar's timer as the votes') */
  draw_bottom(bars, 1);
  /* AT and MT: the arcade's own words, on the transmission select's colours (the AT's blue glow, the MT's red), the one in use lit */
  {
    static const int GLOW[2] = { JID(0x1dbe), JID(0x1cdd) };
    float cx[2];
    setting_chips(cx);
    for (int k = 0; k < 2; ++k) {
      int word = anm_sprite(SET_TEXT + 2 + k), glow = anm_sprite(GLOW[k]);
      float w = anm_sprite_width(word), h = anm_sprite_height(word), a = m->manual == k ? 1.0f : 0.35f;
      /* (the glow sprites' light sits right of their middle: shifted back under the words; the chosen one breathes) */
      float pulse = m->manual == k ? 0.5f + 0.5f * cosf((float)clock * 0.12f) : 0.0f, sc = 1.0f + 0.08f * pulse, mx = cx[k] + 0.5f * w - (k ? 7.5f : 5.6f), my = SET_ROW_Y + 0.5f * h, hw = (0.5f * w + 22.0f) * sc, hh = (0.5f * h + 14.0f) * sc;
      if (glow >= 0) anm_draw_part(glow, (float[4]){ 0, 0, anm_sprite_width(glow), anm_sprite_height(glow) }, (float[4]){ mx - hw, my - hh, mx + hw, my + hh }, m->manual == k ? 0.75f + 0.25f * pulse : a);
      anm_draw_sprite(word, cx[k], SET_ROW_Y, a);
    }
  }
  int hint = anm_sprite(SET_TEXT + 4);
  if (!bars && !m->tune_confirm) /* (the pre-race check has DONE in its place) */
    if (bar_keys()) anm_draw_sprite(hint, 320.0f - anm_sprite_width(hint) * 0.5f, 427.0f, 0.75f + 0.25f * cosf((float)clock * 0.1f));
}

/* The setting's sprites (the wheel prompt's too), kept up from the colour select on and
 * uploaded one a frame, so neither the setting's first frame nor a tap on FULL TUNE takes
 * the uploads. */
static void setting_warm(void) {
  enum { MAX_WARM = 96 };
  static int list[MAX_WARM], n = -1;
  if (n < 0) {
    static const int ids[] = { T_SET_BAND, T_SET_POWER, T_SET_HANDLING, T_SET_LIGHT, T_SET_LIGHT + 1, T_SET_LIGHT + 2,
                               T_SET_EMPTY, T_SET_HP, T_SET_COURSE_BAND, T_SET_TITLE };
    n = 0;
    for (int k = 0; k < 5; ++k) list[n++] = anm_sprite(SET_TEXT + k);
    for (int k = 0; k < (int)(sizeof ids / sizeof ids[0]); ++k) list[n++] = anm_sprite(ids[k]);
    for (int k = 0; k < 10; ++k) list[n++] = anm_sprite(T_SET_DIGIT + k);
    for (int k = 0; k < 5; ++k) list[n++] = anm_sprite(SET_BADGE[k][0]), list[n++] = anm_sprite(SET_BADGE[k][1]);
    n += anm_sprites(ID.set_handle, list + n, MAX_WARM - n);
  }
  int fresh = 0;
  for (int i = 0; i < n; ++i) fresh += anm_warm_sprite(list[i], !fresh);
}

/* ---- the port's OPTION screen (not the arcade's: its cabinet has a wheel), from the gear
 * key on the SAVE DATA screen or the maker select, in the car setting's style: CONTROLS,
 * steering by the touch keys (BUTTONS) or by tilting the iPad (GYRO) and the tilt's
 * sensitivity (1..10, the left/right keys or a tap on its lights); GRAPHICS, the bridge
 * reflection on the car, the FSR sharpening, the cars' baked ambient occlusion
 * (tools/stage/bake_ao.c) and the back mirror (app.c draw_backmirror). The back key goes
 * back to the screen it came from. ---- */
#define OPT_TEXT 101900 /* menupack.py OPTION_TEXT_BASE */
enum {
  O_CONTROLS, O_BUTTONS, O_GYRO, O_STEERING, O_SENS, O_GRAPHICS, O_REFLECTION, O_ON, O_OFF, O_TILT, O_TITLE,
  O_SHARP, O_HIDDEN /* the maker select's hidden cars key */, O_PEDALS, O_AO, O_BM,
  O_ALL_ON, O_ALL_OFF /* the colour select's ALL PARTS key */,
  O_HD, O_BLOOM, O_SSAO, O_WET, O_PAINT, O_AA, O_4X, O_2X /* Android's GRAPHICS rows */,
  O_TEST /* the testers' switch's note: off, then the ten divisions */ , O_N = O_TEST + 11
};

/* the maker layout's row, moved and scaled (MAKER_A ...): not its strip behind it, which is the width of the screen */
static void draw_maker_row(int f, const anm_swap *sw, int n) {
  int back = anm_sprite(T_MK_BACK), all[256], nall = anm_sprites(ID.maker_layout, all, 256);
  anm_swap only[256], moved[8];
  int no = 0, nm = 0;
  for (int i = 0; i < nall && no < 256; ++i)
    if (all[i] != back) only[no++] = (anm_swap){ all[i], -1 };
  anm_draw_ex(ID.maker_layout, f, 1, 0, 0, only, no); /* (the strip alone, where it was) */
  for (int i = 0; i < n && nm < 7; ++i) moved[nm++] = sw[i];
  moved[nm++] = (anm_swap){ back, -1 };
  anm_draw_xy(ID.maker_layout, f, 1, MAKER_A, MAKER_B, MAKER_Z, MAKER_Z, moved, nm);
}

/* a rectangle of the layout (x0, y0, x1, y1) as drawn: the row's place and size */
static void maker_rect(float x0, float y0, float x1, float y1, float out[4]) {
  out[0] = MAKER_A + MAKER_Z * x0, out[1] = MAKER_B + MAKER_Z * y0, out[2] = MAKER_A + MAKER_Z * x1, out[3] = MAKER_B + MAKER_Z * y1;
}

/* The maker select's Special Vehicles button, one step on from the row's last maker, as the arcade's own buttons: the white face
 * (the left panel's frame, shrunk) with a question mark for its logo, dim when not on, on it the red glow, the soft disc and the arrows
 * under it, and the left panel with the name plate. f is a frame of the maker layout (the last maker's idle one when on). */
static void draw_maker_hidden(const menu_state *m, int f, int lit) {
  int last = anm_sprite(T_MK_LAST), maru = anm_sprite(T_MK_MARU), waku = anm_sprite(T_MK_WAKU), light = anm_sprite(T_MK_LIGHT);
  int name = anm_sprite(T_MK_NAME), q = anm_sprite(T_QMARK);
  float r[4], la = 0.75f, d[4];
  if (last < 0 || waku < 0 || !anm_sprite_rect(ID.maker_layout, f, last, r)) {
    if (lit) draw_maker_row(f, NULL, 0);
    return;
  }
  float bx = r[0] + MAKER_DX, by = r[1], lw = r[2] - r[0], lh = r[3] - r[1]; /* (the button's sprite, in the layout's own units) */
  /* the glow's pulse on the last maker's button (the layout crossfades it from the one before) */
  float lr[4], lal = 0;
  for (int k = 0; k < 4; ++k)
    if (anm_sprite_rect_at(ID.maker_layout, f, light, k, lr, &lal) && fabsf(lr[0] - r[0]) < 1.0f) la = lal;
  static const float zero[4] = { 0, 0, 0, 0 };
  /* the layout's arrows (its two sprites, scaled and fading) go in a pass of their own, one step on: it blanks all the rest */
  int arrow[2] = { anm_sprite(T_MK_ARROW), anm_sprite(T_MK_ARROW2) };
  int all[256], nall = anm_sprites(ID.maker_layout, all, 256);
  anm_swap arrows[256];
  int narrow = 0;
  for (int i = 0; i < nall; ++i)
    if (all[i] != arrow[0] && all[i] != arrow[1]) arrows[narrow++] = (anm_swap){ all[i], -1 };
  if (lit) {
    anm_swap rest[5] = { { maru, -1 }, { light, -1 }, { last, -1 }, { arrow[0], -1 }, { arrow[1], -1 } };
    draw_maker_row(f, rest, 5);
    if (maru >= 0) { /* (the disc under the button on: the layout's own, moved) */
      maker_rect(bx - 64.0f, by - 64.0f, bx - 64.0f + anm_sprite_width(maru), by - 64.0f + anm_sprite_height(maru), d);
      anm_draw_part(maru, (float[4]){ 0, 0, anm_sprite_width(maru), anm_sprite_height(maru) }, d, 1);
    }
    float src[4] = { 0, 0, lw, lh };
    const float dim[4] = { 179.0f / 256.0f, 179.0f / 256.0f, 179.0f / 256.0f, 1 };
    maker_rect(r[0], r[1], r[2], r[3], d);
    anm_draw_part_col(last, src, d, dim, zero); /* (the last maker's button, as it is not on any longer) */
  }
  const float k = lit ? 1.0f : 179.0f / 256.0f;
  const float mul[4] = { k, k, k, 1 };
  const float face[4] = { 54, 63, 202, 189 }; /* (the frame's face in its 256 px sprite) */
  maker_rect(bx + 20.0f, by + 27.0f, bx + 108.0f, by + 102.0f, d); /* (a maker button's face in its 128 px sprite) */
  anm_draw_part_col(waku, face, d, mul, zero);
  if (q >= 0) {
    maker_rect(bx + 36.0f, by + 37.0f, bx + 92.0f, by + 93.0f, d);
    anm_draw_part_col(q, (float[4]){ 0, 0, 128, 128 }, d, mul, zero);
  }
  if (!lit) {
    /* the last maker on is no more the row's end: it has the arrows on its right too (the layout's are on its left alone), mirrored on its middle */
    if (m->sel == CARDB_MAKERS - 1 && m->from < 0)
      anm_draw_xy(ID.maker_layout, f, 1, MAKER_A + MAKER_Z * (2.0f * r[0] + 128.0f), MAKER_B, -MAKER_Z, MAKER_Z, arrows, narrow);
    return;
  }
  if (light >= 0) {
    maker_rect(bx, by, bx + anm_sprite_width(light), by + anm_sprite_height(light), d);
    anm_draw_part(light, (float[4]){ 0, 0, anm_sprite_width(light), anm_sprite_height(light) }, d, la);
  }
  anm_draw_xy(ID.maker_layout, f, 1, MAKER_A + MAKER_Z * MAKER_DX, MAKER_B, MAKER_Z, MAKER_Z, arrows, narrow); /* (the arrows beside the button, as the layout's, one step on) */
  /* the left panel: the frame, a question mark in it, the name on its plate (the plate's own grey, stretched, the key's caption on it) */
  anm_draw_sprite(waku, -27.0f, 60.0f, 1);
  if (q >= 0) anm_draw_part(q, (float[4]){ 0, 0, 128, 128 }, (float[4]){ 51, 136, 151, 236 }, 1);
  if (name >= 0) anm_draw_part(name, (float[4]){ 62, 20, 72, 44 }, (float[4]){ 30, 253, 172, 286 }, 1);
  int cap = anm_sprite(OPT_TEXT + O_HIDDEN);
  if (cap >= 0) anm_draw_sprite(cap, 105.3f - 0.5f * anm_sprite_width(cap), 269.5f - 0.5f * anm_sprite_height(cap), 1);
}

/* An iPad's RESOLUTION row (res_row; for the Retina iPads after the iPad 3): its
 * words in the HUD's own letters (no pre-rendered ones), at the 640x480 screen's x, y; its two
 * chips' left edges and widths */
static const char *const OPT_RES_PAD[2] = { "1024 X 768", "2048 X 1536" };
/* an iPhone's (res_phone): the 4's 480x320 or the Retina 960x640 (the 4S) */
static const char *const OPT_RES_PHONE[2] = { "480 X 320", "960 X 640" };
static const char *const *opt_res(const menu_state *m) { return m->res_phone ? OPT_RES_PHONE : OPT_RES_PAD; }

/* ---- the OPTION screen's KEY LAYOUT (MENU_LAYOUT, the touch keys' size and
 * place the player's own): the race's keys over the screen as they will be, a group dragged
 * by a finger moves (kept on the screen), the panel's - and + size the chosen group, RESET ALL
 * puts every group back, DONE (or the back key) back to OPTION. Its key on the OPTION screen
 * on the PEDALS row's right. Its words the HUD's (no pre-rendered ones), in the 1024 x 768
 * layout. ---- */
enum { LAY_SMALLER, LAY_BIGGER, LAY_RESET, LAY_DONE, LAY_BUTTONS };
static const float LAY_PANEL[4] = { 272, 228, 480, 270 };
static const float LAY_BUTTON[LAY_BUTTONS][4] = { { 296, 356, 110, 56 }, { 618, 356, 110, 56 }, { 296, 426, 204, 56 },
                                                  { 524, 426, 204, 56 } };
static const char *const LAY_GROUP[3] = { "TOP KEYS", "LEFT KEYS", "RIGHT KEYS" };
#define LAY_K_STEP 10

static int layout_button(float x, float y) {
  for (int b = 0; b < LAY_BUTTONS; ++b) {
    const float *r = LAY_BUTTON[b];
    if (x >= r[0] - 6 && x < r[0] + r[2] + 6 && y >= r[1] - 6 && y < r[1] + r[3] + 6) return b;
  }
  return -1;
}

static void layout_clamp(menu_state *m, int g) {
  float dx = (float)m->key_dx[g], dy = (float)m->key_dy[g];
  hud_layout_clamp(g, m->gyro == HUD_STEER_GYRO, (float)m->key_k[g] * 0.01f, &dx, &dy);
  m->key_dx[g] = (int)floorf(dx + 0.5f), m->key_dy[g] = (int)floorf(dy + 0.5f);
}

static void layout_apply(const menu_state *m) {
  for (int g = 0; g < HUD_KEY_GROUPS; ++g) hud_layout_set(g, (float)m->key_k[g] * 0.01f, (float)m->key_dx[g], (float)m->key_dy[g]);
}

static void layout_enter(menu_state *m) {
  go(m, MENU_LAYOUT);
  m->lay_drag = m->lay_tap = 0;
  m->lay_touch = 0;
  if (m->lay_sel < 0 || m->lay_sel >= HUD_KEY_GROUPS) m->lay_sel = HUD_KEYS_RIGHT;
  layout_apply(m);
}

/* its touches (menu_touch's, all phases): a panel button, or a group taken and dragged; 0 for
 * one not its own (the back key's) */
static int layout_touch(menu_state *m, void *id, int phase, float x, float y) {
  if (phase == TOUCH_BEGAN && !m->lay_drag) {
    int b = layout_button(x, y);
    if (b >= 0) {
      m->lay_tap = b + 1;
      return 1;
    }
    layout_apply(m);
    int g = hud_layout_hit(m->gyro == HUD_STEER_GYRO, x, y);
    if (g < 0) return 0;
    m->lay_sel = g;
    m->lay_drag = 1, m->lay_touch = id;
    m->lay_x = x - (float)m->key_dx[g], m->lay_y = y - (float)m->key_dy[g];
    return 1;
  }
  if (!m->lay_drag || id != m->lay_touch) return phase != TOUCH_BEGAN;
  int g = m->lay_sel;
  m->key_dx[g] = (int)floorf(x - m->lay_x + 0.5f), m->key_dy[g] = (int)floorf(y - m->lay_y + 0.5f);
  layout_clamp(m, g);
  layout_apply(m);
  if (phase == TOUCH_ENDED) {
    m->lay_drag = 0, m->lay_touch = 0;
    m->garage_changed = 1;
  }
  return 1;
}

static void bindings_enter(menu_state *m);
static void layout_step(menu_state *m, const menu_in *in) {
  int b = m->lay_tap - 1;
  m->lay_tap = 0;
  if (in->back || in->ok || b == LAY_DONE) {
    snd_menu(SND_CANCEL);
    m->lay_drag = 0, m->lay_touch = 0;
    bindings_enter(m);
    return;
  }
  int g = m->lay_sel;
  if (in->view) { /* the next group of keys */
    snd_menu(SND_CURSOR);
    m->lay_sel = (g + 1) % HUD_KEY_GROUPS;
    return;
  }
  if (in->dx || in->dy) { /* the group moves (kept on the screen) */
    m->key_dx[g] += 8 * in->dx, m->key_dy[g] += 8 * in->dy;
    layout_clamp(m, g);
    m->garage_changed = 1;
    layout_apply(m);
    return;
  }
  if (b == LAY_SMALLER || b == LAY_BIGGER) {
    int k = m->key_k[g] + (b == LAY_BIGGER ? LAY_K_STEP : -LAY_K_STEP);
    if (k < (int)(HUD_KEY_K_MIN * 100 + 0.5f) || k > (int)(HUD_KEY_K_MAX * 100 + 0.5f)) return;
    snd_menu(SND_CURSOR);
    m->key_k[g] = k;
    layout_clamp(m, g);
    m->garage_changed = 1;
  } else if (b == LAY_RESET) {
    snd_menu(SND_SELECT);
    for (int i = 0; i < HUD_KEY_GROUPS; ++i) m->key_k[i] = 100, m->key_dx[i] = m->key_dy[i] = 0;
    m->garage_changed = 1;
  }
  layout_apply(m);
}

static void layout_text_mid(const char *s, float y, float px, float r, float g, float b) {
  hud_ui_text(s, 512.0f - 0.5f * hud_ui_text_width(s, px), y, px, r, g, b, 1);
}

static void draw_layout(const menu_state *m, int clock, float screen_w, float screen_h) {
  draw_back(ID.bg, clock % 600, 1);
  anm_fill(0, 0, 0, 0.6f);
  hud_ui_begin(screen_w, screen_h);
  const float *p = LAY_PANEL;
  hud_ui_rect(p[0], p[1], p[2], p[3], 0.03f, 0.04f, 0.06f, 0.82f);
  hud_ui_rect(p[0], p[1], p[2], 3, 1.0f, 0.42f, 0.05f, 0.95f);
  layout_text_mid("KEY LAYOUT", p[1] + 16, 4, 1, 1, 1);
  layout_text_mid("DRAG A GROUP OF KEYS TO MOVE IT", p[1] + 62, 2, 0.8f, 0.8f, 0.8f);
  char line[48];
  int g = m->lay_sel >= 0 && m->lay_sel < 3 ? m->lay_sel : 0;
  layout_text_mid(LAY_GROUP[g], p[1] + 92, 2.5f, 1.0f, 0.55f, 0.15f);
  static const char *const LABEL[LAY_BUTTONS] = { "-", "+", "RESET ALL", "DONE" };
  for (int b = 0; b < LAY_BUTTONS; ++b) {
    const float *r = LAY_BUTTON[b];
    float px = b < 2 ? 5.0f : 3.0f;
    hud_ui_button(r[0], r[1], r[2], r[3], m->lay_tap == b + 1);
    hud_ui_text(LABEL[b], r[0] + 0.5f * (r[2] - hud_ui_text_width(LABEL[b], px)), r[1] + 0.5f * (r[3] - 7.0f * px), px, 1, 1,
                1, 1);
  }
  snprintf(line, sizeof line, "SIZE %d", m->key_k[g]);
  layout_text_mid(line, LAY_BUTTON[0][1] + 0.5f * (LAY_BUTTON[0][3] - 7.0f * 3), 3, 1, 1, 1);
  hud_ui_end();
  layout_apply(m);
  hud_draw_layout(screen_w, screen_h, m->gyro, g);
}

/* ---- the game options (MENU_OPTION, the settings selector's GAME OPTIONS): a list of rows, each a switch (ON / OFF) or a few chips, then the
 * button to the DEBUG OPTIONS (MENU_HUDOPT: the input readout, the fps counter, the physics panel). Up and down the rows, left and right the
 * chips (left ON), OK flips a switch or opens the sub-menu, the pointer chooses a row (select) and a chip on it (tab). The rows are built
 * each time (the graphics rows the build offers: Android's, an iPad's resolution). The touch controls are the CONTROLS screen's. ---- */
static int option_touch(void) { return hud_style() == HUD_STYLE_TOUCH; }

enum { OK_SWITCH, OK_MIP, OK_RES, OK_AA, OK_SUB, OK_INFO, OK_RUMBLE, OK_PCT, OK_WIN };
#define IS_RES(r) ((r)->kind == OK_RES || (r)->kind == OK_WIN) /* a choice of two, the value the chip (0 the left) */
#define IS_SUB(r) ((r)->kind == OK_SUB || (r)->kind == OK_INFO || (r)->kind == OK_RUMBLE) /* a row that is a button to another screen */
typedef struct { const char *name, *note; int kind; int *v; } orow;

static const char *const MIP_NAME[4] = { "SOFT", "NORMAL", "SHARP", "SHARPER" };
static const float MIP_BIAS[4] = { 0.0f, -1.0f, -2.0f, -3.0f };
float menu_mip_bias(const menu_state *m) { return MIP_BIAS[m->mip >= 0 && m->mip < 4 ? m->mip : 2]; }

#define OPTL_Y0 118.0f /* (640 x 480) the first row's middle */
#define OPTL_X_ON 330.0f
#define OPTL_GAP 10.0f
#define OPTL_DY 28.0f /* the options' rows (25 with more than 9 of them, so the last still clears the note); the debug options' are further apart */
static float optl_dy(int n) { return n > 9 ? 25.0f : OPTL_DY; }
#define OPTL_DY_DEBUG 36.0f

/* a row's chip c: its left edge and width */
static void optl_chip(const orow *r, int c, float *x, float *w) {
  float w0 = IS_RES(r) ? 112.0f : IS_SUB(r) ? 110.0f : 70.0f;
  *w = w0, *x = OPTL_X_ON + (w0 + OPTL_GAP) * (float)c;
}

static int option_rows(menu_state *m, orow r[12]) {
  int n = 0;
  r[n++] = (orow){ "BACK MIRROR", "THE MIRROR SHOWN IN BUMPER CAMERA", OK_SWITCH, &m->bm_on };
  r[n++] = (orow){ "TEXTURES", "HOW SHARP THE ROAD'S AND THE SCENERY'S TEXTURES STAY IN THE DISTANCE", OK_MIP, &m->mip };
  r[n++] = (orow){ "REFLECTION", "THE CARS' PAINT AND GLASS REFLECT THE SCREEN AND THE STREET LIGHTS, AS IN THE ARCADE", OK_SWITCH, &m->over_on };
  r[n++] = (orow){ "MILEAGE SCREEN", "THE MILEAGE TALLY AFTER A FINISHED SINGLE PLAYER GAME (OFF SKIPS IT)", OK_SWITCH, &m->milage_on };
  if (m->hq_rows) {
    r[n++] = (orow){ "HD TEXTURES", "USE THE OPTIONAL UPSCALED TEXTURES", OK_SWITCH, &m->hd_tex };
    r[n++] = (orow){ "WET ROAD", "ADDS SCREENSPACE REFLECTIONS TO THE ROAD SURFACE", OK_SWITCH, &m->wet_on };
    r[n++] = (orow){ "ANTI-ALIASING", "MSAA: SMOOTHS HARD EDGES", OK_AA, &m->aa4 };
  }
  if (m->res_row) r[n++] = (orow){ "RESOLUTION", "TAKES EFFECT AT THE NEXT START", OK_RES, &m->hires };
  if (m->win_row) r[n++] = (orow){ "WINDOW MODE", "WINDOWED OR FULLSCREEN (F11 OR ALT+ENTER ANYTIME)", OK_WIN, &m->fullscreen };
  r[n++] = (orow){ "VIBRATION", "CONTROLLER RUMBLE: WALL HITS, EXPANSION JOINTS AND TIRE SLIP", OK_RUMBLE, NULL };
  r[n++] = (orow){ "DEBUG OPTIONS", "ADDITIONAL INFORMATION READOUTS", OK_SUB, NULL };
  r[n++] = (orow){ "INFORMATION", "ABOUT THIS PORT", OK_INFO, NULL };
  return n;
}

static int debug_rows(menu_state *m, orow r[4]) {
  r[0] = (orow){ "INPUT READOUT", "THE STEERING, GAS AND BRAKE BARS AND THE RAW AXES WHILE DRIVING", OK_SWITCH, &m->readout_on };
  r[1] = (orow){ "FPS COUNTER", "THE FRAME RATE AT THE TOP OF THE RACE (THE TOUCH HUD ALWAYS HAS ITS OWN)", OK_SWITCH, &m->fps_on };
  r[2] = (orow){ "PHYSICS PANEL", "THE CAR'S SLIP, GRIP IN USE AND G FORCES OVER THE RACE (F4 ON A KEYBOARD)", OK_SWITCH, &m->phys_on };
  return 3;
}

/* chip c of a row chosen (not for the sub-menu's button) */
static void optl_chip_set(menu_state *m, const orow *r, int c) {
  if (IS_SUB(r) || !r->v) return;
  if (r->kind == OK_PCT) { /* (the vibration's tenths: the left chip one less, the right one more) */
    int to = *r->v + (c == 0 ? -1 : 1);
    if (to < 0 || to > 10) {
      snd_menu(SND_CANCEL);
      return;
    }
    *r->v = to, m->garage_changed = 1;
    snd_menu(SND_CURSOR);
    return;
  }
  if (r->kind == OK_MIP) { /* (a four-step setting: the left chip one softer, the right one sharper) */
    int to = *r->v + (c == 0 ? -1 : 1);
    if (to < 0 || to > 3) {
      snd_menu(SND_CANCEL);
      return;
    }
    *r->v = to, m->garage_changed = 1;
    snd_menu(SND_CURSOR);
    return;
  }
  int want = IS_RES(r) ? c : c == 0; /* (RESOLUTION: the first or the second; the others: ON / 4X is the left) */
  if (*r->v == want) return;
  *r->v = want, m->garage_changed = 1;
  snd_menu(SND_CURSOR);
}

/* one step of a list: 1 if the sub-menu's button was taken */
static int optl_step(menu_state *m, const orow *rows, int n, int *sel, const menu_in *in) {
  if (*sel < 0 || *sel >= n) *sel = 0;
  if (in->select >= 0 && in->select < n) *sel = in->select;
  const orow *r = &rows[*sel];
  if (in->tab >= 0) return IS_SUB(r) ? 1 : (optl_chip_set(m, r, in->tab), 0);
  if (in->dy) {
    int to = *sel + in->dy;
    if (to >= 0 && to < n) snd_menu(SND_CURSOR), *sel = to;
    return 0;
  }
  if (in->dx) {
    if (!IS_SUB(r)) optl_chip_set(m, r, in->dx > 0);
    return 0;
  }
  if (in->ok) {
    if (IS_SUB(r)) return 1;
    if (r->kind == OK_MIP) m->mip = (m->mip + 1) % 4, m->garage_changed = 1, snd_menu(SND_CURSOR);
    else if (r->kind == OK_PCT) *r->v = (*r->v + 1) % 11, m->garage_changed = 1, snd_menu(SND_CURSOR);    else optl_chip_set(m, r, IS_RES(r) ? !*r->v : (*r->v ? 1 : 0));
  }
  return 0;
}

/* a tap: the row (select) and the chip on it (tab) */
static void optl_pointer(const menu_state *m, menu_in *in, const orow *rows, int n, float dy) {
  float x = m->tap_x, y = m->tap_y;
  int row = (int)floorf((y - (OPTL_Y0 - 0.5f * dy)) / dy);
  if (row < 0 || row >= n) return;
  in->select = row;
  if (IS_SUB(&rows[row])) {
    in->tab = 0;
    return;
  }
  for (int c = 0; c < 2; ++c) {
    float cx, cw;
    optl_chip(&rows[row], c, &cx, &cw);
    if (x >= cx - 0.5f * OPTL_GAP && x < cx + cw + 0.5f * OPTL_GAP && y >= OPTL_Y0 + dy * (float)row - 14.0f && y < OPTL_Y0 + dy * (float)row + 14.0f) in->tab = c;
  }
}

static void draw_optlist(const menu_state *m, const orow *rows, int n, int sel, float dy, float screen_w, float screen_h) {
  const float k = 1.6f, half = 0.5f * dy - 2.0f, name_px = dy > 30.0f ? 3.0f : 2.4f, chip_px = dy > 30.0f ? 2.6f : 2.0f, chip_h = dy > 30.0f ? 24.0f : 20.0f;
  if (sel < 0 || sel >= n) sel = 0;
  hud_ui_begin(screen_w, screen_h);
  for (int r = 0; r < n; ++r) {
    const orow *o = &rows[r];
    float y = OPTL_Y0 + dy * (float)r;
    int on = o->v ? *o->v : 0, row_on = r == sel;
    hud_ui_rect(0, (y - half) * k, 640.0f * k, 2.0f * half * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
    if (row_on) {
      hud_ui_rect(2.0f * k, (y - half) * k, 4.0f * k, 2.0f * half * k, 1.0f, 0.42f, 0.05f, 1.0f);
      hud_ui_rect(2.0f * k, (y + half) * k, 636.0f * k, 2.0f, 1.0f, 0.42f, 0.05f, 0.85f);
    }
    hud_ui_text(o->name, 20.0f * k, y * k - 3.5f * name_px, name_px, 1, 1, 1, 1);
    for (int c = 0; c < (IS_SUB(o) ? 1 : 2); ++c) {
      float x, w;
      optl_chip(o, c, &x, &w);
      int lit = o->kind == OK_MIP || o->kind == OK_PCT || IS_SUB(o) ? 1 : IS_RES(o) ? on == c : on == !c;
      hud_ui_rect(x * k, (y - 0.5f * chip_h) * k, w * k, chip_h * k, lit ? 0.35f : 0.0f, lit ? 0.15f : 0.0f, 0.0f, lit ? 0.9f : 0.4f);
      if (lit) hud_ui_rect(x * k, (y + 0.5f * chip_h - 2.0f) * k, w * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
      const char *s = IS_SUB(o) ? "OPEN >" : o->kind == OK_MIP ? (c ? "SHARPER" : "SOFTER") : o->kind == OK_PCT ? (c ? "MORE" : "LESS") : o->kind == OK_WIN ? (c ? "FULLSCREEN" : "WINDOWED") : o->kind == OK_RES ? (m->res_phone ? (c ? "960 X 640" : "480 X 320") : (c ? "2048 X 1536" : "1024 X 768"))
                    : o->kind == OK_AA ? (c ? "2X" : "4X") : c ? "OFF" : "ON";
      float px = o->kind == OK_MIP || IS_RES(o) || o->kind == OK_PCT ? 1.8f : chip_px;
      hud_ui_text(s, (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(s, px), y * k - 3.5f * px, px, 1, 1, 1, lit ? 1.0f : 0.45f);
    }
    if (o->kind == OK_MIP || o->kind == OK_PCT) {
      float x, w;
      optl_chip(o, 1, &x, &w);
      char pct[8];
      snprintf(pct, sizeof pct, "%d%%", on * 10);
      hud_ui_text(o->kind == OK_MIP ? MIP_NAME[on >= 0 && on < 4 ? on : 2] : pct, (x + w + 14.0f) * k, y * k - 3.5f * chip_px, chip_px, 1, 0.55f, 0.15f, 1);
    }
  }
  hud_ui_text(rows[sel].note, 20.0f * k, 380.0f * k, 1.7f, 0.85f, 0.85f, 0.85f, 1);
  hud_ui_end();
}

static void option_enter(menu_state *m) {
  go(m, MENU_OPTION);
  m->settings_direct = 0;
  m->focus = 0;
}

static void hudopt_enter(menu_state *m) {
  go(m, MENU_HUDOPT);
  m->hud_row = 0;
}

static void portinfo_enter(menu_state *m) {
  go(m, MENU_PORTINFO);
}

/* the vibration screen (MENU_RUMBLE): each effect's strength in tenths; the controller's rumble is the sum of what they make */
static int rumble_rows(menu_state *m, orow r[4]) {
  r[0] = (orow){ "OVERALL", "EVERYTHING AT ONCE", OK_PCT, &m->rum[0] };
  r[1] = (orow){ "WALL HITS", "THE JOLT OF A HIT AND THE BUZZ OF A SCRAPE", OK_PCT, &m->rum[1] };
  r[2] = (orow){ "EXPANSION JOINTS", "THE TAPS OF THE ROAD'S JOINTS, ZEBRA AND GUARD RAIL STRIPS", OK_PCT, &m->rum[2] };
  r[3] = (orow){ "TIRE SLIP", "THE RUMBLE THAT GROWS AS THE CAR SLIDES SIDEWAYS", OK_PCT, &m->rum[3] };
  return 4;
}

static void rumble_enter(menu_state *m) {
  go(m, MENU_RUMBLE);
  m->rum_row = 0;
}

/* back to the game options, with the cursor on the row of the screen just left (a button: OK_SUB, OK_INFO or OK_RUMBLE) */
static void option_back_to(menu_state *m, int kind) {
  orow o[12];
  int n = option_rows(m, o);
  option_enter(m);
  for (int i = 0; i < n; ++i)
    if (o[i].kind == kind) m->focus = i;
}

static void rumble_step(menu_state *m, const menu_in *in) {
  if (in->back) {
    snd_menu(SND_CANCEL);
    option_back_to(m, OK_RUMBLE);
    return;
  }
  orow r[4];
  int n = rumble_rows(m, r);
  optl_step(m, r, n, &m->rum_row, in);
}

static void draw_rumble(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  const float k = 1.6f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  draw_bottom(bars, 1);
  int tb = draw_title_bar(bars, 6);
  hud_ui_begin(screen_w, screen_h);
  if (!tb) hud_ui_text("VIBRATION", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
  hud_ui_end();
  orow r[4];
  int n = rumble_rows((menu_state *)m, r);
  draw_optlist(m, r, n, m->rum_row, OPTL_DY_DEBUG, screen_w, screen_h);
}

static void option_step(menu_state *m, const menu_in *in) {
  orow r[12];
  int n = option_rows(m, r);
  if (optl_step(m, r, n, &m->focus, in)) {
    snd_menu(SND_SELECT);
    int kind = m->focus >= 0 && m->focus < n ? r[m->focus].kind : OK_SUB;
    if (kind == OK_INFO) portinfo_enter(m);
    else if (kind == OK_RUMBLE) rumble_enter(m);
    else hudopt_enter(m);
  }
}

/* ---- the port's information screen (MENU_PORTINFO, the game options' INFORMATION): the credits and notice ---- */
static void portinfo_step(menu_state *m, const menu_in *in) {
  if (!in->back && !in->ok) return;
  snd_menu(SND_CANCEL);
  option_back_to(m, OK_INFO);
}

static void draw_portinfo(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  const float k = 1.6f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  draw_bottom(bars, 1);
  int tb = draw_title_bar(bars, 4);
  hud_ui_begin(screen_w, screen_h);
  if (!tb) hud_ui_text("INFORMATION", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
  /* (a line starting with # is a heading) */
  static const char *const lines[] = {
    "#SUPER 3DX+",
    "A NATIVE, CONSOLE-STYLE PORT OF WANGAN MIDNIGHT MAXIMUM TUNE",
    "3DX+ WITH CONTROLLER SUPPORT, OVERHAULED MENUS AND GARAGE,",
    "A MOUNTAIN OF BUGFIXES, RESTORATIONS, AND MORE.",
    "",
    "#CREDITS",
    "DEVELOPED BY SMOLDERYGUEST.",
    "GITHUB.COM/SMOLDERYGUEST/SUPER3DXPLUS",
    "X.COM/SMOLDERYGUEST",
    "BASED ON RE3DX+ BY ALLANNN-SUDO (MIT LICENCE).",
    "",
    "#NOTICE",
    "WANGAN MIDNIGHT MAXIMUM TUNE IS A TRADEMARK OF BANDAI",
    "NAMCO ENTERTAINMENT, INC. THIS IS AN UNOFFICIAL FAN",
    "PROJECT AND IS NOT AFFILIATED IN ANY WAY, SHAPE OR FORM,",
    "WRITTEN OR IMPLIED.",
  };
  float y = 92.0f;
  for (int i = 0; i < (int)(sizeof lines / sizeof *lines); i++, y += 20.0f) {
    const char *s = lines[i];
    if (*s == '#') hud_ui_text(s + 1, 24.0f * k, y * k, 2.9f, 1.0f, 0.55f, 0.15f, 1);
    else hud_ui_text(s, 24.0f * k, y * k, 2.2f, 0.9f, 0.9f, 0.9f, 1);
  }
  hud_ui_end();
}

static void hudopt_step(menu_state *m, const menu_in *in) {
  if (in->back) {
    snd_menu(SND_CANCEL);
    option_back_to(m, OK_SUB);
    return;
  }
  orow r[4];
  int n = debug_rows(m, r);
  optl_step(m, r, n, &m->hud_row, in);
}

static void draw_option(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(OPT_TEXT + O_TITLE) };
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
  orow r[12];
  int n = option_rows((menu_state *)m, r);
  draw_optlist(m, r, n, m->focus, optl_dy(n), screen_w, screen_h);
}

static void draw_hudopt(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  const float k = 1.6f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  draw_bottom(bars, 1);
  int tb = draw_title_bar(bars, 5);
  hud_ui_begin(screen_w, screen_h);
  if (!tb) hud_ui_text("DEBUG OPTIONS", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
  hud_ui_end();
  orow r[4];
  int n = debug_rows((menu_state *)m, r);
  draw_optlist(m, r, n, m->hud_row, OPTL_DY_DEBUG, screen_w, screen_h);
}


/* ---- the volume mixer (MENU_VOLUME, from the OPTION screen's VOLUME key): overall, the car's sounds (engine, turbo, tyres, wall, crashes),
 * the music and the effects (the menus'); each in tenths. Up and down the rows, left and right the level (a tap on the bar sets it), back
 * leaves. ---- */
#define VOL_ROWS 6
static const char *const VOL_NAME[VOL_ROWS] = { "OVERALL", "CAR SOUNDS", "MUSIC", "SOUND EFFECTS", "ENGINE", "TURBO" };
static const char *const VOL_NOTE[VOL_ROWS] = { "MASTER AUDIO LEVEL", "ENGINE, TURBO, TIRES, CRASHES, ETC",
                                                "MUSIC PLAYED IN BOTH THE MENUS AND RACES", "INTERACTIVE SOUNDS PLAYED IN MENUS",
                                                "ENGINE AND EXHAUST LEVEL (100% = ARCADE)",
                                                "TURBO WHINE LEVEL (100% = ARCADE)" };
#define VOL_Y0 118.0f /* (640 x 480) */
#define VOL_DY 44.0f
#define VOL_BAR_X 250.0f
#define VOL_CELL 32.0f

static void volume_enter(menu_state *m) {
  go(m, MENU_VOLUME);
  m->vol_row = 0;
}

static void volume_set(menu_state *m, int row, int v) {
  v = v < 0 ? 0 : v > 10 ? 10 : v;
  if (m->vol[row] == v) return;
  m->vol[row] = v;
  m->garage_changed = 1;
  snd_menu(SND_CURSOR); /* (the effects row is heard at the level just set) */
}

static void selmenu_enter(menu_state *m, int kind, int sel);
static void volume_step(menu_state *m, const menu_in *in) {
  if (in->back && m->settings_direct) { /* (opened from the settings selector: back to it) */
    snd_menu(SND_CANCEL);
    m->settings_direct = 0;
    selmenu_enter(m, 2, 2);
    return;
  }
  if (in->back) {
    snd_menu(SND_CANCEL);
    option_enter(m);
    return;
  }
  if (in->select >= 0 && in->select < VOL_ROWS) m->vol_row = in->select;
  if (in->tab >= 0) { /* a level chosen on the bar (0..10) */
    volume_set(m, m->vol_row, in->tab);
    return;
  }
  if (in->dy) {
    int r = m->vol_row + in->dy;
    if (r >= 0 && r < VOL_ROWS) snd_menu(SND_CURSOR), m->vol_row = r;
    return;
  }
  if (in->dx) volume_set(m, m->vol_row, m->vol[m->vol_row] + in->dx);
}

static void draw_volume(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  const float k = 1.6f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  draw_bottom(bars, 1);
  int tb = draw_title_bar(bars, 1);
  hud_ui_begin(screen_w, screen_h);
  if (!tb) hud_ui_text("VOLUME", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
  for (int r = 0; r < VOL_ROWS; ++r) {
    float y = VOL_Y0 + VOL_DY * (float)r;
    int row_on = r == m->vol_row;
    hud_ui_rect(0, (y - 20.0f) * k, 640.0f * k, 40.0f * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
    if (row_on) {
      hud_ui_rect(2.0f * k, (y - 20.0f) * k, 4.0f * k, 40.0f * k, 1.0f, 0.42f, 0.05f, 1.0f);
      hud_ui_rect(2.0f * k, (y + 20.0f) * k, 636.0f * k, 2.0f, 1.0f, 0.42f, 0.05f, 0.85f);
    }
    hud_ui_text(VOL_NAME[r], 20.0f * k, (y - 8.0f) * k, 3.0f, 1, 1, 1, 1);
    for (int c = 0; c < 10; ++c) {
      int lit = c < m->vol[r];
      float x = VOL_BAR_X + VOL_CELL * (float)c;
      hud_ui_rect(x * k, (y - 12.0f) * k, (VOL_CELL - 4.0f) * k, 24.0f * k, lit ? 1.0f : 0.0f, lit ? 0.42f : 0.0f, lit ? 0.05f : 0.0f, lit ? 0.95f : 0.4f);
    }
    char s[8];
    snprintf(s, sizeof s, "%d%%", m->vol[r] * (r >= 4 ? 20 : 10));
    hud_ui_text(s, (VOL_BAR_X + VOL_CELL * 10.0f + 12.0f) * k, (y - 7.0f) * k, 2.6f, 1, 1, 1, 1);
  }
  hud_ui_text(VOL_NOTE[m->vol_row], 20.0f * k, 380.0f * k, 1.7f, 0.85f, 0.85f, 0.85f, 1);
  hud_ui_end();
}

/* ---- the key bindings (MENU_BINDINGS, from the OPTION screen's KEY BINDINGS key): the menus' actions and the race's (bind.h), each with
 * two keyboard keys and two pad buttons. Up and down the rows, left and right the cells, OK waits for an input to put in the cell
 * (a key for a key cell, a pad button, the stick's direction or a trigger for a pad cell; Escape cancels, Delete empties it), the
 * view button the other list, back leaves; the last row of a list puts its defaults back. A change is saved with the options. ---- */
#define BIND_X_NAME 16.0f /* (640 x 480) */
#define BIND_X_COL 188.0f
#define BIND_COL_W 110.0f
#define BIND_Y_ROW 128.0f
#define BIND_ROW_DY 23.0f
#define FR_Y0 124.0f /* (the wheel list has more rows: a tighter step) */
#define FR_DY 20.0f
#define BIND_Y_TAB 90.0f
/* the lists: the menus' and the race's keys, the analog axes (BA_*), and a steering wheel's force feedback (ffb.h; no bindings, its own rows) */
#define BIND_FFB (BIND_CONTEXTS + 1)
#define BIND_TABS (BIND_CONTEXTS + 2)
static const char *const BIND_TAB[BIND_TABS] = { "MENUS", "RACE", "AXES", "WHEEL" };
/* the wheel list's rows: the device, the two switches, the four strengths (tenths), the test; the settings' ids, and the names */
enum { FR_DEVICE, FR_ON, FR_INVERT, FR_OVERALL, FR_STEER, FR_CENTER, FR_DAMP, FR_ROAD, FR_CURVE, FR_MIN, FR_TEST, FR_COUNT };
static const signed char FR_ID[FR_COUNT] = { FFB_DEVICE, FFB_ON, FFB_INVERT, FFB_OVERALL, FFB_STEER, FFB_CENTER, FFB_DAMP, FFB_ROAD, FFB_CURVE, FFB_MIN, -1 };
static const char *const FR_NAME[FR_COUNT] = { "DEVICE", "FORCE FEEDBACK", "INVERT FORCE", "OVERALL", "STEERING FORCE", "CENTERING", "DAMPING", "ROAD EFFECTS", "STEERING RESPONSE", "MINIMUM FORCE", "TEST" };
static const char *const FR_NOTE[FR_COUNT] = {
  "THE STEERING WHEEL: LEFT AND RIGHT CHOOSE. STEERING AND PEDALS ARE SET ON THE AXES LIST",
  "THE WHEEL'S MOTOR ON OR OFF",
  "SWITCH IT IF THE WHEEL PULLS AWAY FROM STRAIGHT AHEAD INSTEAD OF TOWARDS IT",
  "THE DEVICE'S GAIN: ALL THE FORCES AT ONCE",
  "THE PULL OF THE FRONT TYRES; IT GOES LIGHT AT THE LIMIT AND FOLLOWS A SLIDE",
  "A SPRING TO STRAIGHT AHEAD THAT GROWS WITH SPEED",
  "RESISTANCE TO FAST TURNS OF THE WHEEL, FOR STEADINESS",
  "THE SHAKE OF EXPANSION JOINTS, WALL HITS AND SLIDING TYRES",
  "MAKES THE SMALL PULLS OF GENTLE CURVES STRONGER (0 % AS BEFORE); FOR A WHEEL THAT FEELS NOTHING ON THE ROAD",
  "THE FAINTEST FORCE GETS AT LEAST THIS MUCH OF THE MOTOR; FOR A DRIVER THAT IGNORES WEAK FORCES (T300)",
  "A SWEEP FROM SIDE TO SIDE, THEN A SHAKE: HOLD THE WHEEL LOOSELY"
};
#define FR_CELL_W 330.0f /* the value cell's width (it starts at the axes list's first cell) */
/* the RACE list has one row that is no action, under STEER LEFT / RIGHT: BUTTON STEERING SPEED (RS_ROW); the rows past it are the actions one on */
#define RS_ROW 2
static int bind_act(int ctx, int row) { return ctx == BIND_RACE && row > RS_ROW ? row - 1 : row; }
static int bind_is_speed(int ctx, int row) { return ctx == BIND_RACE && row == RS_ROW; }
static int bind_rows(int ctx) { return ctx == BIND_FFB ? FR_COUNT : ctx == BIND_CONTEXTS ? BA_COUNT + 3 : ctx == BIND_RACE ? bind_actions(ctx) + 1 : bind_actions(ctx); } /* (the AXES list: the axes, then the steering filter's switch and its two numbers) */
/* the filter's number of a row of the AXES list past the switch (half steps, 0..20) */
static int *filter_value(menu_state *m, int row) {
  return row == BA_COUNT + 1 ? &m->filter_smooth : row == BA_COUNT + 2 ? &m->filter_counter : &m->btn_steer; /* (+3: the buttons' steering speed, 1..10, whole steps) */
}
static void filter_step(menu_state *m, int row, int by, int wrap) {
  int *v = filter_value(m, row), to = *v + by, hi = row == BA_COUNT + 3 ? 10 : 20, lo = row == BA_COUNT + 3 ? 1 : 0;
  if (to > hi) to = wrap ? lo : hi;
  if (to < lo) to = lo;
  if (to != *v) *v = to, m->garage_changed = 1, snd_menu(SND_CURSOR);
}
static const int BIND_DEAD[] = { 0, 2, 5, 8, 10, 15, 20, 25, 30, 40 };
static const int BIND_SENS[] = { 50, 60, 70, 80, 90, 100, 115, 130, 150, 175, 200 };
/* the next step of a list past the value (the first again after the last) */
static int bind_next(const int *list, int n, int v) {
  for (int i = 0; i < n; ++i)
    if (list[i] > v) return list[i];
  return list[0];
}
/* the AXES list's cells (640 x 480): the axis, invert, the dead zone, the sensitivity, and the live value */
static const float AX_X[5] = { 188, 322, 388, 454, 520 }, AX_W[5] = { 130, 62, 62, 62, 112 };
static const char *const BIND_HEAD[BIND_SLOTS] = { "KEY 1", "KEY 2", "PAD 1", "PAD 2" };

static void bindings_enter(menu_state *m) {
  go(m, MENU_BINDINGS);
  m->bind_ctx = BIND_MENU, m->bind_row = 0, m->bind_col = 0, m->bind_wait = 0;
  m->bind_note[0] = 0;
}

static void bindings_note(menu_state *m, const char *s) { snprintf(m->bind_note, sizeof m->bind_note, "%s", s); }
/* the wait for an input ends by itself after this long (a pad has no Escape: it must never be left waiting) */
#define BIND_WAIT_FRAMES (8 * 60)
static void bind_wait_on(menu_state *m) {
  m->bind_wait = 1, m->bind_wait_t = 0;
  memcpy(m->axis_base, m->axis_raw, sizeof m->axis_base);
}
static void bind_wait_off(menu_state *m, const char *note) {
  m->bind_wait = 0;
  bindings_note(m, note);
  snd_menu(SND_CANCEL);
}

int menu_bind_capturing(const menu_state *m) { return m->screen == MENU_BINDINGS && m->bind_wait; }

int menu_bind_wants(const menu_state *m, int kind, int code) {
  return menu_bind_capturing(m) || (m->screen == MENU_BINDINGS && kind == BIND_KEY && code == BK_DELETE);
}

/* an input for the cell: the capture, or Delete on a cell */
void menu_bind_capture(menu_state *m, int kind, int code) {
  if (m->screen != MENU_BINDINGS) return;
  int ctx = m->bind_ctx, row = m->bind_row, slot = m->bind_col;
  if (ctx == BIND_FFB) return; /* (the wheel list binds nothing) */
  if (ctx == BIND_CONTEXTS) { /* the axes' list: Esc cancels, Delete takes the axis away, a key is no axis */
    if (kind == BIND_KEY && code == BK_ESCAPE && m->bind_wait) {
      m->bind_wait = 0;
      bindings_note(m, "");
      snd_menu(SND_CANCEL);
    } else if (kind == BIND_KEY && code == BK_DELETE && row < BA_COUNT && slot == 0) {
      const bind_axis *a = bind_axis_get(row);
      bind_axis_set(row, AXIS_NONE, a->invert, a->dead);
      m->bind_wait = 0;
      m->garage_changed = 1;
      snd_menu(SND_CANCEL);
    } else if (m->bind_wait && !(kind == BIND_PAD && (code == BP_RT || code == BP_LT || (code >= BP_STICK_LEFT && code < BP_W0)))) {
      bind_wait_off(m, "THAT CELL TAKES A STICK OR A TRIGGER"); /* (a button or a key: the wait ends; a trigger or the stick come as the axis) */
    }
    return;
  }
  if (bind_is_speed(ctx, row)) return;
  row = bind_act(ctx, row);
  if (row >= bind_actions(ctx)) return;
  if (kind == BIND_KEY && code == BK_ESCAPE && m->bind_wait) {
    m->bind_wait = 0;
    bindings_note(m, "");
    snd_menu(SND_CANCEL);
    return;
  }
  if (kind == BIND_KEY && code == BK_DELETE) {
    m->bind_wait = 0;
    if (bind_clear(ctx, row, slot)) {
      bindings_note(m, "");
      m->garage_changed = 1;
      snd_menu(SND_CANCEL);
    } else {
      bindings_note(m, "A MENU ACTION KEEPS ONE KEY AND ONE PAD BUTTON");
      snd_menu(SND_CANCEL);
    }
    return;
  }
  if (!m->bind_wait) return;
  if (kind != (slot < BIND_KEYS ? BIND_KEY : BIND_PAD)) { /* (the other kind of input: the wait ends, the cell stays) */
    bind_wait_off(m, slot < BIND_KEYS ? "THAT CELL TAKES A KEYBOARD KEY" : "THAT CELL TAKES A PAD BUTTON");
    return;
  }
  if (bind_set(ctx, row, slot, kind, code)) {
    m->garage_changed = 1;
    bindings_note(m, "");
    snd_menu(SND_SELECT);
  } else {
    bindings_note(m, "A MENU ACTION KEEPS ONE KEY AND ONE PAD BUTTON");
    snd_menu(SND_CANCEL);
  }
  m->bind_wait = 0;
}

/* an axis moved: the wait for an axis takes one pushed past 60 % */
int menu_axis_capture(menu_state *m, int axis, float v) {
  if (m->screen != MENU_BINDINGS || !m->bind_wait || m->bind_ctx != BIND_CONTEXTS || m->bind_col != 0 || m->bind_row >= BA_COUNT) return 0;
  if (axis >= AXIS_W0) { /* a wheel's axis: moved far from where it rested (a pedal rests at an end), not far from the middle */
    if (fabsf(v - m->axis_base[axis]) < 0.5f) return 0;
  } else if (v < 0.6f && v > -0.6f) return 0;
  const bind_axis *a = bind_axis_get(m->bind_row);
  bind_axis_set(m->bind_row, axis, a->invert, a->dead);
  m->bind_wait = 0;
  m->garage_changed = 1;
  bindings_note(m, "");
  snd_menu(SND_SELECT);
  return 1;
}

/* ---- the touch controls (the CONTROLS screen's other face, bind_touch: shown instead of the key lists while the last input was a touch, on a
 * touch HUD): the steering (BUTTONS, GYRO, WHEEL, STICK), the gyro's sensitivity, the slider pedals and the key to KEY LAYOUT. A pad or key
 * press brings the key lists back. Rows in bind_row; a tap sends the row (select) and the value (tab). ---- */
static int bind_touch(const menu_state *m) { return option_touch() && m->ctl_touch; }
static const char *const TC_STEER[HUD_STEER_MODES] = { "BUTTONS", "GYRO", "WHEEL", "STICK" };
enum { TC_STEERING, TC_SENS, TC_PEDALS, TC_LAYOUT, TC_ROWS };
static const char *const TC_NAME[TC_ROWS] = { "STEERING", "SENSITIVITY", "SLIDER PEDALS", "KEY LAYOUT" };
#define TC_Y0 118.0f
#define TC_DY 36.0f
#define TC_X 250.0f
#define TC_CELL 24.0f
#define TC_PX 2.0f

static void tc_steer_chip(int c, float *x, float *w) {
  float at = TC_X;
  for (int i = 0; i <= c; ++i) {
    *w = hud_ui_text_width(TC_STEER[i], TC_PX) / 1.6f + 16.0f, *x = at;
    at += *w + 8.0f;
  }
}

static void tc_set(menu_state *m, int *v, int want) {
  if (*v == want) return;
  *v = want, m->garage_changed = 1;
  snd_menu(SND_CURSOR);
}

static void touchctl_step(menu_state *m, const menu_in *in) {
  if (in->select >= 0 && in->select < TC_ROWS) m->bind_row = in->select;
  if (m->bind_row < 0 || m->bind_row >= TC_ROWS) m->bind_row = 0;
  int row = m->bind_row;
  if (in->tab >= 0) { /* a value outright */
    if (row == TC_STEERING && in->tab < HUD_STEER_MODES) tc_set(m, &m->gyro, in->tab);
    else if (row == TC_SENS && m->gyro == HUD_STEER_GYRO && in->tab >= 1 && in->tab <= 10) tc_set(m, &m->gyro_sens, in->tab);
    else if (row == TC_PEDALS) tc_set(m, &m->pedal_slide, in->tab != 0);
    return;
  }
  if (in->dy) {
    int r = row + in->dy;
    if (r >= 0 && r < TC_ROWS) snd_menu(SND_CURSOR), m->bind_row = r;
    return;
  }
  if (in->dx) {
    if (row == TC_STEERING) {
      int s = m->gyro + in->dx;
      if (s >= 0 && s < HUD_STEER_MODES) tc_set(m, &m->gyro, s);
    } else if (row == TC_SENS) {
      int s = m->gyro_sens + in->dx;
      if (m->gyro == HUD_STEER_GYRO && s >= 1 && s <= 10) tc_set(m, &m->gyro_sens, s);
    } else if (row == TC_PEDALS) {
      tc_set(m, &m->pedal_slide, in->dx < 0); /* (left is ON) */
    }
    return;
  }
  if (in->ok) {
    if (row == TC_LAYOUT) {
      snd_menu(SND_SELECT);
      layout_enter(m);
    } else if (row == TC_PEDALS) {
      tc_set(m, &m->pedal_slide, !m->pedal_slide);
    } else if (row == TC_STEERING) {
      tc_set(m, &m->gyro, (m->gyro + 1) % HUD_STEER_MODES);
    }
  }
}

/* a tap */
static void touchctl_pointer(const menu_state *m, menu_in *in) {
  float x = m->tap_x, y = m->tap_y;
  int row = (int)floorf((y - (TC_Y0 - 0.5f * TC_DY)) / TC_DY);
  if (row < 0 || row >= TC_ROWS) return;
  in->select = row;
  if (row == TC_STEERING) {
    for (int c = 0; c < HUD_STEER_MODES; ++c) {
      float cx, cw;
      tc_steer_chip(c, &cx, &cw);
      if (x >= cx - 4.0f && x < cx + cw + 4.0f) in->tab = c;
    }
  } else if (row == TC_SENS) {
    int i = (int)floorf((x - TC_X) / TC_CELL);
    if (i >= 0 && i < 10) in->tab = i + 1;
  } else if (row == TC_PEDALS) {
    if (x >= TC_X && x < TC_X + 70.0f) in->tab = 1;
    else if (x >= TC_X + 80.0f && x < TC_X + 150.0f) in->tab = 0;
  } else if (row == TC_LAYOUT) {
    in->ok = 1;
  }
}

static void draw_touchctl(const menu_state *m, float screen_w, float screen_h) {
  const float k = 1.6f, half = 0.5f * TC_DY - 2.0f;
  (void)screen_w, (void)screen_h;
  for (int r = 0; r < TC_ROWS; ++r) {
    float y = TC_Y0 + TC_DY * (float)r;
    int row_on = r == m->bind_row, dim = r == TC_SENS && m->gyro != HUD_STEER_GYRO;
    hud_ui_rect(0, (y - half) * k, 640.0f * k, 2.0f * half * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
    if (row_on) {
      hud_ui_rect(2.0f * k, (y - half) * k, 4.0f * k, 2.0f * half * k, 1.0f, 0.42f, 0.05f, 1.0f);
      hud_ui_rect(2.0f * k, (y + half) * k, 636.0f * k, 2.0f, 1.0f, 0.42f, 0.05f, 0.85f);
    }
    hud_ui_text(TC_NAME[r], 20.0f * k, y * k - 3.5f * 3.0f, 3.0f, 1, 1, 1, dim ? 0.45f : 1.0f);
    if (r == TC_STEERING) {
      for (int c = 0; c < HUD_STEER_MODES; ++c) {
        float x, w;
        tc_steer_chip(c, &x, &w);
        int lit = m->gyro == c;
        hud_ui_rect(x * k, (y - 12.0f) * k, w * k, 24.0f * k, lit ? 0.35f : 0.0f, lit ? 0.15f : 0.0f, 0.0f, lit ? 0.9f : 0.4f);
        if (lit) hud_ui_rect(x * k, (y + 10.0f) * k, w * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_text(TC_STEER[c], (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(TC_STEER[c], TC_PX), y * k - 3.5f * TC_PX, TC_PX, 1, 1, 1, lit ? 1.0f : 0.45f);
      }
    } else if (r == TC_SENS) {
      for (int i = 0; i < 10; ++i) {
        int lit = i < m->gyro_sens;
        float a = dim ? 0.3f : 1.0f;
        hud_ui_rect((TC_X + TC_CELL * (float)i + 2.0f) * k, (y - 10.0f) * k, (TC_CELL - 4.0f) * k, 20.0f * k, lit ? (i < 4 ? 0.3f : i < 8 ? 0.9f : 1.0f) : 0.0f,
                    lit ? (i < 4 ? 0.85f : i < 8 ? 0.8f : 0.3f) : 0.0f, lit ? 0.3f : 0.0f, lit ? 0.95f * a : 0.4f * a);
      }
      char t[8];
      snprintf(t, sizeof t, "%d", m->gyro_sens);
      hud_ui_text(t, (TC_X + TC_CELL * 10.0f + 12.0f) * k, y * k - 3.5f * 3.0f, 3.0f, 1, 1, 1, dim ? 0.45f : 1.0f);
    } else if (r == TC_PEDALS) {
      for (int c = 0; c < 2; ++c) {
        float x = TC_X + 80.0f * (float)c;
        int lit = m->pedal_slide == !c;
        hud_ui_rect(x * k, (y - 12.0f) * k, 70.0f * k, 24.0f * k, lit ? 0.35f : 0.0f, lit ? 0.15f : 0.0f, 0.0f, lit ? 0.9f : 0.4f);
        if (lit) hud_ui_rect(x * k, (y + 10.0f) * k, 70.0f * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        const char *s = c ? "OFF" : "ON";
        hud_ui_text(s, (x + 35.0f) * k - 0.5f * hud_ui_text_width(s, 2.6f), y * k - 3.5f * 2.6f, 2.6f, 1, 1, 1, lit ? 1.0f : 0.45f);
      }
    } else {
      hud_ui_rect(TC_X * k, (y - 12.0f) * k, 110.0f * k, 24.0f * k, 0.35f, 0.15f, 0.0f, 0.9f);
      hud_ui_rect(TC_X * k, (y + 10.0f) * k, 110.0f * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
      hud_ui_text("OPEN >", (TC_X + 55.0f) * k - 0.5f * hud_ui_text_width("OPEN >", 2.6f), y * k - 3.5f * 2.6f, 2.6f, 1, 1, 1, 1);
    }
  }
}

/* a row of the wheel list changed: by (-1, +1) with left and right, or OK (wrap: the strengths go round past 100 %) */
static void wheel_change(menu_state *m, int row, int by, int ok) {
  if (row == FR_TEST) {
    if (ffb_device_count() && *ffb_val(FFB_DEVICE) > 0 && *ffb_val(FFB_ON)) {
      ffb_test();
      snd_menu(SND_SELECT);
      bindings_note(m, "TESTING...");
    } else {
      snd_menu(SND_CANCEL);
      bindings_note(m, "CHOOSE A DEVICE AND SWITCH FORCE FEEDBACK ON FIRST");
    }
    return;
  }
  int *v = ffb_val(FR_ID[row]);
  if (row == FR_DEVICE) {
    int count = ffb_device_count() + 1, to = (*v + by + count) % count;
    if (to != *v) *v = to, m->garage_changed = 1, snd_menu(SND_CURSOR);
    return;
  }
  if (row == FR_ON || row == FR_INVERT) {
    *v = !*v;
  } else {
    int to = *v + by;
    if (to > 10) to = ok ? 0 : 10;
    if (to < 0) to = 0;
    if (to == *v) return;
    *v = to;
  }
  m->garage_changed = 1;
  snd_menu(SND_CURSOR);
}

static void bindings_step(menu_state *m, const menu_in *in) {
  if (bind_touch(m) && !m->bind_wait) {
    if (in->back) {
      snd_menu(SND_CANCEL);
      if (m->settings_direct) m->settings_direct = 0, selmenu_enter(m, 2, 1);
      else option_enter(m);
      return;
    }
    touchctl_step(m, in);
    return;
  }
  int n = bind_rows(m->bind_ctx), cols = m->bind_ctx == BIND_CONTEXTS ? 4 : BIND_SLOTS;
  if (m->bind_wait) {
    if (in->back || in->aux) { /* (the screen's back key, the pointer's: the bound inputs themselves go to the capture) */
      bind_wait_off(m, "");
    } else if (++m->bind_wait_t > BIND_WAIT_FRAMES) {
      bind_wait_off(m, "NOTHING PRESSED: CANCELLED");
    }
    return;
  }
  if (in->back && m->settings_direct) {
    snd_menu(SND_CANCEL);
    m->settings_direct = 0;
    selmenu_enter(m, 2, 1);
    return;
  }
  if (in->back) {
    snd_menu(SND_CANCEL);
    option_enter(m);
    return;
  }
  if (in->tab >= 0 && in->tab < BIND_TABS) {
    if (in->tab != m->bind_ctx) snd_menu(SND_CURSOR);
    m->bind_ctx = in->tab, m->bind_row = 0, m->bind_wait = 0;
    bindings_note(m, "");
    return;
  }
  if (in->select >= 0) {
    m->bind_row = in->select / BIND_SLOTS;
    m->bind_col = in->select % BIND_SLOTS;
    if (m->bind_row > n) m->bind_row = n;
    if (m->bind_col >= cols) m->bind_col = cols - 1;
  }
  if (in->view) {
    snd_menu(SND_CURSOR);
    m->bind_ctx = (m->bind_ctx + 1) % BIND_TABS, m->bind_row = 0, m->bind_col = 0;
    bindings_note(m, "");
    return;
  }
  if (in->dy) {
    int r = m->bind_row + in->dy;
    if (r >= 0 && r <= n) snd_menu(SND_CURSOR), m->bind_row = r;
    return;
  }
  if (m->bind_ctx == BIND_FFB) { /* the wheel list: left and right change the value, OK steps it on (the last row puts the defaults back) */
    if (m->bind_row >= n) {
      if (in->ok) {
        ffb_defaults();
        m->garage_changed = 1;
        bindings_note(m, "DEFAULTS BACK");
        snd_menu(SND_SELECT);
      }
    } else if (in->dx || in->ok) {
      wheel_change(m, m->bind_row, in->dx ? in->dx : 1, in->ok);
    }
    return;
  }
  if (in->dx && m->bind_ctx == BIND_CONTEXTS && m->bind_row == BA_COUNT) { /* the steering filter's row: left or right flips it */
    m->filter_on = !m->filter_on, m->garage_changed = 1;
    snd_menu(SND_CURSOR);
    return;
  }
  if (in->dx && bind_is_speed(m->bind_ctx, m->bind_row)) { /* the buttons' steering speed: left and right step it */
    filter_step(m, BA_COUNT + 3, in->dx, 0);
    return;
  }
  if (in->ok && bind_is_speed(m->bind_ctx, m->bind_row)) {
    filter_step(m, BA_COUNT + 3, 1, 1);
    return;
  }
  if (in->dx && m->bind_ctx == BIND_CONTEXTS && m->bind_row > BA_COUNT && m->bind_row < BA_COUNT + 3) { /* a number of the filter: left and right step it by 0.5 */
    filter_step(m, m->bind_row, in->dx, 0);
    return;
  }
  if (in->dx) {
    int c = m->bind_col + in->dx;
    if (c >= 0 && c < cols && m->bind_row < n) snd_menu(SND_CURSOR), m->bind_col = c;
    return;
  }
  if (in->ok && m->bind_ctx == BIND_CONTEXTS && m->bind_row > BA_COUNT && m->bind_row < BA_COUNT + 3) { /* OK: up by 0.5, round to 0 past 10 */
    filter_step(m, m->bind_row, 1, 1);
    return;
  }
  if (in->ok) {
    if (m->bind_ctx == BIND_CONTEXTS && m->bind_row == BA_COUNT) {
      m->filter_on = !m->filter_on, m->garage_changed = 1;
      snd_menu(SND_CURSOR);
      return;
    }
    if (m->bind_ctx == BIND_CONTEXTS && m->bind_row < BA_COUNT) { /* an axis cell: the axis (waits for a push), invert, the dead zone's next step */
      const bind_axis *a = bind_axis_get(m->bind_row);
      if (m->bind_col == 0) {
        bind_wait_on(m);
        bindings_note(m, "");
        snd_menu(SND_CURSOR);
      } else if (m->bind_col == 1) {
        bind_axis_set(m->bind_row, a->axis, !a->invert, a->dead);
        m->garage_changed = 1;
        snd_menu(SND_CURSOR);
      } else if (m->bind_col == 2) {
        bind_axis_set(m->bind_row, a->axis, a->invert, bind_next(BIND_DEAD, (int)(sizeof BIND_DEAD / sizeof *BIND_DEAD), a->dead));
        m->garage_changed = 1;
        snd_menu(SND_CURSOR);
      } else {
        bind_axis_set_sens(m->bind_row, bind_next(BIND_SENS, (int)(sizeof BIND_SENS / sizeof *BIND_SENS), a->sens));
        m->garage_changed = 1;
        snd_menu(SND_CURSOR);
      }
      return;
    }
    if (m->bind_row >= n) { /* the list's defaults */
      if (m->bind_ctx == BIND_CONTEXTS) bind_axes_defaults();
      else bind_context_defaults(m->bind_ctx), m->btn_steer = m->bind_ctx == BIND_RACE ? 10 : m->btn_steer;
      m->garage_changed = 1;
      bindings_note(m, "DEFAULTS BACK");
      snd_menu(SND_SELECT);
    } else {
      bind_wait_on(m);
      bindings_note(m, "");
      snd_menu(SND_CURSOR);
    }
  }
}

/* a cell's place (640 x 480) */
static void bindings_cell(int col, float *x, float *w) {
  *x = BIND_X_COL + BIND_COL_W * (float)col;
  *w = BIND_COL_W - 4.0f;
}

static void draw_bindings(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  const float k = 1.6f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  draw_bottom(bars, 1);
  int ctx = m->bind_ctx, n = bind_rows(ctx);
  int touch = bind_touch(m), tb = 0;
  if (touch) {
    anm_swap title = { anm_sprite(T_TOPBAR_MES), -1 };
    draw_topbar(bars, 1, &title);
  } else {
    tb = draw_title_bar(bars, 0);
  }
  hud_ui_begin(screen_w, screen_h);
  if (touch) {
    hud_ui_text("TOUCH CONTROLS", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
    draw_touchctl(m, screen_w, screen_h);
    hud_ui_end();
    return;
  }
  if (!tb) hud_ui_text("KEY BINDINGS", 24.0f * k, 40.0f * k, 4.0f, 1, 1, 1, 1);
  for (int c = 0; c < BIND_TABS; ++c) { /* the lists */
    float x = BIND_X_NAME + 96.0f * (float)c, w = 90.0f;
    hud_ui_rect(x * k, BIND_Y_TAB * k, w * k, 20.0f * k, c == ctx ? 1.0f : 0.0f, c == ctx ? 0.42f : 0.0f, c == ctx ? 0.05f : 0.0f,
                c == ctx ? 0.9f : 0.45f);
    hud_ui_text(BIND_TAB[c], (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(BIND_TAB[c], 2.2f), (BIND_Y_TAB + 3.0f) * k, 2.2f, 1, 1, 1, 1);
  }
  if (ctx == BIND_FFB) { /* the wheel: the device, the switches, the strengths as bars, the test */
    for (int r = 0; r <= n; ++r) {
      float y = FR_Y0 + FR_DY * (float)r;
      int row_on = r == m->bind_row;
      hud_ui_rect(0, y * k, 640.0f * k, (FR_DY - 2.0f) * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
      if (row_on) hud_ui_rect(2.0f * k, y * k, 4.0f * k, (FR_DY - 2.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
      if (r == n) {
        hud_ui_text("RESET TO DEFAULT", BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, row_on ? 1.0f : 0.8f, row_on ? 0.55f : 0.8f, row_on ? 0.15f : 0.8f, 1);
        continue;
      }
      hud_ui_text(FR_NAME[r], BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, 1);
      const float cx = AX_X[0], cw = FR_CELL_W;
      hud_ui_rect(cx * k, (y + 1.0f) * k, cw * k, (FR_DY - 4.0f) * k, row_on ? 0.35f : 0.0f, row_on ? 0.15f : 0.0f, 0.0f, row_on ? 0.85f : 0.4f);
      if (row_on) hud_ui_rect(cx * k, (y + 1.0f) * k, cw * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
      const int v = r == FR_TEST ? 0 : *ffb_val(FR_ID[r]);
      char txt[160];
      if (r == FR_DEVICE) snprintf(txt, sizeof txt, "< %s >", v > 0 ? ffb_device_name(v - 1) : ffb_device_count() ? "NONE" : "NONE FOUND");
      else if (r == FR_ON || r == FR_INVERT) snprintf(txt, sizeof txt, "%s", v ? "ON" : "OFF");
      else if (r == FR_TEST) snprintf(txt, sizeof txt, "PLAY");
      else {
        snprintf(txt, sizeof txt, "< %d %% >", v * 10);
        hud_ui_rect(cx * k, (y + FR_DY - 6.0f) * k, cw * ((float)v / 10.0f) * k, 3.0f, 1.0f, 0.42f, 0.05f, 0.9f); /* (the strength under the number) */
      }
      float px = 1.9f;
      while (px > 1.2f && hud_ui_text_width(txt, px) > (cw - 8.0f) * k) px -= 0.1f;
      hud_ui_text(txt, (cx + 0.5f * cw) * k - 0.5f * hud_ui_text_width(txt, px), (y + 5.0f) * k, px, 1, 1, 1, v == 0 && r == FR_DEVICE ? 0.55f : 1.0f);
    }
    const char *dev = ffb_selected_name();
    char line[240];
    snprintf(line, sizeof line, "%s", m->bind_note[0] ? m->bind_note : m->bind_row < FR_COUNT ? FR_NOTE[m->bind_row] : "PRESS VIEW TO SWITCH CATEGORY");
    hud_ui_text(line, 20.0f * k, 368.0f * k, 1.7f, 0.85f, 0.85f, 0.85f, 1);
    snprintf(line, sizeof line, "%s%s%s", dev ? dev : ffb_device_count() ? "NO DEVICE CHOSEN" : "NO WHEEL OR CONTROLLER FOUND", dev && ffb_status()[0] ? ": " : "", dev ? ffb_status() : "");
    hud_ui_text(line, 20.0f * k, 384.0f * k, 1.7f, 1.0f, 0.55f, 0.15f, 1);
    hud_ui_end();
    return;
  }
  if (ctx == BIND_CONTEXTS) { /* the analog axes: which axis, inverted, the dead zone, and what it gives now */
    static const char *const HEAD[5] = { "AXIS", "INVERT", "DEADZONE", "SENS.", "NOW" };
    for (int c = 0; c < 5; ++c)
      hud_ui_text(HEAD[c], (AX_X[c] + 0.5f * AX_W[c]) * k - 0.5f * hud_ui_text_width(HEAD[c], 1.8f), (BIND_Y_ROW - 17.0f) * k, 1.8f, 0.8f, 0.8f, 0.8f, 1);
    for (int r = 0; r <= n; ++r) {
      float y = BIND_Y_ROW + BIND_ROW_DY * (float)r;
      int row_on = r == m->bind_row;
      hud_ui_rect(0, y * k, 640.0f * k, (BIND_ROW_DY - 2.0f) * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
      if (row_on) hud_ui_rect(2.0f * k, y * k, 4.0f * k, (BIND_ROW_DY - 2.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
      if (r == n) {
        hud_ui_text("RESET TO DEFAULT", BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, row_on ? 1.0f : 0.8f, row_on ? 0.55f : 0.8f, row_on ? 0.15f : 0.8f, 1);
        continue;
      }
      if (r == BA_COUNT) { /* the steering filter: direct (as the car has always taken the axis) or filtered (smoothing and countersteer feel, tuned in the race with F5) */
        hud_ui_text("STEERING FILTER", BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, 1);
        const char *fs = m->filter_on ? "FILTERED" : "DIRECT";
        hud_ui_rect(AX_X[0] * k, (y + 1.0f) * k, AX_W[0] * k, (BIND_ROW_DY - 4.0f) * k, row_on ? 0.35f : 0.0f, row_on ? 0.15f : 0.0f, 0.0f, row_on ? 0.85f : 0.4f);
        if (row_on) hud_ui_rect(AX_X[0] * k, (y + 1.0f) * k, AX_W[0] * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_text(fs, (AX_X[0] + 0.5f * AX_W[0]) * k - 0.5f * hud_ui_text_width(fs, 1.9f), (y + 5.0f) * k, 1.9f, 1, 1, 1, 1);
        continue;
      }
      if (r > BA_COUNT && r < BA_COUNT + 3) { /* the filter's smoothing and countersteer feel: < value > */
        static const char *const FN[3] = { "SMOOTHING", "COUNTERSTEER FEEL", "BUTTON STEERING SPEED" };
        int btn = r == BA_COUNT + 3;
        hud_ui_text(FN[r - BA_COUNT - 1], BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, m->filter_on || btn ? 1.0f : 0.55f);
        char vt[24];
        if (btn) snprintf(vt, sizeof vt, "< %d >", m->btn_steer);
        else snprintf(vt, sizeof vt, "< %.1f >", (float)*filter_value((menu_state *)m, r) * 0.5f);
        hud_ui_rect(AX_X[0] * k, (y + 1.0f) * k, AX_W[0] * k, (BIND_ROW_DY - 4.0f) * k, row_on ? 0.35f : 0.0f, row_on ? 0.15f : 0.0f, 0.0f, row_on ? 0.85f : 0.4f);
        if (row_on) hud_ui_rect(AX_X[0] * k, (y + 1.0f) * k, AX_W[0] * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_text(vt, (AX_X[0] + 0.5f * AX_W[0]) * k - 0.5f * hud_ui_text_width(vt, 1.9f), (y + 5.0f) * k, 1.9f, 1, 1, 1, 1);
        continue;
      }
      hud_ui_text(bind_axis_action_name(r), BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, 1);
      const bind_axis *a = bind_axis_get(r);
      char dead[16], sens[16];
      snprintf(dead, sizeof dead, "%d %%", a->dead);
      snprintf(sens, sizeof sens, "%d %%", a->sens);
      const char *txt[4] = { a->axis == AXIS_NONE ? "--" : bind_axis_name(a->axis), a->invert ? "ON" : "OFF", dead, sens };
      for (int c = 0; c < 4; ++c) {
        int on = row_on && c == m->bind_col;
        hud_ui_rect(AX_X[c] * k, (y + 1.0f) * k, AX_W[c] * k, (BIND_ROW_DY - 4.0f) * k, on ? 0.35f : 0.0f, on ? 0.15f : 0.0f, 0.0f, on ? 0.85f : 0.4f);
        if (on) {
          float al = m->bind_wait ? 0.55f + 0.25f * cosf((float)clock * 0.2f) : 0.0f;
          if (al > 0) hud_ui_rect(AX_X[c] * k, (y + 1.0f) * k, AX_W[c] * k, (BIND_ROW_DY - 4.0f) * k, 1.0f, 0.42f, 0.05f, al);
          hud_ui_rect(AX_X[c] * k, (y + 1.0f) * k, AX_W[c] * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
          hud_ui_rect(AX_X[c] * k, (y + BIND_ROW_DY - 4.0f) * k, AX_W[c] * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        }
        const char *s = on && m->bind_wait && c == 0 ? "MOVE IT..." : txt[c];
        float px = 1.9f;
        while (px > 1.2f && hud_ui_text_width(s, px) > (AX_W[c] - 6.0f) * k) px -= 0.1f;
        hud_ui_text(s, (AX_X[c] + 0.5f * AX_W[c]) * k - 0.5f * hud_ui_text_width(s, px), (y + 5.0f) * k, px, 1, 1, 1, a->axis == AXIS_NONE && c == 0 ? 0.45f : 1.0f);
      }
      /* what it gives now: a bar (the steering from its middle) */
      float v = bind_axis_value(r, m->axis_raw), bx = AX_X[4], bw = AX_W[4];
      hud_ui_rect(bx * k, (y + 5.0f) * k, bw * k, (BIND_ROW_DY - 12.0f) * k, 0, 0, 0, 0.6f);
      if (r == BA_STEER) {
        hud_ui_rect((bx + 0.5f * bw) * k - 1.0f, (y + 3.0f) * k, 2.0f, (BIND_ROW_DY - 8.0f) * k, 1, 1, 1, 0.8f);
        float w = 0.5f * bw * (v < 0 ? -v : v);
        hud_ui_rect((v < 0 ? bx + 0.5f * bw - w : bx + 0.5f * bw) * k, (y + 7.0f) * k, w * k, (BIND_ROW_DY - 16.0f) * k, v < 0 ? 0.2f : 1.0f, v < 0 ? 0.55f : 0.42f, v < 0 ? 1.0f : 0.05f, 0.95f);
      } else {
        hud_ui_rect(bx * k, (y + 7.0f) * k, bw * v * k, (BIND_ROW_DY - 16.0f) * k, r == BA_GAS ? 0.3f : 1.0f, r == BA_GAS ? 0.85f : 0.25f, 0.25f, 0.95f);
      }
    }
    const char *ahint = m->bind_wait ? "MOVE A STICK OR PULL A TRIGGER   ANY BUTTON CANCELS" : "PRESS VIEW TO SWITCH CATEGORY";
    hud_ui_text(m->bind_note[0] ? m->bind_note : ahint, 20.0f * k, 388.0f * k, 1.7f, 0.85f, 0.85f, 0.85f, 1);
    hud_ui_end();
    return;
  }
  for (int s = 0; s < BIND_SLOTS; ++s) { /* the columns' names */
    float x, w;
    bindings_cell(s, &x, &w);
    hud_ui_text(BIND_HEAD[s], (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(BIND_HEAD[s], 1.8f), (BIND_Y_ROW - 17.0f) * k, 1.8f, 0.8f, 0.8f, 0.8f, 1);
  }
  for (int r = 0; r <= n; ++r) {
    float y = BIND_Y_ROW + BIND_ROW_DY * (float)r;
    int row_on = r == m->bind_row;
    hud_ui_rect(0, y * k, 640.0f * k, (BIND_ROW_DY - 2.0f) * k, 0, 0, 0, r & 1 ? 0.25f : 0.38f);
    if (r == n) {
      hud_ui_text("RESET TO DEFAULT", BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, row_on ? 1.0f : 0.8f, row_on ? 0.55f : 0.8f,
                  row_on ? 0.15f : 0.8f, 1);
      if (row_on) hud_ui_rect(2.0f * k, y * k, 4.0f * k, (BIND_ROW_DY - 2.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
      continue;
    }
    if (bind_is_speed(ctx, r)) { /* not an action: < speed > in the first cell's place */
      hud_ui_text("BUTTON STEERING SPEED", BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, 1);
      if (row_on) hud_ui_rect(2.0f * k, y * k, 4.0f * k, (BIND_ROW_DY - 2.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
      float x, w;
      bindings_cell(0, &x, &w);
      char vt[16];
      snprintf(vt, sizeof vt, "< %d >", m->btn_steer);
      hud_ui_rect(x * k, (y + 1.0f) * k, w * k, (BIND_ROW_DY - 4.0f) * k, row_on ? 0.35f : 0.0f, row_on ? 0.15f : 0.0f, 0.0f, row_on ? 0.85f : 0.4f);
      if (row_on) hud_ui_rect(x * k, (y + 1.0f) * k, w * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
      hud_ui_text(vt, (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(vt, 1.9f), (y + 5.0f) * k, 1.9f, 1, 1, 1, 1);
      continue;
    }
    int ra = bind_act(ctx, r); /* (the action of the row) */
    hud_ui_text(bind_action_name(ctx, ra), BIND_X_NAME * k, (y + 5.0f) * k, 2.0f, 1, 1, 1, 1);
    if (row_on) hud_ui_rect(2.0f * k, y * k, 4.0f * k, (BIND_ROW_DY - 2.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
    for (int s = 0; s < BIND_SLOTS; ++s) {
      float x, w;
      bindings_cell(s, &x, &w);
      int on = row_on && s == m->bind_col;
      const bind_slot *b = bind_get(ctx, ra, s);
      hud_ui_rect(x * k, (y + 1.0f) * k, w * k, (BIND_ROW_DY - 4.0f) * k, on ? 0.35f : 0.0f, on ? 0.15f : 0.0f, 0.0f, on ? 0.85f : 0.4f);
      if (on) { /* the cell: an orange frame (the wait: filled) */
        float a = m->bind_wait ? 0.55f + 0.25f * cosf((float)clock * 0.2f) : 0.0f;
        if (a > 0) hud_ui_rect(x * k, (y + 1.0f) * k, w * k, (BIND_ROW_DY - 4.0f) * k, 1.0f, 0.42f, 0.05f, a);
        hud_ui_rect(x * k, (y + 1.0f) * k, w * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_rect(x * k, (y + BIND_ROW_DY - 4.0f) * k, w * k, 2.0f, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_rect(x * k, (y + 1.0f) * k, 2.0f, (BIND_ROW_DY - 4.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
        hud_ui_rect((x + w) * k - 2.0f, (y + 1.0f) * k, 2.0f, (BIND_ROW_DY - 4.0f) * k, 1.0f, 0.42f, 0.05f, 1.0f);
      }
      const char *name = on && m->bind_wait ? "PRESS..." : bind_name(b);
      float px = 1.9f;
      while (px > 1.2f && hud_ui_text_width(name, px) > (w - 6.0f) * k) px -= 0.1f;
      hud_ui_text(name, (x + 0.5f * w) * k - 0.5f * hud_ui_text_width(name, px), (y + 5.0f) * k, px, 1, 1, 1, b->kind == BIND_NONE && !(on && m->bind_wait) ? 0.45f : 1.0f);
    }
  }
  const char *hint = m->bind_wait ? (m->bind_col < BIND_KEYS ? "PRESS A KEY   ESC OR ANY OTHER INPUT CANCELS"
                                                              : "PRESS A PAD BUTTON, THE STICK OR A TRIGGER   ANY KEY CANCELS")
                                  : "PRESS VIEW TO SWITCH CATEGORY";
  hud_ui_text(m->bind_note[0] ? m->bind_note : hint, 20.0f * k, 388.0f * k, 1.7f, m->bind_note[0] ? 1.0f : 0.85f, m->bind_note[0] ? 0.7f : 0.85f,
              m->bind_note[0] ? 0.3f : 0.85f, 1);
  hud_ui_end();
}


/* ---- the port's SAVE DATA screen (the arcade keeps a player on a card, checked between
 * START and the car select; the iPad keeps MENU_SAVES saves instead), in the OPTION
 * screen's style: NEW GAME or LOAD GAME on the course band (a tap, or the left/right keys),
 * under it the saves, each its cars, their story episodes won and 10-in-a-row gold medals
 * and when it was last saved. LOAD GAME loads a tapped save with data; NEW GAME starts over
 * in a tapped empty save, in one with data on a second tap. The back key goes back to the
 * title screen. ---- */
#define SAVE_TEXT 101950  /* menupack.py SAVE_TEXT_BASE */
#define SAVE_GLYPH 101980 /* menupack.py SAVE_GLYPH_BASE: 0..9, '/', ':' in font 1 */
#define SAVE_ADV 8.0f     /* font 1's advance */
/* the player's name's characters (namedb.h) in font 8, 10 and 5: menupack.py NAME_GLYPH8_BASE
 * (then the grid's back space and space marks), NAME_GLYPH10_BASE, NAME_GLYPH5_BASE */
#define NAME_GLYPH8 106000
#define NAME_GLYPH10 106300
#define NAME_GLYPH5 106600
enum {
  S_TITLE, S_NEW, S_LOAD, S_SLOT, S_NO_DATA = S_SLOT + MENU_SAVES, S_STORY, S_MEDALS, S_CARS, S_HINT_NEW, S_HINT_LOAD,
  S_HINT_AGAIN, S_ERASE_HINT = S_HINT_AGAIN + MENU_SAVES, S_DELETE, S_HINT_DEL, S_HINT_DEL_AGAIN
};
#define SAVE_Y0 152.0f /* the first save's band */
#define SAVE_DY 64.0f
#define SAVE_H 58.0f
#define SAVE_INFO_X 176.0f
/* the "i" key on the chips' row and its page (MENU_INFO, who made the port and
 * that it is not Bandai Namco's): menupack.py INFO_KEY, INFO_PAGE (tools/infopage.py, drawn
 * there at 1.6 times this space's size) */
#define INFO_KEY 108000
#define INFO_PAGE 108001
#define INFO_SCALE 1.6f
static const float INFO_KEY_RECT[4] = { 572, 96, 40, 40 };

static void save_enter(menu_state *m) {
  go(m, MENU_SAVE);
  m->save_new = m->save_del = 0;
  m->save_confirm = 0;
  m->save_scan = 1;
  m->focus = 0;
  /* one card for now (the saves' screen stays for the cards to come): its save taken at once, the screen after it
   * follows (menu_step) once the app has loaded it */
  if (!m->save_skip) {
    m->save_pick = 1;
    m->save_pick_new = 0;
    m->save_auto = 1;
  }
}

/* the chips: NEW GAME, LOAD GAME, DELETE */
static int save_chip_text(int k) { return anm_sprite(SAVE_TEXT + (k < 2 ? S_NEW + k : S_DELETE)); }

/* the chips' left edges */
static void save_chips(float x[3]) {
  x[0] = 24.0f;
  for (int k = 1; k < 3; ++k) x[k] = x[k - 1] + anm_sprite_width(save_chip_text(k - 1)) + SET_CHIP_GAP;
}

static int save_chip(const menu_state *m) { return m->save_del ? 2 : m->save_new ? 0 : 1; }

static void save_chip_set(menu_state *m, int k) {
  snd_menu(SND_CURSOR);
  m->save_new = k == 0;
  m->save_del = k == 2;
  m->save_confirm = 0;
}

/* one step of the save screen; returns 1 when a save is chosen (save_pick set) */
static int save_step(menu_state *m, const menu_in *in) {
  if (in->dx) {
    int k = save_chip(m) + in->dx;
    if (k >= 0 && k < 3) save_chip_set(m, k);
    return 0;
  }
  if (in->dy) { /* the focused save: up and down the list */
    int f = m->focus + in->dy;
    if (f >= 0 && f < MENU_SAVES) snd_menu(SND_CURSOR), m->focus = f;
    return 0;
  }
  if (in->aux) { /* the "i" key */
    if (anm_sprite(INFO_KEY) < 0) return 0;
    snd_menu(SND_SELECT);
    m->save_confirm = 0;
    go(m, MENU_INFO);
    return 0;
  }
  if (in->tab >= 0) {
    if (in->tab != save_chip(m)) save_chip_set(m, in->tab);
    return 0;
  }
  if (in->select >= 0) m->focus = in->select; /* (the mouse and the controls share the focus) */
  if (!in->ok) return 0;
  if (m->focus < 0 || m->focus >= MENU_SAVES) m->focus = 0;
  int k = m->focus;
  const menu_save_info *s = &m->save[k];
  if (!m->save_new && !s->used) {
    snd_menu(SND_CANCEL);
    return 0;
  }
  if ((m->save_new || m->save_del) && s->used && m->save_confirm != k + 1) {
    snd_menu(SND_CURSOR);
    m->save_confirm = k + 1;
    return 0;
  }
  if (m->save_del) {
    /* tapped again: deleted (the app takes it off and scans the saves again) */
    snd_menu(SND_SELECT);
    m->save_confirm = 0;
    m->save_erase = k + 1;
    return 0;
  }
  snd_menu(SND_SELECT);
  m->save_confirm = 0;
  m->save_pick = k + 1;
  m->save_pick_new = m->save_new;
  m->sel = k;
  return 1;
}

/* a number or a date in font 1's glyphs from (x, y); returns the x after it */
static float save_glyphs(const char *text, float x, float y, float a) {
  for (const char *c = text; *c; ++c, x += SAVE_ADV) {
    int g = *c >= '0' && *c <= '9' ? *c - '0' : *c == '/' ? 10 : *c == ':' ? 11 : -1;
    if (g >= 0) anm_draw_sprite(anm_sprite(SAVE_GLYPH + g), x, y, a);
  }
  return x;
}

static void draw_save(const menu_state *m, int clock, int bars) {
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  anm_draw_sprite(anm_sprite(T_SET_COURSE_BAND), -1.7f, 84.3f, 0.5f);
  float x[3];
  save_chips(x);
  for (int k = 0; k < 3; ++k) anm_draw_sprite(save_chip_text(k), x[k], SET_ROW_Y, save_chip(m) == k ? 1.0f : 0.3f);
  static const float BAND[4] = { 192, 0, 832, 128 };
  for (int k = 0; k < MENU_SAVES; ++k) {
    const menu_save_info *s = &m->save[k];
    float y = SAVE_Y0 + SAVE_DY * (float)k;
    /* a save to be tapped again blinks; LOAD GAME dims the empty ones */
    float band = m->save_confirm == k + 1 ? 0.5f + 0.3f * cosf((float)clock * 0.2f) : 0.5f;
    if (k == m->focus) band = fminf(1.0f, band + 0.4f); /* the focused save, for the controls */
    float a = !m->save_new && !s->used ? 0.3f : 1.0f;
    const float dst[4] = { 0, y, 640, y + SAVE_H };
    anm_draw_part(anm_sprite(T_SET_BAND), BAND, dst, band);
    anm_draw_sprite(anm_sprite(SAVE_TEXT + S_SLOT + k), 16.0f, y + 14.0f, a);
    if (!s->used) {
      anm_draw_sprite(anm_sprite(SAVE_TEXT + S_NO_DATA), SAVE_INFO_X, y + 19.0f, 0.5f * a);
      continue;
    }
    char num[32];
    float lx = SAVE_INFO_X;
    const int label[3] = { S_CARS, S_STORY, S_MEDALS }, value[3] = { s->cars, s->story, s->medals };
    static const float LABEL_X[3] = { 0, 90, 260 };
    for (int i = 0; i < 3; ++i) {
      int t = anm_sprite(SAVE_TEXT + label[i]);
      anm_draw_sprite(t, lx + LABEL_X[i], y + 8.0f, a);
      snprintf(num, sizeof num, "%d", value[i]);
      save_glyphs(num, lx + LABEL_X[i] + anm_sprite_width(t) - 4.0f, y + 8.0f, a);
    }
    time_t when = (time_t)s->when;
    struct tm tm;
    if (when > 0 && localtime_r(&when, &tm)) {
      strftime(num, sizeof num, "%Y/%m/%d %H:%M", &tm);
      save_glyphs(num, lx, y + 30.0f, a);
    }
    /* the player's name, in the name grid's font */
    for (int i = 0; i < 5 && s->name[i] >= 0; ++i)
      anm_draw_sprite(anm_sprite(NAME_GLYPH8 + s->name[i]), lx + 170.0f + 24.0f * (float)i, y + 23.0f, a);
  }
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(SAVE_TEXT + S_TITLE) };
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
  int hint = anm_sprite(SAVE_TEXT + (m->save_del ? (m->save_confirm ? S_HINT_DEL_AGAIN + m->save_confirm - 1 : S_HINT_DEL)
                                     : m->save_confirm ? S_HINT_AGAIN + m->save_confirm - 1
                                     : m->save_new ? S_HINT_NEW : S_HINT_LOAD));
  if (!bars) if (bar_keys()) anm_draw_sprite(hint, 320.0f - anm_sprite_width(hint) * 0.5f, 427.0f, 0.75f + 0.25f * cosf((float)clock * 0.1f));
  int key = anm_sprite(INFO_KEY);
  if (key >= 0) {
    float src[4] = { 0, 0, anm_sprite_width(key), anm_sprite_height(key) };
    const float *r = INFO_KEY_RECT;
    float dst[4] = { r[0], r[1], r[0] + r[2], r[1] + r[3] };
    anm_draw_part(key, src, dst, 1);
  }
}

/* the "i" page: the port's own words over the SAVE DATA screen's background; a tap or the
 * back key back to it */
static void draw_info(int clock, int bars) {
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  int page = anm_sprite(INFO_PAGE);
  if (page >= 0) {
    float w = anm_sprite_width(page), h = anm_sprite_height(page);
    float src[4] = { 0, 0, w, h };
    /* between the top bar and the bottom one */
    float k = fminf(1.0f / INFO_SCALE, 322.0f / h), x = 320.0f - 0.5f * w * k, y = 72.0f;
    float dst[4] = { x, y, x + w * k, y + h * k };
    anm_draw_part(page, src, dst, 1);
  }
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(SAVE_TEXT + S_TITLE) };
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
}

/* ---- the save's cars: each one of the arcade's cards (clV386CardData: its car, tune,
 * parts, story and 10-in-a-row results). menu_state keeps the tune and parts per car (by
 * src/cardb.h's maker and car, so a save has one car of a kind), the story and the
 * 10-in-a-row per car too, the driven car's in st_div, st_bits and ten_result. ---- */
int menu_own_find(const menu_state *m, int maker, int car, int dup) {
  for (int k = 0; k < m->n_own; ++k)
    if (m->own[k].maker == maker && m->own[k].car == car && m->own[k].dup == dup) return k;
  return -1;
}

int menu_free_dup(const menu_state *m, int maker, int car) {
  for (int d = 0; d < MENU_DUPS; ++d)
    if (menu_own_find(m, maker, car, d) < 0) return d;
  return -1;
}

/* the slot of the per-car arrays of the car on hand: the card loaded (own_load, garage_show), or while a new car is chosen (the
 * maker, car and colour selects, then its setting before it is registered) the free one of its model */
int menu_slot(const menu_state *m) {
  int d = m->dup;
  if (m->new_card || m->screen == MENU_MAKER || m->screen == MENU_CAR || (m->screen == MENU_COLOR && !m->cc_mode)) {
    d = menu_free_dup(m, m->maker, m->car);
    if (d < 0) d = 0;
  }
  return m->car + MENU_GARAGE_CARS * d;
}

void menu_own_store(menu_state *m) {
  if (m->own_sel < 0 || m->own_sel >= m->n_own) return;
  int mk = m->own[m->own_sel].maker, sl = menu_own_slot(&m->own[m->own_sel]);
  m->st_div_car[mk][sl] = m->st_div;
  m->st_bits_car[mk][sl] = m->st_bits;
  memcpy(m->ten_car[mk][sl], m->ten_result, sizeof m->ten_result);
  /* (the driven car's name is the player's; but while a new car's name is typed (the app saves the garage meanwhile, which stores) m->name is that one's, not its) */
  if (!(m->screen == MENU_NAME && m->name_car == 2)) memcpy(m->own[m->own_sel].name, m->name, sizeof m->name);
}

void menu_own_load(menu_state *m, int k) {
  if (k < 0 || k >= m->n_own) return;
  const menu_own *o = &m->own[k];
  int mk = o->maker, c = o->car, sl = menu_own_slot(o);
  m->own_sel = k;
  m->maker = mk, m->car = c, m->color = o->color, m->dup = o->dup;
  m->st_div = m->st_div_car[mk][sl];
  m->st_bits = m->st_bits_car[mk][sl];
  if (getenv("WMMT_AERO")) m->aero[mk][sl] = (signed char)atoi(getenv("WMMT_AERO")); /* test: this aero set on the car shown */
  if (getenv("WMMT_VINYL")) m->vinyl[mk][sl] = (signed char)atoi(getenv("WMMT_VINYL")); /* test: this sticker on the car shown */
  if (m->dress_lv[mk][sl] < 63) m->dress_lv[mk][sl] = 63, m->garage_changed = 1; /* (every dress-up part and special colour is open from the start, a car of an older save too) */
  memcpy(m->ten_result, m->ten_car[mk][sl], sizeof m->ten_result);
  memcpy(m->name, o->name, sizeof m->name);
  m->own[k].used = (long long)time(NULL);
}

void menu_own_sort(menu_state *m) {
  signed char mk = -1, car = 0, dup = 0;
  int view = m->own_view >= 0 && m->own_view < m->n_own, vk = view ? m->own_view : 0;
  if (m->own_sel >= 0 && m->own_sel < m->n_own) mk = m->own[m->own_sel].maker, car = m->own[m->own_sel].car, dup = m->own[m->own_sel].dup;
  signed char vmk = view ? m->own[vk].maker : -1, vcar = view ? m->own[vk].car : 0, vdup = view ? m->own[vk].dup : 0;
  for (int i = 1; i < m->n_own; ++i) { /* (an insertion sort, stable: the cars never driven keep their order) */
    menu_own t = m->own[i];
    int j = i;
    for (; j > 0 && m->own[j - 1].used < t.used; --j) m->own[j] = m->own[j - 1];
    m->own[j] = t;
  }
  for (int i = 0; i < m->n_own; ++i) {
    if (m->own[i].maker == mk && m->own[i].car == car && m->own[i].dup == dup) m->own_sel = i;
    if (view && m->own[i].maker == vmk && m->own[i].car == vcar && m->own[i].dup == vdup) m->own_view = i;
  }
}

/* ---- the class (GetClass 0x857bb0c over the table at 0x9332980, src/hud.h HUD_CLASSES
 * rows): the sum of four counts, each the last row k such that rows 0..k all hold, 46 at
 * most: the maxi coins (card+0x1a8) and the points at card+0x180 at least the rows' first and
 * second columns (the ghost battles' and the versus', which the port has not: 0 here, no row
 * past N), the story by the third column (over 99: the story's loops, getStoryClearLoop, at
 * least column / 100; else isStoryClearOnce(column - 1)), plus the 10-in-a-row's courses
 * with gold (3) on all of their levels 0..9 (getTenResult). ---- */
static const short CLASS_STORY[47] = { 1, 10, 20, 30, 40, 50, 60, 70, 80, 100, 200, 300, 400, 500, 700, 1000,
                                       1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000,
                                       7500, 8000, 8500, 9000, 9500, 10000, 10500, 11000, 11500, 12000, 12500,
                                       13000, 13500, 14000, 14500, 15000, 15500, 16000, 16500 };

/* clV386CardData::isStoryClearOnce(n): the story's episode n (0..99, the five divisions of
 * 20 of a loop) won once: card+0x270 the divisions cleared (st_div), +0x188 the episodes of
 * the one under way (st_bits) */
static int story_clear_once(int div, int bits, int n) {
  if (div / 5 > 0) return 1;
  int d = n % 100 / 20;
  return d < div % 5 || (d == div % 5 && (bits >> (n % 20) & 1));
}

int menu_class(const menu_state *m, int mk, int sl) {
  int div = m->st_div_car[mk][sl], bits = m->st_bits_car[mk][sl], story = 0, ten = 0;
  for (int k = 0; k < 47; ++k) {
    int col = CLASS_STORY[k];
    if (!(col > 99 ? div / 5 >= col / 100 : story_clear_once(div, bits, col - 1))) break;
    story = k;
  }
  for (int course = 0; course < 10; ++course) {
    int all = 1;
    for (int level = 0; level < 10; ++level) all &= m->ten_car[mk][sl][course][level] >= 3;
    ten += all;
  }
  return story + ten > 46 ? 46 : story + ten;
}

void menu_class_update(menu_state *m) {
  menu_own_store(m);
  for (int k = 0; k < m->n_own; ++k) {
    int mk = m->own[k].maker, sl = menu_own_slot(&m->own[k]), cls = menu_class(m, mk, sl);
    if (cls > m->class_car[mk][sl]) m->class_car[mk][sl] = (signed char)cls;
  }
}

/* a new card's (and a deleted one's): stock, no parts, no results */
static void own_clear(menu_state *m, int mk, int sl) {
  m->tune_full[mk][sl] = 0, m->tune_power[mk][sl] = CAR_TUNE_MAX;
  m->st_power[mk][sl] = m->st_handling[mk][sl] = m->st_gauge[mk][sl] = 0;
  m->dress_lv[mk][sl] = m->aero[mk][sl] = m->mirror[mk][sl] = m->wing[mk][sl] = m->bonnet[mk][sl] = 0;
  m->wheel[mk][sl] = 0;
  m->neon[mk][sl] = 0;
  m->vinyl[mk][sl] = 0;
  m->st_div_car[mk][sl] = m->st_bits_car[mk][sl] = 0;
  memset(m->ten_car[mk][sl], 0, sizeof m->ten_car[mk][sl]);
  m->class_car[mk][sl] = 0;
  m->vs_stars_car[mk][sl] = m->vs_count_car[mk][sl] = 0;
  m->st_bonus_car[mk][sl] = 0;
  m->meter_car[mk][sl] = m->sound_car[mk][sl] = 0;
  m->card_plays[mk][sl] = 0, m->ccolor[mk][sl] = 0;
}

/* the colour select's ALL PARTS key (a new kind of car's card, or one already in the save) */
static int all_parts_shown(const menu_state *m) {
  (void)m;
  return 0; /* (the choice is STOCK or FULL TUNED, after the colour: sel_menu SELMENU_TUNE) */
}

static signed char new_full, new_power; /* the mode a new card starts in (new_car_choose) */

/* the car the colour select decided, as a new card (one of its kind already in the save is
 * that one, in the new colour); it is the car driven from now on */
static void own_register(menu_state *m) {
  menu_own_store(m);
  int mk = m->maker, c = m->car;
  /* always a new card, of the model's first free dup (a second car of a model is its own card); only with all MENU_DUPS taken is it
   * the first one again, in the new colour */
  int dup = menu_free_dup(m, mk, c), k = -1;
  if (dup < 0 || (m->save_skip && menu_own_find(m, mk, c, 0) >= 0)) dup = 0, k = menu_own_find(m, mk, c, 0); /* (a test that skips the saves meets its car again, not a new card each time) */
  int sl = c + MENU_GARAGE_CARS * dup;
  m->tune_locked = k >= 0;
  if (k < 0 && m->n_own < MENU_OWN_MAX) {
    k = m->n_own++;
    m->own[k] = (menu_own){ (signed char)mk, (signed char)c, (signed char)m->color, (long long)time(NULL), 0, (signed char)dup, { -1, -1, -1, -1, -1 } };
    /* (the mode the setting chose kept) */
    signed char full = m->tune_full[mk][sl], power = m->tune_power[mk][sl];
    own_clear(m, mk, sl);
    if (m->new_card) {
      m->tune_full[mk][sl] = new_full, m->tune_power[mk][sl] = new_power;
      /* a fully tuned car starts the story's second loop (st_div 5, no episode cleared in it): the rivals' level is the loop, and they keep up with the car */
      if (new_full) {
        m->st_div_car[mk][sl] = 5, m->st_bits_car[mk][sl] = 0;
        /* and what the first loop's end earns (st_bonus_car 1 the story clear bonus: Maximum Tune and 2's soundtracks, 2 the undefeated
         * bonus: Wangan Midnight R's, 4 the second: the SPECIAL meter), which a car that skips it would never have */
        m->st_bonus_car[mk][sl] = 1 | 2 | 4;
      }
    } else m->tune_full[mk][sl] = full, m->tune_power[mk][sl] = power;
  }
  if (m->all_parts && k >= 0) {
    /* ALL PARTS: every dress-up part won (aero sets, wings, bonnets, mirrors, wheels, neons,
     * stickers: level 63); the tune stays the setting's (STOCK the story's own, from none: it
     * gave a full tune with STOCK chosen) */
    m->dress_lv[mk][sl] = 63;
    m->garage_changed = 1;
  }
  m->all_parts = 0;
  m->new_card = 0;
  if (k < 0) return;
  m->own[k].color = (signed char)m->color;
  menu_own_load(m, k);
  m->garage_changed = 1;
}

/* ---- HAVE: clSeqCardHaveSelectThread ("Do you have a card?"): its spin box
 * (menu_layout_yn_card_1_gold, the time zone select's) with the YES card
 * (menu_p_yn_card_have_yes, 0x1720) in place of the morning one and NO (0x13c5) in place
 * of the night one; here YES goes to the save's cars and NO to a new car, captioned so
 * (menupack.py CARD_HAVE). The left/right keys or a tap on the arrows or the other card
 * turn it, a tap on the card at rest decides it. The port's KEEP is the same box after a new
 * car's card: YES keeps the car in the save, NO takes it out again. ---- */
#define CARD_TEXT 105000 /* menupack.py CARD_TEXT_BASE: */
enum {
  C_HAVE_TITLE, C_GARAGE_TITLE, C_GARAGE_HINT, C_CARD_HINT, C_PLAYER, C_TITLE0 = C_PLAYER + MENU_SAVES, C_HP, C_KM,
  C_KEEP_TITLE, C_HAVE_YES, C_HAVE_NO, C_KEEP_YES, C_KEEP_NO
};
#define CARD_GLYPH2 105020 /* font 2: 0..9, '-', ',' */
#define CARD_GLYPH5 105040 /* font 5: 0..9, '-' */
#define CARD_CAR 105100    /* each car's name, two lines */
#define CARD_COLOR 105200  /* the colour names, by cardb's text (100002 on) */
#define CARD_CLASS 107200  /* (menu_extra.bin's: menu.bin's at 105400 were run over by the colour names) the title row by class: "B7級　/湾岸の新人" */
#define CARD_CLASSP 107300 /* and without the title: "B7級　/" */
/* the titles' characters (src/titledb.h TITLEDB_CHAR) in fonts 1, 5 and 7: menupack.py
 * TITLE_GLYPH1_BASE, TITLE_GLYPH5_BASE, TITLE_GLYPH7_BASE */
#define TITLE_GLYPH1 110000
#define TITLE_GLYPH5 112000
#define TITLE_GLYPH7 114000
enum { T_CARD_MENU = JID(0x846), S_CARD_MENU_R35 = JID(0x11ed), S_CARD_TOPBAR = JID(0x1db2) };

static void have_enter(menu_state *m) {
  go(m, MENU_HAVE);
  m->music = 0;
  m->have_sel = 0;
  m->slide = m->card_t = 0;
  m->bars = 1;
  m->keep_ask = 0;
  menu_own_store(m);
  menu_own_sort(m);
  m->grid_sel = m->n_own ? 1 : 0; /* (the car last used first: the new car's tile is before it) */
}

static void keep_enter(menu_state *m) {
  have_enter(m);
  m->keep_ask = 1;
}

/* KEEP's NO: the new car out of the save again, as if never chosen */
static void keep_drop(menu_state *m) {
  int k = menu_own_find(m, m->maker, m->car, m->dup);
  if (k < 0) return;
  int sl = menu_own_slot(&m->own[k]);
  /* (its ALL PARTS on again for the card made next) */
  if (m->dress_lv[m->maker][sl] == 63) m->all_parts = 1;
  own_clear(m, m->maker, sl);
  for (int i = k; i + 1 < m->n_own; ++i) m->own[i] = m->own[i + 1];
  m->n_own--;
  if (m->own_sel == k) m->own_sel = -1;
  else if (m->own_sel > k) m->own_sel--;
  m->garage_changed = 1;
}

/* the tap on the card at rest (sprite 0), or near a sprite of the spin box (its arrows) */
static int have_on(const menu_state *m, int sprite) {
  float r[4];
  int f = 100 * m->have_sel + m->card_t % 60;
  if (sprite) {
    if (!anm_sprite_rect(ID.day_cards, f, anm_sprite(sprite), r)) return 0;
    return m->tap_x >= r[0] - 24.0f && m->tap_x < r[2] + 24.0f && m->tap_y >= r[1] - 24.0f && m->tap_y < r[3] + 24.0f;
  }
  if (!anm_sprite_rect(ID.day_cards, f, anm_sprite(m->have_sel ? T_NIGHT : T_MORNING), r)) return 0;
  float cx = (r[0] + r[2]) * 0.5f, cy = (r[1] + r[3]) * 0.5f, rad = (r[2] - r[0]) * 0.5f;
  return (m->tap_x - cx) * (m->tap_x - cx) + (m->tap_y - cy) * (m->tap_y - cy) < rad * rad;
}

/* one step of HAVE; returns 1 when it decides */
static int have_step(menu_state *m, const menu_in *in) {
  if (m->slide) {
    if (--m->slide == 0) {
      m->have_sel ^= 1;
      m->card_t = 0;
    }
    return 0;
  }
  m->card_t++;
  if (in->dx) { /* the wheel turns the spin box to the other card */
    snd_menu(SND_CURSOR);
    m->slide = SLIDE;
    return 0;
  }
  if (in->ok) {
    snd_menu(SND_SELECT);
    return 1;
  }
  return 0;
}

static void draw_have(const menu_state *m, int clock, float fade, int bars) {
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  int z = 100 * m->have_sel, f = z + m->card_t % 60;
  if (m->slide) f = z + 90 + SLIDE - m->slide;
  if (m->deciding) {
    int t = DECIDE_DAY + FADE - m->deciding;
    f = z + 60 + (t < DECIDE_DAY ? t : DECIDE_DAY - 1);
  }
  int yes = m->keep_ask ? C_KEEP_YES : C_HAVE_YES, no = m->keep_ask ? C_KEEP_NO : C_HAVE_NO;
  anm_swap cards[2] = { { anm_sprite(T_MORNING), anm_sprite(CARD_TEXT + yes) },
                        { anm_sprite(T_NIGHT), anm_sprite(CARD_TEXT + no) } };
  anm_swap rings[2] = { { anm_sprite(T_RING), anm_sprite(JID(0xb95)) }, { anm_sprite(T_RING_BLUE), anm_sprite(JID(0x1b0b)) } };
  anm_draw_ex(ID.day_cards, f, fade, 0, 0, cards, 2);
  if (!m->slide) anm_draw_ex(ID.day_ring, z + (m->card_t < 59 ? m->card_t : 59), fade, 0, 0, rings, 2);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(CARD_TEXT + (m->keep_ask ? C_KEEP_TITLE : C_HAVE_TITLE)) };
  draw_topbar(bars, fade, &title);
  draw_bottom(bars, fade);
}

/* the select's tap: a tile (its place), or a page tab (-2 - the page); -1 none */
static int grid_hit(const menu_state *m, float x, float y) {
  int n = grid_slots(m), page = m->grid_sel / 5, pages = (n + 4) / 5, p0 = grid_tab0(m);
  if (y >= GRID_TAB_Y - 4.0f && y < GRID_TAB_Y + GRID_TAB_H + 4.0f) {
    for (int t = 0; t < GRID_TABS && p0 + t < pages; ++t)
      if (x >= GRID_TAB_X + (GRID_TAB_W + 6.0f) * (float)t && x < GRID_TAB_X + (GRID_TAB_W + 6.0f) * (float)(t + 1)) return -2 - (p0 + t);
    return -1;
  }
  for (int i = 0; i < 5; ++i) {
    int k = page * 5 + i;
    if (k < n && x >= grid_x(i) && x < grid_x(i) + GRID_W && y >= GRID_Y && y < GRID_Y + GRID_H) return k;
  }
  return -1;
}

/* one step of the select; returns 1 when a car (or a new one) is chosen (have_sel 1 a new car) */
static int grid_step(menu_state *m, const menu_in *in) {
  int n = grid_slots(m);
  m->card_t++;
  if (in->dx || in->dy || in->ok || in->tab >= 0 || in->select >= 0) m->erase_armed = 0;
  if (m->grid_sel >= n) m->grid_sel = n - 1;
  if (in->tab >= 0) { /* a page tab: at its first car (the first page's, the first of its cars) */
    int to = in->tab ? in->tab * 5 : (n > 1 ? 1 : 0);
    if (to < n && to / 5 != m->grid_sel / 5) snd_menu(SND_CURSOR), m->grid_sel = to;
    return 0;
  }
  if (in->dx) { /* past a page's side: the next page's */
    int to = m->grid_sel + in->dx; /* (no wrap: the ends stop) */
    if (to >= 0 && to < n) snd_menu(SND_CURSOR), m->grid_sel = to;
    return 0;
  }
  if (in->dy) { /* the other page's same place */
    int to = m->grid_sel + 5 * in->dy;
    if (to >= 0 && to < n) snd_menu(SND_CURSOR), m->grid_sel = to;
    else if (to >= n && m->grid_sel / 5 < (n - 1) / 5) snd_menu(SND_CURSOR), m->grid_sel = n - 1;
    return 0;
  }
  if (in->select >= 0 && in->select < n && in->select != m->grid_sel) {
    snd_menu(SND_CURSOR);
    m->grid_sel = in->select;
    return 0;
  }
  if (!in->ok) return 0;
  if (m->grid_sel == 0 && m->n_own >= MENU_OWN_MAX) {
    snd_menu(SND_CANCEL);
    return 0;
  }
  snd_menu(SND_SELECT);
  m->have_sel = m->grid_sel == 0;
  return 1;
}

/* a class's name (N, C9 .. C1, B9 .. A1, S9 .. S1, SS9 .. SS1, SSS: the card's rows, menupack.py CLASS_NAMES) */
static void grid_class_name(int k, char *buf, size_t n) {
  if (k <= 0) snprintf(buf, n, "N");
  else if (k <= 36) snprintf(buf, n, "%c%d", "CBAS"[(k - 1) / 9], 9 - (k - 1) % 9);
  else if (k <= 45) snprintf(buf, n, "SS%d", 9 - (k - 37));
  else snprintf(buf, n, "SSS");
}

/* a sprite fitted into w x h at (x, y), its proportions kept, centred in it */
static void grid_fit(int sprite, float x, float y, float w, float h, float zmax, float alpha) {
  if (sprite < 0) return;
  float sw = anm_sprite_width(sprite), sh = anm_sprite_height(sprite), z = w / sw < h / sh ? w / sw : h / sh;
  if (z > zmax) z = zmax;
  const float src[4] = { 0, 0, sw, sh }, dst[4] = { x + (w - sw * z) * 0.5f, y + (h - sh * z) * 0.5f, x + (w + sw * z) * 0.5f, y + (h + sh * z) * 0.5f };
  anm_draw_part(sprite, src, dst, alpha);
}

/* The arrows of every selectable menu (menu_layout_carsel_car_arrow: the double chevrons and their pulse, half transparent): the
 * animation's own records, those on its left side mirrored, moved so each side's group sits on the cursor tile's edge. */
/* the pulsing arrows' colour (the transmission select's AT has blue ones): a multiplier and an addition, as the layout's records */
static float arrow_mul[3] = { 1, 1, 1 }, arrow_add[3] = { 0, 0, 0 };
static void arrows_pair(int clock, float xl, float xr, float cy, float fade, int show_l, int show_r) {
  int ids[4], cnt = anm_sprites(ID.car_arrow, ids, 4);
  float lx1 = -1e9f, rx0 = 1e9f; /* (the left group's right edge, the right group's left edge, frame 0) */
  for (int q = 0; q < cnt; ++q)
    for (int kk = 0; kk < 8; ++kk) {
      float r[4], al;
      if (!anm_sprite_rect_at(ID.car_arrow, 0, ids[q], kk, r, &al)) break;
      if (r[0] + r[2] < 640.0f) lx1 = r[2] > lx1 ? r[2] : lx1;
      else rx0 = r[0] < rx0 ? r[0] : rx0;
    }
  if (lx1 < -1e8f || rx0 > 1e8f) return;
  float dl = xl - lx1, dr = xr - rx0, dy = cy - 304.0f;
  for (int q = cnt - 1; q >= 0; --q) /* (the glow under the chevrons) */
    for (int kk = 0; kk < 8; ++kk) {
      float r[4], al;
      if (!anm_sprite_rect_at(ID.car_arrow, clock % 60, ids[q], kk, r, &al)) break;
      int left = r[0] + r[2] < 640.0f;
      if (left ? !show_l : !show_r) continue; /* (no arrow toward an end the cursor is at) */
      float w = anm_sprite_width(ids[q]), h = anm_sprite_height(ids[q]), d = left ? dl : dr;
      const float src[4] = { 0, 0, w, h }, dst[4] = { left ? r[2] + d : r[0] + d, r[1] + dy, left ? r[0] + d : r[2] + d, r[3] + dy };
      const float mu[4] = { arrow_mul[0], arrow_mul[1], arrow_mul[2], al * fade }, ad[4] = { arrow_add[0], arrow_add[1], arrow_add[2], 0 };
      anm_draw_part_col(ids[q], src, dst, mu, ad);
    }
}

static void grid_arrows(int clock, float tile_x, float fade, int show_l, int show_r) {
  arrows_pair(clock, tile_x + 14.0f, tile_x + GRID_W - 14.0f, GRID_Y + GRID_H * 0.5f, fade, show_l, show_r);
}

static void draw_grid(const menu_state *m, int clock, float fade, int bars, void (*draw3d)(void *), void *ctx, float screen_w, float screen_h) {
  const float k = 1.6f;
  int n = grid_slots(m), page = m->grid_sel / 5, pages = (n + 4) / 5, p0 = grid_tab0(m);
  draw_back(ID.bg, clock % 600, fade); /* (the card screen's: the silhouettes going by) */
  /* the tiles and the page tabs, the cars over them */
  hud_ui_begin(screen_w, screen_h);
  for (int i = 0; i < 5; ++i) {
    int j = page * 5 + i;
    float x = grid_x(i);
    if (j < n) {
      hud_ui_button(x * k, GRID_Y * k, GRID_W * k, GRID_H * k, 0);
      if (j == 0) hud_ui_rrect(x * k, GRID_Y * k, GRID_W * k, GRID_H * k, 12.0f, 0.0f, 0.0f, 0.0f, 0.38f); /* (the new car card darker) */
    } else {
      hud_ui_rrect(x * k, GRID_Y * k, GRID_W * k, GRID_H * k, 12.0f, 0.7f, 0.7f, 0.7f, 0.35f);
    }
    if (j == m->grid_sel) { /* (the cursor: a rounded outline, pulsing as the brand select's) */
      float e = 3.0f, pulse = 0.65f + 0.35f * cosf((float)clock * (6.2832f / 59.0f));
      hud_ui_srrect((x - e * 0.5f) * k, (GRID_Y - e * 0.5f) * k, (GRID_W + e) * k, (GRID_H + e) * k, 15.0f, e * k, 0.88f, 0.08f, 0.10f, pulse);
    }
  }
  for (int t = 0; t < GRID_TABS && p0 + t < pages; ++t)
    hud_ui_button((GRID_TAB_X + (GRID_TAB_W + 6.0f) * (float)t) * k, GRID_TAB_Y * k, GRID_TAB_W * k, GRID_TAB_H * k, p0 + t == page);
  hud_ui_end();
  if (draw3d) draw3d(ctx);
  hud_ui_begin(screen_w, screen_h);
  for (int t = 0; t < GRID_TABS && p0 + t < pages; ++t) {
    int p = p0 + t, from = p ? p * 5 : 1, to = p * 5 + 4 < n - 1 ? p * 5 + 4 : n - 1; /* (the cars' numbers on it) */
    char tab[16];
    if (to < from) snprintf(tab, sizeof tab, "NEW");
    else snprintf(tab, sizeof tab, "%d-%d", from, to);
    float tx = GRID_TAB_X + (GRID_TAB_W + 6.0f) * (float)t + GRID_TAB_W * 0.5f;
    hud_ui_text(tab, tx * k - 0.5f * hud_ui_text_width(tab, 2.5f), (GRID_TAB_Y + GRID_TAB_H * 0.5f) * k - 3.5f * 2.5f, 2.5f, 1, 1, 1, 1);
  }
  for (int i = 0; i < 5; ++i) {
    int j = page * 5 + i;
    float x = grid_x(i);
    if (j >= n) continue;
    if (j == 0) { /* a new car's tile */
      continue;
    }
    hud_ui_rrect((x + 6.0f) * k, (GRID_Y + 179.0f) * k, (GRID_W - 12.0f) * k, 30.0f * k, 10.0f, 0.91f, 0.91f, 0.91f, 1.0f); /* (behind the model badge) */
    {
      /* its class on a pill (grey at every class) */
      const menu_own *po = &m->own[j - 1];
      int cls = m->class_car[po->maker][menu_own_slot(po)];
      char cn[8];
      grid_class_name(cls, cn, sizeof cn);
      float pr = 0.62f, pg = 0.62f, pb = 0.62f; /* (gray at every class, as the arcade's pill stays) */
      float pw = 48.0f, ph = 17.0f, px = x + (GRID_W - pw) * 0.5f, py = GRID_Y + 119.0f;
      hud_ui_rrect(px * k, py * k, pw * k, ph * k, 9.0f, pr, pg, pb, 1.0f);
      hud_ui_text(cn, (px + pw * 0.5f) * k - 0.5f * hud_ui_text_width(cn, 2.2f), (py + ph * 0.5f) * k - 3.5f * 2.2f, 2.2f, 1, 1, 1, 1);
    }
    char num[8];
    snprintf(num, sizeof num, "%d", j);
    hud_ui_text(num, (x + 8.0f) * k, (GRID_Y + 6.0f) * k, 2.5f, 1, 1, 1, 1);
  }
  hud_ui_end();
  for (int i = 0; i < 5; ++i) {
    int j = page * 5 + i;
    float x = grid_x(i);
    if (j < 1 || j >= n) continue;
    const menu_own *o = &m->own[j - 1];
    const cardb_car *c = &CARDB_CARS[o->maker][o->car];
    int sl = menu_own_slot(o);
    /* its name (centred, the fullwidth glyphs a little left), its level badge and title, its model badge, the horsepower of its tune */
    int shown = 0;
    while (shown < 5 && o->name[shown] >= 0) shown++;
    for (int g = 0; g < shown; ++g) {
      int sp = anm_sprite(NAME_GLYPH8 + o->name[g]);
      if (sp >= 0) anm_draw_sprite(sp, x + (GRID_W - 22.0f * (float)shown) * 0.5f - 5.5f + 22.0f * (float)g, GRID_Y + 86.0f, 1);
    }
    int title = m->title_car[o->maker][sl] > 0 ? m->title_car[o->maker][sl] : TITLE_FIRST;
    const unsigned short *g;
    int ng = title_glyphs(title, &g);
    if (ng > 0 && anm_sprite(TITLE_GLYPH5 + g[0]) >= 0) {
      /* the title's lines (its "/" breaks them: a two-line title fills the space under the first), each centred, one zoom for all */
      int ls[3] = { 0 }, le[3] = { 0 }, nl = 1;
      for (int a = 0; a < ng && nl < 3; ++a)
        if (title_glyph_slash(g[a])) le[nl - 1] = a, ls[nl++] = a + 1;
      le[nl - 1] = ng;
      float lw[3], wmax = 0, z = 0.5f;
      for (int l = 0; l < nl; ++l) {
        lw[l] = 0;
        for (int a = ls[l]; a < le[l]; ++a) lw[l] += title_glyph_half(g[a]) ? 16.0f : 28.0f;
        if (lw[l] > wmax) wmax = lw[l];
      }
      if (wmax * z > GRID_W - 8.0f) z = (GRID_W - 8.0f) / wmax;
      for (int l = 0; l < nl; ++l) {
        float tx = x + (GRID_W - lw[l] * z) * 0.5f, ty = GRID_Y + 139.0f + 17.0f * (float)l;
        for (int a = ls[l]; a < le[l]; ++a) {
          int sp = anm_sprite(TITLE_GLYPH5 + g[a]);
          float w = anm_sprite_width(sp), h = anm_sprite_height(sp);
          const float src[4] = { 0, 0, w, h }, dst[4] = { tx, ty, tx + w * z, ty + h * z };
          /* (font 5 in white, font 4: the same glyphs with the colour inverted: c * -1 + 1) */
          const float inv[4] = { -1, -1, -1, 1 }, one[4] = { 1, 1, 1, 0 };
          anm_draw_part_col(sp, src, dst, inv, one);
          tx += (title_glyph_half(g[a]) ? 16.0f : 28.0f) * z;
        }
      }
    }
    grid_fit(anm_sprite(c->logo), x + 6.0f, GRID_Y + 180.0f, GRID_W - 12.0f, 28.0f, 3.0f, 1);
    int power = m->tune_full[o->maker][sl] ? m->tune_power[o->maker][sl] : m->st_power[o->maker][sl];
    int hp = car_tune_hp(c->hp, power);
    float hx = x + (GRID_W - (64.0f + anm_sprite_width(anm_sprite(T_SET_HP)))) * 0.5f - 1.0f; /* (the digits and HP centred: equal gaps to the edges) */
    for (int d = 0, div = 100; d < 3; ++d, div /= 10)
      anm_draw_sprite(anm_sprite(T_SET_DIGIT + hp / div % 10), hx + 20.0f * (float)d, GRID_Y + 199.0f, 1);
    anm_draw_sprite(anm_sprite(T_SET_HP), hx + 64.0f, GRID_Y + 215.0f, 1);
  }
  if (page == 0) {
    /* the new car's card: "Register / a / new car" in the game's font 7 (menupack.py REGISTER_TEXT_BASE), centred, narrow enough for the card */
    float wmax = 0, z = 1.0f;
    for (int ln = 0; ln < 3; ++ln) {
      float w = anm_sprite_width(anm_sprite(REGISTER_TEXT + ln));
      if (w > wmax) wmax = w;
    }
    if (wmax > GRID_W - 2.0f) z = (GRID_W - 2.0f) / wmax; /* (the sprites carry a cell of room on the right: the text sits left in them) */
    for (int ln = 0; ln < 3; ++ln) {
      int sp = anm_sprite(REGISTER_TEXT + ln);
      if (sp < 0) continue;
      float w = anm_sprite_width(sp) * z, h = anm_sprite_height(sp) * z, x = grid_x(0) + (GRID_W - w) * 0.5f + 12.0f * z, y = GRID_Y + 90.0f + 30.0f * z * (float)ln;
      const float src[4] = { 0, 0, anm_sprite_width(sp), anm_sprite_height(sp) }, dst[4] = { x, y, x + w, y + h };
      anm_draw_part(sp, src, dst, fade);
    }
  }
  grid_arrows(clock, grid_x(m->grid_sel % 5), fade, m->grid_sel > 0, m->grid_sel < n - 1); /* (over the tiles' text) */
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(CARD_TEXT + C_GARAGE_TITLE) };
  draw_topbar(bars, fade, &title);
  draw_bottom(bars, fade);
}

/* tools/extrapack.py's EX_DISC_BASE and EX_TITLE_BASE (menu_extra.bin: the selectors' discs and top bar titles) */
#define EX_DISC 110000
#define EX_TITLE 110100

/* four discs, centred (layout px): the one on large, the others small, room beside the one on for the arrows; the centre of disc i with disc `sel` on */
#define D4_BIG 200.0f
#define D4_SMALL 88.0f
static float disc4_cx(int sel, int i) {
  float tot = 0.0f, x;
  for (int k = 0; k < 4; ++k) tot += (k == sel ? D4_BIG : D4_SMALL) + (k < 3 ? (k == sel || k + 1 == sel ? 44.0f : 12.0f) : 0.0f);
  x = 320.0f - 0.5f * tot;
  for (int k = 0; k < i; ++k) x += (k == sel ? D4_BIG : D4_SMALL) + (k == sel || k + 1 == sel ? 44.0f : 12.0f);
  return x + 0.5f * (i == sel ? D4_BIG : D4_SMALL);
}

/* the selector's discs (sel_menu): at the layout's places for this frame, a ring and a disc, lit for the one on, its text in the middle */
static void draw_selmenu(const menu_state *m, int clock, int frame, float fade, float screen_w, float screen_h) {
  static const int ICON[3] = { JID(2825), JID(2824), JID(2826) }, PIC[3] = { JID(3224), JID(3223), JID(3225) }, LINE[3] = { JID(2317), JID(2316), JID(2318) };
  static const char *const TEXT[9][4] = { { "", "", "", "" }, { "PLAY", "SETTINGS", "EXIT", "" }, { "GAME", "CONTROLS", "SOUND", "" }, { "STOCK", "FULL TUNED", "", "" }, { "AT", "MT", "", "" },
                                          { "Solo", "Versus\nbattle", "Car\nsettings", "" }, { "Transmission", "Customize", "Meter\n& BGM", "Delete\ncar" }, { "Keep", "Delete", "", "" },
                                          { "Dress-up\nparts", "Paint\ncolor", "", "" } };
  static const char *const DESC[9][4] = {
    { "", "", "", "" },
    { "Choose a car and race.", "Configure your controls, graphics, and sound.", "Close the game.", "" },
    { "Configure graphics and the game's options.", "Configure the controller, keyboard, and axis inputs.", "Configure the volume of cars, music and sound effects.", "" },
    { "Win races and tune up your car.\nRecommended for new players.", "Your car is fully upgraded, and all parts are unlocked.\nStory mode starts on Ch. 101.", "", "" },
    { "", "", "", "" }, /* (the arcade's Select transmission has no line) */
    { "Story mode, 10 out run, and time attack.", "Race other players over LAN or online.", "Change your car's transmission, parts, meter, and more.", "" },
    { "Choose between automatic and manual shifting.", "Change the look of this car.", "Choose the meter and music for this car.", "Remove this car from your garage." },
    { "Cancel the operation.", "Delete this car and its records.\nThis action cannot be undone.", "", "" },
    { "Exchange the visual parts on this car.", "Change the paint color of this car.", "", "" }
  };
  const float k = 1.6f;
  int n = SEL_TWO(m) ? 2 : m->sel_menu == SELMENU_CAR ? 4 : 3;
  (void)PIC, (void)LINE;
  /* the discs centred between the top bar (down to 135 points) and the description's band (its top: the lines it needs, as drawn below) */
  int nlines = 1;
  if (m->sel >= 0 && m->sel < n)
    for (const char *p = DESC[m->sel_menu][m->sel]; *p; ++p) nlines += *p == '\n';
  int has_desc = m->sel >= 0 && m->sel < n && DESC[m->sel_menu][m->sel][0];
  float band_top = has_desc ? 612.0f - (2.0f * 15.8f + 22.4f + (float)(nlines - 1) * 33.6f) : 640.0f /* (no band: down to the bottom bar) */, dy = 0.5f * (135.0f + band_top) / k - 190.6f;
  float sel_bottom = 0.0f, sel_cx = 0, sel_cy = 0, sel_d = 0; /* (the disc that is on; its lowest point in 1024 x 768 points) */
  /* the baked discs (menu_extra.bin) in the layout's own: the story's mode select's two for the tuning, the versus select's three for the others */
  static const int ICON3[3] = { JID(2825), JID(2824), JID(2826) };
  const int base = m->sel_menu == SELMENU_TUNE ? 6 : m->sel_menu == SELMENU_SETTINGS ? 3 : m->sel_menu == SELMENU_MODE ? 8 : m->sel_menu == SELMENU_CAR ? 11
                   : m->sel_menu == SELMENU_DEL ? 15 : m->sel_menu == SELMENU_CUSTOM ? 17 : 0, tune = SEL_TWO(m); /* (tools/extrapack.py's DISCS) */
  /* a disc's picture: baked in menu_extra.bin, and the arcade's own AT and MT for the transmission (menu_p_yn_mission_at, _mt) */
#define DISC_PIC(i) (m->sel_menu == SELMENU_TRANS ? anm_sprite((i) ? JID(0xd4a) : JID(0xd49)) : anm_sprite(EX_DISC + base + (i)))
  int baked = 1;
  for (int i = 0; i < n; ++i) baked &= DISC_PIC(i) >= 0;
  if (baked && (tune || n == 4)) {
    /* two discs, centred: the one on large, the other small; they change places over the move. The three-disc layout cannot show just
     * two, so each is put together from its pieces as the layout does (glow, disc, lit rim, picture: menu_layout_mode_sel_3's records). */
    const int GLOW = anm_sprite(JID(6378)), DISC = anm_sprite(JID(950)), RIM = anm_sprite(JID(3403));
    float rcx = 0, rcy = 0;
    int resting = 0;
    for (int i = 0; i < n; ++i) {
      const float BIG = n == 4 ? D4_BIG : 236.0f, SMALL = n == 4 ? D4_SMALL : 118.0f, X0 = 320.0f - 118.0f, X1 = 320.0f + 118.0f;
      float p = m->from >= 0 ? (float)m->t / MOVE_MODE : 1.0f, so = i == m->sel ? 1.0f : 0.0f;
      if (m->from >= 0) so = i == m->sel ? p : (i == m->from ? 1.0f - p : 0.0f);
      float d = SMALL + (BIG - SMALL) * so, box = d * (256.0f / 236.0f);
      float cx = i == 0 ? X0 - 20.0f * so + 10.0f : X1 + 20.0f * so - 10.0f, cy = 190.6f + dy;
      if (n == 4) {
        int f0 = m->from >= 0 ? m->from : m->sel;
        cx = disc4_cx(f0, i) + (disc4_cx(m->sel, i) - disc4_cx(f0, i)) * p;
      }
      const float dst[4] = { cx - box * 0.5f, cy - box * 0.5f, cx + box * 0.5f, cy + box * 0.5f }, src[4] = { 0, 0, 256, 256 };
      const float dim = 0.5f + 0.5f * so, mul[4] = { dim, dim, dim, 1 }, add[4] = { 0, 0, 0, 0 };
      /* the glow and the rim as the layout's idle frames have them (the rim turns, the glow breathes): the record's turn and size
       * about the disc's middle, its opacity */
      /* the transmission's discs wear their colours (the arcade's AT blue, MT red): the glow and the turning light of another sprite, a ring over the disc */
      static const int V_GLOW[2] = { JID(0x1dbe), JID(0x1cdd) }, V_RIM[2] = { JID(0x1a1e), JID(0xd4b) }, V_RING[2] = { JID(0xb96), JID(0x9ff) };
      const int trans = m->sel_menu == SELMENU_TRANS;
      for (int q = 0; q < 3; ++q) {
        const int pc = q == 0 ? GLOW : q == 1 ? DISC : RIM;
        int dp = pc; /* (the sprite drawn; the transform is the default one's) */
        if (trans && q == 0 && anm_sprite(V_GLOW[i]) >= 0) dp = anm_sprite(V_GLOW[i]);
        if (trans && q == 2 && anm_sprite(V_RIM[i]) >= 0) dp = anm_sprite(V_RIM[i]);
        float xf[6], mu[4], ad[4];
        if (pc < 0 || (q != 1 && so <= 0.0f)) continue;
        if (q == 1 || !anm_sprite_xf(ID.vs_sel, clock % 60, pc, xf, mu, ad)) {
          anm_draw_part(dp, src, dst, (q == 1 ? 1.0f : so) * fade);
          continue;
        }
        float sc = box / 256.0f, t[6] = { 0, 0, xf[2] * sc, xf[3] * sc, xf[4] * sc, xf[5] * sc };
        t[0] = cx - 128.0f * (t[2] + t[4]), t[1] = cy - 128.0f * (t[3] + t[5]);
        anm_draw_sprite_xf(dp, t, so * mu[3] * fade);
      }
      if (trans && anm_sprite(V_RING[i]) >= 0) anm_draw_part_col(anm_sprite(V_RING[i]), src, dst, mul, add);
      if (i == m->sel && so >= 1.0f) rcx = cx, rcy = cy, resting = 1;
      anm_draw_part_col(DISC_PIC(i), src, dst, mul, add);
    }
    /* the ring's pulse (the three-disc layout's, for its first disc at 138.6, 191.7), moved to the disc that is on */
    if (resting && m->from < 0 && !m->deciding && m->t < 60) anm_draw_ex(ID.vs_sel_ring, m->t, fade, rcx - 138.6f, rcy - 191.7f, NULL, 0);
  } else if (baked) {
    anm_swap sw[3];
    for (int i = 0; i < 3; ++i) sw[i] = (anm_swap){ anm_sprite(ICON3[i]), DISC_PIC(i) };
    anm_draw_ex(ID.vs_sel, frame, fade, 0, dy, sw, 3);
    if (m->from < 0 && !m->deciding && m->t < 60) anm_draw_ex(ID.vs_sel_ring, m->sel * 100 + m->t, fade, 0, dy, NULL, 0);
  }
  hud_ui_begin(screen_w, screen_h);
  for (int i = 0; i < n; ++i) {
    float r[4], cx, cy, d;
    int on = i == m->sel;
    if (SEL_TWO(m)) {
      /* two discs, centred: the one on large, the other small; they change places over the move */
      const float BIG = 236.0f, SMALL = 118.0f, X0 = 320.0f - 118.0f, X1 = 320.0f + 118.0f;
      float p = m->from >= 0 ? (float)m->t / MOVE_MODE : 1.0f, size_on = on ? 1.0f : 0.0f;
      if (m->from >= 0) size_on = on ? p : (i == m->from ? 1.0f - p : 0.0f);
      d = SMALL + (BIG - SMALL) * size_on;
      cx = i == 0 ? X0 - 20.0f * size_on + 10.0f : X1 + 20.0f * size_on - 10.0f;
      cy = 190.6f + dy;
    } else if (n == 4) {
      /* four discs (the game mode select's places): the one on large, the others small; they slide over the move */
      float p = m->from >= 0 ? (float)m->t / MOVE_MODE : 1.0f, size_on = on ? 1.0f : 0.0f;
      int f0 = m->from >= 0 ? m->from : m->sel;
      if (m->from >= 0) size_on = on ? p : (i == m->from ? 1.0f - p : 0.0f);
      cx = disc4_cx(f0, i) + (disc4_cx(m->sel, i) - disc4_cx(f0, i)) * p;
      d = D4_SMALL + (D4_BIG - D4_SMALL) * size_on;
      cy = 190.6f + dy;
    } else {
      if (!anm_sprite_rect(ID.vs_sel, frame, anm_sprite(ICON[i]), r)) continue;
      float w = r[2] - r[0], h = r[3] - r[1];
      d = w < h ? w : h, cx = (r[0] + r[2]) * 0.5f, cy = (r[1] + r[3]) * 0.5f + dy;
    }
    float a = fade;
    if (on) sel_bottom = (cy + d * 0.5f) * k, sel_cx = cx, sel_cy = cy, sel_d = d;
    if (baked) continue; /* (drawn above) */
    hud_ui_rrect((cx - d * 0.5f) * k, (cy - d * 0.5f) * k, d * k, d * k, d * 0.5f * k, on ? 1.0f : 0.78f, on ? 0.82f : 0.8f, on ? 0.3f : 0.82f, a);
    float in = d * 0.04f + 3.0f;
    hud_ui_rrect((cx - d * 0.5f + in) * k, (cy - d * 0.5f + in) * k, (d - 2 * in) * k, (d - 2 * in) * k, (d * 0.5f - in) * k, on ? 0.92f : 0.06f,
                 on ? 0.55f : 0.09f, on ? 0.08f : 0.16f, a);
    float px = d / 22.0f;
    /* the text, one line or two (split at \n), each centred and the whole within its disc */
    const char *t = TEXT[m->sel_menu][i];
    char ln[2][24];
    int nl = 0;
    for (const char *p = t; *p && nl < 2;) {
      int len = 0;
      while (p[len] && p[len] != '\n' && len < 23) ++len;
      memcpy(ln[nl], p, (size_t)len);
      ln[nl++][len] = 0;
      p += len + (p[len] == '\n');
    }
    float wmax = 0.0f;
    for (int l = 0; l < nl; ++l) wmax = hud_ui_text_width(ln[l], 1.0f) > wmax ? hud_ui_text_width(ln[l], 1.0f) : wmax;
    if (wmax * px > 0.78f * d * k) px = 0.78f * d * k / wmax; /* (the text within its disc) */
    for (int l = 0; l < nl; ++l) {
      float w = hud_ui_text_width(ln[l], px), y = cy * k - 3.5f * px + (nl == 2 ? (l ? 6.0f : -6.0f) * px : 0.0f);
      hud_ui_text(ln[l], cx * k - 0.5f * w, y, px, 1, 1, 1, a);
    }
  }
  /* what the one on does, where the mode select's line is (the band's left, 316 down) */
  if (m->sel >= 0 && m->sel < n && DESC[m->sel_menu][m->sel][0]) {
    float tx = 42.0f, px = 3.2f, cap = 7.0f * px, pitch = 1.5f * cap, pad = 15.8f; /* (1024 x 768 points; the capitals 7 * px tall) */
    const char *d = DESC[m->sel_menu][m->sel];
    char line[2][96];
    int nl = 0;
    for (const char *p = d; *p && nl < 2;) { /* (the lines, split at \n) */
      int len = 0;
      while (p[len] && p[len] != '\n' && len < 95) ++len;
      memcpy(line[nl], p, (size_t)len);
      line[nl++][len] = 0;
      p += len + (p[len] == '\n');
    }
    float wmax = 0, text_h = cap + (float)(nl - 1) * pitch, top = 0.5f * (sel_bottom + 640.0f) - 0.5f * text_h, h = text_h + 2 * pad; /* (the band centred between the disc's lowest point and the bottom bar, at 640) */
    for (int l = 0; l < nl; ++l) wmax = hud_ui_text_width(line[l], px) > wmax ? hud_ui_text_width(line[l], px) : wmax;
    /* the translucent black band behind it: the same room above and below the letters, its right end fading out from past the text */
    float solid = tx + wmax + 50.0f;
    hud_ui_rect(0, top - pad, solid, h, 0, 0, 0, 0.5f * fade);
    for (int q = 0; q < 18; ++q) hud_ui_rect(solid + 6.0f * (float)q, top - pad, 6.0f, h, 0, 0, 0, 0.5f * fade * (1.0f - (float)(q + 1) / 19.0f));
    for (int l = 0; l < nl; ++l) {
      float y = top + (float)l * pitch;
      hud_ui_text(line[l], tx + 2.0f, y + 2.0f, px, 0, 0, 0, 0.6f * fade);
      hud_ui_text(line[l], tx, y, px, 1, 1, 1, fade);
    }
  }
  hud_ui_end();
  /* the pulsing arrows of the selectable menus, either side of the disc that is on (none toward an end) */
  if (m->sel_menu == SELMENU_TRANS && m->sel == 0) { /* (the AT's blue arrows) */
    arrow_mul[0] = 0.25f, arrow_mul[1] = 0.5f, arrow_mul[2] = 1.0f, arrow_add[0] = 0.0f, arrow_add[1] = 0.12f, arrow_add[2] = 0.45f;
  }
  if (sel_d > 0.0f && (!baked || tune || n == 4)) arrows_pair(clock, sel_cx - sel_d * 0.5f + 14.0f, sel_cx + sel_d * 0.5f - 14.0f, sel_cy, fade, m->sel > 0, m->sel < n - 1);
  arrow_mul[0] = arrow_mul[1] = arrow_mul[2] = 1.0f, arrow_add[0] = arrow_add[1] = arrow_add[2] = 0.0f;
}

/* ---- GARAGE: the port's look at the save's cars, on the colour select's turntable in the
 * parts they wear, with the car select's name panel and colour plate; the left/right keys
 * (or a tap either side) go round them, a tap on the car takes it (then its card). ---- */
static void garage_show(menu_state *m, int k) {
  m->own_view = k;
  m->maker = m->own[k].maker, m->car = m->own[k].car, m->color = m->own[k].color, m->dup = m->own[k].dup;
}

static void garage_enter(menu_state *m, int k) {
  go(m, MENU_GARAGE);
  m->music = 0;
  garage_show(m, k >= 0 && k < m->n_own ? k : 0);
  m->spin = -24.0f;
  m->bars = 1;
  m->erase_armed = 0;
}

/* the port's: the car shown deleted from the save with its tune, parts and results (the
 * key, a bin in the gear key's place, pressed twice); the next one shown, none left: a new
 * car */
static void garage_erase(menu_state *m) {
  int k = m->own_view;
  if (k < 0 || k >= m->n_own) return;
  menu_own_store(m);
  own_clear(m, m->own[k].maker, menu_own_slot(&m->own[k]));
  for (int i = k; i + 1 < m->n_own; ++i) m->own[i] = m->own[i + 1];
  m->n_own--;
  if (m->own_sel == k) m->own_sel = -1;
  else if (m->own_sel > k) m->own_sel--;
  m->garage_changed = 1;
  m->erase_armed = 0;
  if (!m->n_own) {
    car_select_enter(m, MENU_MAKER);
    m->bars = 1;
    return;
  }
  garage_show(m, k < m->n_own ? k : m->n_own - 1);
  m->spin = -24.0f;
}

static int garage_step(menu_state *m, const menu_in *in) {
  m->spin -= 0.3f;
  int d = in->dx;
  if (d) {
    if (m->n_own < 2) return 0;
    snd_menu(SND_CURSOR);
    garage_show(m, (m->own_view + d + m->n_own) % m->n_own);
    m->spin = -24.0f;
    return 0;
  }
  if (!in->ok || m->loading) return 0;
  snd_menu(SND_SELECT);
  return 1;
}

#define GARAGE_HP_X 492.0f
#define GARAGE_HP_Y 112.0f

static void draw_garage(const menu_state *m, int clock, void (*draw3d)(void *), void *ctx, int bars) {
  const cardb_car *c = chosen_car(m, m->car);
  draw_back(ID.bg_carsel, clock % 240, 1);
  draw_bg_logo(c, clock % 240, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  if (draw3d) draw3d(ctx);
  anm_swap sw[3 + 12 + 1];
  int n = car_swaps(c, sw);
  anm_draw_ex(ID.car_info, clock % 240, 1, 0, 0, sw, n);
  if (m->n_own > 1) anm_draw(ID.car_arrow, clock % 60, 1);
  anm_swap none = { anm_sprite(T_COL_FONT), -1 };
  anm_draw_ex(ID.col_name, 0, 1, 0, 0, &none, 1);
  int name = anm_sprite(cur_color(m)->name);
  anm_draw_sprite(name, 624.0f - anm_sprite_width(name), 316.0f, 1);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(CARD_TEXT + C_GARAGE_TITLE) };
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
  if (bars) return;
  /* which of them: n/N */
  char num[16];
  snprintf(num, sizeof num, "%d/%d", m->own_view + 1, m->n_own);
  save_glyphs(num, 600.0f - SAVE_ADV * (float)strlen(num), 92.0f, 1);
  /* its horsepower (the setting's digits): the full tune's, else the story's steps */
  int mk = m->maker, cr = m->car, sl = menu_slot(m);
  int power = m->tune_full[mk][sl] ? m->tune_power[mk][sl] : m->st_power[mk][sl];
  int hp = car_tune_hp(c->hp, power);
  for (int i = 0, div = 100; i < 3; ++i, div /= 10)
    anm_draw_sprite(anm_sprite(T_SET_DIGIT + hp / div % 10), GARAGE_HP_X + 22.0f * (float)i, GARAGE_HP_Y, 1);
  anm_draw_sprite(anm_sprite(T_SET_HP), GARAGE_HP_X + 70.0f, GARAGE_HP_Y + 20.0f, 1);
  int hint = anm_sprite(m->erase_armed ? SAVE_TEXT + S_ERASE_HINT : CARD_TEXT + C_GARAGE_HINT);
  if (bar_keys()) anm_draw_sprite(hint, 320.0f - anm_sprite_width(hint) * 0.5f, 427.0f, 0.75f + 0.25f * cosf((float)clock * 0.1f));
}

/* ---- CARD: clSeqCardDisp1, the card's first page ("Card data confirmed (1/9)", the top bar
 * 0x1db2): menu_g_cardcheck_1_waku (Machine Registration; the R35's menu 0x11ed in place of
 * 0x846) over menu_g_bg_image, menu_layout_cardcheck_1's fields in its fonts (datrums 11):
 * font 5 the name (0x7ff), the card's number (0x58b, card+0x11c % 10000 as "%02d-%02d"; here
 * the save's and the car's) and the title (0x800); font 2 the car's name in two lines
 * (0x7da, 0x7db: get_car_model_data +0, +4), the colour (0x7dc, after its swatch in place of
 * 0x1de2), the horsepower (0x7dd), the mileage (0x7de, getOdoCount, "%d km" with commas);
 * the mileage password and the card's plays left (0x7df, 0x7e0) are the arcade card's own.
 * A tap goes on to the car setting. ---- */
enum {
  T_CARD_NO = JID(0x58b), T_CARD_NAME = JID(0x7ff), T_CARD_TITLE = JID(0x800), T_CARD_CAR1 = JID(0x7da), T_CARD_CAR2 = JID(0x7db),
  T_CARD_COLOR = JID(0x7dc), T_CARD_HP = JID(0x7dd), T_CARD_ODO = JID(0x7de), T_CARD_PASS = JID(0x7df), T_CARD_LIFE = JID(0x7e0),
  T_CARD_SWATCH = JID(0x1de2)
};

static void card_enter(menu_state *m, int back) {
  go(m, MENU_CARD);
  m->card_page = 0;
  m->music = 0; /* (the car's entry music again: the select music is the mode select's on) */
  m->card_back = back;
  m->bars = 1;
  m->spin = -24.0f;
}

/* ---- the player's name (seqCardNameSelect, clSeqCardNameSelectThread), asked for as soon
 * as a NEW GAME starts, as the arcade asks a new card's:
 * - menu_g_name_name_back behind the five big characters (font 10, at 89 + 86i, 96), each over
 *   its menu_use_name line (138 + 86i, 196; the next one's menu_g_name_line blinking);
 * - menu_layout_name_select's frame and in it the page's grid (font 8, namedb.h's columns
 *   and rows); under it the page tabs and END (menu_g_name_*, at 285 + 68k, 362; the digits'
 *   tab is the target search's);
 * - the wheel moves the arcade's cursor and the gas pedal puts its character in, which pops
 *   (1 + 0.1n, n 5..0): here a tap on a character puts it in, on a tab changes the page (as do
 *   the left/right keys), on END takes the name (not an empty one: the arcade's
 *   menu_g_name_no_name). ← and the back key take the last character off, the back key on an
 *   empty name goes back to the saves; ゛ and ゜ voice the character before. ---- */
#define NAME_POP 5
/* the cursor ring's centre on a grid cell (from its glyph's top left): the glyph's ink centre, measured off a capture (it sat a little left and low) */
#define NAME_RING_DX 16.0f
#define NAME_RING_DY 15.0f
#define NAME_X 89.0f
#define NAME_DX 86.0f
#define NAME_Y 96.0f
/* the tabs sit on the bottom bar between its keys (the arcade's 285 + 68k, 362
 * is the frame's last row, under the grid's) */
#define NAME_TAB_X 183.0f
#define NAME_TAB_DX 68.0f
#define NAME_TAB_Y 361.0f /* (the keyboard's bottom row) */
#define NAME_ROW_Y(r) (249.0f + 32.0f * (float)(r)) /* the characters' rows: between the panel's dividers, not on them */
#define S_NAME_TOPBAR JID(6351) /* menu_p_topbar_mes_name_1 */
#define S_NAME_RING JID(5587)   /* menu_p_name_select_ring */

static int name_len(const menu_state *m) {
  int n = 0;
  while (n < 5 && m->name[n] >= 0) n++;
  return n;
}

static void name_enter(menu_state *m) {
  go(m, MENU_NAME);
  memset(m->name, -1, sizeof m->name);
#if WMMT_LANG_EN
  m->name_page = 2; /* the English build: the upper case letters first (the tabs: katakana, hiragana, upper case, lower case) */
#else
  m->name_page = 0;
#endif
  m->name_key = -1;
  m->name_pop = m->name_char_pop = 0;
  m->name_x = m->name_y = m->name_ctl = m->focus = 0;
  m->name_ctl = !bar_keys(); /* (the cursor on the first character from the start, unless the touch screen is the input) */
  m->bars = 1;
}

/* ---- the main menu, the settings and a new car's start: the versus select's three discs (menu_layout_mode_sel_3) with the port's own
 * choices on them (MENU_VS_SEL, sel_menu): PLAY, SETTINGS and EXIT; GAME, CONTROLS and SOUND; STOCK and FULL TUNED. The discs are drawn
 * here, with their text, until the art for them is baked into menu.bin. ---- */
static void selmenu_enter(menu_state *m, int kind, int sel) {
  m->sel_menu = kind;
  go(m, MENU_VS_SEL);
  m->sel_age = 0;
  if (kind == SELMENU_TRANS) m->spin = -24.0f;
  m->sel = sel;
  m->bars = 1;
}

/* after the car's card: SOLO, VERSUS / BATTLE and CAR / SETTINGS */
static void mode_menu_enter(menu_state *m, int sel) {
  selmenu_enter(m, SELMENU_MODE, sel);
  m->music = 1;
}

/* the first screen after START (and where the car select's back goes) */
static void main_menu_enter(menu_state *m) {
  if (getenv("MT3DX_GOTO")) { /* test: straight to this screen (36: the versus, LAN) */
    m->vs.kind = MENU_VS_LAN;
    go(m, atoi(getenv("MT3DX_GOTO")));
    m->bars = 1;
    return;
  }
  selmenu_enter(m, SELMENU_MAIN, 0);
}

/* a new kind of car, its colour chosen: STOCK or FULL TUNED registers it (all parts and the full tune), then its name, its card, on */
static void new_car_choose(menu_state *m, int full) {
  m->new_card = 1;
  m->tune_locked = 0;
  new_full = (signed char)(full != 0); /* (kept for the new card: another card of the model may be on the slot the car shown has) */
  new_power = (signed char)(full ? CAR_TUNE_TOTAL / 2 : CAR_TUNE_MAX); /* (the full tune balanced, B; the setting before a race moves it) */
  m->all_parts = 1; /* (the parts and special colours are all open on a stock car too) */
  short typed[5];
  memcpy(typed, m->new_named ? m->new_name : m->name, sizeof typed); /* (the name asked before) */
  m->new_named = 0;
  if (m->own_sel >= 0 && m->own_sel < m->n_own) memcpy(m->name, m->own[m->own_sel].name, sizeof typed); /* (registering stores the car driven so far with its own name) */
  own_register(m);
  m->new_card = 0;
  memcpy(m->name, typed, sizeof typed);
  menu_own_store(m);
  m->garage_changed = 1;
  m->card_color_was = -1;
  card_enter(m, MENU_HAVE); /* (its card, whose KEEP question is next) */
}

static void selmenu_decide(menu_state *m) {
  switch (m->sel_menu) {
  case SELMENU_MAIN:
    if (m->sel == 0) {
      m->sel_menu = SELMENU_VS;
      if (m->n_own) have_enter(m);
      else car_select_enter(m, MENU_MAKER), m->bars = 1;
    } else if (m->sel == 1) {
      selmenu_enter(m, SELMENU_SETTINGS, 0);
    } else {
      m->exit_req = 1;
      selmenu_enter(m, SELMENU_MAIN, 2);
    }
    break;
  case SELMENU_SETTINGS:
    m->opt_from = MENU_VS_SEL;
    if (m->sel == 0) {
      option_enter(m);
    } else if (m->sel == 1) {
      bindings_enter(m);
      m->settings_direct = 1;
    } else {
      volume_enter(m);
      m->settings_direct = 1;
    }
    break;
  case SELMENU_TUNE: /* (then the transmission: the arcade's Select transmission, before the card) */
    m->trans_full = m->sel == 1;
    selmenu_enter(m, SELMENU_TRANS, m->manual ? 1 : 0);
    break;
  case SELMENU_TRANS:
    m->manual = m->sel == 1;
    m->garage_changed = 1;
    if (m->trans_edit) { /* (a car already made: back to the Car / Settings discs) */
      m->trans_edit = 0;
      selmenu_enter(m, SELMENU_CAR, 0);
      break;
    }
    new_car_choose(m, m->trans_full);
    break;
  case SELMENU_MODE: /* after the car's card */
    if (m->sel == 0) {
      car_select_enter(m, MENU_GAME);
    } else if (m->sel == 1) {
      m->game = MENU_GAME_VS;
      m->sel_menu = SELMENU_VS;
      car_select_enter(m, MENU_VS_SEL);
    } else {
      selmenu_enter(m, SELMENU_CAR, 0);
    }
    break;
  case SELMENU_CAR:
    m->carmenu = 0;
    if (m->sel == 0) {
      m->trans_edit = 1;
      selmenu_enter(m, SELMENU_TRANS, m->manual ? 1 : 0);
    } else if (m->sel == 1) {
      if (tg_no_bodyparts(m)) { /* the R35 has no special colours: CUSTOMIZE goes straight to its dress-up (wheels, neon) */
        m->carmenu = 1;
        m->dress_free = 1;
        m->dress_kind = -1;
        m->tg.maker = 3;
        tg_select_open(m);
      } else {
        selmenu_enter(m, SELMENU_CUSTOM, 0);
      }
    } else if (m->sel == 2) {
      m->carmenu = 1;
      cardset_open(m);
    } else {
      selmenu_enter(m, SELMENU_DEL, 0);
    }
    break;
  case SELMENU_CUSTOM:
    if (m->sel == 0) { /* the card's dress-up, from the part last changed */
      m->carmenu = 1;
      m->dress_free = 1;
      m->dress_kind = -1;
      m->tg.maker = 3;
      tg_select_open(m);
    } else { /* the colour select of the paints the car has won (the arcade has no other stock colour to change to) */
      m->carmenu = 2;
      ccolor_open(m);
      if (m->screen != MENU_COLOR) m->carmenu = 0; /* (none won yet: it only sounds, the discs stay) */
    }
    break;
  case SELMENU_DEL:
    if (m->sel == 0) {
      selmenu_enter(m, SELMENU_CAR, 3);
    } else {
      int k = menu_own_find(m, m->maker, m->car, m->dup);
      if (k < 0) {
        selmenu_enter(m, SELMENU_CAR, 3);
        break;
      }
      m->own_view = k;
      garage_erase(m); /* (with its tune, parts and results; none left: a new car) */
      if (m->n_own) have_enter(m); /* (the cars that are left: which to drive) */
    }
    break;
  }
}

int menu_exit_requested(const menu_state *m) { return m->exit_req; }

static void name_erase(menu_state *m) {
  int n = name_len(m);
  if (!n) return;
  snd_menu(SND_CANCEL);
  m->name[n - 1] = -1;
  m->name_char_pop = 0;
}

/* OK (the gas) on the name screen, on what the cursor is on: a tab (the page, or END: returns 1 when the name is taken) or a
 * character of the grid */
static int name_press(menu_state *m) {
  int n = name_len(m);
  if (m->name_y == 3) {
    int k = m->focus;
    if (k == 4) {
      if (!n) {
        snd_menu(SND_CANCEL);
        return 0;
      }
      snd_menu(SND_SELECT);
      m->garage_changed = 1;
      return 1;
    }
    if (k != m->name_page) {
      snd_menu(SND_CURSOR);
      m->name_page = k;
    }
    return 0;
  }
  int key = m->name_y * 20 + m->name_x, v = NAMEDB_GRID[m->name_page][key];
  if (v == NAMEDB_BS) {
    if (n) name_erase(m);
    else snd_menu(SND_CANCEL);
  } else if (v == NAMEDB_DAKU || v == NAMEDB_HANDAKU) {
    int to = n ? NAMEDB_VOICED[m->name[n - 1]][v == NAMEDB_HANDAKU] : -1;
    if (to < 0) {
      snd_menu(SND_CANCEL);
      return 0;
    }
    snd_menu(SND_CURSOR);
    m->name[n - 1] = (short)to;
    m->name_char_pop = NAME_POP;
  } else if (v >= 0) {
    if (n == 5) {
      snd_menu(SND_CANCEL);
      return 0;
    }
    snd_menu(SND_CURSOR);
    m->name[n] = (short)v;
    m->name_char_pop = NAME_POP;
  } else {
    return 0;
  }
  m->name_key = key;
  m->name_pop = NAME_POP;
  return 0;
}

/* the cursor off an empty cell of the page: to the next character to its right */
static void name_fix(menu_state *m) {
  for (int s = 0; s < 20; ++s) {
    int c = (m->name_x + s) % 20;
    if (NAMEDB_GRID[m->name_page][m->name_y * 20 + c] != -1) {
      m->name_x = c;
      return;
    }
  }
}

/* The controls on the name screen: the cursor moves over the grid (left, right, up, down; the empty cells are skipped) and
 * under it over the page tabs and END (a fourth row); OK, the gas, puts the character or takes the tab (step); the view button
 * turns the page. The back key erases. */
static void name_pad(menu_state *m, const menu_in *in) {
  m->name_ctl = 1;
  if (in->view) {
    snd_menu(SND_CURSOR);
    m->name_page = (m->name_page + 1) % 4;
    if (m->name_y < 3) name_fix(m);
    else m->focus = m->name_page;
    return;
  }
  if (in->dy) {
    int y = m->name_y + in->dy;
    if (y >= 0 && y <= 3) {
      snd_menu(SND_CURSOR);
      m->name_y = y;
      if (y == 3) m->focus = m->name_page;
      else name_fix(m);
    }
    return;
  }
  int d = in->dx;
  if (m->name_y == 3) {
    int k = m->focus + d;
    if (k >= 0 && k <= 4) snd_menu(SND_CURSOR), m->focus = k;
    return;
  }
  for (int s = 1; s <= 20; ++s) {
    int c = ((m->name_x + d * s) % 20 + 20) % 20;
    if (NAMEDB_GRID[m->name_page][m->name_y * 20 + c] != -1) {
      snd_menu(SND_CURSOR);
      m->name_x = c;
      return;
    }
  }
}

/* a glyph sprite (square) from (x, y), scaled about its middle */
static void name_glyph(int sprite, float x, float y, float size, float scale, float a) {
  if (sprite < 0) return;
  const float src[4] = { 0, 0, size, size };
  float c = size * 0.5f, h = c * scale;
  const float dst[4] = { x + c - h, y + c - h, x + c + h, y + c + h };
  anm_draw_part(sprite, src, dst, a);
}

static void draw_name(const menu_state *m, int clock, int bars) {
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  anm_draw(ID.name_back, 0, 1);
  int n = name_len(m);
  for (int i = 0; i < 5; ++i) {
    float x = NAME_X + NAME_DX * (float)i;
    anm_draw_ex(ID.name_use, i < n ? 0 : 1, 1, x + 49.0f, 196.0f, NULL, 0);
    if (i == n) anm_draw_ex(ID.name_line, clock % anm_frames(ID.name_line), 1, x + 49.0f, 196.0f, NULL, 0);
    if (i < n) {
      float pop = i == n - 1 ? 1.0f + 0.1f * (float)m->name_char_pop : 1.0f;
      name_glyph(anm_sprite(NAME_GLYPH10 + m->name[i]), x, NAME_Y, 100.0f, pop, 1);
    }
  }
  anm_draw(ID.name_select, 0, 1);
  for (int key = 0; key < 60; ++key) {
    int v = NAMEDB_GRID[m->name_page][key];
    int g = v == NAMEDB_BS ? NAMEDB_N : v == NAMEDB_DAKU ? NAMEDB_DAKU_MARK : v == NAMEDB_HANDAKU ? NAMEDB_HANDAKU_MARK
          : v >= 0 && NAMEDB_WCHAR[v] == 0x3000 ? NAMEDB_N + 1 : v;
    if (g < 0) continue;
    float x = (float)NAMEDB_GRID_X[key % 20], y = NAME_ROW_Y(key / 20);
    int hit = key == m->name_key && m->name_pop;
    if (hit) {
      float ring = anm_sprite_width(anm_sprite(S_NAME_RING));
      name_glyph(anm_sprite(S_NAME_RING), x + NAME_RING_DX - ring * 0.5f, y + NAME_RING_DY - ring * 0.5f, ring, 1.0f,
                 (float)m->name_pop / NAME_POP);
    }
    name_glyph(anm_sprite(NAME_GLYPH8 + g), x, y, 32.0f, hit ? 1.0f + 0.1f * (float)m->name_pop : 1.0f, 1);
  }
  if (m->name_ctl && !bars) { /* the cursor of the controls: the arcade's ring, on a character or on a tab */
    int ring = anm_sprite(S_NAME_RING);
    float size = anm_sprite_width(ring), a = 0.65f + 0.35f * cosf((float)clock * 0.2f);
    if (m->name_y < 3) {
      float x = (float)NAMEDB_GRID_X[m->name_x % 20], y = NAME_ROW_Y(m->name_y);
      name_glyph(ring, x + NAME_RING_DX - size * 0.5f, y + NAME_RING_DY - size * 0.5f, size, 0.65f, a); /* (tight round the letter) */
    } else {
      float x = NAME_TAB_X + NAME_TAB_DX * (float)m->focus, y = NAME_TAB_Y;
      name_glyph(ring, x - size * 0.5f, y - size * 0.5f, size, 1.4f, a); /* (centred on the button) */
    }
  }
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_NAME_TOPBAR) };
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
  for (int k = 0; k < 5 && !bars; ++k) {
    int lit = k == 4 ? n > 0 : k == m->name_page;
    anm_draw_ex(ID.name_tab[k], lit ? 0 : anm_frames(ID.name_tab[k]) - 1, 1, NAME_TAB_X + NAME_TAB_DX * (float)k,
                NAME_TAB_Y, NULL, 0);
  }
}

/* text in a font's glyphs (CARD_GLYPH2 / 5: digits, then the extra characters) from (x, y);
 * returns the x after it */
static float card_glyphs(int base, const char *extra, float adv, const char *text, float x, float y, float a) {
  for (const char *c = text; *c; ++c, x += adv) {
    const char *e = strchr(extra, *c);
    int g = *c >= '0' && *c <= '9' ? *c - '0' : e ? 10 + (int)(e - extra) : -1;
    if (g >= 0 && *c) anm_draw_sprite(anm_sprite(base + g), x, y, a);
  }
  return x;
}

/* the place of a field of menu_layout_cardcheck_1 */
static int card_field(int t, float *x, float *y) {
  float r[4];
  if (!anm_sprite_rect(ID.card_layout, 0, anm_sprite(t), r)) return 0;
  *x = r[0], *y = r[1];
  return 1;
}

#include "cardpages.h"

/* the card's pages have arrows at either side for the touch screen (the view change and the steering turn them on the controls): shown where
 * the pages can be turned, tapped in the band they stand in, CARD_ARROW_W from each edge (the menu's 640 wide space) */
#define CARD_ARROW_W 64.0f
#define CARD_ARROW_Y0 140.0f
#define CARD_ARROW_Y1 400.0f
static int card_arrows_on(const menu_state *m) { return m->card_back != MENU_SETTING && !m->deciding && cardpages_ok(); }

static void draw_card(const menu_state *m, int clock, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  const menu_own *o = &m->own[m->own_sel >= 0 ? m->own_sel : 0];
  const cardb_car *c = &CARDB_CARS[o->maker][o->car];
  const cardb_color *oc = color_of(o->maker, o->car, o->color, m->ccolor[o->maker][menu_own_slot(o)]);
  draw_back(ID.bg_carsel, clock % 240, fade); /* (the flat horizon, as the car selects') */
  anm_draw(ID.line, clock % anm_frames(ID.line), fade);
  if (draw3d) draw3d(ctx); /* (the car turning, behind the registration's layer) */
  anm_swap menu = { anm_sprite(T_CARD_MENU), anm_sprite(has(c->label, "[R35]") ? S_CARD_MENU_R35 : T_CARD_MENU) };
  anm_draw_ex(ID.card_waku, 0, fade, 0, 0, &menu, 1);
  anm_swap sw[11] = {
    { anm_sprite(T_CARD_NO), -1 }, { anm_sprite(T_CARD_NAME), -1 }, { anm_sprite(T_CARD_TITLE), -1 },
    { anm_sprite(T_CARD_CAR1), -1 }, { anm_sprite(T_CARD_CAR2), -1 }, { anm_sprite(T_CARD_COLOR), -1 },
    { anm_sprite(T_CARD_HP), -1 }, { anm_sprite(T_CARD_ODO), -1 }, { anm_sprite(T_CARD_PASS), -1 },
    { anm_sprite(T_CARD_LIFE), -1 }, { anm_sprite(T_CARD_SWATCH), anm_sprite(oc->swatch) },
  };
  anm_draw_ex(ID.card_layout, 0, fade, 0, 0, sw, 11);
  float x, y;
  char text[32];
  int slot = m->save_slot >= 1 && m->save_slot <= MENU_SAVES ? m->save_slot : 1;
  if (card_field(T_CARD_NO, &x, &y)) {
    snprintf(text, sizeof text, "%02d-%02d", slot, (m->own_sel >= 0 ? m->own_sel : 0) + 1);
    card_glyphs(CARD_GLYPH5, "-", 16.0f, text, x, y, fade);
  }
  if (card_field(T_CARD_NAME, &x, &y)) {
    /* the player's name in font 5 (its full width 28), a save without one PLAYER n */
    if (m->name[0] >= 0 && anm_sprite(NAME_GLYPH5) >= 0)
      for (int i = 0; i < 5 && m->name[i] >= 0; ++i) anm_draw_sprite(anm_sprite(NAME_GLYPH5 + m->name[i]), x + 28.0f * (float)i, y, fade);
    else
      anm_draw_sprite(anm_sprite(CARD_TEXT + C_PLAYER + slot - 1), x, y, fade);
  }
  if (card_field(T_CARD_TITLE, &x, &y)) {
    /* the class's "B7級　/" (its advances: the sprite less the cell, plus the '/''s half),
     * then the car's title in font 5's glyphs (a menu.bin from before them: the row with
     * 湾岸の新人, or before the classes the title alone) */
    int cls = m->class_car[o->maker][menu_own_slot(o)], pre = anm_sprite(CARD_CLASSP + cls);
    int title = m->title_car[o->maker][menu_own_slot(o)] > 0 ? m->title_car[o->maker][menu_own_slot(o)] : TITLE_FIRST;
    const unsigned short *g;
    int n = title_glyphs(title, &g);
    if (pre >= 0 && n > 0 && anm_sprite(TITLE_GLYPH5 + g[0]) >= 0) {
      anm_draw_sprite(pre, x, y, fade);
      x += anm_sprite_width(pre) - 32.0f + 16.0f;
      for (int i = 0; i < n; ++i) {
        if (!title_glyph_slash(g[i])) anm_draw_sprite(anm_sprite(TITLE_GLYPH5 + g[i]), x, y, fade);
        x += title_glyph_half(g[i]) ? 16.0f : 28.0f;
      }
    } else {
      int row = anm_sprite(CARD_CLASS + cls);
      anm_draw_sprite(row >= 0 ? row : anm_sprite(CARD_TEXT + C_TITLE0), x, y, fade);
    }
  }
  int flat = o->car;
  for (int mk = 0; mk < o->maker; ++mk) flat += CARDB_NCARS[mk];
  if (card_field(T_CARD_CAR1, &x, &y)) anm_draw_sprite(anm_sprite(CARD_CAR + 2 * flat), x, y, fade);
  if (card_field(T_CARD_CAR2, &x, &y)) anm_draw_sprite(anm_sprite(CARD_CAR + 2 * flat + 1), x, y, fade);
  if (card_field(T_CARD_COLOR, &x, &y))
    anm_draw_sprite(anm_sprite(CARD_COLOR + oc->name - 100002), x, y, fade);
  if (card_field(T_CARD_HP, &x, &y)) {
    int power = m->tune_full[o->maker][menu_own_slot(o)] ? m->tune_power[o->maker][menu_own_slot(o)] : m->st_power[o->maker][menu_own_slot(o)];
    snprintf(text, sizeof text, "%d", car_tune_hp(c->hp, power));
    x = card_glyphs(CARD_GLYPH2, "-,", 8.0f, text, x, y, fade);
    anm_draw_sprite(anm_sprite(CARD_TEXT + C_HP), x, y, fade);
  }
  if (card_field(T_CARD_ODO, &x, &y)) {
    long km = (long)(o->odo / 1000.0);
    char digits[24];
    snprintf(digits, sizeof digits, "%ld", km);
    int n = (int)strlen(digits), j = 0;
    for (int i = 0; i < n; ++i) {
      if (i && (n - i) % 3 == 0) text[j++] = ',';
      text[j++] = digits[i];
    }
    text[j] = 0;
    const float x0 = x;
    x = card_glyphs(CARD_GLYPH2, "-,", 8.0f, text, x, y, fade);
    anm_draw_sprite(anm_sprite(CARD_TEXT + C_KM), x + 8.0f, y, fade);
    /* and what was driven since the game started (the newer builds' "this time's distance", issue 11) under it */
    snprintf(text, sizeof text, "THIS TIME %ld km", (long)((o->odo - o->odo0) / 1000.0));
    ex_text(2, text, x0, y + 26.0f, fade);
  }
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(110100 + 9) >= 0 ? anm_sprite(110100 + 9) : anm_sprite(S_CARD_TOPBAR) }; /* ("(1/7)") */
  draw_topbar(bars, fade, &title);
  draw_bottom(bars, fade);
  int hint = anm_sprite(CARD_TEXT + C_CARD_HINT);
  if (!bars) if (bar_keys()) anm_draw_sprite(hint, 320.0f - anm_sprite_width(hint) * 0.5f, 427.0f, fade * (0.75f + 0.25f * cosf((float)clock * 0.1f)));
}

/* ---- the race's pause menu: the arcade's continue dialog (clSeqContinueThread sets up
 * menu_layout_yn_card_1_gold with YES 0x13ca and NO 0x10a1 in place of the morning and
 * night cards, under "Continue?" 0x1ab0), at 60 % in a box (menu_p_maxi_tornament_waku,
 * drawn without its rule) over the race. A tap on YES decides it; a tap on NO slides the
 * spin box over to it, then decides it. ---- */
enum { T_YES = JID(0x13ca), T_NO = JID(0x10a1), T_CONTINUE = JID(0x1ab0), T_PANEL = JID(0x1a75) };
#define PAUSE_K 0.6f
#define PAUSE_DX (320.0f * (1.0f - PAUSE_K)) /* the cards scaled about (320, 262) */
#define PAUSE_DY (262.0f * (1.0f - PAUSE_K))
#define PAUSE_IN 8 /* frames the box takes to fade in */

void menu_pause_open(menu_state *m) {
  memset(&m->pause, 0, sizeof m->pause);
  m->pause.open = 1;
  snd_menu(SND_CURSOR);
}

/* whether the pause menu's tap is on card k (0 YES, 1 NO) */
static int pause_on_card(const menu_pause *p, int k) {
  float r[4];
  if (!anm_sprite_rect(ID.day_cards, 100 * p->sel + p->card_t % 60, anm_sprite(k ? T_NIGHT : T_MORNING), r)) return 0;
  float cx = PAUSE_DX + PAUSE_K * (r[0] + r[2]) * 0.5f, cy = PAUSE_DY + PAUSE_K * (r[1] + r[3]) * 0.5f;
  float rad = PAUSE_K * (r[2] - r[0]) * 0.42f + 10.0f; /* the disc, and a finger's slack */
  return (p->tap_x - cx) * (p->tap_x - cx) + (p->tap_y - cy) * (p->tap_y - cy) < rad * rad;
}

static void pause_decide(menu_pause *p) {
  snd_menu(p->sel ? SND_CANCEL : SND_SELECT);
  p->deciding = DECIDE_DAY + FADE;
}

/* the pause menu's pointer: a tap on the card in front decides it, on the other slides to it and decides (select = that card) */
static void pause_pointer(const menu_pause *p, menu_in *in) {
  if (pause_on_card(p, p->sel)) in->ok = 1;
  else if (pause_on_card(p, !p->sel)) in->select = !p->sel;
}

static int pause_step(menu_pause *p) {
  menu_in in = { .dx = p->act_dx, .select = -1, .tab = -1, .ok = p->act_ok, .back = p->act_back };
  int tap = p->tap;
  p->tap = p->act_dx = p->act_ok = p->act_back = 0;
  if (tap && p->open >= PAUSE_IN) pause_pointer(p, &in);
  p->open++;
  if (p->deciding) return --p->deciding ? MENU_PAUSED : p->sel ? MENU_LEAVE : MENU_RESUME;
  if (p->slide) {
    if (--p->slide == 0) {
      p->sel ^= 1;
      p->card_t = 0;
      if (p->go) pause_decide(p);
    }
    return MENU_PAUSED;
  }
  p->card_t++;
  if (p->open < PAUSE_IN) return MENU_PAUSED;
  if (in.back) { /* BACK is YES */
    p->sel = 0;
    pause_decide(p);
  } else if (in.ok) {
    pause_decide(p);
  } else if (in.select >= 0 || (in.dx < 0 && p->sel) || (in.dx > 0 && !p->sel)) {
    snd_menu(SND_CURSOR);
    p->slide = SLIDE;
    p->go = in.select >= 0; /* (chosen outright: decided once it has come round) */
  }
  return MENU_PAUSED;
}

static void go(menu_state *m, int screen);
static void tune_open(menu_state *m);
static void lvup_open(menu_state *m);
static void poem_open(menu_state *m);
static void ending_open(menu_state *m);
static void shogo_open(menu_state *m);

void menu_vs_closed(menu_state *m, const char *note) {
  m->vs.state = MENU_VS_OFF;
  m->vs.n = 0;
  m->finish_story = 0;
  m->finish = 0;
  m->pause.open = 0;
  go(m, MENU_VS);
  m->t = 0;
  m->bars = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
  snprintf(m->vs.note, sizeof m->vs.note, "%s", note);
}

/* The race left with the screen white (the pause menu's NO) or black (its end): back on the
 * time zone and BGM select (a story race: the game mode select), out of the white like the car select from the title, or out of
 * the black as from the course select */
static int milage_enter(menu_state *m, int after_race);

/* the menus after a race: the 10-in-a-row back to the game mode select too, the time attack's to its course select */
static void back_to_modes(menu_state *m, int white) {
  int game = m->game != MENU_GAME_TA;
  go(m, game ? MENU_GAME : MENU_COURSE); /* (the time attack: its course select, the screen before the time zone) */
  m->t = white ? FADE : 0; /* white: no fade in from black */
  m->sel = game ? game_slot(m->game) : m->course;
  if (!game) m->course_pos = m->sel <= 3 ? m->sel : m->sel >= 9 ? m->sel - 6 : 3;
  m->slide = m->card_t = 0;
  m->bars = 1;
  m->music = 1;
  m->flash = white ? FLASH_LEN : 0;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static void back_from_race(menu_state *m, int white) {
  /* a story race goes back to the game mode select; one whose select runs a second race on
   * (menu_finish_story's next) to that race's title */
  if (m->finish_story == 3) {
    /* clRaceThread runs the select's next periods on without the menus: no title page, the
     * screen in the race's fade while the next one loads (initStory), then that one (its
     * realtime movie first) in from it */
    m->finish_story = 0;
    go(m, MENU_PRE);
    m->pre_quiet = 1;
    m->pre_fade = m->finish_fade;
    m->t = 0;
    m->tap = m->key = m->key_held = m->back = m->leaving = 0;
    return;
  }
  /* an online versus back to its lobby (the class-up and the rest wait for the next race) */
  if (m->vs.state == MENU_VS_RACE) {
    m->vs.state = MENU_VS_LOBBY;
    m->finish_story = 0;
    /* the stars won or lost first (seqJoStarGet) */
    if (m->star_delta && !white) {
      menu_vs_star_open(m);
      return;
    }
    m->star_delta = 0;
    go(m, MENU_VS);
    m->t = white ? FADE : 0;
    m->bars = 1;
    m->flash = white ? FLASH_LEN : 0;
    m->tap = m->key = m->key_held = m->back = m->leaving = 0;
    return;
  }
  /* the class risen: the class-up screen first (seqLvUp, right after the race), out of the black */
  if (m->lvup_due && !white) {
    lvup_open(m);
    return;
  }
  m->lvup_due = 0;
  /* new titles: the title select (judgeTitle and seqTitleSelect right after seqLvUp), out of
   * the black */
  if (m->title_n && !white) {
    shogo_open(m);
    return;
  }
  m->title_n = 0;
  /* a full story gauge: the tuning screen (mseqStory's seqStTune), out of the black */
  if (m->game == MENU_GAME_STORY && m->st_tune_due && !white) {
    tune_open(m);
    m->tap = m->key = m->key_held = m->back = m->leaving = 0;
    return;
  }
  /* a won select's poem (mseqStory's seqStPoem, after the tuning) */
  if (m->game == MENU_GAME_STORY && m->poem >= 0 && !white) {
    poem_open(m);
    return;
  }
  m->poem = -1;
  if (m->game == MENU_GAME_STORY && m->ending == 1 && !white) {
    ending_open(m);
    return;
  }
  m->ending = 0;
  /* an aero set won: the dress-up screens, out of the black */
  if (!white && tg_due(m)) return;
  /* the end of a single player game (the arcade: the title screens, then the mileage tally, then Continue?): the tally, once the screens of the race are
   * through, not when the race was backed out of (white) */
  if (!white && milage_enter(m, 1)) return;
  back_to_modes(m, white);
}

int menu_pause_update(menu_state *m, float dt) {
  m->acc += dt;
  if (m->acc > 0.25f) m->acc = 0.25f;
  while (m->acc >= 1.0f / 60.0f) {
    m->acc -= 1.0f / 60.0f;
    int r = pause_step(&m->pause);
    if (r == MENU_PAUSED) continue;
    m->pause.open = 0;
    if (r == MENU_LEAVE) back_from_race(m, 1);
    return r;
  }
  return MENU_PAUSED;
}

/* ---- the end of a time attack (clRaceThread::result2). At the goal and when the time runs
 * out the consoles go (clStandardConsole and the time attack's draw only in yt::CState 1 and
 * 2: app.c) and the race's song stops as the race ends; it runs on 90 frames past the goal,
 * 60 once the car has stopped after the time ran out (no goal camera in the time attack).
 * Then, with the result music:
 * - clTaResult, over the race running on: FINISH (menu_g_finish_finish) with finish1, and by
 *   its counter finish2 at 150, then the result's rows, each with cutin2: the course (180),
 *   the course's record (220), the car type's (255), the player's own (290), the sections
 *   (325), TOTAL TIME (370), the top speed and the ranking (440). A record the run set has
 *   its NEW mark (template 0x1f59) pulse, and the first of them starts CONGRATULATIONS. A tap
 *   once the counter is past 440 (or the counter past 4199) leaves: the result goes, the
 *   screen fades to black in 1/6 s and stays black 60 frames.
 * - clTaTimeUp: the screen fades to black in 1/6 s, TIME UP (menu_g_finish_timeup_ta) plays
 *   280 frames over the black, 30 frames of black follow.
 * Then the menus come back out of the black (the arcade goes on to its card and continue
 * screens). ---- */
#define FINISH_GOAL_WAIT 90
#define FINISH_TIMEUP_WAIT 60
enum { RES_FINISH2 = 150, RES_COURSE = 180, RES_COURSE_BEST = 220, RES_CAR_BEST = 255, RES_PERSONAL = 290,
       RES_SECTIONS = 325, RES_TOTAL = 370, RES_SPEED = 440, RES_TIMEOUT = 4199 };
#define RES_FINISH_LEN 180
#define RES_ROW_LEN 70       /* the best rows' animation (the NEW mark's pulse) and TOTAL TIME's */
#define RES_TOTAL_LOOP 10    /* TOTAL TIME's zoom in, then frames 10..69 looped */
#define RES_CONGRATS_LEN 240 /* CONGRATULATIONS lights up, then frames 120..239 looped */
#define RES_CONGRATS_LOOP 120
#define RES_OUT_FADE 10
#define STORY_FADE 20 /* clRaceThread's clFader::setFade between a select's periods: 1/3 s */
#define RES_OUT_BLACK 60
#define TIMEUP_FADE 10
#define TIMEUP_LEN 280
#define TIMEUP_BLACK 30
/* nsTaData's digits (menu_p_finish_ta_0..9, 0x93d22c0) and TOTAL TIME's (0x93d2320); the
 * templates setTime7 fills with tens of minutes and m'ss"mmm, for the best rows (0x93d22e8)
 * and TOTAL TIME (0x93d2348), and setTime6 with m'ss"mmm for each section (0x93d2380...);
 * the sections' numbers (0x93d2428) */
#define RES_DIGIT JID(2291)
#define RES_BIG_DIGIT JID(6219)
static const int BEST_TEMPLATE[7] = { JID(7271), JID(7265), JID(7266), JID(7267), JID(7268), JID(7269), JID(7270) };
static const int TOTAL_TEMPLATE[7] = { JID(6220), JID(6221), JID(6222), JID(6223), JID(6224), JID(6225), JID(6226) };
static const int SECTION_TEMPLATE[4][6] = {
  { JID(7275), JID(7276), JID(7277), JID(7278), JID(7279), JID(7280) }, { JID(7281), JID(7282), JID(7283), JID(7284), JID(7285), JID(7286) },
  { JID(7287), JID(7288), JID(7289), JID(7290), JID(7291), JID(7292) }, { JID(7293), JID(7294), JID(7295), JID(7296), JID(7297), JID(7298) },
};
static const int SECTION_LABEL[7] = { JID(7265), JID(7266), JID(7267), JID(7268), JID(7269), JID(7270), JID(7271) };
/* the NEW mark (menu_p_finish_ta_overwrite), the top speed's hundreds, tens and ones, the
 * ranking's; the course's name and time zone (as on the race's title page) */
enum { T_RES_NEW = JID(0x1f59), RES_NEW = 0x1a70, T_RES_COURSE = JID(0x1b9a), T_RES_TIME = JID(0x15ed) };
static const int SPEED_TEMPLATE[3] = { JID(0x1c60), JID(0x1c61), JID(0x1c62) };
static const int RANK_TEMPLATE[3] = { JID(0x1c80), JID(7267), JID(7268) };
static const int RES_TIME_NAME[2] = { JID(4751), JID(3301) };

/* clTaResult's constructor sets the rows' sprites once (setImageReplace) */
static struct {
  anm_swap course[2];
  anm_swap best[3][8]; /* the time's 7 digits, the NEW mark */
  anm_swap sections[4 * 7], sections2[3 * 7];
  anm_swap total[7];
  anm_swap speed[3], rank[3];
} RES;

/* nsTaData::setTime7: (int)(s * 1000) to 99'59"999, tens of minutes blank below 10' */
static void set_time7(anm_swap *sw, const int *tmpl, int digit, float s) {
  int ms = (int)(s * 1000.0f);
  ms = ms < 0 ? 0 : ms >= 6000000 ? 5999999 : ms;
  int d[7] = { ms / 600000, ms % 600000 / 60000, ms % 60000 / 10000, ms % 10000 / 1000, ms % 1000 / 100, ms % 100 / 10, ms % 10 };
  for (int i = 0; i < 7; ++i) sw[i] = (anm_swap){ anm_sprite(tmpl[i]), i == 0 && ms <= 599999 ? -1 : anm_sprite(digit + d[i]) };
}

/* nsTaData::setTime6: m'ss"mmm to 9'59"999 */
static void set_time6(anm_swap *sw, const int *tmpl, float s) {
  int ms = (int)(s * 1000.0f);
  ms = ms < 0 ? 0 : ms > 599999 ? 599999 : ms;
  int d[6] = { ms / 60000, ms % 60000 / 10000, ms % 10000 / 1000, ms % 1000 / 100, ms % 100 / 10, ms % 10 };
  for (int i = 0; i < 6; ++i) sw[i] = (anm_swap){ anm_sprite(tmpl[i]), anm_sprite(RES_DIGIT + d[i]) };
}

static void result_setup(const menu_state *m) {
  const menu_result *r = &m->result;
  RES.course[0] = (anm_swap){ anm_sprite(T_RES_COURSE), anm_sprite(COURSE_NAME[car_course_id(m->course)]) };
  RES.course[1] = (anm_swap){ anm_sprite(T_RES_TIME), anm_sprite(RES_TIME_NAME[m->zone]) };
  const float best[3] = { r->course_record, r->car_record, r->personal };
  const int news[3] = { r->new_course, r->new_car, r->new_personal };
  for (int k = 0; k < 3; ++k) {
    set_time7(RES.best[k], BEST_TEMPLATE, RES_DIGIT, best[k]);
    RES.best[k][7] = (anm_swap){ anm_sprite(T_RES_NEW), news[k] ? anm_sprite(RES_NEW) : -1 };
  }
  /* menu_g_finish_ta_section_1's four rows (section_2's 5..7 are for course ids 12 and 13) */
  for (int k = 0; k < 4; ++k) {
    RES.sections[7 * k] = (anm_swap){ anm_sprite(SECTION_LABEL[k]), anm_sprite(RES_DIGIT + 1 + k) };
    set_time6(&RES.sections[7 * k + 1], SECTION_TEMPLATE[k], r->section[k]);
  }
  /* section_2's rows 5..7, with rows 1..3's templates */
  for (int k = 0; k < 3; ++k) {
    RES.sections2[7 * k] = (anm_swap){ anm_sprite(SECTION_LABEL[4 + k]), anm_sprite(RES_DIGIT + 5 + k) };
    set_time6(&RES.sections2[7 * k + 1], SECTION_TEMPLATE[k], r->section[4 + k]);
  }
  set_time7(RES.total, TOTAL_TEMPLATE, RES_BIG_DIGIT, r->total);
  int v = r->top_speed < 0 ? 0 : r->top_speed > 999 ? 999 : r->top_speed;
  const int speed[3] = { v / 100, v / 10 % 10, v % 10 };
  for (int i = 0; i < 3; ++i) RES.speed[i] = (anm_swap){ anm_sprite(SPEED_TEMPLATE[i]), anm_sprite(RES_DIGIT + speed[i]) };
  v = r->rank < 0 ? 0 : r->rank > 999 ? 999 : r->rank;
  const int rank[3] = { v / 100, v / 10 % 10, v % 10 };
  for (int i = 0; i < 3; ++i)
    RES.rank[i] = (anm_swap){ anm_sprite(RANK_TEMPLATE[i]), (i == 0 && v < 100) || (i == 1 && v < 10) ? -1 : anm_sprite(RES_DIGIT + rank[i]) };
}

void menu_finish_open(menu_state *m, int goal, const menu_result *res) {
  m->finish = 1;
  m->finish_goal = goal != 0;
  m->finish_wait = goal ? FINISH_GOAL_WAIT : FINISH_TIMEUP_WAIT;
  m->finish_out = 0;
  m->finish_story = 0;
  m->tap = 0;
  m->acc = 0;
  if (goal && res) {
    m->result = *res;
    result_setup(m);
  }
}

void menu_finish_vs(menu_state *m, int place, int n, int timeup) {
  m->finish_story = place == 1 ? 5 : 6;
  m->finish_goal = 1;
  m->vs.timeup = timeup != 0;
  m->finish_wait = timeup ? FINISH_TIMEUP_WAIT : FINISH_GOAL_WAIT;
  m->vs.last_place = place;
  m->vs.last_n = n;
  for (int id = 2625; id <= 2631; ++id) anm_warm_sprite(anm_sprite(id), 1); /* (menu_finish_draw's places) */
}

void menu_finish_story(menu_state *m, int won, int next, float gap, int fade) {
  m->finish_story = next ? 3 : won ? 1 : 2;
  m->finish_goal = 1;
  /* clRaceThread::race: the Suspend (play state 10) fades out at once, no FINISH */
  m->finish_wait = next ? 0 : FINISH_GOAL_WAIT;
  m->finish_fade = next ? fade : 0;
  m->st_show = 0; /* menu_story_points sets it */
  m->st_gap = gap;
}

/* The 10-in-a-row's end (clSeqTenResultThread::run 0x89632d0, clTenResult): after a TIME UP
 * (or a retire) clMenuFade first darkens the screen in 10 frames and stays black under it.
 * Three phases, each run till its update gives 0 (clTenResult::update), the next one's
 * reset then: 1. WIN (all beaten) or TIME UP for 181 frames; 2. the faces of the round's
 * rivals (menu_layout_or_result_face) with an X (menu_g_or_finish_X, set into the face's
 * dot) over each beaten one, the "10 in a row" header and the medal (menu_layout_or_finish_result);
 * 3. the summary (menu_layout_or_result_A): course, level, the clear time, a character's
 * line picked at random for the result. No skip. */
enum { TEN_FADE = 10, TEN_R1_LEN = 180, TEN_R3_LEN = 360, TEN_MAX = 32, MAX_TEN_WARM = 128 };
static int is_template(int id);
/* clSpriteAnime::setAnime's state: mode 1 a still of frame start, 2 hidden, 4 played from
 * start for len frames (0: to the end) and held */
typedef struct {
  int anim, mode, start, len, f;
} ten_anim;
static void ta_set(ten_anim *a, int mode, int start, int len) {
  a->mode = mode;
  a->start = a->f = start;
  a->len = len;
}
static void ta_update(ten_anim *a) {
  int end = a->len ? a->start + a->len : anm_frames(a->anim);
  if (a->mode == 4 && a->f + 1 < end) a->f++;
}
static void ta_draw(const ten_anim *a, float dx, float dy, const anm_swap *sw, int n) {
  if (a->mode != 2) anm_draw_ex(a->anim, a->f, 1, dx, dy, sw, n);
}

/* the faces by level (0x9770b20; the last one's is the 0x73c template, "?", till he is the
 * one left), the result's medal frames by getTenResult (0x93d04b0), its course names
 * (0x93d1fe0) and levels (0x93d2020), the clear time's templates and digits (0x93d1de8,
 * 0x93d1dc0) and what it blanks when not all were beaten (set_clear_time) */
static const short TEN_FACE_ID[11][TEN_MAX] = {
  { JID(1821), JID(1822), JID(1823), JID(1824), JID(1825), JID(1826), JID(1827), JID(1831), JID(1829), JID(1850) },
  { JID(1822), JID(1824), JID(1825), JID(1831), JID(1830), JID(1841), JID(1842), JID(1845), JID(1846), JID(1851) },
  { JID(1825), JID(1853), JID(1829), JID(1831), JID(1832), JID(1841), JID(1842), JID(1843), JID(1849), JID(1851) },
  { JID(1853), JID(1826), JID(1825), JID(1827), JID(1828), JID(1847), JID(1842), JID(1848), JID(1850), JID(1851) },
  { JID(1837), JID(1838), JID(1839), JID(1840), JID(1836), JID(1828), JID(1826), JID(1833), JID(1834), JID(1830) },
  { JID(1824), JID(1828), JID(1841), JID(1829), JID(1844), JID(1849), JID(1835), JID(1848), JID(1836), JID(1851) },
  { JID(1829), JID(1835), JID(1848), JID(1837), JID(1836), JID(1828), JID(1833), JID(1826), JID(1834), JID(1830) },
  { JID(1835), JID(1838), JID(1839), JID(1840), JID(1834), JID(1836), JID(1843), JID(1844), JID(1848), JID(1849) },
  { JID(1831), JID(1841), JID(1853), JID(1843), JID(1844), JID(1845), JID(1846), JID(1847), JID(1848), JID(1850) },
  { JID(1853), JID(1842), JID(1843), JID(1845), JID(1846), JID(1847), JID(1848), JID(1849), JID(1850), JID(1851) },
  { JID(1821), JID(1822), JID(1823), JID(1824), JID(1825), JID(1826), JID(1827), JID(1828), JID(1829), JID(1830), JID(1831), JID(1832), JID(1833), JID(1834), JID(1835), JID(1836),
    JID(1837), JID(1838), JID(1839), JID(1840), JID(1841), JID(1842), JID(1853), JID(1843), JID(1844), JID(1846), JID(1847), JID(1848), JID(1845), JID(1849), JID(1850), JID(1851) },
};
enum { T_TEN_FACE = JID(0x73c), T_TEN_DOT = JID(0x6b4), T_TEN_OR = JID(0xf36), T_TEN_COURSE = JID(0xcbe), T_TEN_LEVEL = JID(0xb1e),
       T_TEN_MSG_FACE = JID(0xb0d), T_TEN_MSG_NAME = JID(0xb18), T_TEN_MSG = JID(0x915) };
static const short TEN_MEDAL_AT[6] = { 400, 400, 400, 100, 200, 300 };
static const short TEN_COURSE_NAME[10] = { JID(3262), JID(3263), JID(3264), JID(3265), JID(3266), JID(3267), JID(3270), JID(3268), JID(3269), JID(3271) };
static const short TEN_LEVEL_NAME[11] = { JID(2846), JID(2847), JID(2848), JID(2849), JID(2850), JID(2851), JID(2852), JID(2853), JID(2854), JID(2855), JID(5599) };
static const int TEN_TIME_TEMPLATE[7] = { JID(6219), JID(6220), JID(6221), JID(6222), JID(6223), JID(6224), JID(6225) };
static const short TEN_TIME_BLANK[10] = { JID(0x184b), JID(0x184c), JID(0x184d), JID(0x184e), JID(0x184f), JID(0x1850), JID(0x1851), JID(0x1d4a), JID(0x1d4b), JID(0x1289) };
/* the lines (face, name, words) by result: 0x93d1e04 counts (a pick of 0..n), the tables
 * 0x93d1e20, 0x93d1e80, 0x93d1ec0, 0x93d1f20, 0x93d1fa0, 0x93d1fc4 */
static const short TEN_MSG_N[6] = { 5, 4, 6, 8, 2, 0 };
static const short TEN_MSG[6][9][3] = {
  { { JID(3230), JID(3256), JID(2835) }, { JID(3233), JID(3259), JID(2838) }, { JID(2832), JID(2843), JID(2328) }, { JID(3880), JID(3892), JID(3255) }, { JID(5590), JID(5595), JID(4740) },
    { JID(2834), JID(2845), JID(2330) } },
  { { JID(3871), JID(3884), JID(3246) }, { JID(2323), JID(2332), JID(1866) }, { JID(2831), JID(2842), JID(2327) }, { JID(2324), JID(2333), JID(1867) }, { JID(5593), JID(5598), JID(4743) } },
  { { JID(3231), JID(3257), JID(2836) }, { JID(3232), JID(3258), JID(2837) }, { JID(4734), JID(4744), JID(3882) }, { JID(2833), JID(2844), JID(2329) }, { JID(3878), JID(3890), JID(3253) },
    { JID(5591), JID(5596), JID(4741) }, { JID(5592), JID(5597), JID(4742) } },
  { { JID(2830), JID(2841), JID(2326) }, { JID(5589), JID(5594), JID(4739) }, { JID(3872), JID(3885), JID(3247) }, { JID(2322), JID(2331), JID(1865) }, { JID(3873), JID(3883), JID(3248) },
    { JID(3875), JID(3887), JID(3250) }, { JID(3876), JID(3888), JID(3251) }, { JID(3877), JID(3889), JID(3252) }, { JID(3879), JID(3891), JID(3254) } },
  { { JID(2829), JID(2840), JID(2325) }, { JID(1854), JID(1868), JID(1562) }, { JID(3234), JID(3260), JID(2839) } },
  { { JID(3874), JID(3886), JID(3249) } },
};

static struct {
  int phase, t, fade;
  int level, course, beaten, total, result, msg;
  float time;
  ten_anim r1, face[TEN_MAX], x[TEN_MAX], head, medal, r3;
  int step[TEN_MAX];
  int warm[MAX_TEN_WARM], nwarm;
} TEN;

static int ten_extend(void) { return TEN.level == 10; }
/* the faces drawn: 10, or in the extend mode those beaten and 4 more, 11 to 32 */
static int ten_faces(void) {
  if (!ten_extend()) return 10;
  int n = TEN.beaten + 4;
  return n < 11 ? 11 : n > TEN_MAX ? TEN_MAX : n;
}
static int ten_face_id(int i) {
  int n = ten_extend() ? TEN_MAX : 10;
  return i < n - 1 || TEN.beaten >= n - 1 ? TEN_FACE_ID[TEN.level][i] : T_TEN_FACE;
}
/* the extend mode's faces come in from the right in steps (0x93d0580: 6 frames, 60 px a
 * step) at the times of 0x93d0620 (face i's step k at 5 (i + k) up to 50, then 130 + 10 a
 * step), as far as 0x93d0d20 lets them for the count beaten */
static int ten_step_at(int i, int k) {
  int j = i + k;
  return k > 12 || j > 32 ? -1 : j <= 10 ? 5 * j : 130 + 10 * (j - 11);
}
static int ten_step_max(int i) {
  int m = 3 - i + (TEN.beaten > 7 ? TEN.beaten : 7);
  return m < 0 ? 0 : m > 13 ? 13 : m;
}

static void ten_phase_start(void) {
  TEN.t = 0;
  if (TEN.phase == 0) {
    TEN.r1.anim = TEN.beaten == TEN.total ? ID.win : ID.ten_timeup;
    ta_set(&TEN.r1, 4, 0, 0);
  } else if (TEN.phase == 1) {
    int n = ten_extend() ? TEN_MAX : 10;
    for (int i = 0; i < n; ++i) {
      TEN.face[i].anim = ID.ten_face;
      TEN.x[i].anim = ID.ten_x;
      /* 0x93d04e0: the normal levels' faces stand in a row, 60 px apart from the left */
      if (ten_extend()) ta_set(&TEN.face[i], 2, 0, 0);
      else ta_set(&TEN.face[i], 1, 220 - 20 * i, 1);
      ta_set(&TEN.x[i], 2, 0, 0);
      TEN.step[i] = 0;
    }
    TEN.head.anim = TEN.medal.anim = ID.ten_finish;
    ta_set(&TEN.head, 4, 0, 60);
    ta_set(&TEN.medal, 2, 0, 0);
  } else {
    TEN.r3.anim = ID.ten_summary;
    ta_set(&TEN.r3, 4, 0, 0);
  }
}

/* one frame of the phase's update; 0 when it is over */
static int ten_phase_update(void) {
  int t = TEN.t;
  if (TEN.phase == 0) {
    if (t > TEN_R1_LEN) return 0;
    ta_update(&TEN.r1);
  } else if (TEN.phase == 1) {
    if (t > TEN.beaten * 10 + 220) return 0;
    int ext = ten_extend(), n = ten_faces();
    if (ext)
      for (int i = 0; i < n; ++i) {
        int k = TEN.step[i], at = ten_step_at(i, k);
        if (k <= ten_step_max(i) && at >= 0 && at == t) {
          ta_set(&TEN.face[i], 4, 10 + 20 * k, 6);
          TEN.step[i]++;
        }
      }
    /* the beaten ones crossed out 10 frames apart (from 10, or 60 in the extend mode), the
     * medal 10 frames after the last */
    int first = ext ? 60 : 10;
    for (int i = 0; i < TEN.beaten && i < TEN_MAX; ++i)
      if (t == first + 10 * i) {
        ta_set(&TEN.x[i], 4, 0, 0);
        snd_menu(SND_RESULT2);
        break;
      }
    if (t == TEN.beaten * 10 + first) {
      ta_set(&TEN.medal, 4, TEN_MEDAL_AT[TEN.result], 20);
      snd_menu(SND_RESULT_PASS);
    }
    for (int i = 0; i < n; ++i) {
      ta_update(&TEN.face[i]);
      ta_update(&TEN.x[i]);
    }
    ta_update(&TEN.head);
    ta_update(&TEN.medal);
  } else {
    if (t > TEN_R3_LEN) return 0;
    if (t == 0 || t == 50 || t == 65 || t == 80 || t == 140 || (t == 95 && TEN.beaten == TEN.total)) snd_menu(SND_CUTIN2);
    ta_update(&TEN.r3);
  }
  TEN.t++;
  return 1;
}

static void draw_ten_result(void) {
  if (TEN.fade) anm_fill(0, 0, 0, TEN.fade < TEN_FADE ? (float)TEN.fade / TEN_FADE : 1.0f);
  if (TEN.fade && TEN.fade < TEN_FADE) return;
  anm_swap or = { anm_sprite(T_TEN_OR), anm_sprite(T_TEN_OR + ten_extend()) };
  if (TEN.phase == 0) {
    ta_draw(&TEN.r1, 0, 0, NULL, 0);
  } else if (TEN.phase == 1) {
    int dot = anm_sprite(T_TEN_DOT);
    for (int i = 0; i < ten_faces(); ++i) {
      const ten_anim *f = &TEN.face[i];
      if (f->mode == 2) continue;
      anm_swap sw[2] = { { anm_sprite(T_TEN_FACE), anm_sprite(ten_face_id(i)) }, { dot, -1 } };
      ta_draw(f, 0, 0, sw, 2);
      /* the X stands in for the face's dot (setImageReplace(0x6b4, X)) */
      float r[4];
      if (TEN.x[i].mode != 2 && anm_sprite_rect(f->anim, f->f, dot, r)) ta_draw(&TEN.x[i], r[0], r[1], NULL, 0);
    }
    ta_draw(&TEN.head, 0, 0, &or, 1);
    anm_swap none = { anm_sprite(T_TEN_OR), -1 }; /* setImageReplace(0xf36, 1): no sprite */
    ta_draw(&TEN.medal, 0, 0, &none, 1);
  } else if (TEN.phase == 2) {
    anm_swap sw[16];
    int n = 0;
    sw[n++] = or;
    sw[n++] = (anm_swap){ anm_sprite(T_TEN_COURSE), anm_sprite(TEN_COURSE_NAME[TEN.course]) };
    sw[n++] = (anm_swap){ anm_sprite(T_TEN_LEVEL), anm_sprite(TEN_LEVEL_NAME[TEN.level]) };
    const short *msg = TEN_MSG[TEN.result][TEN.msg];
    sw[n++] = (anm_swap){ anm_sprite(T_TEN_MSG_FACE), anm_sprite(msg[0]) };
    sw[n++] = (anm_swap){ anm_sprite(T_TEN_MSG_NAME), anm_sprite(msg[1]) };
    sw[n++] = (anm_swap){ anm_sprite(T_TEN_MSG), anm_sprite(msg[2]) };
    if (TEN.beaten != TEN.total) {
      for (int k = 0; k < 10; ++k) sw[n++] = (anm_swap){ anm_sprite(TEN_TIME_BLANK[k]), -1 };
    } else {
      set_time7(sw + n, TEN_TIME_TEMPLATE, RES_BIG_DIGIT, TEN.time);
      n += 7;
    }
    ta_draw(&TEN.r3, 0, 0, sw, n);
  }
}

static void ten_warm_add(int sprite) {
  if (sprite < 0 || is_template(anm_sprite_id(sprite))) return;
  for (int k = 0; k < TEN.nwarm; ++k)
    if (TEN.warm[k] == sprite) return;
  if (TEN.nwarm < (int)(sizeof TEN.warm / sizeof TEN.warm[0])) TEN.warm[TEN.nwarm++] = sprite;
}

void menu_finish_ten(menu_state *m, int beaten, float time, int medal) {
  memset(&TEN, 0, sizeof TEN);
  TEN.level = m->ten_level < 0 ? 0 : m->ten_level > 10 ? 10 : m->ten_level;
  TEN.course = menu_ten_course(m);
  if (TEN.course < 0) TEN.course = 0;
  if (TEN.course > 9) TEN.course = 9;
  TEN.total = ten_extend() ? TEN_MAX : 10;
  TEN.beaten = beaten < 0 ? 0 : beaten > TEN.total ? TEN.total : beaten;
  TEN.time = time;
  /* getTenResult (0x857f008): all beaten, the medal + 2 (3..5); else by how many were */
  if (TEN.beaten == TEN.total) TEN.result = (medal < 1 ? 1 : medal > 3 ? 3 : medal) + 2;
  else if (ten_extend()) TEN.result = TEN.beaten <= 25 ? 0 : TEN.beaten <= 30 ? 1 : 2;
  else TEN.result = TEN.beaten <= 6 ? 0 : TEN.beaten <= 8 ? 1 : 2;
  /* clTenResult3::reset: clMTRandom over 0..n */
  TEN.msg = rand() % (TEN_MSG_N[TEN.result] + 1);
  m->finish_story = 4;
  m->finish_goal = 1;
  m->finish_wait = TEN.beaten == TEN.total ? FINISH_GOAL_WAIT : FINISH_TIMEUP_WAIT;
  TEN.fade = TEN.beaten != TEN.total ? 1 : 0;
  TEN.phase = -1;
  /* the sprites to have up before they show (menu_finish_warm) */
  int s[MAX_TEN_WARM];
  const int anims[6] = { TEN.beaten == TEN.total ? ID.win : ID.ten_timeup, ID.ten_face, ID.ten_x, ID.ten_finish,
                         ID.ten_summary, -1 };
  for (int a = 0; anims[a] >= 0; ++a) {
    int ns = anm_sprites(anims[a], s, MAX_TEN_WARM);
    for (int i = 0; i < ns; ++i) ten_warm_add(s[i]);
  }
  for (int i = 0; i < ten_faces(); ++i) ten_warm_add(anm_sprite(ten_face_id(i)));
  ten_warm_add(anm_sprite(T_TEN_OR + ten_extend()));
  ten_warm_add(anm_sprite(TEN_COURSE_NAME[TEN.course]));
  ten_warm_add(anm_sprite(TEN_LEVEL_NAME[TEN.level]));
  for (int k = 0; k < 3; ++k) ten_warm_add(anm_sprite(TEN_MSG[TEN.result][TEN.msg][k]));
  for (int d = 0; d < 10; ++d) ten_warm_add(anm_sprite(RES_BIG_DIGIT + d));
}

/* one frame of the 10-in-a-row's end from its counter t (after the wait); 0 when it is over */
static int ten_result_step(int t) {
  if (TEN.fade && TEN.fade < TEN_FADE) {
    TEN.fade++;
    return 1;
  }
  if (TEN.phase < 0) {
    TEN.phase = 0;
    ten_phase_start();
  }
  (void)t;
  if (!ten_phase_update()) {
    if (++TEN.phase > 2) return 0;
    ten_phase_start();
  }
  return 1;
}

/* The story result (clRaceResultConsole_story, point_disp 0x85de618): from its counter's t0
 * (30 after WIN, 0 after LOSE; point_calc), at t0+121 the advantage row (menu_g_finish_tg_result's
 * first frame: |advantage| m, a minus when not ahead), at t0+180 the gauge's frame
 * (menu_g_finish_point_waku), its title (menu_g_finish_st_moji 0) and the gauge
 * (menu_g_finish_st_bar: frame = points / 5) at the points before the race; at t0+220 it
 * counts up 4 frames a tick (SE 0x43 looped till there); full, it goes on 1 a frame through
 * the flash to 299, SE 0x22 at 200 and 10 frames on the title gives way to the next tuning's
 * message (frames 80..158; T <= 19 basic, then the car's tuner: get_tuner_data). The console
 * lasts 7 s (its timer from point_calc) with no skip. */
enum { ST_RES_LEN = 420, ST_RES_ADV = 121, ST_RES_GAUGE = 180, ST_RES_COUNT = 220, ST_BAR_STEP = 4,
       ST_BAR_FULL = 200, ST_BAR_END = 299, ST_LOSE_LEN = 180 };
enum { T_ADV_MINUS = JID(0xafd), T_ADV_D0 = JID(0x1843), T_ADV_D1 = JID(0x1844), T_ADV_COMMA = JID(0xafc), T_ADV_D2 = JID(0x1845),
       T_ADV_D3 = JID(0x1846), T_ADV_D4 = JID(0x1847), T_ST_GET = JID(0x1d47), T_ST_GET2 = JID(0x1f3c),
       ADV_DIGIT = JID(1193) /* menu_p_finish_0..9 (0x93d0260) */ };
static int st_t0(const menu_state *m) { return m->finish_story == 1 ? 30 : 0; }
static int st_full(const menu_state *m) { return m->st_prev + m->st_gain >= 1000; }
/* the counter the gauge fills at (point_disp's +0x520: then SE 0x22, 10 on the message) */
static int st_fill_at(const menu_state *m) { return ST_RES_COUNT + (int)(m->st_gain * 0.2f / ST_BAR_STEP) + 10; }

/* the gauge's frame at counter t from t0 (+0x514 stepped toward +0x518) */
static int st_bar_frame(const menu_state *m, int t) {
  int pos = m->st_prev / 5;
  if (t <= ST_RES_COUNT || !m->st_gain) return pos;
  int target = st_full(m) ? ST_BAR_END : pos + (int)(m->st_gain * 0.2f);
  for (int k = ST_RES_COUNT + 1; k < t; ++k) {
    if (pos >= ST_BAR_FULL) pos++;
    else if (pos != target) pos += ST_BAR_STEP;
    if (pos > target) pos = target;
  }
  return pos;
}

static void st_result_step(const menu_state *m, int t) {
  if (!m->st_show) return;
  t -= st_t0(m);
  if (t == ST_RES_COUNT && m->st_gain) snd_menu_loop(SND_TUNING_BAR, 1);
  if (t <= ST_RES_COUNT || !m->st_gain) return;
  if (st_full(m)) {
    if (t == st_fill_at(m) - 10) {
      snd_menu_loop(SND_TUNING_BAR, 0);
      snd_menu(SND_CUTIN2);
    }
  } else if (st_bar_frame(m, t) >= st_bar_frame(m, ST_RES_LEN) && st_bar_frame(m, t - 1) < st_bar_frame(m, ST_RES_LEN)) {
    snd_menu_loop(SND_TUNING_BAR, 0);
  }
}

static void draw_st_result(const menu_state *m, int t) {
  t -= st_t0(m);
  if (t >= ST_RES_ADV) {
    /* point_calc's digits: ten thousands .. ones with a comma from 1000, leading zeros blank
     * and the minus just before the first digit, blank too when ahead */
    static const int SLOT[7] = { T_ADV_MINUS, T_ADV_D0, T_ADV_D1, T_ADV_COMMA, T_ADV_D2, T_ADV_D3, T_ADV_D4 };
    int v = (int)fabsf(m->st_gap);
    if (v > 99999) v = 99999;
    int d[7] = { 10, v / 10000 % 10, v / 1000 % 10, v > 999 ? 12 : 11, v / 100 % 10, v / 10 % 10, v % 10 };
    if (!d[1]) { d[1] = 10; d[0] = 11;
      if (!d[2]) { d[3] = 10; d[2] = 11; d[1] = 11;
        if (!d[4]) { d[4] = 10; d[3] = 11;
          if (!d[5]) { d[5] = 10; d[4] = 11; } } } }
    if (m->st_gap > 0)
      for (int k = 0; k < 7; ++k)
        if (d[k] == 10) d[k] = 11;
    anm_swap sw[7];
    for (int k = 0; k < 7; ++k) {
      sw[k].from = anm_sprite(SLOT[k]);
      sw[k].to = d[k] == 11 ? -1 : anm_sprite(d[k] < 10 ? ADV_DIGIT + d[k] : d[k] == 10 ? T_ADV_MINUS : T_ADV_COMMA);
    }
    anm_draw_ex(ID.st_adv, 0, 1, 0, 0, sw, 7);
    /* the win streak (getStorySuccessiveVictoryCount), "111 WINS" under the advantage as the arcade's WIN result has it, from the second in a row */
    if (m->finish_story == 1 && m->finish_streak >= 2) {
      float r[4], al, y = 200.0f;
      if (anm_sprite_rect_at(ID.st_adv, 0, anm_sprite(T_ADV_D4), 0, r, &al)) y = r[3] + 12.0f;
      char s[16];
      snprintf(s, sizeof s, "%d", m->finish_streak > 99999 ? 99999 : m->finish_streak);
      const float a = t - ST_RES_ADV >= 20 ? 1.0f : (float)(t - ST_RES_ADV + 1) / 20.0f;
      float x = 320.0f - 0.5f * (ex_width(5, s) + 10.0f + ex_width(5, "WINS"));
      x = ex_text(5, s, x, y, a);
      ex_text(5, "WINS", x + 10.0f, y, a);
    }
  }
  if (!m->st_show || t < ST_RES_GAUGE) return;
  anm_draw(ID.st_waku, 0, 1);
  int fill = st_full(m) ? st_fill_at(m) : 1 << 30;
  if (t >= fill) {
    int f = 80 + (t - fill < 78 ? t - fill : 78);
    anm_swap sw[2] = { { anm_sprite(T_ST_GET), anm_sprite(m->st_get) }, { anm_sprite(T_ST_GET2), anm_sprite(m->st_get) } };
    anm_draw_ex(ID.st_moji, f, 1, 0, 0, sw, 2);
  } else {
    anm_draw(ID.st_moji, 0, 1);
  }
  anm_draw(ID.st_bar, st_bar_frame(m, t), 1);
}

/* the counter of the first best row with a new record (CONGRATULATIONS starts with it), or -1 */
static int congrats_at(const menu_result *r) {
  return r->new_course ? RES_COURSE_BEST : r->new_car ? RES_CAR_BEST : r->new_personal ? RES_PERSONAL : -1;
}

static int finish_leave(menu_state *m) {
  m->finish = m->finish_out = 0;
  back_from_race(m, 0);
  return MENU_LEAVE;
}

int menu_finish_update(menu_state *m, float dt) {
  m->acc += dt;
  if (m->acc > 0.25f) m->acc = 0.25f;
  while (m->finish && m->acc >= 1.0f / 60.0f) {
    m->acc -= 1.0f / 60.0f;
    int tap = m->tap;
    m->tap = 0;
    int t = m->finish++ - m->finish_wait; /* clTaResult's or clTaTimeUp's counter */
    if (t < 0) continue;
    if (!m->finish_goal) {
      if (t >= TIMEUP_FADE + TIMEUP_LEN + TIMEUP_BLACK) return finish_leave(m);
      continue;
    }
    if (m->finish_out) {
      if (++m->finish_out > RES_OUT_FADE + RES_OUT_BLACK) return finish_leave(m);
      continue;
    }
    if (m->finish_story == 4) {
      if (!ten_result_step(t)) m->finish_out = 1;
      continue;
    }
    if (m->finish_story == 3) {
      /* Suspend: clRaceThread::race fades out over 1/3 s (getFade; none: at once) and returns,
       * no result and the race's BGM on; clRaceThread::run sets up the select's next period */
      if (!m->finish_fade || t + 1 >= STORY_FADE) return finish_leave(m);
      continue;
    }
    if (m->finish_story >= 5) { /* a versus: (TIME UP first, when the clock ended it,) the place, then out (a tap from 2 s) */
      int tv = t - (m->vs.timeup ? TIMEUP_FADE + TIMEUP_LEN : 0);
      if (tv < 0) continue;
      if (tv == 0) snd_menu(m->finish_story == 5 ? SND_FINISH : SND_FINISH2);
      if (tv >= 300 || (tap && tv >= 120)) m->finish_out = 1;
      continue;
    }
    if (m->finish_story) {
      /* WIN plays its 150 frames, LOSE its first 180, under the advantage and the points */
      if (t == 0) snd_menu(m->finish_story == 1 ? SND_FINISH : SND_FINISH2);
      st_result_step(m, t);
      if (t >= ST_RES_LEN) {
        snd_menu_loop(SND_TUNING_BAR, 0);
        m->finish_out = 1;
      }
      continue;
    }
    if (t == 0) snd_menu(SND_FINISH);
    if (t == RES_FINISH2) snd_menu(SND_FINISH2);
    if (t == RES_COURSE || t == RES_COURSE_BEST || t == RES_CAR_BEST || t == RES_PERSONAL || t == RES_SECTIONS ||
        t == RES_TOTAL || t == RES_SPEED)
      snd_menu(SND_CUTIN2);
    if (t > RES_TIMEOUT || (tap && t > RES_SPEED)) m->finish_out = 1;
  }
  return MENU_PAUSED;
}

static void draw_result(const menu_state *m, int t) {
  const menu_result *r = &m->result;
  if (t < RES_FINISH_LEN) anm_draw(ID.finish[1], t, 1);
  if (t >= RES_COURSE) anm_draw_ex(ID.res_course, 0, 1, 0, 0, RES.course, 2);
  static const int AT[3] = { RES_COURSE_BEST, RES_CAR_BEST, RES_PERSONAL };
  const int news[3] = { r->new_course, r->new_car, r->new_personal };
  for (int k = 0; k < 3; ++k) {
    if (t < AT[k]) continue;
    /* a new record's row loops (its NEW mark pulses), the others play once and hold */
    int f = t - AT[k];
    anm_draw_ex(ID.res_best[k], news[k] ? f % RES_ROW_LEN : f < RES_ROW_LEN ? f : RES_ROW_LEN - 1, 1, 0, 0, RES.best[k], 8);
  }
  int c = congrats_at(r);
  if (c >= 0 && t >= c) {
    c = t - c;
    anm_draw(ID.res_congrats, c < RES_CONGRATS_LEN ? c : RES_CONGRATS_LOOP + (c - RES_CONGRATS_LEN) % (RES_CONGRATS_LEN - RES_CONGRATS_LOOP), 1);
  }
  if (t >= RES_SECTIONS) anm_draw_ex(ID.res_sections, 0, 1, 0, 0, RES.sections, 4 * 7);
  /* clTaResult::run: section_2 too on course ids 12 and 13 (Tokyo, Kanagawa) */
  int id = car_course_id(m->course);
  if (t >= RES_SECTIONS && (id == 12 || id == 13) && ID.res_sections2 >= 0)
    anm_draw_ex(ID.res_sections2, 0, 1, 0, 0, RES.sections2, 3 * 7);
  if (t >= RES_TOTAL) {
    int f = t - RES_TOTAL;
    anm_draw_ex(ID.res_total, f < RES_ROW_LEN ? f : RES_TOTAL_LOOP + (f - RES_ROW_LEN) % (RES_ROW_LEN - RES_TOTAL_LOOP), 1, 0, 0,
                RES.total, 7);
  }
  if (t >= RES_SPEED) {
    anm_draw_ex(ID.res_speed, 0, 1, 0, 0, RES.speed, 3);
    /* the ranking only for a place in the top 100 (the password only with a card) */
    if (r->rank >= 1 && r->rank <= 100) anm_draw_ex(ID.res_rank, 0, 1, 0, 0, RES.rank, 3);
  }
}

void menu_finish_draw(const menu_state *m) {
  if (!m->finish || m->finish <= m->finish_wait) return;
  int t = m->finish - 1 - m->finish_wait;
  if (!m->finish_goal) {
    anm_fill(0, 0, 0, t < TIMEUP_FADE ? (float)(t + 1) / TIMEUP_FADE : 1.0f);
    if (t >= TIMEUP_FADE && t < TIMEUP_FADE + TIMEUP_LEN) anm_draw(ID.finish[0], t - TIMEUP_FADE, 1);
    return;
  }
  if (m->finish_story == 3) {
    float g = m->finish_fade == 2 ? 1.0f : 0.0f;
    if (m->finish_fade) anm_fill(g, g, g, t + 1 >= STORY_FADE ? 1.0f : (float)(t + 1) / STORY_FADE);
  } else if (m->finish_out) anm_fill(0, 0, 0, m->finish_out < RES_OUT_FADE ? (float)m->finish_out / RES_OUT_FADE : 1.0f);
  else if (m->finish_story >= 5) {
    /* clRaceFinishConsole (update 0x85d23d8): once anyone is home, menu_g_finish_win 90..99 then
     * 1..59 (held), its 0xa44 the place (0x93cfc34: cons_p_vs_place_1st..4th) and its 0xa47 the
     * machines in the race (0x93cfc44 by clNet::getJoinCount: s_2..s_4); behind the first, its
     * clMenuFade darkens the screen to black over a second (setFade 1 -> 0, 60 frames) */
    static const int PLACE[4] = { JID(2625), JID(2626), JID(2627), JID(2628) },
                     OF[5] = { 1, 1, JID(2629), JID(2630), JID(2631) };
    int place = m->vs.last_place < 1 ? 1 : m->vs.last_place > 4 ? 4 : m->vs.last_place;
    int n = m->vs.last_n < 0 ? 0 : m->vs.last_n > 4 ? 4 : m->vs.last_n;
    if (m->vs.timeup) { /* TIME UP over the race running on, then black, the places after it */
      if (t < TIMEUP_FADE + TIMEUP_LEN) {
        anm_fill(0, 0, 0, t < TIMEUP_FADE ? (float)(t + 1) / TIMEUP_FADE : 1.0f);
        if (t >= TIMEUP_FADE) anm_draw(ID.finish[0], t - TIMEUP_FADE, 1);
        return;
      }
      t -= TIMEUP_FADE + TIMEUP_LEN;
    }
    if (m->vs.timeup) anm_fill(0, 0, 0, 1.0f);
    else if (place > 1) anm_fill(0, 0, 0, t + 1 >= 60 ? 1.0f : (float)(t + 1) / 60.0f);
    int f = t < 10 ? 90 + t : 1 + (t - 10 < 58 ? t - 10 : 58);
    anm_swap sw[2] = { { anm_sprite(JID(0xa44)), anm_sprite(PLACE[place - 1]) }, { anm_sprite(JID(0xa47)), anm_sprite(OF[n]) } };
    anm_draw_ex(ID.finish_vs >= 0 ? ID.finish_vs : ID.win, f, 1, 0, 0, sw, 2);
  } else if (m->finish_story == 1 || m->finish_story == 2) {
    int a = m->finish_story == 1 ? ID.win : ID.lose, len = m->finish_story == 1 ? anm_frames(a) : ST_LOSE_LEN;
    anm_draw(a, t < len ? t : len - 1, 1);
    draw_st_result(m, t);
  } else if (m->finish_story == 4) draw_ten_result();
  else draw_result(m, t);
}

/* The end's sprites, one a frame from when the race stops running (the first time they show
 * they would take that frame some 50 ms), those up kept from anm_trim; in the order they
 * show, the templates left out (digits and names take their place). */
static int is_template(int id) {
  return (id >= 7264 && id <= 7271) || (id >= 7275 && id <= 7298) || id == T_RES_NEW || id == T_RES_COURSE || id == T_RES_TIME;
}

void menu_finish_warm(int goal) {
  if (goal == 3) {
    int fresh = 0;
    for (int i = 0; i < TEN.nwarm; ++i) fresh += anm_warm_sprite(TEN.warm[i], !fresh);
    return;
  }
  enum { MAX_WARM = 256 };
  static int list[3][MAX_WARM], n[3] = { -1, -1, -1 };
  int g = goal == 2 ? 2 : goal != 0;
  if (n[g] < 0) {
    n[g] = 0;
    int extra[40], ne = 0;
    for (int d = 0; d < 10; ++d) extra[ne++] = RES_DIGIT + d;
    for (int d = 0; d < 10; ++d) extra[ne++] = RES_BIG_DIGIT + d;
    extra[ne++] = RES_NEW;
    for (int k = 0; k < N_COURSES; ++k) extra[ne++] = COURSE_NAME[car_course_id(k)];
    extra[ne++] = RES_TIME_NAME[0];
    extra[ne++] = RES_TIME_NAME[1];
    const int anims[10] = { ID.finish[1], ID.res_course, ID.res_best[0], ID.res_best[1], ID.res_best[2],
                            ID.res_congrats, ID.res_sections, ID.res_total, ID.res_speed, ID.res_rank };
    int sprites[MAX_WARM];
    const int story[6] = { ID.win, ID.lose, ID.st_adv, ID.st_waku, ID.st_bar, ID.st_moji };
    for (int a = 0; a < (g == 2 ? 6 : g ? 10 : 1); ++a) {
      int ns = anm_sprites(g == 2 ? story[a] : g ? anims[a] : ID.finish[0], sprites, MAX_WARM);
      /* the digits and the names right after FINISH */
      if (g == 1 && a == 1)
        for (int i = 0; i < ne && ns < MAX_WARM; ++i) sprites[ns++] = anm_sprite(extra[i]);
      /* the story result's advantage digits and tuning messages with its row */
      if (g == 2 && a == 2) {
        for (int d = 0; d < 10 && ns < MAX_WARM; ++d) sprites[ns++] = anm_sprite(ADV_DIGIT + d);
        for (int k = 0; k < 18 && ns < MAX_WARM; ++k) sprites[ns++] = anm_sprite(STTUNE_GET[k]);
      }
      for (int i = 0; i < ns; ++i) {
        int s = sprites[i], seen = s < 0 || is_template(anm_sprite_id(s));
        for (int k = 0; k < n[g] && !seen; ++k) seen = list[g][k] == s;
        if (!seen && n[g] < MAX_WARM) list[g][n[g]++] = s;
      }
    }
  }
  int fresh = 0;
  for (int i = 0; i < n[g]; ++i) fresh += anm_warm_sprite(list[g][i], !fresh);
}

void menu_pause_draw(const menu_state *m) {
  const menu_pause *p = &m->pause;
  if (!p->open) return;
  float a = p->open < PAUSE_IN ? (float)p->open / PAUSE_IN : 1.0f;
  int t = p->deciding ? DECIDE_DAY + FADE - p->deciding : 0;
  /* after YES is decided the box fades away; after NO the screen goes white */
  if (p->deciding && !p->sel && t >= DECIDE_DAY) a = (float)p->deciding / FADE;
  anm_fill(0, 0, 0, 0.55f * a);
  int panel = anm_sprite(T_PANEL), title = anm_sprite(T_CONTINUE);
  static const float top_src[4] = { 81, 20, 431, 146 }, top_dst[4] = { 145, 128, 495, 259 };
  static const float bottom_src[4] = { 81, 150, 431, 240 }, bottom_dst[4] = { 145, 259, 495, 353 };
  static const float title_src[4] = { 1, 1, 147, 30 }, title_dst[4] = { 247, 140, 393, 169 };
  anm_draw_part(panel, top_src, top_dst, a);
  anm_draw_part(panel, bottom_src, bottom_dst, a);
  anm_draw_part(title, title_src, title_dst, a);
  int z = 100 * p->sel, f = z + p->card_t % 60;
  if (p->slide) f = z + 90 + SLIDE - p->slide;
  if (p->deciding) f = z + 60 + (t < DECIDE_DAY ? t : DECIDE_DAY - 1);
  anm_swap cards[2] = { { anm_sprite(T_MORNING), anm_sprite(T_YES) }, { anm_sprite(T_NIGHT), anm_sprite(T_NO) } };
  anm_swap rings[2] = { { anm_sprite(T_RING), anm_sprite(JID(0xb95)) }, { anm_sprite(T_RING_BLUE), anm_sprite(JID(0x1b0b)) } };
  anm_draw_xf(ID.day_cards, f, a, PAUSE_DX, PAUSE_DY, PAUSE_K, cards, 2);
  if (!p->slide) anm_draw_xf(ID.day_ring, z + (p->card_t < 59 ? p->card_t : 59), a, PAUSE_DX, PAUSE_DY, PAUSE_K, rings, 2);
  if (p->deciding && p->sel && t >= DECIDE_DAY) anm_fill(1, 1, 1, (float)(t - DECIDE_DAY + 1) / FADE);
}

/* ---- the race's title page (clSeqTaCourseDecideThread), up while the course loads ----
 * menu_g_pre_bg with the course's background for the time zone (template 0xb2f),
 * menu_layout_pre_ta with the course's name (0x1b9a) and the time zone's (0x15ed),
 * menu_g_pre_sound with the race's song (font template 0xbaf, logo 0x1a96). */
#define PRE_LEN 150 /* frames it shows at least */
/* nsMenuCheck::getCourseImageTaBg (morning, night) and getCourseImageTaCourseName by course
 * id (0x93ddac0), getTimeSprite4CourseDecide by time zone (0x93dde34) */
static const int PRE_BG[2][15] = {
  { JID(2863), JID(2866), JID(2869), JID(2865), JID(2870), JID(2873), JID(2880), JID(2880), JID(2357), JID(2355), JID(2885), JID(2885), JID(3930), JID(6244), JID(2359) },
  { JID(3901), JID(3904), JID(3907), JID(3903), JID(3908), JID(3911), JID(3918), JID(3918), JID(3293), JID(3292), JID(3923), JID(3923), JID(5608), JID(6779), JID(3295) },
};
static const int PRE_NAME[15] = { JID(3931), JID(3932), JID(3933), JID(3934), JID(3936), JID(3936), JID(5610), JID(3937), JID(3944), JID(3939), JID(5611), JID(3941), JID(3942), JID(3943), JID(5612) };
static const int PRE_TIME[2] = { JID(4751), JID(3301) };
enum { T_PRE_BG = JID(0xb2f), T_PRE_NAME = JID(0x1b9a), T_PRE_TIME = JID(0x15ed), T_PRE_LOGO = JID(0x1a96), T_LOGO_MAXI3 = JID(0x1643),
       T_PRE_TITLE_TA = JID(2909), S_PRE_TITLE_VS = JID(2910) };

/* A story race's title (clSeqStEnemyInformationThread): menu_g_pre_bg with its ramp's
 * picture, menu_layout_pre_st with the episode's picture and title, its rivals' cars and
 * "<number> WA" in the digit templates (src/storypre.h); menu_g_pre_sound_st. */
enum { T_ST_PICTURE = JID(0x1e10), T_ST_TITLE = JID(0x1295), T_ST_CAR = JID(0x1e0d), T_ST_CAR_SPOT = JID(3947), T_ST_DIGIT = JID(0x1d1d),
       T_ST_WA = JID(0xdb0) };
#define STORY_NUMBER_DX 66.0f

static int draw_pre_story(const menu_state *m, float fade) {
  if (m->game != MENU_GAME_STORY || m->story_sel < 0 || m->story_sel >= STORYPRE_N || ID.pre_st < 0) return 0;
  const storypre_select *s = &STORYPRE[m->story_sel];
  anm_swap bg = { anm_sprite(T_PRE_BG), anm_sprite(s->bg[m->zone != 0]) };
  draw_photo_bg(&bg, fade);
  anm_swap sw[13];
  int n = 0;
  sw[n++] = (anm_swap){ anm_sprite(T_ST_PICTURE), anm_sprite(s->picture) };
  sw[n++] = (anm_swap){ anm_sprite(T_ST_TITLE), anm_sprite(s->title) };
  for (int k = 0; k < 3; ++k) sw[n++] = (anm_swap){ anm_sprite(T_ST_CAR + k), s->cars[k] > 1 ? anm_sprite(s->cars[k]) : -1 };
  sw[n++] = (anm_swap){ anm_sprite(T_ST_CAR_SPOT), -1 };
  for (int k = 0; k < 6; ++k) sw[n + k] = (anm_swap){ anm_sprite(k < 5 ? T_ST_DIGIT + k : T_ST_WA), -1 };
  anm_draw_ex(ID.pre_st, 0, fade, 0, 0, sw, n + 6);
  /* The number: this data's title sprite reads "Chapter" where the arcade's digit places
   * begin, so the digits go after it (STORY_NUMBER_DX) and WA is left out. Drawn on its own,
   * everything but the digit places hidden. */
  int nd = 0;
  anm_swap num[6 + 6];
  char text[8];
  snprintf(text, sizeof text, "%d", s->episode);
  int len = (int)strlen(text);
  for (int k = 0; k < 6; ++k)
    num[nd++] = (anm_swap){ anm_sprite(k < 5 ? T_ST_DIGIT + k : T_ST_WA), k < len && k < 5 ? anm_sprite(STORYPRE_DIGIT[text[k] - '0']) : -1 };
  for (int k = 0; k < n; ++k) num[nd++] = (anm_swap){ sw[k].from, -1 };
  anm_draw_ex(ID.pre_st, 0, fade, STORY_NUMBER_DX, 0, num, nd);
  return 1;
}

/* the 10-in-a-row's (clThread_7_3_LoadDisp::run 0x88a8630): menu_layout_pre_or with the
 * course's background, name, time zone and start ramp by level (src/tendb.h, 0x940d420) and
 * the level (0x940d3e0) in 0x186f */
static int draw_pre_ten(const menu_state *m, float fade) {
  if (m->game != MENU_GAME_TEN || ID.pre_or < 0) return 0;
  const int *p = TENDB_PRE[m->ten_course][m->ten_level];
  anm_swap bg = { anm_sprite(T_PRE_BG), anm_sprite(p[0]) };
  draw_photo_bg(&bg, fade);
  anm_swap sw[4] = { { anm_sprite(T_PRE_NAME), anm_sprite(p[1]) }, { anm_sprite(T_PRE_TIME), anm_sprite(p[2]) },
                     { anm_sprite(JID(0x186f)), anm_sprite(TENDB_PRE_LV[m->ten_level]) }, { anm_sprite(JID(0x1870)), anm_sprite(p[3]) } };
  anm_draw_ex(ID.pre_or, 0, fade, 0, 0, sw, 4);
  return 1;
}

/* (vs: the versus', the time attack's page with menu_p_pre_title_vs for its title) */
static void draw_pre_as(const menu_state *m, float fade, int vs) {
  int k = car_course_id(m->course), z = m->zone;
  int story = !vs && (draw_pre_story(m, fade) || draw_pre_ten(m, fade));
  if (!story) {
    anm_swap bg = { anm_sprite(T_PRE_BG), anm_sprite(PRE_BG[z][k]) };
    draw_photo_bg(&bg, fade);
    anm_swap ta[3] = { { anm_sprite(T_PRE_NAME), anm_sprite(PRE_NAME[k]) }, { anm_sprite(T_PRE_TIME), anm_sprite(PRE_TIME[z]) },
                       { anm_sprite(T_PRE_TITLE_TA), anm_sprite(S_PRE_TITLE_VS) } };
    anm_draw_ex(ID.pre_ta, 0, fade, 0, 0, ta, vs ? 3 : 2);
  }
  /* getBgmName / getBgmIcon of the song drawn (NO BGM has no logo) */
  int set = bgm_set(m), line = m->song > 0 ? m->song : bgm_lines(m) - 1;
  anm_swap snd[2] = { { anm_sprite(T_BGM_FONT), anm_sprite(BGM_TEXT + 30 * set + line) },
                      { anm_sprite(T_PRE_LOGO), m->song > 0 ? anm_sprite(BGM_SETS[set].logo) : -1 } };
  if (ten_pick(m)) { /* (the 10 out run's: the soundtrack picked, from its list) */
    ten_bgm_art(m->ten_set >= 0 && m->ten_set < 5 ? m->ten_set + 1 : TEN_BGM_LINES - 1, &snd[0].to, &snd[1].to);
  }
  /* the name slides in once (30 frames) and stays */
  int sound = story && ID.pre_sound_st >= 0 ? ID.pre_sound_st : ID.pre_sound;
  int last = anm_frames(sound) - 1;
  anm_draw_ex(sound, m->t < last ? m->t : last, fade, 0, 0, snd, 2);
}

static void draw_pre(const menu_state *m, float fade) { draw_pre_as(m, fade, 0); }

/* ---- the story's level and rival select ----
 * seqStEnemyLevelSelect (0x8835c2c, its clSeqStEnemyLevelSelectThread) and seqStEnemyCharaSelect
 * (clThread_5_2_EnemySelect::run 0x88395ec) on the card's story progress (clV386CardData:
 * getStoryClearDivCount C, the loop C / 5, the division D = C % 5, the division's 20 cleared
 * bits). Episode n of a loop is 20 D + 5 level + slot; its story index get_story_index(loop, n).
 * - menu_layout_st_level: the four levels C, B, A, A+ as a spin box (as menu_layout_mode_sel_3:
 *   k*100 idle, +60 decided, +90..94 on to k+1). clMenuSelect puts the levels' names in
 *   0x1645.., the lock over a level not open yet in 0x1c98.. (levels 2..4), hides the class
 *   bubbles 0xb72.. after the first loop's first division and the layout's own arrows (for
 *   menu_layout_st_level_arrow's, k*100, 60 a loop), and colours the backlights 0x1b0c...
 * - menu_layout_st_level_clear (k*100 + 1, 59 a loop): CLEARED over the cleared levels, small
 *   (0x1b96..) and large (0x1b92..). menu_layout_st_level_ring: its flash once at rest.
 * - the level's five episodes at (100 + 110 i, 346), menu_g_st_lv_<colour>: open 1..59,
 *   cleared 101..159 (its frame the plain 0x760), locked 201..259, the picture in 0x1bb1; the
 *   line under them menu_g_st_lv_back 80..84.
 * - menu_layout_st_rival: the five episodes' places (2x2 markers 0x1bb1..0x1bb5), a spin box
 *   again. On each place a menu_g_st_rival_<colour> (its 2 centres an even number of title
 *   glyphs), the chosen one large (open 1, cleared 101, locked 201), the others small (+70):
 *   the picture in 0x96d (grey 0x164a), "第 <loop * 100 + n + 1> 話" in the glyph templates
 *   0x1e18..0x1e1e (the 2s: 0x1e1f first). menu_layout_st_rival_arrow: k*100 (60 a loop),
 *   4 + k when episode k + 1 is locked.
 * - menu_layout_st_rival_ramp: the chapter's name (font 4, for 0x49), START RAMP (0x580) and
 *   the episode's start ramp (0x59f), pre-rendered (src/storysel.h).
 * The arcade's 3D miniature of the course (clMiniatureCourse) is not drawn. */
enum {
  T_LV_NAME = JID(0x1645), T_LV_LOCK = JID(0x1c98), T_LV_FUKI = JID(0xb72), T_LV_ARROW = JID(0x1be), T_LV_ARROW2 = JID(0x383),
  T_LV_BACKLIGHT = JID(0x1b0c), T_LV_CLEAR_S = JID(0x1b96), T_LV_CLEAR_L = JID(0x1b92), S_CLEAR_S = JID(0x716), S_CLEAR_L = JID(0x4b5),
  S_LOCK = JID(0x75f), T_EP_PICTURE = JID(0x1bb1), S_EP_WAKU = JID(0x760), T_EP_STAGE = JID(0x96d), T_EP_GREY = JID(0x164a),
  T_EP_NAME = JID(0xfe9), T_EP_GLYPH = JID(0x1e18), T_EP_GLYPH7 = JID(0x1e1f), S_GLYPH_DIGIT = JID(4153), S_GLYPH_DAI = JID(5700),
  S_GLYPH_WA = JID(4902), T_RAMP_TITLE = JID(0x49), T_RAMP_LABEL = JID(0x580), T_RAMP_NAME = JID(0x59f), S_MES_LEVEL = JID(0x1704),
  S_MES_RIVAL = JID(0x1ab2),
};
static const int LV_BACKLIGHT[4] = { JID(0x1dbe), JID(0x1e69), JID(0x1ed8), JID(0x1cdd) };
static const int LV_WAKU[4] = { JID(0x1327), JID(0x1649), JID(0x1881), JID(0x1044) }; /* menu_p_st_lv_waku_<colour> (0x93fd020) */
#define ST_NONE 99 /* a tap on nothing */

/* (the testers' switch in place of the save's own division: menu.h story_test) */
static int st_div_of(const menu_state *m) { return m->story_test > 0 ? m->story_test - 1 : m->st_div; }
static int st_loop(const menu_state *m) { return st_div_of(m) / 5; }
static int st_division(const menu_state *m) { return st_div_of(m) % 5; }

/* clV386CardData::isStoryClear (0x86c5524): the divisions before the card's are cleared */
static int st_cleared(const menu_state *m, int n) {
  if (m->story_test) return 0;
  int r = n % 100, d = r / 20;
  if (d != st_division(m)) return d < st_division(m);
  return (m->st_bits >> (r % 20)) & 1;
}

/* get_story_index (0x8835590), also get_story_image_data's row: the loops after the first
 * run the tuner's story (100..110) in place of their first ten */
static int st_index(int loop, int n) {
  if (loop <= 0 || n > 9) return n;
  if (loop == 1 || (loop & 1)) return 100 + n;
  return n == 9 ? 110 : 100 + n;
}

/* seqStEnemyLevelSelect: each level's state and its episodes' (0 locked, 1 open, 2 cleared),
 * and where the cursor starts, on the first episode not cleared. In the first loop's later
 * divisions one episode opens after the other; otherwise a level's first four are open and
 * its fifth, the strong rival (IsStoryOpen 0x8835120), once those four are cleared. */
typedef struct { int level[4], ep[4][5], lv, slot; } st_states;
static void st_states_of(const menu_state *m, st_states *s) {
  int base = 20 * st_division(m), found = 0;
  memset(s, 0, sizeof *s);
  if (m->story_test) { /* (the testers' switch: every episode of the division open) */
    for (int lv = 0; lv < 4; ++lv) {
      s->level[lv] = 1;
      for (int k = 0; k < 5; ++k) s->ep[lv][k] = 1;
    }
    return;
  }
  if (st_division(m) > 0 && st_loop(m) == 0) {
    for (int lv = 0; lv < 4 && !found; ++lv) {
      s->level[lv] = 1;
      for (int k = 0; k < 5 && !found; ++k) {
        s->ep[lv][k] = st_cleared(m, base + 5 * lv + k) ? 2 : 1;
        if (s->ep[lv][k] == 1) s->lv = lv, s->slot = k, found = 1;
      }
      if (!found) s->level[lv] = 2;
    }
    return;
  }
  for (int lv = 0; lv < 4; ++lv) {
    int cleared = 0;
    s->level[lv] = 1;
    for (int k = 0; k < 4; ++k) {
      s->ep[lv][k] = st_cleared(m, base + 5 * lv + k) ? 2 : 1;
      if (s->ep[lv][k] == 2) cleared++;
      else if (!found) s->lv = lv, s->slot = k, found = 1;
    }
    if (st_cleared(m, base + 5 * lv + 4)) {
      s->ep[lv][4] = s->level[lv] = 2;
    } else {
      s->ep[lv][4] = cleared == 4;
      if (!found) s->lv = lv, s->slot = 4, found = 1;
    }
  }
}

void menu_story_clear(menu_state *m, int n) {
  if (n < 0 || m->story_test) return; /* (a tester's race clears nothing) */
  int r = n % 100;
  if (r / 20 == st_division(m)) m->st_bits |= 1 << (r % 20);
  /* the division's last episode: setStoryClearDivCount(+1), which clears the bits */
  if (r % 20 == 19) {
    m->st_div = m->st_div + 1 > 0x1391 ? m->st_div - 9 : m->st_div + 1;
    m->st_bits = 0;
  }
  m->garage_changed = 1;
}

/* the level's names: the division's, a later loop's first division the tuner's story's first */
static int st_level_name(const menu_state *m, int k) {
  return st_loop(m) > 0 && st_division(m) == 0 && k < 2 ? STORYSEL_TUNER[k] : STORYSEL_LEVEL[st_division(m)][k];
}

/* the spin boxes' frame (as the mode select's) */
static int st_frame(const menu_state *m) {
  if (m->from >= 0) return m->from < m->sel ? m->from * 100 + 91 + m->t : m->sel * 100 + 94 - m->t;
  if (m->deciding) {
    int t = DECIDE_MODE + FADE - m->deciding;
    return m->sel * 100 + 60 + (t < DECIDE_MODE ? t : DECIDE_MODE - 1);
  }
  return m->sel * 100 + m->t % 60;
}

/* where the rival select's episode k sits in the layout's frame f */
static int st_place(int f, int k, float *x, float *y) {
  float r[4];
  if (!anm_sprite_rect(ID.st_rival, f, anm_sprite(T_EP_PICTURE + k), r)) return 0;
  *x = r[0] + 1.0f;
  *y = r[1] + 1.0f;
  return 1;
}

/* Which way a tap points: -1, +1, 0 the chosen item, ST_NONE */
static int st_tap(const menu_state *m) {
  int f = st_frame(m), best = -1;
  float bd = 1e9f;
  if (m->screen == MENU_ST_LEVEL) {
    /* the levels' names, or the episodes below (the chosen level) */
    if (m->tap_y > 300.0f && m->tap_y < 410.0f) return 0;
    for (int k = 0; k < 4; ++k) {
      float r[4];
      if (!anm_sprite_rect(ID.st_level, f, anm_sprite(T_LV_NAME + k), r)) continue;
      float cx = (r[0] + r[2]) * 0.5f, cy = (r[1] + r[3]) * 0.5f, h = (r[2] - r[0]) * 0.5f;
      float d = hypotf(m->tap_x - cx, m->tap_y - cy);
      if (d < h + 10.0f && d < bd) bd = d, best = k;
    }
  } else {
    for (int k = 0; k < 5; ++k) {
      float x, y;
      if (!st_place(f, k, &x, &y)) continue;
      float d = hypotf(m->tap_x - x, m->tap_y - y);
      if (d < (k == m->sel ? 100.0f : 55.0f) && d < bd) bd = d, best = k;
    }
  }
  if (best < 0) return ST_NONE;
  return best == m->sel ? 0 : best < m->sel ? -1 : 1;
}

/* ---- the bottom bar's middle key (640 x 480; the bar's keys at 22..133 and 507..618): the time attack's TRAFFIC. ----*/
static const float CH_KEY[4] = { 205, 407.5f, 230, 35 };
static int ch_hit(const menu_state *m, const float *r) {
  return m->tap_x >= r[0] && m->tap_x < r[0] + r[2] && m->tap_y >= r[1] - 6 && m->tap_y < r[1] + r[3] + 6;
}
/* ---- the time attack's TRAFFIC key: the course select's bottom bar, in its middle; the pointer's tap on it is the view button (menu_pointer) ---- */
/* where the key is: the touch HUD's bar has its keys either side of the middle (the key in the middle); the arcade bar's glyphs are on its left, the key at its right end */
static const float TRAFFIC_RIGHT[4] = { 486, 407.5f, 130, 35 };
static const float *traffic_key(void) { return hud_style() == HUD_STYLE_TOUCH ? CH_KEY : TRAFFIC_RIGHT; }
static void draw_traffic(const menu_state *m, float screen_w, float screen_h) {
  const float k = 1.6f, *r = traffic_key();
  float px = 3.0f;
  const char *t = m->ta_traffic ? "TRAFFIC: ON" : "TRAFFIC: OFF";
  hud_ui_begin(screen_w, screen_h);
  while (px > 1.6f && hud_ui_text_width(t, px) > (r[2] - 10.0f) * k) px -= 0.2f;
  hud_ui_button(r[0] * k, r[1] * k, r[2] * k, r[3] * k, 0);
  hud_ui_text(t, (r[0] + 0.5f * r[2]) * k - 0.5f * hud_ui_text_width(t, px), (r[1] + 0.5f * r[3]) * k - 3.5f * px, px,
              1, m->ta_traffic ? 0.8f : 1, m->ta_traffic ? 0.3f : 1, 1);
  hud_ui_end();
}

static void st_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->from >= 0) {
    if (m->t >= MOVE_MODE) m->from = -1, m->t = 0;
    return;
  }
  int ok = in->ok;
#ifdef WMMT_EGL
  if (getenv("WMMT_ST_TAP_GO") && in->any) ok = 1; /* test: any press decides (a menu.bin without the episode pictures) */
#endif
  if (!in->dx && !ok) return;
  int d = ok ? 0 : in->dx;
  st_states s;
  st_states_of(m, &s);
  int rival = m->screen == MENU_ST_RIVAL;
  if (d == 0) {
    if (!(rival ? s.ep[m->st_level][m->sel] : s.level[m->sel])) {
      snd_menu(SND_CANCEL);
      return;
    }
    snd_menu(SND_SELECT);
    m->deciding = DECIDE_MODE + FADE;
    m->t = 0;
    return;
  }
  int to = m->sel + (d > 0 ? 1 : -1);
  if (to < 0 || to >= (rival ? 5 : 4)) return;
  if (rival && !s.ep[m->st_level][to]) return; /* the arrows stop before a locked episode */
  snd_menu(SND_CURSOR);
  m->from = m->sel;
  m->sel = to;
  m->t = 0;
}

/* on to the level select (from the mode select), its cursor on the level to play next */
static void st_level_enter(menu_state *m) {
  st_states s;
  st_states_of(m, &s);
  go(m, MENU_ST_LEVEL);
  m->sel = s.lv;
  m->card_t = 0;
}

static void st_rival_enter(menu_state *m) {
  st_states s;
  st_states_of(m, &s);
  go(m, MENU_ST_RIVAL);
  m->sel = 0;
  m->card_t = 0;
  if (s.lv == m->st_level) {
    m->sel = s.slot;
  } else {
    for (int k = 4; k >= 0; --k)
      if (s.ep[m->st_level][k] == 1) m->sel = k;
  }
}

/* ---- the story's points and tuning (clRaceResultConsole::point_calc, seqStTune) ---- */

/* a FULL TUNE car races the story on its full tune, a stock one on the story's steps */
void menu_story_tune(const menu_state *m, int *power, int *handling) {
  if (m->tune_full[m->maker][menu_slot(m)]) {
    menu_tune(m, power, handling);
    return;
  }
  *power = m->st_power[m->maker][menu_slot(m)];
  *handling = m->st_handling[m->maker][menu_slot(m)];
}

int menu_story_loop(const menu_state *m) { return st_loop(m); }

int menu_story_losses(const menu_state *m) { return m->st_lost_n == m->story_n ? m->st_lost : 0; }

void menu_story_lost(menu_state *m, int won) {
  if (!won && m->story_n >= 0) m->st_bonus_car[m->maker][menu_slot(m)] |= 8, m->garage_changed = 1; /* hasStoryLose */
  if (won || m->st_lost_n != m->story_n) m->st_lost = 0;
  m->st_lost_n = m->story_n;
  if (!won && m->st_lost < 99) m->st_lost++;
}

/* ---- the story's poem (seqStPoem 0x883e1d8, clStPoem; "5-9. finish: one picture"): mseqStory
 * after a won story select (getRank 0), after the tuning screens (seqStTune, seqStTuneEff) and
 * before the ending (select 99) and the dress-up: the select's picture (the table at 0x93fd420
 * by getStorySelect, 0 none) played once (setAnime, layer 4) between clMenuFade's fade in and
 * out, no tap skipping it; then on as from the tuning work. ---- */
static const struct { short select; const char *anim; } POEMS[] = {
  { 9, "menu_p_poem_10" }, { 14, "menu_p_poem_15" }, { 19, "menu_p_poem_20" }, { 29, "menu_p_poem_30" },
  { 39, "menu_p_poem_90" }, { 49, "menu_p_poem_40" }, { 59, "menu_p_poem_60" }, { 69, "menu_p_poem_80" },
  { 79, "menu_p_poem_50" }, { 84, "menu_p_poem_isida85" }, { 89, "menu_p_poem_zero90" }, { 99, "menu_p_poem_fd100" },
  { 109, "menu_p_poem_tuner110" }, { 110, "menu_p_poem_tuner110" },
};
#define N_POEMS ((int)(sizeof POEMS / sizeof POEMS[0]))
#define POEM_FADE 60 /* clMenuFade's 1 s */

static int poem_anim(const menu_state *m) { return m->poem >= 0 && m->poem < N_POEMS ? anm_find(POEMS[m->poem].anim) : -1; }

static void poem_open(menu_state *m) {
  fprintf(stderr, "story poem: select %d, %s\n", m->story_pick, m->poem >= 0 ? POEMS[m->poem].anim : "-");
  go(m, MENU_POEM);
  m->poem_t = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static int tg_due(menu_state *m);

/* on as from the tuning work: the dress-up screens due, else the game mode select */
static void ending_open(menu_state *m);

static void poem_leave(menu_state *m) {
  m->poem = -1;
  if (m->ending == 1) {
    ending_open(m);
    return;
  }
  if (tg_due(m)) return;
  go(m, MENU_GAME);
  m->sel = game_slot(MENU_GAME_STORY);
  m->slide = m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

static void poem_step(menu_state *m) {
  int id = poem_anim(m), len = id >= 0 ? anm_frames(id) : 0;
  if (id >= 0 && m->poem_t == 0) anm_preload(id);
  if (++m->poem_t >= POEM_FADE + len + POEM_FADE || id < 0) poem_leave(m);
}

static void draw_poem(const menu_state *m) {
  int id = poem_anim(m), len = id >= 0 ? anm_frames(id) : 0, t = m->poem_t;
  anm_fill(0, 0, 0, 1);
  if (id < 0 || len <= 0) return;
  int f = t - POEM_FADE;
  anm_draw(id, f < 0 ? 0 : f >= len ? len - 1 : f, 1);
  float a = t < POEM_FADE ? (float)t / POEM_FADE : t >= POEM_FADE + len ? 1.0f - (float)(t - POEM_FADE - len) / POEM_FADE : 1.0f;
  anm_fill(0, 0, 0, 1.0f - (a < 0 ? 0 : a));
}

/* ---- the story's ending (seqRaceEnding 0x8906b68): mseqStory's after episode 100 (the
 * select in its loop 99) is won, after the poem: clSeqNowLoading, then clRaceThread in play
 * mode 3 (clRaceThread::initEnding 0x88f860c): clEffStoryType 111's period 0, a realtime
 * movie (MovieTableData 90, KgAirport_Out: the player's car and two 964s, series 145), the
 * BGM getBgm 54 (maxi3/sys_05) from frame 150; clEndingConsole (0x85ca700, update 0x85cba50)
 * plays its 56 events (0x93cf260: frame, cluster, anime, from, frames, screen) over it: black
 * masks (menu_layout_ed_base_*), the staff roll (_roll_1..7), fades and the BANDAI NAMCO logo
 * (_bng). A roll's screen 1 or 2 puts the movie in the bottom or top 640 x 192
 * (clSubScreen::setScreen_), 0 back on the whole screen. Its button (0x40; here a tap) jumps
 * to frame 9420 (the logo; clRaceThread::race fades the BGM out over 120 frames there), the
 * race ends at 9780. Then the menus as after the poem. The port shows the band as the movie's
 * own picture there (the rest black) where the arcade renders the movie into it. ---- */
enum { ED_FADE_BLACK, ED_FADE_WHITE, ED_BNG, ED_BASE1, ED_ROLL1 = ED_BASE1 + 4, ED_NONE = ED_ROLL1 + 7, ED_ANIMS };
static const char *const ED_NAME[ED_NONE] = {
  "menu_layout_ed_fade_black", "menu_layout_ed_fade_white", "menu_layout_ed_bng", "menu_layout_ed_base_1",
  "menu_layout_ed_base_2", "menu_layout_ed_base_3", "menu_layout_ed_base_4", "menu_layout_ed_roll_1",
  "menu_layout_ed_roll_2", "menu_layout_ed_roll_3", "menu_layout_ed_roll_4", "menu_layout_ed_roll_5",
  "menu_layout_ed_roll_6", "menu_layout_ed_roll_7",
};
#define FB ED_FADE_BLACK
#define FW ED_FADE_WHITE
#define B(k) (ED_BASE1 + (k) - 1)
#define R(k) (ED_ROLL1 + (k) - 1)
static const struct { short frame; unsigned char layer, anim; short from, len; unsigned char screen; } ED[] = {
  { 0, 3, FB, 1, 119, 0 }, { 318, 0, B(1), 1, 119, 0 }, { 438, 0, B(1), 121, 1, 0 }, { 438, 2, R(1), 1, 581, 0 },
  { 1021, 0, B(1), 121, 1, 0 }, { 1021, 3, FB, 241, 119, 0 }, { 1141, 0, B(2), 121, 1, 0 }, { 1141, 3, FB, 361, 119, 0 },
  { 1261, 0, B(2), 121, 1, 0 }, { 1261, 2, R(2), 1, 581, 0 }, { 1843, 2, ED_NONE, 0, 0, 0 }, { 1843, 0, B(2), 121, 1, 0 },
  { 1843, 3, FB, 241, 119, 0 }, { 1963, 0, B(3), 121, 1, 0 }, { 1963, 3, FB, 361, 119, 0 }, { 2083, 0, B(3), 121, 1, 0 },
  { 2083, 2, R(3), 1, 797, 0 }, { 2881, 0, B(3), 121, 1, 0 }, { 2881, 3, FB, 121, 119, 0 }, { 3001, 2, ED_NONE, 0, 0, 0 },
  { 3001, 0, B(4), 121, 1, 0 }, { 3001, 3, FB, 1, 119, 0 }, { 3121, 0, B(4), 121, 1, 0 }, { 3121, 2, R(4), 1, 1397, 1 },
  { 4519, 2, ED_NONE, 0, 0, 0 }, { 4519, 0, B(4), 121, 1, 0 }, { 4519, 3, FB, 241, 119, 0 }, { 4639, 0, B(3), 121, 1, 0 },
  { 4639, 3, FB, 361, 119, 0 }, { 4759, 0, B(3), 121, 1, 0 }, { 4759, 2, R(5), 1, 1403, 2 }, { 6163, 2, ED_NONE, 0, 0, 0 },
  { 6163, 0, B(3), 121, 1, 0 }, { 6163, 3, FB, 241, 119, 0 }, { 6283, 0, B(1), 121, 1, 0 }, { 6283, 3, FB, 361, 119, 0 },
  { 6403, 0, B(1), 121, 1, 0 }, { 6403, 2, R(6), 1, 1800, 0 }, { 8209, 0, B(1), 121, 1, 0 }, { 8209, 3, FB, 241, 119, 0 },
  { 8329, 0, B(4), 121, 1, 0 }, { 8329, 3, FB, 361, 119, 0 }, { 8449, 0, B(4), 121, 1, 0 }, { 8449, 2, R(7), 1, 270, 0 },
  { 8719, 3, FB, 121, 119, 0 }, { 8839, 2, ED_NONE, 0, 0, 0 }, { 8839, 3, FB, 120, 1, 0 }, { 8839, 2, R(7), 560, 190, 0 },
  { 9300, 0, B(4), 121, 1, 3 }, { 9300, 3, FW, 121, 119, 0 }, { 9420, 2, ED_NONE, 0, 0, 0 }, { 9420, 0, ED_BNG, 0, 0, 0 },
  { 9420, 3, FW, 1, 119, 0 }, { 9540, 0, ED_BNG, 0, 0, 0 }, { 9660, 0, ED_BNG, 0, 0, 0 }, { 9660, 3, FB, 121, 119, 0 },
};
#undef FB
#undef FW
#undef B
#undef R
#define N_ED ((int)(sizeof ED / sizeof ED[0]))

/* layer's last event by frame t (-1 none); a skip clears the layers (update's 0x40 branch) */
static int ed_event(int layer, int t, int skipped) {
  int e = -1;
  for (int k = 0; k < N_ED && ED[k].frame <= t; ++k)
    if (ED[k].layer == layer && (!skipped || ED[k].frame >= MENU_ENDING_SKIP)) e = k;
  return e;
}

int menu_ending_band(int t) {
  int e = ed_event(2, t, 0);
  return e >= 0 && (ED[e].screen == 1 || ED[e].screen == 2) ? ED[e].screen : 0;
}

void menu_ending_draw(const menu_state *m) {
  static int id[ED_NONE], found;
  if (!found) {
    for (int k = 0; k < ED_NONE; ++k) id[k] = anm_find(ED_NAME[k]);
    found = 1;
  }
  int t = m->ending_t;
  for (int layer = 0; layer < 4; ++layer) {
    int e = ed_event(layer, t, m->ending_skip);
    if (e < 0 || ED[e].anim >= ED_NONE || id[ED[e].anim] < 0) continue;
    int f = ED[e].from + (t - ED[e].frame), last = ED[e].len > 0 ? ED[e].from + ED[e].len - 1 : ED[e].from;
    int n = anm_frames(id[ED[e].anim]);
    if (f > last) f = last;
    if (f >= n) f = n - 1;
    if (f >= 0) anm_draw(id[ED[e].anim], f, 1);
  }
}

static void ending_open(menu_state *m) {
  fprintf(stderr, "story ending\n");
  go(m, MENU_PRE);
  m->pre_quiet = 1;
  m->pre_fade = 0;
  m->ending = 2;
  m->ending_t = m->ending_skip = 0;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static void bonus_next(menu_state *m);

void menu_ending_done(menu_state *m) {
  m->ending = m->ending_t = m->ending_skip = 0;
  m->finish = 0;
  if (m->bonus_due) {
    bonus_next(m);
    return;
  }
  if (tg_due(m)) return;
  go(m, MENU_GAME);
  m->sel = game_slot(MENU_GAME_STORY);
  m->slide = m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

/* ---- the bonuses after the ending (mseqStory 0x88ccd4f..): the story clear bonus
 * (seqStStoryModeClearBonus 0x8840ff0, clStStoryModeClearBonus) once a card (+0x279), then
 * with no story race lost this loop (before_card.hasStoryLose) the undefeated bonus
 * (seqStUndefeatedBonus 0x88403b8, +0x186) and the second (seqStUndefeatedBonus2 0x8841ba4,
 * +0x278), each once. Each (ctor 0x8840e68, 0x884026c, 0x8841a58; run 0x88407f0, 0x883fc30,
 * 0x8841450): menu_g_shogo_bg_1 and menu_g_bg_line behind, on them menu_layout_cd_cd's frames
 * 0..179 (the card; the clear bonus' with 0x712, 0x713 swapped for its design 0x905, 0x906) or
 * menu_layout_meter_meter2's (the second's meter), held at 179; clMenuFade in over 1/6 s, SE
 * 0x22 (cutin2), the decide button from frame 100 or frame 601 (the second: 901) leaves, the
 * fade out over 1/6 s. The card designs and the meter they give are not in the port (it keeps
 * the card's flags). ---- */
#define BONUS_FADE 10
static void bonus_next(menu_state *m) {
  int k = m->bonus_due & 1 ? 0 : m->bonus_due & 2 ? 1 : 2;
  m->bonus_due &= ~(1 << k);
  fprintf(stderr, "story bonus %d\n", k);
  go(m, MENU_BONUS);
  m->bonus_kind = k;
  m->bonus_t = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static void bonus_step(menu_state *m, const menu_in *in) {
  int t = ++m->bonus_t, f = t - BONUS_FADE, end = m->bonus_kind == 2 ? 901 : 601;
  if (f == 0) snd_menu(SND_CUTIN2);
  if (m->leaving) {
    if (++m->leaving > BONUS_FADE) {
      m->leaving = 0;
      if (m->bonus_due) {
        bonus_next(m);
        return;
      }
      if (tg_due(m)) return;
      go(m, MENU_GAME);
      m->sel = game_slot(MENU_GAME_STORY);
      m->slide = m->card_t = 0;
      m->bars = 1;
      m->music = 1;
    }
    return;
  }
  if (f >= end || (in->any && f > 100)) m->leaving = 1;
}

static void draw_bonus(const menu_state *m, int clock) {
  static int cd = -2, meter;
  if (cd == -2) cd = anm_find("menu_layout_cd_cd"), meter = anm_find("menu_layout_meter_meter2");
  anm_fill(0, 0, 0, 1);
  if (ID.shogo_bg1 >= 0) anm_draw(ID.shogo_bg1, clock % anm_frames(ID.shogo_bg1), 1);
  if (ID.line >= 0) anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  int f = m->bonus_t - BONUS_FADE, id = m->bonus_kind == 2 ? meter : cd;
  if (f < 0) f = 0;
  if (f > 179) f = 179;
  if (id >= 0) {
    int n = anm_frames(id);
    if (f >= n) f = n - 1;
    anm_swap sw[2] = { { anm_sprite(JID(0x712)), anm_sprite(JID(0x905)) }, { anm_sprite(JID(0x713)), anm_sprite(JID(0x906)) } };
    anm_draw_ex(id, f, 1, 0, 0, m->bonus_kind == 0 ? sw : NULL, m->bonus_kind == 0 ? 2 : 0);
  }
  float a = m->bonus_t < BONUS_FADE ? (float)m->bonus_t / BONUS_FADE : 1.0f;
  if (m->leaving) a = 1.0f - (float)m->leaving / BONUS_FADE;
  anm_fill(0, 0, 0, 1.0f - (a < 0 ? 0 : a));
}

/* ---- the mileage tally at the end of a session (menu_layout_milage, the arcade's card hand-back: the title's update, then this, "Continue?" and the card
 * ejected): the car on a grey ground, its name, class and title, power, and under them "Your mileage is" (what this card drove since the game
 * started) and "Total mileage" in big italic digits with km; the first counts down to 0 while the second counts up by it, and the screen leaves on a
 * press once it is done or by itself. The arcade's mileage password (a string printed on the card to carry the distance to a new one) is not in the
 * port, the saves keep the distance. ---- */
#define MILAGE_FADE 10
#define MILAGE_COUNT_AT 40
#define MILAGE_HOLD 150
enum { M_BG = JID(1557), M_KM = JID(1558), M_COMMA = JID(2823), M_BAR = JID(5584), M_DIGIT0 = JID(1224) };

/* the card to show: the one in hand if it drove a km this time, else the one that drove most */
static int milage_card(const menu_state *m) {
  if (m->own_sel >= 0 && m->own_sel < m->n_own && m->own[m->own_sel].odo - m->own[m->own_sel].odo0 >= 1000.0) return m->own_sel;
  int best = -1;
  double most = 999.0;
  for (int k = 0; k < m->n_own; ++k)
    if (m->own[k].odo - m->own[k].odo0 > most) most = m->own[k].odo - m->own[k].odo0, best = k;
  return best;
}

static void milage_to_title(menu_state *m) {
  for (int k = 0; k < m->n_own; ++k) m->own[k].odo0 = m->own[k].odo; /* (the next game counts from here) */
  m->leaving = 0;
  if (m->milage_race) { /* (after a race: on to the menus the race was going back to) */
    m->milage_race = 0;
    back_to_modes(m, 0);
    return;
  }
  go(m, MENU_TITLE);
  m->flash = FLASH_LEN;
}

static int milage_dur(const menu_state *m) { return m->milage_this < 40 ? 40 : m->milage_this > 100 ? 100 : (int)m->milage_this; }

/* the km moved from this time's to the total by frame t */
static long milage_moved(const menu_state *m, int t) {
  if (t <= MILAGE_COUNT_AT) return 0;
  const int d = milage_dur(m);
  return t - MILAGE_COUNT_AT >= d ? m->milage_this : m->milage_this * (t - MILAGE_COUNT_AT) / d;
}

/* nsMenuCheck::checkAura: no aura up to 5000 km, then the level by the card's mileage (10000, 30000, 100000, 500000 km); the colour is by two bytes of the
 * arcade's card (a byte * 1/3 - 1 against 0: both under is purple, which a card with the bytes at 0 gets: the port keeps none, so purple) */
static void menu_aura_of(const menu_state *m, menu_car *c) {
  long km = -1;
  int color = 1;
  if (m->screen == MENU_MILAGE) km = m->milage_total0 + m->milage_this;
  else if (m->own_sel >= 0 && m->own_sel < m->n_own) km = (long)(m->own[m->own_sel].odo / 1000.0);
  const char *e = getenv("MT3DX_AURA"); /* test: "<colour 0..3> <km>" */
  if (e) color = atoi(e) & 3, km = strchr(e, ' ') ? atol(strchr(e, ' ') + 1) : 600000;
  if (km <= 4999) return;
  c->aura = color + 1;
  c->aura_level = km > 500000 ? 4 : km > 100000 ? 3 : km > 30000 ? 2 : km > 10000 ? 1 : 0;
}

/* the tally in place of the way to the title, when a card drove a km this time */
static int milage_enter(menu_state *m, int after_race) {
  int k = milage_card(m);
  if (k < 0) return 0;
  if (!m->milage_on) { /* (skipped in the options: what was driven is counted in all the same) */
    for (int i = 0; i < m->n_own; ++i) m->own[i].odo0 = m->own[i].odo;
    return 0;
  }
  const menu_own *o = &m->own[k];
  go(m, MENU_MILAGE);
  m->milage_card = k;
  m->milage_this = (long)((o->odo - o->odo0) / 1000.0);
  m->milage_total0 = (long)(o->odo0 / 1000.0);
  m->milage_t = 0;
  m->milage_race = after_race;
  m->bars = 0;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
  return 1;
}

static void milage_step(menu_state *m, const menu_in *in) {
  m->spin -= 0.3f; /* (the car turns, as on the card) */
  if (getenv("MT3DX_MILAGE") && m->milage_t == 0 && m->milage_this == 0) { /* test: this many km driven, the total before 5449 */
    m->milage_card = 0, m->milage_this = atol(getenv("MT3DX_MILAGE")), m->milage_total0 = 5449;
  }
  const int t = ++m->milage_t, done_at = MILAGE_COUNT_AT + milage_dur(m);
  if (m->leaving) {
    if (++m->leaving > MILAGE_FADE) milage_to_title(m);
    return;
  }
  if (t >= done_at + MILAGE_HOLD || (in->any && t > done_at)) m->leaving = 1;
}

/* a number's digits from the right edge xr at the top y, an advance of 32 px each, the comma in a 16 px cell */
static void milage_number(long v, float xr, float y, float a) {
  char s[24];
  snprintf(s, sizeof s, "%ld", v < 0 ? 0 : v);
  const int n = (int)strlen(s);
  float x = xr;
  for (int i = n - 1; i >= 0; --i) {
    x -= 32.0f;
    anm_draw_sprite(anm_sprite(M_DIGIT0 + (s[i] - '0')), x, y, a);
    if (i && (n - i) % 3 == 0) {
      x -= 16.0f;
      anm_draw_sprite(anm_sprite(M_COMMA), x, y, a);
    }
  }
}

static void draw_milage(const menu_state *m, int clock, void (*draw3d)(void *), void *ctx) {
  (void)clock;
  float a = m->milage_t < MILAGE_FADE ? (float)m->milage_t / MILAGE_FADE : 1.0f;
  if (m->leaving) a = 1.0f - (float)m->leaving / MILAGE_FADE;
  if (a < 0) a = 0;
  anm_fill(0, 0, 0, 1);
  anm_draw_sprite(anm_sprite(M_BG), -192.0f, -16.0f, 1);
  if (draw3d) draw3d(ctx); /* (the car on its turntable) */
  anm_draw_sprite(anm_sprite(M_BAR), -192.0f, 14.0f, 1);
  const menu_own *o = &m->own[m->milage_card >= 0 && m->milage_card < m->n_own ? m->milage_card : 0];
  const cardb_car *c = &CARDB_CARS[o->maker][o->car];
  float x = 12.0f;
  const float y = 30.0f, k = 0.7f;
  /* the name in font 5, then the class and the title, the power */
  int slot = m->save_slot >= 1 && m->save_slot <= MENU_SAVES ? m->save_slot : 1;
  if (m->name[0] >= 0 && anm_sprite(NAME_GLYPH5) >= 0) {
    for (int i = 0; i < 5 && m->name[i] >= 0; ++i) {
      const float xf[6] = { x + 28.0f * k * (float)i, y, k, 0, 0, k };
      anm_draw_sprite_xf(anm_sprite(NAME_GLYPH5 + m->name[i]), xf, a);
    }
  } else {
    const float xf[6] = { x, y, k, 0, 0, k };
    anm_draw_sprite_xf(anm_sprite(CARD_TEXT + C_PLAYER + slot - 1), xf, a);
  }
  x = 136.0f;
  {
    int cls = m->class_car[o->maker][menu_own_slot(o)], pre = anm_sprite(CARD_CLASSP + cls);
    int title = m->title_car[o->maker][menu_own_slot(o)] > 0 ? m->title_car[o->maker][menu_own_slot(o)] : TITLE_FIRST;
    const unsigned short *g;
    int n = title_glyphs(title, &g);
    const float kt = 0.55f;
    if (pre >= 0 && n > 0 && anm_sprite(TITLE_GLYPH5 + g[0]) >= 0) {
      const float xf[6] = { x, y + 4.0f, kt, 0, 0, kt };
      anm_draw_sprite_xf(pre, xf, a);
      x += (anm_sprite_width(pre) - 32.0f + 16.0f) * kt;
      for (int i = 0; i < n; ++i) {
        if (!title_glyph_slash(g[i])) {
          const float xg[6] = { x, y + 4.0f, kt, 0, 0, kt };
          anm_draw_sprite_xf(anm_sprite(TITLE_GLYPH5 + g[i]), xg, a);
        }
        x += (title_glyph_half(g[i]) ? 16.0f : 28.0f) * kt;
      }
    }
  }
  {
    int power = m->tune_full[o->maker][menu_own_slot(o)] ? m->tune_power[o->maker][menu_own_slot(o)] : m->st_power[o->maker][menu_own_slot(o)];
    char hp[24];
    snprintf(hp, sizeof hp, "%dHP", car_tune_hp(c->hp, power));
    ex_text(2, hp, 560.0f - ex_width(2, hp), y + 6.0f, a);
  }
  /* the two rows: what was driven this time, counting down, and the total, counting up by it */
  const long moved = milage_moved(m, m->milage_t);
  ex_text(5, "Your mileage is", 15.0f, 76.0f, a);
  ex_text(5, "Total mileage", 15.0f, 126.0f, a);
  milage_number(m->milage_this - moved, 556.0f, 70.0f, a);
  milage_number(m->milage_total0 + moved, 556.0f, 120.0f, a);
  for (int r = 0; r < 2; ++r) {
    anm_draw_sprite(anm_sprite(M_KM), 566.0f, 70.0f + 50.0f * (float)r, a);
  }
  anm_fill(0, 0, 0, 1.0f - a);
}

/* point_calc 0x85df20c, the story's branch: the share of a full gauge (1000 points, 20
 * units) the race earns, by the tune so far (T), the loop, the episode's place in it and
 * whether it was cleared before; a full gauge's surplus is lost */
int menu_story_points(menu_state *m, int won) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), n = m->story_n;
  if (n < 0) return 0;
  int p, h;
  menu_story_tune(m, &p, &h);
  /* (a full tune is past the gauge's 33 steps: no points, no tuning screen) */
  int t = p + h, sel = n % 100, cleared = st_cleared(m, n) || m->story_test, u = 0;
  m->st_prev = m->st_gauge[mk][sl] * 50;
  if (t > 33) {
    u = 0;
  } else if (st_loop(m) > 0) {
    u = cleared || !won ? 0 : 1;
  } else if (sel <= 19) {
    if (t > 19) u = 0;
    else if (cleared) u = won ? (t <= 9 ? 10 : 7) : 0;
    else u = won ? 20 : t <= 9 ? 10 : 7;
  } else if (sel <= 39) {
    u = t > 29 ? 0 : cleared ? (won ? 5 : 0) : won ? 10 : 5;
  } else if (sel <= 59) {
    u = t > 32 || cleared || !won ? 0 : 2;
  } else {
    u = cleared || !won ? 0 : 1;
  }
  /* the result shows the gauge but past 33 steps, and in a later loop only when it gains */
  m->st_show = t <= 33 && (st_loop(m) == 0 || u);
  m->st_gain = m->st_prev + u * 50 > 1000 ? 1000 - m->st_prev : u * 50;
  m->st_get = t <= 19 ? T_ST_GET : STTUNE_GET[t <= 29 ? STTUNE_CAR[mk][c].t20 : STTUNE_CAR[mk][c].t30];
  int g = m->st_gauge[mk][sl] + u;
  if (g >= 20) {
    m->st_gauge[mk][sl] = 0;
    m->st_tune_due = 1;
  } else {
    m->st_gauge[mk][sl] = (signed char)g;
  }
  if (u) m->garage_changed = 1;
  /* mseqStory: getStorySelectInLoop 99, the last episode, won: seqRaceEnding after the poem */
  m->ending = won && n % 100 == 99;
  /* and the bonuses after it (by the card before the race: hasStoryLose, then cleared by
   * finiStory's episode 100 won) */
  m->bonus_due = 0;
  if (m->ending) {
    unsigned char *b = &m->st_bonus_car[mk][sl];
    if (!(*b & 1)) m->bonus_due |= 1;
    if (!(*b & 8) && !(*b & 2)) m->bonus_due |= 2;
    if (!(*b & 8) && !(*b & 4)) m->bonus_due |= 4;
    *b = (unsigned char)((*b | m->bonus_due) & ~8);
    m->garage_changed = 1;
  }
  m->poem = -1;
  for (int k = 0; won && k < N_POEMS; ++k)
    if (POEMS[k].select == m->story_pick) m->poem = k;
  return u;
}

/* The tuning screen (clSeqStTuneThread): menu_layout_st_tune over menu_g_bg_image_st_tune,
 * idling on frames 1..59; the part (template 0x1ef9), the tuner's shop (0x1ea2) and
 * portrait (0x19d7), the step line (ＳＴＥＰn：the part, font 1 on 0x6a8), the tuner's name
 * and three lines on menu_layout_st_tune_serifu (frame 12: 0x1a9c, 0x580..0x582), the top
 * bar's message the tuner's. Power over handling, each a row of cells (menu_g_setting_gage_1:
 * x from 0x93fc818, 19 apart; y 333 and 371), the next step's cell lit big (90..94) on the
 * chosen one, and their P and H markers (menu_g_setting_p_h, clMenuGSettingPHAnime). The
 * left key or a tap on the Power row chooses power, the right key or Handling handling
 * (SE 0x19; not when the step is fixed): the other's step cell goes out (61..64), the
 * chosen one's comes on (71..74), the red and blue bars swap, and changeTune crossfades to
 * the new part and tuner (frames 71..75 for power, 81..85 for handling: templates 0x16af and
 * 0x133d). A tap anywhere else decides (SE 0x1a, the arcade's gas pedal; its 10 s
 * countdown is the port's infinity). Not ported: the R35's own layout and the tuner's
 * work on the car after (seqStTuneEff). */
enum { T_TUNE_PARTS = JID(0x1ef9), T_TUNE_SHOP = JID(0x1ea2), T_TUNE_PORTRAIT = JID(0x19d7), T_TUNE_PARTS2 = JID(0x16af),
       T_TUNE_PORTRAIT2 = JID(0x133d), T_TUNE_FONT = JID(0x6a8), T_TUNE_NAME = JID(0x1a9c), T_TUNE_LINE = JID(0x580),
       T_TUNE_RED = JID(0x132a), T_TUNE_BLUE = JID(0x16ad), T_PH_TENS_P = JID(0x964), T_PH_ONES_P = JID(0x963),
       T_PH_TENS_H = JID(0x95c), T_PH_ONES_H = JID(0x95b), S_PH_DIGIT_P = JID(2402), S_PH_DIGIT_H = JID(2392) };
#define TUNE_PARTS_ENGINE JID(0x16cb) /* the second power part of a car whose model has +0x28 in {1, 2} */
#define TUNE_PARTS_SWAP JID(0x16d6)   /* car 0x24's first */
#define TUNE_WARN_NAME JID(0x1bc2)    /* Rikako's name and portrait (menu_p_st_tuner_rikako10) */
#define TUNE_WARN_PORTRAIT JID(0x1893)
static const float TUNE_CELL_X[2][3] = { { 171, 381, 0 }, { 137, 339, 541 } };
static const float TUNE_ROW_Y[2] = { 333, 371 };
/* the cells' states (0x93fc080: start, frames, mode): lit loops, the others hold */
static const int TUNE_STATE[5][3] = { { 0, 0, 1 }, { 1, 59, 6 }, { 61, 4, 4 }, { 71, 4, 4 }, { 90, 5, 4 } };

typedef struct { int chara, parts, shop, name, line[3]; } tune_view;

/* the constructor's (and changeTune's) part, tuner and lines for the step at these values */
static tune_view tune_view_of(const menu_state *m, int power, int handling, int choice) {
  const sttune_car *car = &STTUNE_CAR[m->maker][m->car];
  int t = m->tune.total;
  tune_view v = { 0, 1, 1, -1, { -1, -1, -1 } };
  const sttune_row *r = NULL;
  if (t <= 19) {
    if (!choice) {
      r = car->vip ? &STTUNE_HAND_VIP[handling] : &STTUNE_HAND[handling];
    } else {
      r = car->vip ? &STTUNE_POWER_VIP[power] : car->na && power == 1 ? &STTUNE_POWER_NA
          : car->e24 && power == 1 ? &STTUNE_POWER_24 : &STTUNE_POWER[power];
    }
    v.chara = r->chara;
    v.parts = r->parts;
    v.name = r->name;
    for (int k = 0; k < 3; ++k) v.line[k] = r->line[k];
    if (choice && car->k28 && power == 2) v.parts = TUNE_PARTS_ENGINE;
    if (choice && car->e24 && power == 1) v.parts = TUNE_PARTS_SWAP;
  } else if (t <= 29) {
    int i = t - 20;
    v.chara = car->t20;
    v.name = STTUNE_MID_NAME[car->vip][i];
    v.parts = STTUNE_MID_PARTS[car->vip][i];
    v.shop = STTUNE_CHARA[v.chara][2];
    const short (*l)[3] = STTUNE_MID_LINES[v.chara < 17 && STTUNE_MID_HAS[v.chara] ? v.chara : 2];
    for (int k = 0; k < 3; ++k) v.line[k] = l[i][k];
  } else {
    v.chara = car->t30;
    v.shop = STTUNE_CHARA[v.chara][2];
    int i = t - 30 < 4 ? t - 30 : 3;
    for (int k = 0; k < 3; ++k) v.line[k] = STTUNE_TOP_LINES[v.chara][i][k];
  }
  return v;
}

/* Rikako's warning, one side three steps ahead of the other (steps 1..20 only) */
static int tune_warn(const menu_state *m, int choice) {
  if (m->tune.total > 19) return 0;
  return (m->tune.power - m->tune.handling > 2 && choice) || (m->tune.handling - m->tune.power > 2 && !choice);
}

static void tune_cell(menu_state *m, int row, int i, int state) {
  if (i < 0 || i >= 24) return;
  m->tune.state[row][i] = (unsigned char)state;
  m->tune.since[row][i] = m->t;
}

/* clMenuLayout's constructor */
static void tune_open(menu_state *m) {
  int p = m->st_power[m->maker][menu_slot(m)], h = m->st_handling[m->maker][menu_slot(m)], t = p + h;
  memset(&m->tune, 0, sizeof m->tune);
  go(m, MENU_ST_TUNE);
  m->tune.total = t;
  m->tune.cells = t > 29 ? t - 9 : 20;
  if (m->tune.cells > 24) m->tune.cells = 24;
  m->tune.power = p;
  m->tune.handling = h;
  if (t <= 19) {
    m->tune.fixed = !(p <= 9 && h <= 9);
    if (p > h || p > 9) m->tune.handling = h + 1, m->tune.choice = 0;
    else m->tune.power = p + 1, m->tune.choice = 1;
  } else {
    m->tune.power = p + 1;
    m->tune.choice = 1;
  }
  m->tune.choice0 = m->tune.choice;
  m->tune.power0 = m->tune.power;
  m->tune.handling0 = m->tune.handling;
  m->tune.changed = -1;
  for (int row = 0; row < 2; ++row) {
    int v = row ? m->tune.handling : m->tune.power;
    for (int i = 0; i < m->tune.cells; ++i) tune_cell(m, row, i, i < v);
  }
  int c = m->tune.choice;
  tune_cell(m, !c, (c ? m->tune.power : m->tune.handling) - 1, 4);
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

/* key 1 (power) or 2 (handling) */
static void tune_choose(menu_state *m, int power) {
  if (m->tune.fixed || m->tune.choice == power) return;
  snd_menu(SND_CURSOR);
  m->tune.choice = power;
  int *was = power ? &m->tune.handling : &m->tune.power, *now = power ? &m->tune.power : &m->tune.handling;
  tune_cell(m, power ? 1 : 0, *was - 1, 2);
  (*was)--;
  (*now)++;
  tune_cell(m, power ? 0 : 1, *now - 1, 3);
  m->tune.changed = m->t;
}

/* left chooses power, right handling; OK (the gas) decides */
static void tune_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (in->dx) {
    tune_choose(m, in->dx < 0);
    return;
  }
  if (!in->ok) return;
  snd_menu(SND_SELECT);
  m->deciding = FADE;
}

/* the screen decided: setTunePower / setTuneHandling, on to the mode select */
static void tune_done(menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m);
  if (m->tune.choice) m->st_power[mk][sl] = (signed char)(m->st_power[mk][sl] + 1 > 24 ? 24 : m->st_power[mk][sl] + 1);
  else m->st_handling[mk][sl] = (signed char)(m->st_handling[mk][sl] + 1 > 24 ? 24 : m->st_handling[mk][sl] + 1);
  m->st_tune_due = 0;
  m->garage_changed = 1;
}

/* a cell's frame of menu_g_setting_gage_1 */
static int tune_cell_frame(const menu_state *m, int row, int i) {
  const int *s = TUNE_STATE[m->tune.state[row][i]];
  int t = m->t - m->tune.since[row][i], base = 200 * (i / 10);
  if (s[2] == 6) return base + s[0] + t % s[1];
  return base + s[0] + (t < s[1] ? t : s[1]);
}

/* a view's top bar message, name and portrait, Rikako's when she warns */
static void tune_face(const tune_view *v, int warn, int *name, int *portrait, int *top) {
  *name = warn ? TUNE_WARN_NAME : STTUNE_CHARA[v->chara][0];
  *portrait = warn ? TUNE_WARN_PORTRAIT : STTUNE_CHARA[v->chara][1];
  *top = warn ? 0 : STTUNE_CHARA[v->chara][3];
}

static int tune_text(int k) { return k < 0 ? -1 : anm_sprite(STTUNE_TEXT + k); }

static void draw_st_tune(const menu_state *m, int clock, float fade, int bars) {
  tune_view v0 = tune_view_of(m, m->tune.power0, m->tune.handling0, m->tune.choice0);
  tune_view v = tune_view_of(m, m->tune.power, m->tune.handling, m->tune.choice);
  int w0 = m->tune.total <= 19 && ((m->tune.power0 - m->tune.handling0 > 2 && m->tune.choice0) ||
                                   (m->tune.handling0 - m->tune.power0 > 2 && !m->tune.choice0));
  int w = tune_warn(m, m->tune.choice);
  int name0, portrait0, top0, name, portrait, top;
  tune_face(&v0, w0, &name0, &portrait0, &top0);
  tune_face(&v, w, &name, &portrait, &top);
  if (w && m->tune.changed >= 0) portrait0 = portrait;
  for (int k = 0; k < 3 && w; ++k) v.line[k] = k == 1 ? STTUNE_WARN[m->tune.power > m->tune.handling ? 0 : 1] : -1;
  draw_back(ID.bg, clock % 600, fade);
  draw_back(ID.bg_st_tune, clock % anm_frames(ID.bg_st_tune), fade);
  int f = 1 + m->card_t % 59;
  if (m->tune.changed >= 0) {
    int t = m->t - m->tune.changed;
    f = (m->tune.choice ? 71 : 81) + (t < 4 ? t : 4);
  }
  anm_swap sw[9] = {
    { anm_sprite(T_TUNE_PARTS), anm_sprite(v0.parts) }, { anm_sprite(T_TUNE_SHOP), anm_sprite(v0.shop) },
    { anm_sprite(T_TUNE_PORTRAIT), anm_sprite(portrait0) }, { anm_sprite(T_TUNE_PARTS2), anm_sprite(v.parts) },
    { anm_sprite(T_TUNE_PORTRAIT2), anm_sprite(portrait) }, { anm_sprite(T_TUNE_FONT), -1 },
    { anm_sprite(T_TUNE_RED), m->tune.choice ? anm_sprite(T_TUNE_RED) : -1 },
    { anm_sprite(T_TUNE_BLUE), m->tune.choice ? -1 : anm_sprite(T_TUNE_BLUE) },
  };
  anm_draw_ex(ID.st_tune, f, fade, 0, 0, sw, 8);
  /* the step line where the layout has its font */
  float r[4];
  if (anm_sprite_rect(ID.st_tune, f, anm_sprite(T_TUNE_FONT), r)) {
    int step = anm_sprite(STTUNE_TEXT + STTUNE_STEP[m->tune.total < 35 ? m->tune.total : 34]);
    anm_draw_sprite(step, r[0], r[1], fade);
    if (m->tune.total <= 29 && v.name >= 0)
      anm_draw_sprite(tune_text(STTUNE_COLON[v.name]), r[0] + anm_sprite_width(step), r[1], fade);
  }
  /* the gauges, then the P and H markers over them */
  int gtype = m->tune.total > 29;
  for (int row = 0; row < 2; ++row) {
    for (int i = 0; i < m->tune.cells; ++i) {
      float x = TUNE_CELL_X[gtype][i / 10] + 19.0f * (float)(i % 10), y = TUNE_ROW_Y[row];
      if (m->tune.state[row][i] == 0) {
        int locked = m->tune.total <= 19 && i > 9;
        anm_draw_ex(locked ? ID.gage_waku : ID.gage_waku2, 0, fade, x, y, NULL, 0);
      } else {
        anm_draw_ex(ID.gage, tune_cell_frame(m, row, i), fade, x, y, NULL, 0);
      }
    }
  }
  for (int row = 0; row < 2; ++row) {
    int val = row ? m->tune.handling : m->tune.power, idx = val - 1;
    if (idx < 0 || idx >= 24) continue;
    int frame = 3 * row + (idx <= 8 ? 0 : idx <= 19 ? 1 : 2);
    int digit = row ? S_PH_DIGIT_H : S_PH_DIGIT_P;
    anm_swap ph[2] = { { anm_sprite(row ? T_PH_TENS_H : T_PH_TENS_P), anm_sprite(digit + val / 10 % 10) },
                       { anm_sprite(row ? T_PH_ONES_H : T_PH_ONES_P), anm_sprite(digit + val % 10) } };
    float x = 15.0f + TUNE_CELL_X[gtype][idx / 10] + 19.0f * (float)(idx % 10);
    anm_draw_ex(ID.ph, frame, fade, x, 55.0f + (row ? 304.0f : 266.0f), ph, 2);
  }
  /* the tuner's lines */
  anm_swap ln[4] = { { anm_sprite(T_TUNE_NAME), anm_sprite(name) } };
  for (int k = 0; k < 3; ++k) ln[1 + k] = (anm_swap){ anm_sprite(T_TUNE_LINE + k), tune_text(v.line[k]) };
  anm_draw_ex(ID.st_tune_serifu, 12, fade, 0, 0, ln, 4);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), top > 0 ? anm_sprite(top) : -1 };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

static void draw_st_level(const menu_state *m, int clock, float fade, int bars) {
  st_states s;
  st_states_of(m, &s);
  int f = st_frame(m), lv = m->sel, rest = m->from < 0 && !m->deciding;
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  anm_swap sw[24];
  int n = 0;
  for (int k = 0; k < 4; ++k) sw[n++] = (anm_swap){ anm_sprite(T_LV_NAME + k), anm_sprite(st_level_name(m, k)) };
  for (int k = 1; k < 4; ++k) sw[n++] = (anm_swap){ anm_sprite(T_LV_LOCK + k - 1), s.level[k] ? -1 : anm_sprite(S_LOCK) };
  if (st_division(m) > 0 || st_loop(m) > 0)
    for (int k = 0; k < 4; ++k) sw[n++] = (anm_swap){ anm_sprite(T_LV_FUKI + k), -1 };
  sw[n++] = (anm_swap){ anm_sprite(T_LV_ARROW), -1 };
  sw[n++] = (anm_swap){ anm_sprite(T_LV_ARROW2), -1 };
  for (int k = 0; k < 4; ++k) sw[n++] = (anm_swap){ anm_sprite(T_LV_BACKLIGHT + k), anm_sprite(LV_BACKLIGHT[k]) };
  anm_draw_ex(ID.st_level, f, fade, 0, 0, sw, n);
  anm_swap cl[8];
  for (int k = 0; k < 4; ++k) {
    cl[2 * k] = (anm_swap){ anm_sprite(T_LV_CLEAR_S + k), s.level[k] == 2 ? anm_sprite(S_CLEAR_S) : -1 };
    cl[2 * k + 1] = (anm_swap){ anm_sprite(T_LV_CLEAR_L + k), s.level[k] == 2 ? anm_sprite(S_CLEAR_L) : -1 };
  }
  if (rest) {
    anm_draw_ex(ID.st_level_clear, lv * 100 + 1 + m->t % 59, fade, 0, 0, cl, 8);
    if (m->t < 60) anm_draw(ID.st_level_ring, lv * 100 + m->t, fade);
    anm_draw(ID.st_level_arrow, lv * 100 + m->t % 60, fade);
  }
  /* the level's five episodes */
  anm_draw(ID.st_lv_back, 80 + (m->t < 4 ? m->t : 4), fade);
  int base = 20 * st_division(m) + 5 * lv;
  for (int i = 0; i < 5; ++i) {
    int st = s.ep[lv][i], row = st_index(st_loop(m), base + i);
    anm_swap ep[2] = { { anm_sprite(T_EP_PICTURE), anm_sprite(STORYSEL_IMAGE[row][0]) },
                       { anm_sprite(LV_WAKU[lv]), st == 2 ? anm_sprite(S_EP_WAKU) : anm_sprite(LV_WAKU[lv]) } };
    int frame = (st == 0 ? 200 : st == 1 ? 0 : 100) + 1 + clock % 59;
    anm_draw_ex(ID.st_lv[lv], frame, fade, 100.0f + 110.0f * (float)i, 346.0f, ep, 2);
  }
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_MES_LEVEL) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

static void draw_st_rival(const menu_state *m, int clock, float fade, int bars) {
  st_states s;
  st_states_of(m, &s);
  int f = st_frame(m), lv = m->st_level, loop = st_loop(m), rest = m->from < 0 && !m->deciding;
  int base = 20 * st_division(m) + 5 * lv;
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  anm_swap marks[5];
  for (int k = 0; k < 5; ++k) marks[k] = (anm_swap){ anm_sprite(T_EP_PICTURE + k), -1 };
  anm_draw_ex(ID.st_rival, f, fade, 0, 0, marks, 5);
  /* the chosen one last, over its neighbours */
  int order[5], no = 0;
  for (int k = 0; k < 5; ++k)
    if (k != m->sel) order[no++] = k;
  order[no++] = m->sel;
  for (int j = 0; j < 5; ++j) {
    int i = order[j];
    float x, y;
    if (!st_place(f, i, &x, &y)) continue;
    int st = s.ep[lv][i], n = base + i, row = st_index(loop, n);
    char num[12];
    snprintf(num, sizeof num, "%d", loop * 100 + n + 1);
    int len = (int)strlen(num), glyphs = len + 2, even = !(glyphs & 1);
    /* the glyph places left to right: 0..6, the 2s 7 first */
    int place[8], places = 0;
    if (even) place[places++] = T_EP_GLYPH7;
    for (int k = 0; k < 7; ++k) place[places++] = T_EP_GLYPH + k;
    int first = (places - glyphs) / 2;
    anm_swap sw[12];
    int nsw = 0;
    sw[nsw++] = (anm_swap){ anm_sprite(T_EP_STAGE), anm_sprite(STORYSEL_IMAGE[row][0]) };
    sw[nsw++] = (anm_swap){ anm_sprite(T_EP_GREY), anm_sprite(STORYSEL_IMAGE[row][1]) };
    sw[nsw++] = (anm_swap){ anm_sprite(T_EP_NAME), -1 };
    for (int k = 0; k < places; ++k) {
      int g = k - first, to = -1;
      if (g == 0) to = WMMT_LANG_EN ? -1 : S_GLYPH_DAI; /* (the English caption is the number and "Story": no 第 first) */
      else if (g == glyphs - 1) to = S_GLYPH_WA;
      else if (g > 0 && g < glyphs - 1) to = S_GLYPH_DIGIT + num[g - 1] - '0';
      sw[nsw++] = (anm_swap){ anm_sprite(place[k]), to >= 0 ? anm_sprite(to) : -1 };
    }
    int frame = (st == 0 ? 200 : st == 1 ? 0 : 100) + (i == m->sel && m->from < 0 ? 1 : 71);
    anm_draw_ex(ID.st_ep[lv][even], frame, fade, x, y, sw, nsw);
  }
  if (rest) {
    int block = m->sel < 4 && !s.ep[lv][m->sel + 1] ? (m->sel ? 4 + m->sel : -1) : m->sel;
    if (block >= 0) anm_draw(ID.st_rival_arrow, block * 100 + m->t % 60, fade);
  }
  /* the chapter and the chosen episode's start ramp */
  int n = base + m->sel, later = loop > 0 && n <= 9;
  int title = later ? 20 + n / 5 : n / 5, ramp = STORYSEL_RAMP[later ? 100 + n : n];
  anm_swap rp[3] = { { anm_sprite(T_RAMP_TITLE), anm_sprite(STORYSEL_TEXT + title) },
                     { anm_sprite(T_RAMP_LABEL), anm_sprite(STORYSEL_START_RAMP) },
                     { anm_sprite(T_RAMP_NAME), anm_sprite(STORYSEL_RAMP_TEXT + ramp) } };
  anm_draw_ex(ID.st_rival_ramp, 0, fade, 0, 0, rp, 3);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_MES_RIVAL) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

/* clSeqTaModeSelectThread's spin box puts a car (clSpriteAnimeCarModel, setImageReplace) where
 * its layout has menu_f_font_dot01 and dot02: the card's car under Personal Best, the official
 * machine (setupTaOfficialCar) under Official Machine. The port has none there yet, and the
 * markers themselves are not to be seen. */
static const int MODE_CAR_TEMPLATE[2] = { JID(1716), JID(1717) };
static anm_swap mode_cars[2];

/* ---- the 10-in-a-row's course and level select (mseqTen) ----
 * seqTenCourseSelect (clThread_7_1_CourseSelect::run 0x88a5c78) is the time attack's course
 * select on menu_layout_or_course: ten cards in menu order (menuOrderFromTenCourseID: C1
 * in, C1 out, the New Belt Line both ways, Wangan, Yokohane, Nagoya, Osaka, Fukuoka,
 * Hakone), at rest on k*100 + 1, the same moves and decide frames, the strip held at its
 * fourth place till the seventh card; mapChange as the time attack's; the top bar's
 * message 0x1be7.
 * seqTenLevelSelect (clSeqTenLevelSelectThread::run 0x88a6d14, its stPimpl 0x88aaf20):
 * - the levels as a carousel of menu_layout_or_lv boxes, each on its place -3..3 from the
 *   chosen one: at rest on the frames 0x940d118[place] (19 off screen, 39, 59, the chosen
 *   one's loop 65..123, 149, 169; 0 hidden), moving a place right on 0x940d100[place + 3]
 *   (5 frames on), left on 0x940d100[place + 2] backwards; decided, the chosen box on
 *   210..220, the others shrinking away on 0x940d100[9 + place]. clBox::init (0x88aada0)
 *   puts the level's name in 0x104c, its tune (0x940d320: the R35's tune step, the others'
 *   horsepower) in 0x19eb and the course's medal (0x940d3a4) in 0x3b6, but not on the
 *   32-man box.
 * - menu_layout_or_lv_ring: its flash (1..59) at each move; menu_layout_or_lv_arrow: both
 *   ways 1..59 looped, HARD only (61..) on the first level, EASY only (121..) on the last.
 * - menu_layout_or_lv_face: the level's rivals (0x940d160 into 0x71d..0x725; the boss is
 *   the "?"), popping in on 1..60, out on 61..70 once decided; on the 32-man level hidden.
 * - SE 0x19 a move, 0x1a decided, 0x1b back. Not ported: its time limit. */
#define TEN_COURSES 10
#define TEN_MOVE 6        /* setAnime(4, start, 5): start..start + 5 */
#define DECIDE_TEN_LV 11  /* 210..220 */
enum { T_OR_LV = JID(0x104c), T_OR_TUNE = JID(0x19eb), T_OR_MEDAL = JID(0x3b6), T_OR_FACE = JID(0x71d), S_MES_OR_COURSE = JID(0x1be7),
       S_MES_OR_LV = JID(0x1703) };
/* menu order -> clEffTenCourseType, and its EStage */
static const int TEN_ORDER[TEN_COURSES] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8 };
static const int TEN_STAGE[TEN_COURSES] = { 333, 334, 335, 336, 337, 338, 341, 339, 340, 342 };
static const short OR_BOX[12] = { 19, 39, 59, 129, 149, 169, 65, 240, 260, 210, 280, 300 };
/* 0x940d320: the level's name, its horsepower, its R35 tune step */
static const short OR_LV[11][3] = {
  { JID(915), JID(3374), JID(6326) }, { JID(916), JID(3376), JID(6327) }, { JID(917), JID(3378), JID(6329) }, { JID(918), JID(3379), JID(6330) }, { JID(919), JID(3380), JID(6332) },
  { JID(920), JID(3381), JID(6333) }, { JID(921), JID(3382), JID(6334) }, { JID(922), JID(3383), JID(6335) }, { JID(923), JID(3385), JID(6337) }, { JID(924), JID(3386), JID(6338) },
  { JID(2514), 1, 1 },
};
static const short OR_MEDAL[4] = { JID(950), JID(2818), JID(2819), JID(1815) }; /* yn_waku, lv_or_bronze, _silver, _gold */
static const short OR_FACE[11][9] = {
  { JID(1821), JID(1822), JID(1823), JID(1824), JID(1825), JID(1826), JID(1827), JID(1831), JID(1829) },
  { JID(1822), JID(1824), JID(1825), JID(1831), JID(1830), JID(1841), JID(1842), JID(1845), JID(1846) },
  { JID(1825), JID(1853), JID(1829), JID(1831), JID(1832), JID(1841), JID(1842), JID(1843), JID(1849) },
  { JID(1853), JID(1826), JID(1825), JID(1827), JID(1828), JID(1847), JID(1842), JID(1848), JID(1850) },
  { JID(1837), JID(1838), JID(1839), JID(1840), JID(1836), JID(1828), JID(1826), JID(1833), JID(1834) },
  { JID(1824), JID(1828), JID(1841), JID(1829), JID(1844), JID(1849), JID(1835), JID(1848), JID(1836) },
  { JID(1829), JID(1835), JID(1848), JID(1837), JID(1836), JID(1828), JID(1833), JID(1826), JID(1834) },
  { JID(1835), JID(1838), JID(1839), JID(1840), JID(1834), JID(1836), JID(1843), JID(1844), JID(1848) },
  { JID(1831), JID(1841), JID(1853), JID(1843), JID(1844), JID(1845), JID(1846), JID(1847), JID(1848) },
  { JID(1853), JID(1842), JID(1843), JID(1845), JID(1846), JID(1847), JID(1848), JID(1849), JID(1850) },
  { JID(1852), JID(1852), JID(1852), JID(1852), JID(1852), JID(1852), JID(1852), JID(1852), JID(1852) },
};

/* the title page's time zone 0x15ed: menu_p_pre_day_night (3301) or _morning */
static int ten_night(int order, int level) { return TENDB_PRE[order][level][2] == 3301; }

int menu_ten_course(const menu_state *m) { return TEN_ORDER[m->ten_course]; }
int menu_ten_stage(const menu_state *m) { return TEN_STAGE[menu_ten_course(m)]; }

static void ten_course_enter(menu_state *m) {
  go(m, MENU_TEN_COURSE);
  if (getenv("WMMT_TEN_COURSE")) m->ten_course = atoi(getenv("WMMT_TEN_COURSE")); /* test: this card */
  m->sel = m->ten_course;
  m->course_pos = m->sel <= 3 ? m->sel : m->sel >= TEN_COURSES - 3 ? m->sel - (TEN_COURSES - 6) : 3;
  m->card_t = 0;
}

/* seqTenLevelSelect: the levels on offer (the 32-man one once all ten have gold), and the
 * first one without a medal (else the tenth) to start on */
static void ten_level_enter(menu_state *m) {
  const signed char *r = m->ten_result[menu_ten_course(m)];
  int gold = 1, first = -1;
  for (int l = 0; l < 10; ++l) {
    if (r[l] <= 2) gold = 0;
    if (r[l] == 0 && first < 0) first = l;
  }
  m->ten_levels = gold ? 11 : 10;
  go(m, MENU_TEN_LEVEL);
  m->sel = first >= 0 ? first : 9;
  m->ten_face_lv = m->sel;
  m->ten_face_t = 0;
  m->card_t = 0;
}

static void ten_level_step(menu_state *m, const menu_in *in) {
  if (m->from >= 0 && m->t >= TEN_MOVE) m->from = -1;
  m->ten_face_t++;
  m->card_t++;
  if (m->from >= 0 || (!in->dx && !in->ok)) return;
  int d = in->ok ? 0 : in->dx;
  if (d == 0) {
    snd_menu(SND_SELECT); /* SE 0x1a */
    m->deciding = DECIDE_TEN_LV + FADE;
    m->t = 0;
    return;
  }
  int to = m->sel + d;
  if (to < 0 || to >= m->ten_levels) return;
  snd_menu(SND_CURSOR); /* SE 0x19 */
  /* the faces: hidden on the 32-man level, popping in again from it */
  if (to == 10) m->ten_face_lv = 10;
  else {
    if (m->ten_face_lv == 10) m->ten_face_t = 0;
    m->ten_face_lv = to;
  }
  m->from = m->sel;
  m->sel = to;
  m->t = 0;
}

static int or_box_frame(const menu_state *m, int i) {
  int s = i - m->sel;
  if (m->deciding) {
    int t = DECIDE_TEN_LV + FADE - m->deciding;
    if (t > 10) t = 10;
    if (s == 0) return OR_BOX[9] + t;
    return s >= -2 && s <= 2 ? OR_BOX[9 + s] + t : 0;
  }
  if (m->from >= 0 && m->t < TEN_MOVE) {
    int s0 = i - m->from, t = m->t < 5 ? m->t : 5;
    if (m->sel < m->from) return s0 >= -3 && s0 < 3 ? OR_BOX[s0 + 3] + t : 0;
    return s0 > -3 && s0 <= 3 ? OR_BOX[s0 + 2] + 5 - t : 0;
  }
  if (s == 0) return OR_BOX[6] + m->t % 59;
  return s >= -2 && s <= 2 ? OR_BOX[s + 3] : 0;
}

static void draw_ten_level(const menu_state *m, int clock, float fade, int bars) {
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  const signed char *res = m->ten_result[menu_ten_course(m)];
  const cardb_car *car = chosen_car(m, m->car);
  int r35 = has(car->label, "[R35]");
  for (int i = 0; i < m->ten_levels; ++i) {
    int f = or_box_frame(m, i);
    if (f == 0) continue;
    anm_swap sw[3] = { { anm_sprite(T_OR_LV), anm_sprite(OR_LV[i][0]) },
                       { anm_sprite(T_OR_TUNE), anm_sprite(OR_LV[i][r35 ? 2 : 1]) },
                       { anm_sprite(T_OR_MEDAL), anm_sprite(OR_MEDAL[res[i] >= 0 && res[i] <= 3 ? res[i] : 0]) } };
    anm_draw_ex(ID.or_lv, f, fade, 0, 0, sw, i < 10 ? 3 : 2);
  }
  if (!m->deciding) {
    /* m->t counts from the last move (or the screen's start) */
    if (m->t < 58) anm_draw(ID.or_lv_ring, 1 + m->t, fade);
    int a = m->sel == 0 ? 61 : m->sel == m->ten_levels - 1 ? 121 : 1;
    anm_draw(ID.or_lv_arrow, a + m->t % 59, fade);
  }
  if (m->ten_face_lv < 10) {
    anm_swap fc[9];
    for (int k = 0; k < 9; ++k) fc[k] = (anm_swap){ anm_sprite(T_OR_FACE + k), anm_sprite(OR_FACE[m->ten_face_lv][k]) };
    int f;
    if (m->deciding) {
      int t = DECIDE_TEN_LV + FADE - m->deciding;
      f = 61 + (t < 9 ? t : 9);
    } else {
      f = 1 + (m->ten_face_t < 59 ? m->ten_face_t : 59);
    }
    anm_draw_ex(ID.or_lv_face, f, fade, 0, 0, fc, 9);
  }
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_MES_OR_LV) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

void menu_init(menu_state *m) {
  ID.title = anm_find("atract_layout_title");
  ID.bg = anm_find("menu_g_bg_image");
  ID.line = anm_find("menu_g_bg_line");
  ID.logo = anm_find("menu_g_bg_logo");
  ID.top = anm_find("menu_g_topbar_bar");
  ID.bottom = anm_find("menu_g_bottombar_bar");
  for (int k = 1; k < BK_COUNT; ++k) bottom_anim[k] = anm_find(BK_NAME[k]);
  cardpages_init();
  ID.mode = anm_find("menu_layout_ta_carsel");
  ID.mode_ring = anm_find("menu_layout_ta_carsel_ring");
  for (int k = 0; k < 2; ++k) mode_cars[k] = (anm_swap){ anm_sprite(MODE_CAR_TEMPLATE[k]), -1 };
  ID.game = anm_find("menu_layout_mode_sel");
  ID.game_image = anm_find("menu_layout_mode_image");
  ID.game_ring = anm_find("menu_layout_mode_ring");
  ID.vs_sel = anm_find("menu_layout_mode_sel_3");
  ID.vs_sel_image = anm_find("menu_layout_mode_sel_3_image");
  ID.vs_sel_ring = anm_find("menu_layout_mode_sel_3_ring");
  ID.vs_course = anm_find("menu_layout_vs_course1");
  ID.vs_icon = anm_find("menu_g_vs_icon");
  ID.vs_icon_omakase = anm_find("menu_g_vs_icon_omakase");
  ID.vs_icon_omakase2 = anm_find("menu_g_vs_icon_omakase2");
  ID.vs_ex_player = anm_find("menu_g_vs_ex_player");
  ID.timer = anm_find("menu_g_timer_x");
  ID.wait1 = anm_find("menu_layout_wait_1");
  ID.wait2 = anm_find("menu_layout_wait_2");
  ID.finish_vs = anm_find("menu_g_finish_vs");
  ID.shogo_bg = anm_find("menu_g_shogo_bg_1");
  for (int n = 0; n < 3; ++n) {
    char a[48];
    snprintf(a, sizeof a, "menu_p_pre_vs_rival_%d_shadow", n + 2);
    ID.pre_shadow[n] = anm_find(a);
    snprintf(a, sizeof a, "menu_p_pre_vs_rival_%d_car", n + 2);
    ID.pre_cars[n] = anm_find(a);
    snprintf(a, sizeof a, "menu_p_pre_vs_rival_%d_vs", n + 2);
    ID.pre_vs[n] = anm_find(a);
  }
  ID.plate = anm_find("menu_g_car_tg_decide");
  ID.plate_ph = anm_find("menu_g_car_tg_decide_ph");
  ID.course = anm_find("menu_layout_ta_course");
  ID.course_map = anm_find("menu_g_course_map");
  ID.arrow = anm_find("menu_layout_ta_course_arrow");
  ID.maker[0] = anm_find("atract_layout_logo_bng");
  ID.maker[1] = anm_find("atract_layout_logo_namco");
  ID.maker[2] = anm_find("atract_layout_logo_nvidia");
  ID.prologue[0] = anm_find("atract_g_02");
  ID.prologue[1] = anm_find("atract_g_01");
  ID.pre_bg = anm_find("menu_g_pre_bg");
  ID.ranking_bg = anm_find("menu_g_ranking_bg2");
  ID.ranking = anm_find("menu_layout_ranking");
  ID.bg_carsel = anm_find("menu_g_bg_carsel_2");
  ID.maker_layout = anm_find("menu_layout_carsel_maker_");
  ID.maker_car = anm_find("menu_g_carsel_maker_car_image");
  ID.car_info = anm_find("menu_g_carsel_car");
  ID.car_arrow = anm_find("menu_layout_carsel_car_arrow");
  ID.car_new = anm_find("menu_layout_carsel_new");
  ID.mes_r35 = anm_find("menu_layout_carsel_mes_r35");
  ID.mes_jzs161 = anm_find("menu_p_carsel_mes_jzs161");
  ID.col_strip[0] = anm_find("menu_g_carsel_col_04");
  ID.col_strip[1] = anm_find("menu_g_carsel_col_05");
  ID.col_strip[2] = anm_find("menu_g_carsel_col_06");
  ID.col_arrow[0] = anm_find("menu_g_carsel_col_04_arrow");
  ID.col_arrow[1] = anm_find("menu_g_carsel_col_05_arrow");
  ID.col_arrow[2] = anm_find("menu_g_carsel_col_06_arrow");
  ID.col_strip[3] = anm_find("menu_g_carsel_col_01"); /* (no arrows) */
  ID.col_arrow[3] = -1;
  ID.col_strip[4] = anm_find("menu_g_carsel_col_10");
  ID.col_strip[5] = anm_find("menu_g_carsel_col_11");
  ID.col_strip[6] = anm_find("menu_g_carsel_col_12");
  ID.col_arrow[4] = anm_find("menu_g_carsel_col_10_arrow");
  ID.col_arrow[5] = anm_find("menu_g_carsel_col_11_arrow");
  ID.col_arrow[6] = anm_find("menu_g_carsel_col_12_arrow");
  ID.col_name = anm_find("menu_g_carsel_col_name");
  ID.mes_maker = anm_find("menu_g_topbar_mes_carsel_maker");
  ID.mes_car = anm_find("menu_g_topbar_mes_carsel_car");
  ID.mes_color = anm_find("menu_g_topbar_mes_carsel_color");
  ID.set_handle = anm_find("menu_g_setting_handle");
  ID.card_waku = anm_find("menu_g_cardcheck_1_waku");
  ID.card_layout = anm_find("menu_layout_cardcheck_1");
  ID.day_cards = anm_find("menu_layout_yn_card_1_gold");
  ID.day_ring = anm_find("menu_layout_yn_ring");
  ID.day_vs = anm_find("menu_layout_yn_day_vs");
  ID.day_vs_ring = anm_find("menu_layout_yn_day_vs_ring");
  ID.day_sound = anm_find("menu_g_yn_day_sound");
  ID.pre_ta = anm_find("menu_layout_pre_ta");
  ID.pre_sound = anm_find("menu_g_pre_sound");
  ID.pre_st = anm_find("menu_layout_pre_st");
  ID.pre_sound_st = anm_find("menu_g_pre_sound_st");
  ID.loading = anm_find("menu_layout_pre_vs_loading");
  ID.name_select = anm_find("menu_layout_name_select");
  ID.name_back = anm_find("menu_g_name_name_back");
  ID.name_use = anm_find("menu_use_name");
  ID.name_line = anm_find("menu_g_name_line");
  ID.name_tab[0] = anm_find("menu_g_name_katakana");
  ID.name_tab[1] = anm_find("menu_g_name_hiragana");
  ID.name_tab[2] = anm_find("menu_g_name_capital");
  ID.name_tab[3] = anm_find("menu_g_name_small");
  ID.name_tab[4] = anm_find("menu_g_name_end");
  static const char *const COLOR[4] = { "blue", "green", "orange", "red" };
  char name[48];
  ID.st_tune = anm_find("menu_layout_st_tune");
  ID.st_tune_serifu = anm_find("menu_layout_st_tune_serifu");
  ID.bg_st_tune = anm_find("menu_g_bg_image_st_tune");
  ID.gage = anm_find("menu_g_setting_gage_1");
  ID.gage_waku = anm_find("menu_g_setting_gage_waku");
  ID.gage_waku2 = anm_find("menu_g_setting_gage_waku_2");
  ID.ph = anm_find("menu_g_setting_p_h");
  ID.shogo_bg1 = anm_find("menu_g_shogo_bg_1");
  ID.shogo_bg2 = anm_find("menu_g_shogo_bg_2");
  ID.lvup = anm_find("menu_layout_lvup");
  ID.star_bg = anm_find("menu_g_vs_star_bg");
  ID.star_logo = anm_find("menu_layout_vs_star_logo");
  ID.star_car = anm_find("menu_layout_vs_star_car");
  ID.star_title = anm_find("menu_layout_vs_star_title");
  ID.star_wins = anm_find("menu_layout_vs_star_wins");
  ID.star_use = anm_find("menu_use_vs_star");
  for (int k = 0; k < 3; ++k) {
    char n[48];
    snprintf(n, sizeof n, "menu_g_vs_star_%d", k + 1);
    ID.star_add[k] = anm_find(n);
    snprintf(n, sizeof n, "menu_g_vs_star_keta_a_%d", k + 1);
    ID.star_keta_a[k] = anm_find(n);
    snprintf(n, sizeof n, "menu_g_vs_star_keta_b_%d", k + 1);
    ID.star_keta_b[k] = anm_find(n);
  }
  ID.star_finish[0] = anm_find("menu_g_vs_star_finish_1");
  ID.star_finish[1] = anm_find("menu_g_vs_star_finish_100");
  ID.star_finish[2] = anm_find("menu_g_vs_star_finish_1000");
  ID.star_finish[3] = anm_find("menu_g_vs_star_finish_10000");
  ID.star_minus[0] = anm_find("menu_g_vs_star_minus1");
  ID.star_minus[1] = anm_find("menu_g_vs_star_minus2");
  ID.shogo_moji = anm_find("menu_g_shogo_bg_moji");
  ID.shogo_title = anm_find("menu_g_shogo_title");
  ID.shogo_n[0] = anm_find("menu_g_shogo_shogo_1");
  ID.shogo_n[1] = anm_find("menu_g_shogo_shogo_2");
  ID.shogo_n[2] = anm_find("menu_g_shogo_shogo_3");
  ID.shogo_rw = anm_find("menu_layout_shogo_rewrite");
  ID.yn_rank = anm_find("menu_layout_yn_rank");
  ID.eff_top = anm_find("menu_g_topbar_st_install");
  ID.eff_result = anm_find("menu_layout_st_instal_result");
  ID.eff_handle = anm_find("menu_layout_st_instal_result_handle");
  ID.eff_complete = anm_find("menu_layout_st_install_complete");
  ID.eff_complete2 = anm_find("menu_layout_st_install_complete2");
  static const char *const INSTALL[13] = { "front_a", "left_a", "left_b", "left_c", "left_d", "left_e", "leftback_a",
                                           "leftback_b", "leftback_c", "leftfront_a", "leftfront_b", "rightback_a",
                                           "rightback_b" };
  for (int k = 0; k < 13; ++k) {
    snprintf(name, sizeof name, "menu_layout_st_install_%s", INSTALL[k]);
    ID.eff_install[k] = anm_find(name);
  }
  ID.tg_bg1 = anm_find("menu_g_tg_bg_1");
  ID.tg_bg3 = anm_find("menu_g_tg_bg_3");
  ID.top_notime = anm_find("menu_g_topbar_bar_notime");
  ID.tg_get_title = anm_find("menu_layout_tg_tune_get_title");
  ID.tg_get_parts = anm_find("menu_layout_tg_tune_get_parts");
  ID.tg_aero = anm_find("menu_layout_tg_tune_aero");
  ID.tg_aero2 = anm_find("menu_layout_tg_tune_aero_2");
  ID.tg_aero3 = anm_find("menu_layout_tg_tune_aero_3");
  ID.tg_aero5 = anm_find("menu_layout_tg_tune_aero_5");
  ID.tg_get_image = anm_find("menu_layout_tg_tune_get_parts_image");
  ID.tg_get_wheel = anm_find("menu_layout_tg_tune_get_wheel");
  ID.tg_bonnet = anm_find("menu_layout_tg_tune_4_bonnet");
  ID.tg_bonnet_arrow = anm_find("menu_layout_tg_tune_4_arrow");
  ID.tg_wing5 = anm_find("menu_layout_tg_tune_5_wing");
  ID.tg_wing5_arrow = anm_find("menu_layout_tg_tune_5_arrow");
  ID.tg_wing6 = anm_find("menu_layout_tg_tune_6_wing");
  ID.tg_wing6_arrow = anm_find("menu_layout_tg_tune_6_arrow");
  ID.tg_mirror = anm_find("menu_layout_tg_tune_2");
  ID.tg_mirror_arrow = anm_find("menu_layout_tg_tune_2_arrow");
  ID.tg_neon = anm_find("menu_layout_tg_tune_8_neon");
  ID.tg_neon_arrow = anm_find("menu_layout_tg_tune_8_neon_arrow");
  ID.tg_vinyl = anm_find("menu_layout_tg_tune_8_vinyl");
  ID.tg_vinyl_col = anm_find("menu_layout_tg_tune_8_vinyl_col");
  ID.tg_vinyl_arrow = anm_find("menu_layout_tg_tune_vinyl_arrow_8");
  ID.tg_vinyl_gem = anm_find("menu_layout_tg_tune_2_gem");
  ID.tg_wear_title = anm_find("menu_layout_tg_tune_get_title_no_lv");
  ID.tg_wear_logo = anm_find("menu_layout_tg_tune_install_wheel_logo");
  ID.whiteout = anm_find("menu_p_whiteout");
  {
    static const char *const BAR[5] = { "10", "5", "16", "8", "11" }, *const ARROW[5] = { "10", "5", "11", "8", "11" };
    char name[64];
    for (int k = 0; k < 5; ++k) {
      snprintf(name, sizeof name, "menu_g_tg_tune_wheel_%s", BAR[k]);
      ID.tg_wheel_bar[k] = anm_find(name);
      snprintf(name, sizeof name, "menu_layout_tg_tune_wheel_arrow_%s", ARROW[k]);
      ID.tg_wheel_arrow[k] = anm_find(name);
    }
  }
  ID.tg_wheel_col = anm_find("menu_layout_tg_tune_wheel_col_col");
  ID.tg_wheel_cursor = anm_find("menu_layout_tg_tune_wheel_col_cursor");
  ID.tg_wheel_logo = anm_find("menu_layout_tg_tune_wheel_col_logo");
  ID.tg_wheel_waku = anm_find("menu_layout_tg_tune_wheel_col_waku_2");
  ID.tg_complete = anm_find("menu_layout_tg_tune_complete");
  ID.tg_all[0] = anm_find("menu_layout_tg_tune_all");
  ID.tg_all[1] = anm_find("menu_layout_tg_tune_gem_all");
  ID.wmaker_bg = anm_find("menu_layout_wheelmaker_bg");
  ID.wmaker_sel = anm_find("menu_layout_wheelmaker_sel");
  ID.st_level = anm_find("menu_layout_st_level");
  ID.st_level_arrow = anm_find("menu_layout_st_level_arrow");
  ID.st_level_clear = anm_find("menu_layout_st_level_clear");
  ID.st_level_ring = anm_find("menu_layout_st_level_ring");
  ID.st_lv_back = anm_find("menu_g_st_lv_back");
  ID.st_rival = anm_find("menu_layout_st_rival");
  ID.st_rival_arrow = anm_find("menu_layout_st_rival_arrow");
  ID.st_rival_ramp = anm_find("menu_layout_st_rival_ramp");
  for (int k = 0; k < 4; ++k) {
    snprintf(name, sizeof name, "menu_g_st_lv_%s", COLOR[k]);
    ID.st_lv[k] = anm_find(name);
    snprintf(name, sizeof name, "menu_g_st_rival_%s", COLOR[k]);
    ID.st_ep[k][0] = anm_find(name);
    snprintf(name, sizeof name, "menu_g_st_rival_%s2", COLOR[k]);
    ID.st_ep[k][1] = anm_find(name);
  }
  ID.finish[0] = anm_find("menu_g_finish_timeup_ta");
  ID.finish[1] = anm_find("menu_g_finish_finish");
  ID.win = anm_find("menu_g_finish_win");
  ID.lose = anm_find("menu_g_finish_lose");
  ID.st_adv = anm_find("menu_g_finish_tg_result");
  ID.st_waku = anm_find("menu_g_finish_point_waku");
  ID.st_bar = anm_find("menu_g_finish_st_bar");
  ID.st_moji = anm_find("menu_g_finish_st_moji");
  ID.res_course = anm_find("menu_g_finish_ta_course");
  ID.res_best[0] = anm_find("menu_g_finish_ta_total_best");
  ID.res_best[1] = anm_find("menu_g_finish_ta_car_best");
  ID.res_best[2] = anm_find("menu_g_parsonal_best");
  ID.res_congrats = anm_find("menu_g_finish_ta_congratulations");
  ID.res_sections = anm_find("menu_g_finish_ta_section_1");
  ID.res_sections2 = anm_find("menu_g_finish_ta_section_2");
  ID.res_total = anm_find("menu_g_finish_ta_totaltime");
  ID.res_speed = anm_find("menu_g_finish_ta_topspeed");
  ID.res_rank = anm_find("menu_g_finish_ta_ranking");
  ID.or_course = anm_find("menu_layout_or_course");
  ID.or_lv = anm_find("menu_layout_or_lv");
  ID.or_lv_ring = anm_find("menu_layout_or_lv_ring");
  ID.or_lv_arrow = anm_find("menu_layout_or_lv_arrow");
  ID.or_lv_face = anm_find("menu_layout_or_lv_face");
  ID.pre_or = anm_find("menu_layout_pre_or");
  ID.ten_timeup = anm_find("menu_g_finish_timeup");
  ID.ten_face = anm_find("menu_layout_or_result_face");
  ID.ten_x = anm_find("menu_g_or_finish_X");
  ID.ten_finish = anm_find("menu_layout_or_finish_result");
  ID.ten_summary = anm_find("menu_layout_or_result_A");
  ranking_init();
  km_init();
  /* the FD3S, the car the port started with */
  m->maker = 1;
  m->car = 2;
  m->color = 0;
  /* getBgmSelect1st: a player without a card starts on RANDOM */
  m->bgm = 0;
  m->song = -1;
  m->music = 0;
  m->zone = 0;
  m->zones = 1;
  menu_garage_reset(m);
  m->gyro = 0, m->gyro_sens = 5, m->over_on = 1, m->milage_on = 1, m->sharp_on = 0, m->ao_on = 0, m->bm_on = 1, m->readout_on = 0, m->fps_on = 0, m->phys_on = 0, m->filter_on = 1, m->filter_smooth = 10, m->filter_counter = 5, m->btn_steer = 10;
  for (int g = 0; g < 3; ++g) m->key_k[g] = 100, m->key_dx[g] = m->key_dy[g] = 0;
  m->poem = -1;
  for (int i = 0; i < 6; ++i) m->vol[i] = 10; /* (the volume mixer, full, but the overall level at half) */
  m->vol[0] = 5;
  m->vol[4] = m->vol[5] = 5; /* (engine and turbo balance: 5 = the arcade's, 100%) */
  m->ten_beep = 1;
  m->rum[0] = 8, m->rum[1] = 10, m->rum[2] = 10, m->rum[3] = 5;
  m->mip = 2;
  /* (the SSAO off: its 20 ms on the Mi 6 cost the 60 fps) */
  m->hd_tex = 1, m->bloom_on = 0, m->ssao_on = 0, m->wet_on = 0, m->paint_on = 1, m->aa4 = 0;
  m->garage_changed = 0;
  m->game = MENU_GAME_TA;
  m->story_sel = -1;
  m->story_pick = m->story_n = -1;
  m->ten_levels = 10;
  m->save_slot = m->save_skip = 0;
  srand((unsigned)time(NULL));
  /* the arcade boots into the maker logos and the prologue; the attract movie follows them */
  m->demo = -1;
  m->screen = MENU_LOGO;
  m->t = 0;
  m->tap = m->key = m->key_held = 0;
}

void menu_garage_reset(menu_state *m) {
  /* every car stock, its full tune leaning to power (the port's tune before the setting) */
  memset(m->tune_full, 0, sizeof m->tune_full);
  memset(m->tune_power, CAR_TUNE_MAX, sizeof m->tune_power);
  m->manual = 0;
  m->st_div = m->st_bits = 0;
  m->st_level = 0;
  m->ten_course = m->ten_level = 0;
  memset(m->ten_result, 0, sizeof m->ten_result);
  memset(m->st_power, 0, sizeof m->st_power);
  memset(m->st_handling, 0, sizeof m->st_handling);
  memset(m->st_gauge, 0, sizeof m->st_gauge);
  memset(m->dress_lv, 0, sizeof m->dress_lv);
  memset(m->aero, 0, sizeof m->aero);
  memset(m->mirror, 0, sizeof m->mirror);
  memset(m->wing, 0, sizeof m->wing);
  memset(m->bonnet, 0, sizeof m->bonnet);
  memset(m->wheel, 0, sizeof m->wheel);
  m->st_tune_due = m->n_dress_due = 0;
  m->st_lost = 0;
  m->n_own = 0;
  m->own_sel = m->own_view = -1;
  memset(m->st_div_car, 0, sizeof m->st_div_car);
  memset(m->st_bits_car, 0, sizeof m->st_bits_car);
  memset(m->ten_car, 0, sizeof m->ten_car);
  memset(m->class_car, 0, sizeof m->class_car);
  memset(m->title_car, 0, sizeof m->title_car);
  memset(m->st_streak_car, 0, sizeof m->st_streak_car);
  memset(m->vs_wins_car, 0, sizeof m->vs_wins_car);
  memset(m->vs_stars_car, 0, sizeof m->vs_stars_car);
  memset(m->vs_count_car, 0, sizeof m->vs_count_car);
  memset(m->st_bonus_car, 0, sizeof m->st_bonus_car);
  memset(m->meter_car, 0, sizeof m->meter_car);
  memset(m->sound_car, 0, sizeof m->sound_car);
  m->title_n = 0;
  memset(m->name, -1, sizeof m->name);
}

static void go(menu_state *m, int screen) {
  m->screen = screen;
  if (screen != MENU_VS_SEL) m->sel_menu = 0; /* (the port's own choices ride on that screen's discs: selmenu_enter) */
  m->pre_quiet = 0;
  m->t = 0;
  m->load_shown = m->load_out = 0;
  m->sel = 0;
  m->from = -1;
  m->deciding = 0;
  m->lock = 0;
}

void menu_open(menu_state *m) {
  car_select_enter(m, m->game != MENU_GAME_TA ? MENU_GAME : MENU_COURSE); /* (no mode select: see MENU_MODE) */
  m->bars = 1;
  m->tap = m->back = m->leaving = 0;
  m->key = m->key_held = 0;
}

static int in_attract(const menu_state *m) { return m->screen <= MENU_PROLOGUE; }

int menu_booting(const menu_state *m) { return m->demo < 0; }

void menu_start(menu_state *m) {
  if (!in_attract(m) || menu_booting(m)) return;
  snd_menu(SND_START);
  go(m, MENU_START);
  m->music = 0;
}

int menu_movie(const menu_state *m) { return m->screen == MENU_MOVIE ? m->demo % 3 : -1; }

void menu_movie_done(menu_state *m) {
  if (m->screen == MENU_MOVIE) go(m, MENU_TITLE);
}

/* the pause menu with the controls: left and right slide the spin box to YES / NO, OK decides the one in front, BACK is YES
 * (its step reads them as menu_in) */
static void pause_action(menu_pause *p, int act) {
  if (act == MENU_ACT_OK) p->act_ok = 1;
  else if (act == MENU_ACT_BACK) p->act_back = 1;
  else if (act == MENU_ACT_LEFT) p->act_dx = -1;
  else if (act == MENU_ACT_RIGHT) p->act_dx = 1;
}

/* The controls (docs/INPUT.md). They are kept for the next step, which makes menu_in of them (and of the pointer's tap, through
 * the screen's adapter): left and right the steering (m->key; the name grid's cursor m->left / right), up and down the shifter
 * (m->up, down), the gas OK (m->act_ok, with m->tap for the screens that wait for any press), BACK the back key, the view button. */
void menu_action(menu_state *m, int act) {
  m->ctl_touch = 0;
  if (m->finish) { /* a result: a press leaves it */
    m->tap = 1;
    return;
  }
  if (m->pause.open) {
    pause_action(&m->pause, act);
    return;
  }
  if (in_attract(m)) { /* any press is START (not while it boots: step) */
    m->tap = 1;
    return;
  }
  switch (act) {
  case MENU_ACT_LEFT: /* (the name grid's cursor moves; everywhere else it is the left key) */
    if (m->screen == MENU_NAME) m->act_left = 1;
    else m->key = -1;
    break;
  case MENU_ACT_RIGHT:
    if (m->screen == MENU_NAME) m->act_right = 1;
    else m->key = 1;
    break;
  case MENU_ACT_UP: m->act_up = 1; break;
  case MENU_ACT_DOWN:
    if (m->screen == MENU_MAKER && !HID_BTN) m->hidden = 1; /* the maker select's "Special Vehicles" key (the hidden cars) */
    else m->act_down = 1;
    break;
  case MENU_ACT_VIEW:
    if (m->screen == MENU_COLOR && all_parts_shown(m)) { /* the colour select's ALL PARTS key */
      m->all_parts ^= 1;
      snd_menu(SND_CURSOR);
    } else {
      m->act_view = 1;
    }
    break;
  case MENU_ACT_BACK: m->back = 1; break;
  case MENU_ACT_OK:
    m->tap = 1;
    m->act_ok = 1;
    break;
  }
}

void menu_touch(menu_state *m, void *id, int phase, float x, float y) {
  if (phase == TOUCH_BEGAN) m->ctl_touch = 1;
  if (m->finish) {
    if (phase == TOUCH_BEGAN) m->tap = 1; /* anywhere: leaves the result */
    return;
  }
  if (m->pause.open) {
    if (phase == TOUCH_BEGAN) {
      m->pause.tap = 1;
      m->pause.tap_x = x * 640.0f / 1024.0f;
      m->pause.tap_y = y * 480.0f / 768.0f;
    }
    return;
  }
  if (m->screen == MENU_LAYOUT && !(id && id == m->back_touch) && layout_touch(m, id, phase, x, y)) return;
  /* the back key works like a button: lit while pressed, it goes back only when the finger
   * comes off it (sliding off cancels), so a stray tap elsewhere never leaves the screen */
  if (id && id == m->back_touch) {
    int on = hud_menu_key(x, y) == HUD_MENU_BACK;
    if (phase == TOUCH_MOVED) {
      m->key_held = on ? HUD_MENU_BACK : 0;
      return;
    }
    if (phase == TOUCH_ENDED) {
      m->back_touch = 0;
      m->key_held = 0;
      if (on && m->screen >= MENU_MAKER) m->back = 1;
      return;
    }
  }
  if (phase == TOUCH_ENDED) m->vs.held = 0, m->test_hold = 0;

  if (phase == TOUCH_ENDED && !m->back_touch) m->key_held = 0;
  if (phase != TOUCH_BEGAN) return;
  if (in_attract(m)) {
    m->tap = 1; /* anywhere: the start button */
    return;
  }
  int key = hud_menu_key(x, y);
  if (key == HUD_MENU_BACK) {
    if (m->screen >= MENU_MAKER) m->back_touch = id, m->key_held = key;
    return;
  }
  /* the versus screen's own buttons; the game mode select's VS key (the OPTION button's place) */
  if (m->screen == MENU_VS) {
    m->vs.held = vs_hit(m, x, y) + 1; /* (the button shows pressed while the finger is on it) */
    m->tap_x = x * 640.0f / 1024.0f;
    m->tap_y = y * 480.0f / 768.0f;
    m->tap = 1;
    return;
  }
  /* the maker select's and the SAVE DATA screen's gear key: the OPTION screen */
  if (key == HUD_MENU_OPTION && (m->screen == MENU_MAKER || (m->screen == MENU_SAVE && !m->save_confirm) || (m->screen == MENU_HAVE && !m->keep_ask && !m->deciding))) {
    m->option = 1;
    m->key_held = key;
    return;
  }
  /* the maker select's hidden cars key */
  if (key == HUD_MENU_HIDDEN && m->screen == MENU_MAKER && !HID_BTN) {
    m->hidden = 1;
    m->key_held = key;
    return;
  }
  /* the colour select's ALL PARTS key (a new kind of car's): on and off */
  if (key == HUD_MENU_HIDDEN && all_parts_shown(m)) {
    m->all_parts ^= 1;
    snd_menu(SND_CURSOR);
    m->key_held = key;
    return;
  }
  /* (the car view's bin and the card's wheel key are gone: DELETE CAR and DRESS-UP are on the Car settings discs) */
  /* the votes' omakase key */
  if (key == HUD_MENU_HIDDEN && vs_omakase_shown(m)) {
    vs_omakase(m);
    m->key_held = key;
    return;
  }
  /* the maker and colour screens have no keys: their items are tapped */
  if ((key == 1 || key == -1) && m->screen != MENU_MAKER && m->screen != MENU_COLOR) {
    m->key = m->key_held = key;
    return;
  }
  /* to the arcade's 640x480 screen */
  m->tap_x = x * 640.0f / 1024.0f;
  m->tap_y = y * 480.0f / 768.0f;
  m->tap = 1;
}

/* Which way a tap points on the current screen: -1 previous, +1 next, 0 the selected item. */
/* the course strip's shift: 88 px a card the selection has gone past its place on screen,
 * sliding 88/6 px a frame while it moves */
static float course_dx(const menu_state *m) {
  float to = (float)(m->sel - m->course_pos);
  if ((m->screen == MENU_COURSE || m->screen == MENU_TEN_COURSE) && m->from >= 0 && m->t < 6) {
    float from = (float)m->course_scroll_from;
    to = from + (to - from) * (float)m->t / 6.0f;
  }
  return -88.0f * to;
}

/* the disc under the tap (640 x 480 points), or -1: the places the selectors draw them at rest, the one on large (the two-disc and four-disc
 * layouts as draw_selmenu puts them, the three-disc as menu_layout_mode_sel_3's) */
static int disc_hit(const menu_state *m) {
  if (m->tap_y > 330.0f || m->tap_y < 80.0f) return -1;
  int vs_kinds = m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_VS; /* (the versus select: its two discs are the two-disc selectors' places) */
  int n = m->screen == MENU_GAME ? 3 : SEL_TWO(m) || vs_kinds ? 2 : m->sel_menu == SELMENU_CAR ? 4 : 3, sel = m->sel >= 0 && m->sel < n ? m->sel : 0, best = -1;
  float bd = 1e9f;
  for (int k = 0; k < n; ++k) {
    float cx, r;
    int on = k == sel;
    if (n == 2) cx = k == 0 ? (on ? 192.0f : 212.0f) : (on ? 448.0f : 428.0f), r = on ? 118.0f : 59.0f;
    else if (n == 4) cx = disc4_cx(sel, k), r = on ? 0.5f * D4_BIG : 0.5f * D4_SMALL;
    else {
      static const float AT[3][3] = { { 139, 376, 554 }, { 84, 320, 554 }, { 84, 272, 503 } };
      cx = AT[sel][k], r = on ? 70.0f : 42.0f;
    }
    float d = fabsf(m->tap_x - cx);
    if (d < r + 6.0f && d < bd) bd = d, best = k;
  }
  return best;
}

static int tap_dir(const menu_state *m) {
  if (m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_VS) { /* the versus kinds' two discs (draw_vs_discs): a tap on the other one moves to it */
    if (m->tap_y > 300.0f) return 0;
    int hit = m->sel;
    for (int k = 0; k < 2; ++k) {
      int on = k == m->sel;
      float cx = k == 0 ? (on ? 192.0f : 212.0f) : (on ? 448.0f : 428.0f);
      if (fabsf(m->tap_x - cx) < (on ? 118.0f : 59.0f)) hit = k;
    }
    return hit < m->sel ? -1 : hit > m->sel ? 1 : 0;
  }
  if (m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_CAR) {
    /* the four discs of the Car / Settings select (menu_layout_mode_sel's places: the chosen one 128 round, the others' 64) */
    static const float AT[4][4] = { { 139, 326, 440, 554 }, { 84, 260, 440, 554 }, { 84, 198, 380, 554 }, { 84, 198, 312, 503 } };
    if (m->tap_y > 300.0f) return 0;
    const float *at = AT[m->sel >= 0 && m->sel < 4 ? m->sel : 0];
    int hit = m->sel;
    for (int k = 0; k < 4; ++k)
      if (fabsf(m->tap_x - at[k]) < (k == m->sel ? 128.0f : 64.0f)) hit = k;
    return hit < m->sel ? -1 : hit > m->sel ? 1 : 0;
  }
  if (m->screen == MENU_GAME || m->screen == MENU_VS_SEL) {
    /* menu_layout_mode_sel_3 at rest: the discs' centres (as the game mode select's) */
    static const float AT[3][3] = { { 139, 376, 554 }, { 84, 320, 554 }, { 84, 272, 503 } };
    if (m->tap_y > 300.0f) return 0;
    const float *at = AT[m->sel >= 0 && m->sel < 3 ? m->sel : 0];
    int hit = m->sel;
    for (int k = 0; k < 3; ++k)
      if (fabsf(m->tap_x - at[k]) < (k == m->sel ? 128.0f : 64.0f)) hit = k;
    return hit < m->sel ? -1 : hit > m->sel ? 1 : 0;
  }
  if (m->screen == MENU_MODE) {
    int item = m->tap_x < 320.0f ? 0 : 1;
    return item - m->sel;
  }
  /* the selected course card moves right by about 88 px per course, the strip as it slides */
  float left = 18.0f + 88.5f * (float)m->sel + course_dx(m), right = left + 164.0f;
  if (m->tap_x < left) return -1;
  if (m->tap_x > right) return 1;
  return 0;
}

/* ---- dress-up (the ghost battle's "TG" parts, src/dressdb.h). The arcade wins its parts in
 * ghost battle, a level a battle won (1..63, get_tb_level_parts); here a story race won or a
 * 10-in-a-row rival beaten. An aero set won comes up on the way back from the race:
 * - MENU_TG_GET, clSeqTgPartsGetAeroThread::run: menu_g_tg_bg_1 and _3, the top bar without
 *   time, menu_layout_tg_tune_get_title (the level's digits in templates 0x1efe, 0x1efc: no
 *   tens below 10; the kind's title in 0x1f96) and menu_layout_tg_tune_get_parts (the part's
 *   name picture in 0x1f7c, get_parts_data + 0x18). The car turns before it: 0.66 degrees a
 *   frame from 0, at (0.5, -5, -9.5) heading angle - 21.3 degrees, the camera 60 degrees
 *   across from (0, 10, 50) at the origin. The screen fades in over 10 frames, then cutin2
 *   (SE 0x22); it goes once the angle is past -185 or on a tap 3.2 s on, fading out over 10.
 * - MENU_TG_AERO, clSeqTgChangePartsAeroThread: the choices are the car's stock body and the
 *   aero sets it has won (get_parts_id_table_from_dressup_jyunsei), then Refuse to change
 *   (not on the gem cars); the layout by their number (2 menu_layout_tg_tune_aero_2, 3 _3,
 *   4 menu_layout_tg_tune_aero, 5 _5), 100 frames a place: choice i is at place
 *   r = (sel - i) mod n, resting on 100r..100r+59, sliding on 100r+60..88 (backwards going
 *   left), decided 100r+90..98. Each place's dot (0x6b4) carries a clSpriteAnimeCPModel:
 *   the car in that aero set, scaled 3 * the dot's scale, turned (10, -30, -4) degrees at
 *   (0, 20, z), seen from (0, 0, 1000) on the axis through the dot; z eases 13.33 a frame
 *   to 0 at places 0 and 1, to -400 at the others. The plate 0x1f48 becomes 0x1bc8, the
 *   names (0x1344, 0x1ec2) Original shape, Aero set A..C (the gem cars': Aero set) or Refuse
 *   to change, NEW (0xb83) only on the newest set. It opens on the newest; cursor and decide
 *   SE 0x19, 0x1a; the decision fades out over 20 frames. The arcade's 10 s limit (the port
 *   has none in the menus: the bar's infinity). ---- */
static int tg_gem(const menu_state *m) {
  const char *p = CARDB_PACKS[chosen_car(m, m->car)->colors[0].pack];
  return p[0] == 'g' && p[1] == 'e' && p[2] == 'm';
}

/* the R35 has no body parts in the arcade (its dealership says so): no aero set, bonnet, wing, mirror or sticker to change to, only the wheels
 * and the neon. The places are not offered (tg_sel_open_ok) and none is won (menu_dress_win): the models of those parts do not exist */
static int tg_no_bodyparts(const menu_state *m) { return has(chosen_car(m, m->car)->label, "[R35]"); }
static int tg_bodypart_kind(int kind) { return kind >= 1 && kind <= 5; } /* aero set, bonnet, wing, mirror, sticker */

void menu_dress_win(menu_state *m, int wins) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), gem = tg_gem(m);
  for (int k = 0; k < wins && m->dress_lv[mk][sl] < 63; ++k) {
    int lv = ++m->dress_lv[mk][sl];
    /* every part but the gem cars' empty levels (kind 9) comes up on its GET screen (the R35's body parts do not exist: none) */
    if (DRESS_LADDER[gem][lv][0] != 9 && !(tg_no_bodyparts(m) && tg_bodypart_kind(DRESS_LADDER[gem][lv][0])) && m->n_dress_due < MENU_DRESS_DUE)
      m->dress_due[m->n_dress_due++] = lv;
    /* a wheel won is chosen on its change screen after its GET (tg_wheel_open) */
    m->garage_changed = 1;
  }
}

const char *menu_wheel_get(int wheel) {
  for (size_t i = 0; wheel > 0 && i < sizeof DRESS_WHEELS / sizeof DRESS_WHEELS[0]; ++i)
    if (DRESS_WHEELS[i].model == wheel - 1) return DRESS_WHEELS[i].menu3d;
  return NULL;
}

const char *menu_wheel_pack(int wheel, int *set) {
  for (size_t i = 0; wheel > 0 && i < sizeof DRESS_WHEELS / sizeof DRESS_WHEELS[0]; ++i)
    if (DRESS_WHEELS[i].model == wheel - 1) {
      *set = DRESS_WHEELS[i].set;
      return DRESS_WHEELS[i].name;
    }
  return NULL;
}

static int tg_maker_newest(const menu_state *m, int maker);
static const signed char WM_MAKER[4];

int menu_wheel_next(const menu_state *m, int *out, int max) {
  int n = 0;
#define WANT(w) do { int w_ = (w); if (w_ > 0 && n < max) out[n++] = w_; } while (0)
  if (m->screen == MENU_TG_GET && m->tg.kind == 0) {
    WANT(m->tg.num + 1);
    WANT(m->wheel[m->maker][menu_slot(m)]);
  } else if (m->screen == MENU_TG_WHEEL) {
    for (int d = 1; d <= 2; ++d) {
      if (m->sel + d < m->tg.n) WANT(tg_wheel_value(m, m->sel + d, 0));
      if (m->sel - d >= 0) WANT(tg_wheel_value(m, m->sel - d, 0));
    }
    for (int c = 1; c < 6 && m->sel > 1; ++c) WANT(tg_wheel_value(m, m->sel, (m->tg.col + c) % 6));
  } else if (m->screen == MENU_TG_WEAR && m->tg.kind == 0) {
    WANT(m->tg.chosen);
  } else if (m->screen == MENU_TG_MAKER) {
    /* the maker's bar opens on its newest */
    WANT(tg_maker_newest(m, WM_MAKER[m->sel]));
  }
#undef WANT
  return n;
}

static void tg_get_open(menu_state *m) {
  int lv = m->dress_due[0], gem = tg_gem(m);
  for (int i = 1; i < m->n_dress_due; ++i) m->dress_due[i - 1] = m->dress_due[i];
  m->n_dress_due--;
  memset(&m->tg, 0, sizeof m->tg);
  go(m, MENU_TG_GET);
  m->tg.lv = lv;
  m->tg.gem = gem;
  m->tg.kind = DRESS_LADDER[gem][lv][0];
  m->tg.num = DRESS_LADDER[gem][lv][1];
  m->tg.name = DRESS_LADDER[gem][lv][2];
  m->tg.image = DRESS_LADDER[gem][lv][3];
  /* the wheel's turn starts at -60 degrees (clSeqTgPartsGetWheelThread::run) */
  if (m->tg.kind == 0) m->tg.angle = TG_WHEEL_START;
  m->card_t = 0;
  m->bars = 0;
  m->music = 1;
}

/* the aero sets won by the car's level, in their order (get_parts_id_table_from_dressup_jyunsei) */
static void tg_aero_open(menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), gem = tg_gem(m), lv = m->dress_lv[mk][sl];
  go(m, MENU_TG_AERO);
  m->tg.gem = gem;
  m->tg.n = 0;
  m->tg.opt[m->tg.n++] = 0;
  for (int l = 1; l <= lv && l < 64; ++l)
    if (DRESS_LADDER[gem][l][0] == 1 && m->tg.n < 4) m->tg.opt[m->tg.n++] = DRESS_LADDER[gem][l][1];
  m->tg.newest = m->tg.n - 1;
  if (!gem) m->tg.opt[m->tg.n++] = -1;
  m->sel = m->tg.newest;
  m->tg.move = m->tg.move_t = m->tg.decided = 0;
  /* the run's 30 frames before it shows: the depths settled */
  for (int i = 0; i < m->tg.n; ++i) {
    int r = ((m->sel - i) % m->tg.n + m->tg.n) % m->tg.n;
    m->tg.z[i] = r <= 1 ? 0.0f : -400.0f;
  }
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

/* after a race (or the story's tuning screen): the dress-up screens due, else the mode select */
static int tg_due(menu_state *m) {
  if (!m->n_dress_due) return 0;
  m->dress_free = 0;
  tg_get_open(m);
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
  return 1;
}


static void tg_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->screen == MENU_TG_GET) {
    int type = tg_get_type(m);
    /* the wheel turns 0.3 degrees a frame till -120 (then only fading out), the car 0.66 */
    if (type != TG_GET_WHEEL) m->tg.angle -= 0.66f;
    else if (m->tg.angle >= TG_WHEEL_END || m->deciding) m->tg.angle -= 0.3f;
    if (m->card_t == TG_FADE) snd_menu(SND_CUTIN2);
    /* the port's: a tap once the screen is in goes on (a 10-in-a-row run brings a part a
     * rival beaten); the arcade's car and wheel screens take one 3.2 s on, the others none */
    int end = type == TG_GET_CAR ? m->tg.angle < TG_GET_END
              : type == TG_GET_WHEEL ? m->tg.angle < TG_WHEEL_END : m->card_t >= TG_FADE + TG_GET_HOLD;
    if (end || (in->any && m->card_t >= TG_FADE)) m->deciding = TG_FADE;
    return;
  }
  for (int i = 0; i < m->tg.n; ++i) {
    if (tg_place(m, i) <= 1) m->tg.z[i] = m->tg.z[i] + 13.333333f > 0.0f ? 0.0f : m->tg.z[i] + 13.333333f;
    else m->tg.z[i] = m->tg.z[i] - 13.333333f < -400.0f ? -400.0f : m->tg.z[i] - 13.333333f;
  }
  if (m->tg.move) {
    if (++m->tg.move_t >= TG_MOVE) m->tg.move = 0;
    return;
  }
  if (m->card_t < TG_FADE) return;
  int d = in->dx;
  if (d) {
    snd_menu(SND_CURSOR);
    m->tg.move = d;
    m->tg.move_t = 0;
    /* the slide plays from the places before the move: kept in from */
    m->from = m->sel;
    m->sel = ((m->sel + d) % m->tg.n + m->tg.n) % m->tg.n;
    return;
  }
  if (in->ok) {
    snd_menu(SND_SELECT);
    m->tg.decided = 1;
    m->t = 0;
    m->deciding = FADE + TG_DECIDE;
  }
}

/* the decided choice: the aero set worn (Refuse to change keeps it) */
static void tg_aero_done(menu_state *m) {
  int o = m->tg.opt[m->sel];
  if (o >= 0 && o != m->aero[m->maker][menu_slot(m)]) {
    m->aero[m->maker][menu_slot(m)] = (signed char)o;
    m->garage_changed = 1;
  }
}

/* ---- the bonnet's, wing's and mirror's change (clSeqTgChangePartsBonnet/Wing/MirrorThread),
 * after their GET screen: menu_g_tg_bg_1, the stand menu_g_tg_bg_3 moved and squashed
 * (setPos, setScale: the bonnet's (20, -36), the wing's (20, 130) at half height, the
 * mirror's (20, 130) at 0.6), the layout (menu_layout_tg_tune_4_bonnet; _6_wing for a car with
 * a wing of its own in the aero set it wears (car_to_wing_number, NormalWingGetFromCarID),
 * else _5_wing; menu_layout_tg_tune_2), its arrows (parts_num_to_arrow_anim) and the top bar's
 * message. The choices (make_*_struct): the bonnet's stock and those won, in the order won;
 * the wing's Refuse to change (the one worn), stock (a wing of its own), none and those won;
 * the mirror's stock and aero. 100 frames a place: resting on 100 sel .. + 59 in a loop,
 * sliding 5 frames on from 100 sel + 79 (the wing's + 80) going right, back from the new
 * place's going left (SE 0x19); the arrows loop from 100 sel, the last choice's from the
 * tables 0x9776a70, 0x9776b20, 0x9776af4. It opens on the newest (NEW on it), the places not
 * won yet locked (setImageReplace 0x19dd, the wing's 0x1700), the dots 0x6b4..0x6b7 off. The
 * car is the card's with the choice on (menu_scene3d). Decide SE 0x1a, then the fade out over
 * 10 frames; the arcade's limit (5 s a choice and more): none here. The Hiace's lower stand
 * and car (flags +0x41, +0x4a): the port has no Hiace. A tap on a place steps towards it, on
 * the one chosen or off the bar decides. ---- */
#define TG_SLIDE 5
/* NormalWingGetFromCarID by CARDB_PACKS: bit a, a wing of its own in aero set a (0x9777348) */
static const unsigned char TG_STOCK_WING[CARDB_NPACKS] = {
  0, 0, 0, 15, 15, 15, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  0, 0, 14, 14, 10, 14, 14, 15, 14, 10, 10, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14,
  10, 0, 14, 15, 14, 15, 0, 0, /* the hidden cars */
};
/* by layout (the bonnet's, the 6 and 5 place wing's, the mirror's): its places, the slide's
 * first frame past 100 sel, the top bar's message, the lock; by place the last choice's
 * arrows, the icon, the NEW template, the template the lock replaces */
typedef struct {
  int slots, slide, topbar, lock_to;
  short arrow_last[8], icon[8], new_t[8], lock[8];
} tg_layout_def;
static const tg_layout_def TG_PART_LAYOUTS[5] = {
  { 4, 79, JID(0x1f51), JID(0x19dd), { 0, 400, 500, 300 }, { JID(0x1da1), JID(0x1da3), JID(0x1da2), JID(0x1da4) }, { 0, JID(0x18ab), JID(0x18ac), JID(0x18ad) },
    { 0, JID(0x1da3), JID(0x1da2), JID(0x1da4) } },
  { 6, 80, JID(0x1ed2), JID(0x1700), { 0, 700, 800, 900, 1000, 500 }, { JID(0x1f4b), JID(0x135b), JID(0x1048), JID(0x135c), JID(0x135d), JID(0x135f) },
    { 0, 0, 0, JID(0x18ac), JID(0x18ad), JID(0x18ae) }, { 0, 0, 0, 0, JID(0x135d), JID(0x135f) } },
  { 5, 80, JID(0x1ed2), JID(0x1700), { 0, 600, 700, 800, 400 }, { JID(0x1f4b), JID(0x1048), JID(0x135c), JID(0x135d), JID(0x135f) },
    { 0, 0, JID(0x18ac), JID(0x18ad), JID(0x18ae) }, { 0, 0, 0, JID(0x135d), JID(0x135f) } },
  { 2, 79, JID(0x1f36), 0, { 0, 100 }, { JID(0x18a9), JID(0x18aa) }, { 0, JID(0xb83) }, { 0, 0 } },
  /* the neons' (the straight neons'; the spot neons' top bar, pictures and lock below) */
  { 8, 5, JID(0x1ab3), JID(0x16f1), { 0, 900, 1000, 1100, 1200, 1300, 1400, 700 },
    { JID(0x1347), JID(0x1049), JID(0x1bc9), JID(0x1bca), JID(0x1bcb), JID(0x1bcc), JID(0x1bcd), JID(0x1bce) },
    { 0, 0, JID(0x18ad), JID(0x18ae), JID(0x18af), JID(0x18b0), JID(0x18b1), JID(0x18b2) }, { 0, 0, JID(0x1bc9), JID(0x1bca), JID(0x1bcb), JID(0x1bcc), JID(0x1bcd), JID(0x1bce) } },
};
/* the spot neons' (make_Neon_struct: wk_s 8): their pictures in the straight neons' places,
 * the lock, the top bar's message */
#define TG_SPOT_ICON JID(0x1bcf)
#define TG_SPOT_LOCK JID(0x16f3)
#define TG_SPOT_TOPBAR JID(0x1ab4)
static int tg_neon_kind(int kind) { return kind == 6 || kind == 8; }

static int tg_part_layout(const menu_state *m) {
  return tg_neon_kind(m->tg.kind) ? 4 : m->tg.kind == 2 ? 0 : m->tg.kind == 4 ? 3 : m->tg.slots == 6 ? 1 : 2;
}

static int tg_part_anim(const menu_state *m, int arrow) {
  switch (tg_part_layout(m)) {
  case 0: return arrow ? ID.tg_bonnet_arrow : ID.tg_bonnet;
  case 1: return arrow ? ID.tg_wing6_arrow : ID.tg_wing6;
  case 2: return arrow ? ID.tg_wing5_arrow : ID.tg_wing5;
  case 4: return arrow ? ID.tg_neon_arrow : ID.tg_neon;
  default: return arrow ? ID.tg_mirror_arrow : ID.tg_mirror;
  }
}

static int tg_part_frame(const menu_state *m) {
  int slide = TG_PART_LAYOUTS[tg_part_layout(m)].slide;
  if (tg_part_layout(m) == 4) {
    /* the neons' (0x9409ec0, 0x9409ee0): held on 10 sel + 1; a move right plays 10 old +
     * 5.. for 5 frames, left the same of the new place backwards */
    if (m->tg.move > 0) return 10 * m->from + slide + m->tg.move_t;
    if (m->tg.move < 0) return 10 * m->sel + slide + TG_SLIDE - 1 - m->tg.move_t;
    return 10 * m->sel + 1;
  }
  if (m->tg.move > 0) return 100 * m->from + slide + m->tg.move_t;
  if (m->tg.move < 0) return 100 * m->sel + slide + TG_SLIDE - 1 - m->tg.move_t;
  return 100 * m->sel + (m->card_t - m->tg.rest_t) % 60;
}

/* the arrows keep looping from the place before a slide until it ends (an_reset) */
static int tg_part_arrow_frame(const menu_state *m) {
  int i = m->tg.move ? m->from : m->sel;
  int base = i == m->tg.n - 1 ? TG_PART_LAYOUTS[tg_part_layout(m)].arrow_last[i] : 100 * i;
  return base + (m->card_t - m->tg.rest_t) % 60;
}

static void tg_part_open(menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), gem = m->tg.gem, lv = m->dress_lv[mk][sl], kind = m->tg.kind;
  go(m, MENU_TG_PART);
  m->tg.n = 0;
  if (kind == 2) {
    m->tg.slots = 4;
    m->tg.opt[m->tg.n++] = 0;
  } else if (kind == 3) {
    int own = TG_STOCK_WING[chosen_car(m, c)->colors[m->color].pack] >> m->aero[mk][sl] & 1;
    m->tg.slots = own ? 6 : 5;
    m->tg.opt[m->tg.n++] = m->wing[mk][sl];
    if (own) m->tg.opt[m->tg.n++] = 0;
    m->tg.opt[m->tg.n++] = 4;
  } else if (tg_neon_kind(kind)) {
    /* get_parts_id_table_from_dressup_neon: none, the one worn (Refuse to change), then the
     * kind's won so far */
    m->tg.slots = 8;
    m->tg.opt[m->tg.n++] = 0;
    m->tg.opt[m->tg.n++] = m->neon[mk][sl];
  } else {
    m->tg.slots = 2;
    m->tg.opt[m->tg.n++] = 0;
    m->tg.opt[m->tg.n++] = 1;
  }
  if (kind != 4)
    for (int l = 1; l <= lv && l < 64; ++l)
      if (DRESS_LADDER[gem][l][0] == kind && m->tg.n < m->tg.slots) m->tg.opt[m->tg.n++] = DRESS_LADDER[gem][l][1];
  m->tg.newest = m->tg.n - 1;
  m->sel = m->tg.newest;
  /* the neons' from the dress-up menu (the arcade's past level 63) open on Refuse to change */
  if (tg_neon_kind(kind) && m->dress_free) m->sel = 1;
  m->from = m->sel;
  m->tg.move = m->tg.move_t = m->tg.decided = m->tg.rest_t = 0;
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

/* the place a tap is on (its icon, as the frame shows it), else -1 */
static int tg_part_hit(const menu_state *m) {
  const tg_layout_def *d = &TG_PART_LAYOUTS[tg_part_layout(m)];
  int lay = tg_part_anim(m, 0), f = tg_part_frame(m), best = -1;
  float best_d = 1e9f;
  /* the nearest place whose picture holds the tap: the neons' pictures have wide clear
   * margins, so their rects overlap the next place's */
  for (int k = 0; k < d->slots; ++k) {
    float r[4];
    if (anm_sprite_rect(lay, f, anm_sprite(d->icon[k]), r) && m->tap_x >= r[0] - 8.0f && m->tap_x <= r[2] + 8.0f &&
        m->tap_y >= r[1] - 8.0f && m->tap_y <= r[3] + 8.0f) {
      float dx = m->tap_x - 0.5f * (r[0] + r[2]), dy = m->tap_y - 0.5f * (r[1] + r[3]);
      if (dx * dx + dy * dy < best_d) best_d = dx * dx + dy * dy, best = k;
    }
  }
  return best;
}

static void tg_part_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->tg.move) {
    if (++m->tg.move_t >= TG_SLIDE) m->tg.move = 0, m->tg.rest_t = m->card_t;
    return;
  }
  if (m->card_t < TG_FADE) return;
  int d = in->dx;
  if (in->select >= 0) d = in->select == m->sel ? 0 : in->select > m->sel ? 1 : -1; /* a place chosen: a step towards it */
  if ((d > 0 && m->sel < m->tg.n - 1) || (d < 0 && m->sel > 0)) {
    snd_menu(SND_CURSOR);
    m->tg.move = d;
    m->tg.move_t = 0;
    m->from = m->sel;
    m->sel += d;
    return;
  }
  if (in->ok && !d) {
    snd_menu(SND_SELECT);
    m->tg.decided = 1;
    m->deciding = TG_FADE;
  }
}

/* the part the card keeps (card+6, +7, +0xc) */
static signed char *tg_part_value(menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m);
  if (tg_neon_kind(m->tg.kind)) return &m->neon[mk][sl];
  return m->tg.kind == 2 ? &m->bonnet[mk][sl] : m->tg.kind == 3 ? &m->wing[mk][sl] : &m->mirror[mk][sl];
}

/* the decided choice worn */
static void tg_part_done(menu_state *m) {
  int o = m->tg.opt[m->sel];
  signed char *v = tg_part_value(m);
  if (*v != o) {
    *v = (signed char)o;
    m->garage_changed = 1;
  }
}

/* ---- the wheel's change (clSeqTgChangePartsWheelThread: run 0x886d390, yield 0x886c8a8,
 * an_reset 0x886caf8), after its GET screen (mseqTarget: seqTgPartsGet, seqTgChangeParts,
 * seqTgWearParts). The places (get_parts_id_table_from_dressup_wheel): stock, No change (the
 * one worn), then the won one's maker's wheels won so far in the ladder's order. The bar
 * (menu_g_tg_tune_wheel_<n>, n the maker's wheels and 2: wheel_maker_to_number) has a place
 * for each, those not won yet locked (menu_p_t_tuning_wheel_no), NEW on the newest, where it
 * opens; its font dots off. It rests on frame 10 sel; a move right plays 10 old + 4..8, left
 * 10 new + 8..4 (SE 0x19). Rays's 16 places scroll the bar 10 pixels a frame past a window:
 * right, the cursor's place goes up to 8, then the bar scrolls till the 14th place; left,
 * down to 3, then it scrolls from the 3rd. The arrows (menu_layout_tg_tune_wheel_arrow_*)
 * loop 59 frames from 100 times the cursor's place (the last place's from
 * wheel_maker_to_left_arrow_frame). A wheel of more than one colour has the colour column:
 * its swatches (col_col, get_parts_data(0, wheel, colour, gem) + 4 in 0x1e60.., frame 6 -
 * colours), the cursor (col_cursor, 80 frames from 0, 80, 180, .. by colour) and the top
 * bar's message 0x1f0a (else 0x1f6e); the panel (col_waku_2, frame 6 - colours) shows the
 * colour's big picture (+ 0x24; No change's: the worn one's first colour; stock none),
 * col_logo the name logo (+ 0x1c). The cabinet's colour button turns the colour: here a tap
 * on a swatch or the panel. A tap on a place steps towards it, off the bar it decides (SE
 * 0x1a), then the fade out; the arcade's limit (5 s a place and 5): none here. ---- */
#define TG_WHEEL_SLIDE 5
enum { T_WHEEL_STOCK = JID(0x16ff), T_WHEEL_KEEP = JID(0x18b4), T_WHEEL_FIRST = JID(0xbb3), T_WHEEL_SWATCH = JID(0x1e60), T_WHEEL_DOT = JID(0x6b4),
       T_WHEEL_LOGO = JID(0x1ed4), S_WHEEL_LOCK = JID(0x18b5), S_WHEEL_MES = JID(0x1f0a), S_WHEEL_MES_ONE = JID(0x1f6e) };
/* by maker: wheel_maker_to_number, wheel_maker_to_left_arrow_frame by the cursor's place */
static const int WHEEL_PLACES[5] = { 10, 5, 16, 8, 11 };
static const short WHEEL_LEFT_ARROW[5][11] = {
  { 0, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 0 },
  { 0, 600, 700, 800, 900, 1500, 1600, 1700, 1800, 1900, 0 },
  { 0, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100 },
  { 0, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1900, 2000, 2100 },
  { 0, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100 },
};
/* the places' NEW marks (0x9408be0: menu_p_t_tuning_new_tmp3..16), the colour cursor's loops */
static const short WHEEL_NEW_T[16] = { 0, 0, JID(0x18ad), JID(0x18ae), JID(0x18af), JID(0x18b0), JID(0x18b1), JID(0x18b2),
                                       JID(0x18b3), JID(0x19df), JID(0x19e0), JID(0x19e1), JID(0x19e2), JID(0x19e3), JID(0x19e4), JID(0x19e5) };
static const short WHEEL_COL_FRAME[6] = { 0, 80, 180, 280, 380, 480 };

/* the bar of the maker's wheels (after a GET: the won one's maker; from the dress-up menu:
 * the maker select's, the gem cars' 4) */
static void tg_wheel_open_at(menu_state *m, int maker) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), gem = m->tg.gem, lv = m->dress_lv[mk][sl];
  go(m, MENU_TG_WHEEL);
  m->tg.kind = 0;
  m->tg.maker = maker;
  int places = WHEEL_PLACES[m->tg.maker];
  m->tg.n = 0;
  m->tg.wl[m->tg.n++] = 0;
  m->tg.wl[m->tg.n++] = m->wheel[mk][sl];
  for (int l = 1; l <= lv && l < 64 && m->tg.n < places; ++l) {
    const dress_wheel_set *w = DRESS_LADDER[gem][l][0] == 0 ? wheel_set(DRESS_LADDER[gem][l][1] + 1, NULL) : NULL;
    if (w && w->maker == m->tg.maker) m->tg.wl[m->tg.n++] = DRESS_LADDER[gem][l][1] + 1;
  }
  m->tg.newest = m->tg.n - 1;
  m->sel = m->from = m->tg.newest;
  m->tg.col = 0;
  /* the bar's window: the moves right from the first place (run 0x886da1b) */
  m->tg.win = 0;
  m->tg.scroll = 0;
  for (int i = 0; i < m->sel; ++i) {
    if (places <= 11 || m->tg.win <= 7 || i > 12) m->tg.win++;
    else m->tg.scroll -= 50.0f;
  }
  m->tg.scroll_dir = 0;
  m->tg.move = m->tg.move_t = m->tg.decided = 0;
  m->tg.reset_t = 0;
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

static void tg_wheel_open(menu_state *m) {
  const dress_wheel_set *won = wheel_set(m->tg.num + 1, NULL);
  tg_wheel_open_at(m, won ? won->maker : 0);
}

/* the place's colours (WheelModelIDMemberGetFromWheelModelID; stock's 6) */
static int tg_wheel_colours(const menu_state *m, int i) {
  const dress_wheel_set *w = i ? wheel_set(m->tg.wl[i], NULL) : NULL;
  return w ? w->n : 6;
}

static int tg_wheel_column(const menu_state *m) { return m->sel > 1 && tg_wheel_colours(m, m->sel) > 1; }

static int tg_wheel_bar_frame(const menu_state *m) {
  if (m->tg.move > 0) return 10 * m->from + 4 + m->tg.move_t;
  if (m->tg.move < 0) return 10 * m->sel + 8 - m->tg.move_t;
  return 10 * m->sel;
}

static int near_rect(const menu_state *m, const float r[4], float dx, float pad) {
  return m->tap_x >= r[0] + dx - pad && m->tap_x <= r[2] + dx + pad && m->tap_y >= r[1] - pad && m->tap_y <= r[3] + pad;
}

/* the place a tap is on, else -1 */
static int tg_wheel_hit(const menu_state *m) {
  int bar = ID.tg_wheel_bar[m->tg.maker], f = tg_wheel_bar_frame(m);
  for (int i = 0; i < m->tg.n; ++i) {
    float r[4];
    int t = i == 0 ? T_WHEEL_STOCK : i == 1 ? T_WHEEL_KEEP : T_WHEEL_FIRST + i;
    if (anm_sprite_rect(bar, f, anm_sprite(t), r) && near_rect(m, r, m->tg.scroll, 4.0f)) return i;
  }
  return -1;
}

/* the colour a tap is on (its swatch; the panel: the next one), else -1 */
static int tg_wheel_colour_hit(const menu_state *m) {
  if (!tg_wheel_column(m)) return -1;
  int n = tg_wheel_colours(m, m->sel);
  for (int b = 0; b < n; ++b) {
    float r[4];
    if (anm_sprite_rect(ID.tg_wheel_col, 6 - n, anm_sprite(T_WHEEL_SWATCH + b), r) && near_rect(m, r, 0, 3.0f)) return b;
  }
  if (m->tap_x < 176.0f && m->tap_y >= 97.0f && m->tap_y < 300.0f) return (m->tg.col + 1) % n;
  return -1;
}

static void tg_wheel_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->tg.move) {
    m->tg.scroll += 10.0f * (float)m->tg.scroll_dir;
    if (++m->tg.move_t >= TG_WHEEL_SLIDE) m->tg.move = 0, m->tg.scroll_dir = 0;
    return;
  }
  if (m->card_t < TG_FADE) return;
  int d = in->dx, decide = in->ok;
  if ((in->view || in->dy) && tg_wheel_column(m)) { /* the view button is the cabinet's colour button: the next colour; the shifter's down and up
                                                      * the next and the one before (a controller has no pointer to tap a swatch with) */
    int n = tg_wheel_colours(m, m->sel);
    snd_menu(SND_CURSOR);
    m->tg.col = (m->tg.col + (in->dy < 0 ? n - 1 : 1)) % n;
    m->tg.reset_t = m->card_t;
    return;
  }
  if (in->tab >= 0) { /* a colour chosen outright */
    if (in->tab != m->tg.col) {
      snd_menu(SND_CURSOR);
      m->tg.col = in->tab;
      m->tg.reset_t = m->card_t;
    }
    return;
  }
  if (in->select >= 0) { /* a place chosen: a step towards it, on it the decision */
    int k = in->select;
    d = k > m->sel ? 1 : k < m->sel ? -1 : 0, decide = k == m->sel;
  }
  if ((d > 0 && m->sel < m->tg.n - 1) || (d < 0 && m->sel > 0)) {
    snd_menu(SND_CURSOR);
    m->tg.move = d;
    m->tg.move_t = 0;
    m->from = m->sel;
    m->sel += d;
    m->tg.col = 0;
    if (WHEEL_PLACES[m->tg.maker] <= 11) m->tg.win = m->sel;
    else if (d > 0 && m->tg.win > 7 && m->sel <= 13) m->tg.scroll_dir = -1;
    else if (d < 0 && m->tg.win <= 3 && m->sel > 2) m->tg.scroll_dir = 1;
    else m->tg.win += d;
    m->tg.reset_t = m->card_t;
    return;
  }
  if (decide) {
    snd_menu(SND_SELECT);
    m->tg.decided = 1;
    m->deciding = TG_FADE;
  }
}

static void draw_tg_wheel(const menu_state *m, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  int mk = m->tg.maker, sel = m->sel, n = m->tg.n, cols = tg_wheel_colours(m, sel), t = m->card_t - m->tg.reset_t;
  const dress_wheel_set *w = sel ? wheel_set(m->tg.wl[sel], NULL) : NULL;
  int column = tg_wheel_column(m);
  anm_draw(ID.tg_bg1, 0, fade);
  if (draw3d) draw3d(ctx);
  /* the panel: the colour's big picture */
  int pic = !w ? -1 : sel == 1 ? w->pic[0] : w->pic[m->tg.col];
  anm_swap big = { anm_sprite(T_WHEEL_DOT), pic > 0 ? anm_sprite(pic) : -1 };
  anm_draw_ex(ID.tg_wheel_waku, 6 - cols, fade, 0, 0, &big, 1);
  if (column) {
    anm_draw(ID.tg_wheel_cursor, WHEEL_COL_FRAME[m->tg.col] + t % 80, fade);
    anm_swap sw[6];
    for (int b = 0; b < 6; ++b) sw[b] = (anm_swap){ anm_sprite(T_WHEEL_SWATCH + b), b < cols ? anm_sprite(w->swatch[b]) : -1 };
    anm_draw_ex(ID.tg_wheel_col, 6 - cols, fade, 0, 0, sw, 6);
  }
  if (w) {
    anm_swap logo = { anm_sprite(T_WHEEL_LOGO), anm_sprite(w->logo) };
    anm_draw_ex(ID.tg_wheel_logo, 0, fade, 0, 0, &logo, 1);
  }
  /* the bar: the places' pictures (get_parts_data + 0x10), the locks, NEW on the newest */
  anm_swap sw[11 + 2 * 16];
  int k = 0;
  for (int i = 0; i < 11; ++i) sw[k++] = (anm_swap){ anm_sprite(T_WHEEL_DOT + i), -1 };
  for (int i = 2; i < WHEEL_PLACES[mk]; ++i) {
    const dress_wheel_set *p = i < n ? wheel_set(m->tg.wl[i], NULL) : NULL;
    sw[k++] = (anm_swap){ anm_sprite(T_WHEEL_FIRST + i), i >= n ? anm_sprite(S_WHEEL_LOCK) : p ? anm_sprite(p->bar) : -1 };
    /* (the dress-up menu's, the arcade's once every part is won: no NEW) */
    sw[k++] = (anm_swap){ anm_sprite(WHEEL_NEW_T[i]), i == m->tg.newest && !m->dress_free ? anm_sprite(T_TG_NEW) : -1 };
  }
  anm_draw_ex(ID.tg_wheel_bar[mk], tg_wheel_bar_frame(m), fade, m->tg.scroll, 0, sw, k);
  int win = m->tg.win < 11 ? m->tg.win : 10;
  anm_draw(ID.tg_wheel_arrow[mk], (sel == n - 1 ? WHEEL_LEFT_ARROW[mk][win] : 100 * win) + t % 59, fade);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(column ? S_WHEEL_MES : S_WHEEL_MES_ONE) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

/* the decided place worn (wk_s + 4) */
static void tg_wheel_done(menu_state *m) {
  int v = tg_wheel_value(m, m->sel, m->tg.col);
  if (m->wheel[m->maker][menu_slot(m)] != v) {
    m->wheel[m->maker][menu_slot(m)] = (unsigned char)v;
    m->garage_changed = 1;
  }
}

/* ---- the stickers' change (clSeqTgChangePartsStickerThread: run 0x8876c1c, initAnim
 * 0x88751a0, make_Sticker_struct 0x88762ac, yield 0x88758a4, check_body 0x8875e24), after
 * their GET screen and from the dress-up menu. The places
 * (get_parts_id_table_from_dressup_sticker): none, then the ladder's stickers in its order
 * (line 1, fire 1, line 2, fire 2, Pac-Man, namco, the works colours, line 3, fire 3,
 * tribal), those not won yet locked (menu_p_tg_tune_vinyl_no), NEW on the newest (its font
 * dots off); the gem cars' none and their works colours. A sticker's number is its place's
 * plus the colour: the lines', fires' and tribal's ten (fire 2's swatches its gradations),
 * the gem cars' works colours' five (swatches 1, 2, 3, 9, 8); the others have no colour
 * (the works colours' by the paint). Up to level 63 it opens on the newest in its first
 * colour, past it (here the dress-up menu) on the one worn. The bar
 * (menu_layout_tg_tune_8_vinyl) rests on 10 sel; a move right plays 10 old + 5..9, left the
 * new place's backwards (SE 0x19); the gem cars' (_2_gem) rest on 100 sel and slide on
 * 79..83. The colour button (SE 0x19) turns the colour, back to the first after the last:
 * here a tap on a swatch picks it, on the column's frame the next. The cursor's place on
 * the bar's window goes up to 5 moving right, then the bar scrolls 70 pixels (14 a frame)
 * till the 8th place; left down to 3, then it scrolls from the 3rd. The arrows
 * (menu_layout_tg_tune_vinyl_arrow_8, the gem cars' _2_arrow) loop 60 frames from 100 times
 * the window's place (the last won place's 0x9776c80). The colour column (_8_vinyl_col)
 * shows on the places with colours, its cursor looping 100 frames from 500 + 100 colour
 * (the gem cars' 100 colour), with the top bar's message 0x1f6d (else 0x1f09). The arcade
 * waits 8 frames after a key before the move: none here. A tap on a place steps towards
 * it, on the one chosen or off the bar decides (SE 0x1a). ---- */
#define TG_VINYL_SLIDE 5
#define TG_VINYL_PLACES 11
enum { S_VINYL_LOCK = JID(0x1702), T_VINYL_NONE = JID(0x16e6), T_VINYL_SWATCH = JID(0x1da5), S_VINYL_GRADATION = JID(0x1f2b),
       T_VINYL_WORKS = JID(0x1cc2), T_VINYL_WORKS_GEM = JID(0x1f06), S_VINYL_MES_COL = JID(0x1f6d), S_VINYL_MES = JID(0x1f09) };
/* the places' pictures (0x9409220) and NEW marks (0x9409260) */
static const short VINYL_ICON[TG_VINYL_PLACES] = { T_VINYL_NONE, JID(0x1bde), JID(0x1bdb), JID(0x1bdf), JID(0x1bdc), JID(0x18cb), JID(0x1aaa),
                                                   T_VINYL_WORKS, JID(0x1be0), JID(0x1bdd), JID(0x1daf) };
static const short VINYL_NEW_T[TG_VINYL_PLACES] = { 0, JID(0x18ac), JID(0x18ad), JID(0x18ae), JID(0x18af), JID(0x18b0), JID(0x18b1), JID(0x18b2),
                                                    JID(0x18b3), JID(0x19df), JID(0x19e0) };
/* the last won place's arrows by the window's place (0x9776c80) */
static const short VINYL_LAST_ARROW[8] = { 0, 900, 1000, 1100, 1200, 1300, 1400, 700 };

/* the colours of the place whose first sticker is v (make_Sticker_struct's 1 and 2: the
 * lines', fires' and tribal's; the gem cars' works colours) */
static int vinyl_colours(int gem, int v) {
  if (gem) return v == 43 ? 5 : 0;
  return v == 1 || v == 11 || v == 21 || v == 31 || v == 48 || v == 58 || v == 68 ? 10 : 0;
}

static int tg_vinyl_value(const menu_state *m) {
  int v = m->tg.wl[m->sel];
  return v + (vinyl_colours(m->tg.gem, v) ? m->tg.col : 0);
}

/* whether s starts with pre (whole: and ends there); the patched iOS SDK has no plain strcmp
 * or strncmp for armv7 */
static int name_is(const char *s, const char *pre, int whole) {
  while (*pre && *s == *pre) s++, pre++;
  return !*pre && (!whole || !*s);
}

/* the works colours' picture: the car's maker's (get_car_data + 0: menu_p_tg_tune_vinyl_maker_*) */
static int vinyl_works_icon(const menu_state *m) {
  const char *p = CARDB_PACKS[chosen_car(m, m->car)->colors[0].pack];
  if (name_is(p, "nis_R35", 0) || name_is(p, "gem", 0)) return 0x1cc2;
  if (name_is(p, "nis_SELEN", 1)) return 0x1cc8;
  if (name_is(p, "nis_KPGC10", 1)) return 0x1cc9;
  static const char *const MK[5] = { "maz", "mit", "nis", "sub", "toy" };
  for (int k = 0; k < 5; ++k)
    if (name_is(p, MK[k], 0)) return 0x1cc3 + k;
  return 0x1cc2;
}

static void tg_vinyl_open(menu_state *m) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), gem = tg_gem(m), lv = m->dress_lv[mk][sl], v = m->vinyl[mk][sl];
  go(m, MENU_TG_VINYL);
  m->tg.kind = 5;
  m->tg.gem = gem;
  m->tg.slots = m->tg.n = 1;
  m->tg.wl[0] = 0;
  for (int l = 1; l < 64 && m->tg.slots < TG_VINYL_PLACES; ++l)
    if (DRESS_LADDER[gem][l][0] == 5) {
      m->tg.wl[m->tg.slots++] = DRESS_LADDER[gem][l][1];
      if (l <= lv) m->tg.n = m->tg.slots;
    }
  m->tg.newest = m->tg.n - 1;
  m->sel = m->tg.newest;
  m->tg.col = 0;
  if (m->dress_free)
    for (int i = 0; i < m->tg.n; ++i) {
      int w = m->tg.wl[i], k = vinyl_colours(gem, w);
      if (v == w || (k && v > w && v < w + k)) m->sel = i, m->tg.col = v - w;
    }
  m->from = m->sel;
  /* the bar's window: the moves right from the first place (run 0x8876f3e) */
  m->tg.win = 0;
  m->tg.scroll = 0;
  for (int i = 0; i < m->sel; ++i) {
    if (gem || m->tg.win <= 4 || i > 7) m->tg.win++;
    else m->tg.scroll -= 70.0f;
  }
  m->tg.scroll_dir = 0;
  m->tg.move = m->tg.move_t = m->tg.decided = 0;
  m->tg.reset_t = 0;
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

static int tg_vinyl_bar(const menu_state *m) { return m->tg.gem ? ID.tg_vinyl_gem : ID.tg_vinyl; }

static int tg_vinyl_bar_frame(const menu_state *m) {
  if (m->tg.gem) return m->tg.move > 0 ? 79 + m->tg.move_t : m->tg.move < 0 ? 83 - m->tg.move_t : 100 * m->sel;
  if (m->tg.move > 0) return 10 * m->from + 5 + m->tg.move_t;
  if (m->tg.move < 0) return 10 * m->sel + 9 - m->tg.move_t;
  return 10 * m->sel;
}

static int tg_vinyl_col_frame(const menu_state *m) {
  return (m->tg.gem ? 0 : 500) + 100 * m->tg.col + (m->card_t - m->tg.reset_t) % 100;
}

/* the place a tap is on, else -1 */
static int tg_vinyl_hit(const menu_state *m) {
  int bar = tg_vinyl_bar(m), f = tg_vinyl_bar_frame(m);
  for (int i = 0; i < m->tg.slots; ++i) {
    float r[4];
    int t = m->tg.gem ? (i ? T_VINYL_WORKS_GEM : T_VINYL_NONE) : VINYL_ICON[i];
    if (anm_sprite_rect(bar, f, anm_sprite(t), r) && near_rect(m, r, m->tg.scroll, 4.0f)) return i;
  }
  return -1;
}

/* the colour a tap is on (its swatch; the column's frame: the next one), else -1 */
static int tg_vinyl_colour_hit(const menu_state *m) {
  int n = vinyl_colours(m->tg.gem, m->tg.wl[m->sel]);
  if (!n) return -1;
  float r[4];
  for (int b = 0; b < n; ++b)
    if (anm_sprite_rect(ID.tg_vinyl_col, tg_vinyl_col_frame(m), anm_sprite(T_VINYL_SWATCH + b), r) && near_rect(m, r, 0, 2.0f))
      return b;
  if (anm_sprite_rect(ID.tg_vinyl_col, tg_vinyl_col_frame(m), anm_sprite(JID(0x1e54)), r) && near_rect(m, r, 0, 4.0f))
    return (m->tg.col + 1) % n;
  return -1;
}

static void tg_vinyl_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->tg.move) {
    m->tg.scroll += 14.0f * (float)m->tg.scroll_dir;
    if (++m->tg.move_t >= TG_VINYL_SLIDE) m->tg.move = 0, m->tg.scroll_dir = 0, m->tg.reset_t = m->card_t;
    return;
  }
  if (m->card_t < TG_FADE) return;
  int d = in->dx, decide = in->ok;
  if (in->view || in->dy) { /* the view button: the next colour (the cabinet's colour button); the shifter's down and up: the next and the one before */
    int n = vinyl_colours(m->tg.gem, m->tg.wl[m->sel]);
    if (n) {
      snd_menu(SND_CURSOR);
      m->tg.col = (m->tg.col + (in->dy < 0 ? n - 1 : 1)) % n;
      m->tg.reset_t = m->card_t;
    }
    return;
  }
  if (in->tab >= 0) { /* a colour chosen outright */
    if (in->tab != m->tg.col) {
      snd_menu(SND_CURSOR);
      m->tg.col = in->tab;
      m->tg.reset_t = m->card_t;
    }
    return;
  }
  if (in->select >= 0) { /* a place chosen: a step towards it, on it the decision */
    int k = in->select;
    d = k > m->sel ? 1 : k < m->sel ? -1 : 0, decide = k == m->sel;
  }
  if ((d > 0 && m->sel < m->tg.n - 1) || (d < 0 && m->sel > 0)) {
    snd_menu(SND_CURSOR);
    m->tg.move = d;
    m->tg.move_t = 0;
    m->from = m->sel;
    m->sel += d;
    m->tg.col = 0;
    /* setScroll: right, the window's place up to 5, then the bar till the 8th place; left,
     * down to 3, then the bar from the 3rd */
    if (m->tg.gem) m->tg.win = m->sel;
    else if (d > 0 && m->tg.win > 4 && m->sel <= 8) m->tg.scroll_dir = -1;
    else if (d < 0 && m->tg.win <= 3 && m->sel > 2) m->tg.scroll_dir = 1;
    else m->tg.win += d;
    return;
  }
  if (decide) {
    snd_menu(SND_SELECT);
    m->tg.decided = 1;
    m->deciding = TG_FADE;
  }
}

static void draw_tg_vinyl(const menu_state *m, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  int sel = m->sel, n = m->tg.n, t = m->card_t - m->tg.reset_t, cols = vinyl_colours(m->tg.gem, m->tg.wl[sel]);
  anm_draw(ID.tg_bg1, 0, fade);
  /* yield: the stand scaled (0.75, 1.55) at (160.5, -184.5) */
  anm_draw_xy(ID.tg_bg3, 0, fade, 160.5f, -184.5f, 0.75f, 1.55f, 0, 0);
  if (draw3d) draw3d(ctx);
  /* the bar: the locks, the works colours' maker, NEW on the newest, the font dots off */
  anm_swap sw[2 * TG_VINYL_PLACES + 2];
  int k = 0;
  sw[k++] = (anm_swap){ anm_sprite(T_TG_DOT), -1 };
  if (m->tg.gem) {
    sw[k++] = (anm_swap){ anm_sprite(T_TG_NEW), m->dress_free ? -1 : anm_sprite(T_TG_NEW) };
  } else {
    for (int i = 1; i < m->tg.slots; ++i) {
      int to = i >= n ? anm_sprite(S_VINYL_LOCK) : VINYL_ICON[i] == T_VINYL_WORKS ? anm_sprite(vinyl_works_icon(m)) : -2;
      if (to != -2) sw[k++] = (anm_swap){ anm_sprite(VINYL_ICON[i]), to };
      sw[k++] = (anm_swap){ anm_sprite(VINYL_NEW_T[i]), i == m->tg.newest && !m->dress_free ? anm_sprite(T_TG_NEW) : -1 };
    }
  }
  anm_draw_ex(tg_vinyl_bar(m), tg_vinyl_bar_frame(m), fade, m->tg.scroll, 0, sw, k);
  /* the colour column (fire 2's gradations; the gem cars' 4th and 5th swatches the 9th and 8th) */
  if (cols) {
    anm_swap cs[10];
    int c = 0;
    if (m->tg.gem) {
      cs[c++] = (anm_swap){ anm_sprite(T_VINYL_SWATCH + 3), anm_sprite(T_VINYL_SWATCH + 8) };
      cs[c++] = (anm_swap){ anm_sprite(T_VINYL_SWATCH + 4), anm_sprite(T_VINYL_SWATCH + 7) };
    } else if (m->tg.wl[sel] == 31) {
      for (int b = 0; b < 10; ++b) cs[c++] = (anm_swap){ anm_sprite(T_VINYL_SWATCH + b), anm_sprite(S_VINYL_GRADATION + b) };
    }
    anm_draw_ex(ID.tg_vinyl_col, tg_vinyl_col_frame(m), fade, 0, 0, cs, c);
  }
  int w = m->tg.win < 8 ? m->tg.win : 7;
  int arrow = m->tg.gem ? 100 * w : sel == n - 1 ? VINYL_LAST_ARROW[w] : 100 * w;
  anm_draw(m->tg.gem ? ID.tg_mirror_arrow : ID.tg_vinyl_arrow, arrow + t % 60, fade);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(cols ? S_VINYL_MES_COL : S_VINYL_MES) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

/* the decided sticker worn (wk_s + 4) */
static void tg_vinyl_done(menu_state *m) {
  int v = tg_vinyl_value(m);
  if (m->vinyl[m->maker][menu_slot(m)] != v) {
    m->vinyl[m->maker][menu_slot(m)] = (unsigned char)v;
    m->garage_changed = 1;
  }
}

/* the dress-up menu left: back to the card check it was opened from */
static void tg_free_leave(menu_state *m) {
  m->dress_free = 0;
  if (m->carmenu) { /* (opened from the Car / Settings discs) */
    m->carmenu = 0;
    selmenu_enter(m, tg_no_bodyparts(m) ? SELMENU_CAR : SELMENU_CUSTOM, tg_no_bodyparts(m) ? 1 : 0); /* (the R35's customize has only the dress-up: back to the discs) */
    return;
  }
  card_enter(m, m->card_back);
}

static void tg_leave(menu_state *m) {
  go(m, MENU_GAME);
  m->sel = game_slot(m->game);
  m->slide = m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

/* ---- the dress-up menu (seqTgDressUpPartsSelect, clTgDressUpPartsSelect: ctor 0x8883188,
 * run 0x8883d50, yield 0x8882eb4): the arcade's comes after a ghost battle once the car has
 * won every part; here the car setting's wheel key opens it at any level.
 * menu_g_tg_bg_1, the stand menu_g_tg_bg_3 at (20, 130) at half height, the card's car
 * turned -45 degrees about y at the origin (environment 0, setInterest((0, 7.5, 50), (0,
 * -2.5, 0))), the places' layout (menu_layout_tg_tune_all, the gem cars' _gem_all; its
 * stand template 0x104a off) and the top bar's message 0x1e5a. The places (0x940a280):
 * Refuse to change, then the aero set, wheel, bonnet, wing, mirror, sticker, neon and neon
 * spot (the gem cars': aero set, wheel, sticker, gull wing). 100 frames a place: resting on
 * 100 sel .. + 59 in a loop; a move right plays 100 old + 90..94, left the new place's
 * backwards (SE 0x19), no wrap; decided 100 sel + 60..69 (SE 0x1a), then the fade. It
 * opens on the kind chosen last (wk_s + 0). The arcade goes on from the part's change and
 * wear (or Refuse to change) to the game; here the wear comes back to the menu, and Refuse
 * to change or the back key goes back to the car setting. The gull wing's screen is not
 * ported: its place only sounds SE 0x1b. A tap on a place
 * steps towards it, on the one chosen or off the places decides. ---- */
#define TG_SEL_SLIDE 5
#define TG_SEL_DECIDE 10
static const signed char TG_SEL_KIND[2][9] = { { 9, 1, 0, 2, 3, 4, 5, 6, 8 }, { 9, 1, 0, 5, 7 } };
static const int TG_SEL_PLACES[2] = { 9, 5 };
/* the places' pictures (Refuse to change's, menu_p_t_tune_parts_*) */
static const short TG_SEL_ICON[2][9] = { { JID(0x16fe), JID(0x18a0), JID(0x19da), JID(0x1a9e), JID(0x18a3), JID(0x1a9f), JID(0x19d9), JID(0x18a2), JID(0x1d75) },
                                         { JID(0x16fe), JID(0x18a0), JID(0x19da), JID(0x19d9), JID(0x18a1) } };
enum { T_SEL_DODAI = JID(0x104a), S_SEL_MES = JID(0x1e5a), S_MAKER_MES = JID(0x1ccf) };

static int tg_sel_ported(int kind) { return kind <= 5 || kind == 9 || tg_neon_kind(kind); }
static void tg_maker_open(menu_state *m);

/* the port's (the arcade's menu comes once every part is won): an aero set, wheel or
 * bonnet with none won and stock on has nothing to change to, so its place only sounds
 * SE 0x1b too */
static int tg_sel_open_ok(const menu_state *m, int kind) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), lv = m->dress_lv[mk][sl];
  if (!tg_sel_ported(kind)) return 0;
  if (tg_no_bodyparts(m) && tg_bodypart_kind(kind)) return 0; /* (the R35's: only the sound of a place that cannot be opened) */
  if (kind != 0 && kind != 1 && kind != 2) return 1;
  int on = kind == 0 ? m->wheel[mk][sl] : kind == 1 ? m->aero[mk][sl] : m->bonnet[mk][sl];
  if (on) return 1;
  for (int l = 1; l <= lv && l < 64; ++l)
    if (DRESS_LADDER[m->tg.gem][l][0] == kind) return 1;
  return 0;
}

/* a change decided with the part kept (Refuse to change, no change, the one on): no wear
 * (the arcade's plays it; the port skips it), back to the dress-up menu, else on to
 * the next part due (a win's) */
static void tg_select_open(menu_state *m);
static int tg_kept(menu_state *m, int old, int now) {
  if (old != now) return 0;
  if (m->dress_free) tg_select_open(m);
  else if (!tg_due(m)) tg_leave(m);
  return 1;
}

static void tg_select_open(menu_state *m) {
  int gem = tg_gem(m);
  go(m, MENU_TG_SELECT);
  m->tg.gem = gem;
  for (int i = 0; i < TG_SEL_PLACES[gem]; ++i)
    if (TG_SEL_KIND[gem][i] == m->dress_kind) m->sel = i;
  m->from = m->sel;
  m->tg.move = m->tg.move_t = m->tg.decided = m->tg.rest_t = 0;
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

static int tg_sel_frame(const menu_state *m) {
  if (m->tg.move > 0) return 100 * m->from + 90 + m->tg.move_t;
  if (m->tg.move < 0) return 100 * m->sel + 94 - m->tg.move_t;
  if (m->tg.decided) {
    int t = TG_SEL_DECIDE + TG_FADE - m->deciding;
    return 100 * m->sel + 60 + (t < TG_SEL_DECIDE ? t : TG_SEL_DECIDE - 1);
  }
  return 100 * m->sel + (m->card_t - m->tg.rest_t) % 60;
}

/* the place a tap is on (the chosen one's picture is big: the others' first), else -1 */
static int tg_sel_hit(const menu_state *m) {
  int gem = m->tg.gem, f = tg_sel_frame(m), hit = -1;
  for (int i = 0; i < TG_SEL_PLACES[gem]; ++i) {
    float r[4];
    if (anm_sprite_rect(ID.tg_all[gem], f, anm_sprite(TG_SEL_ICON[gem][i]), r) && near_rect(m, r, 0, 6.0f) &&
        (hit < 0 || i != m->sel))
      hit = i;
  }
  return hit;
}

/* a move under way (+1, -1) over TG_SEL_SLIDE frames, or a tap or key's step or decision */
static int tg_sel_move(menu_state *m, int places, const menu_in *in) {
  m->card_t++;
  if (m->tg.move) {
    if (++m->tg.move_t >= TG_SEL_SLIDE) m->tg.move = 0, m->tg.rest_t = m->card_t;
    return 0;
  }
  if (m->card_t < TG_FADE) return 0;
  int d = in->dx;
  if (in->select >= 0) d = in->select == m->sel ? 0 : in->select > m->sel ? 1 : -1; /* a place chosen: a step towards it */
  if (!d) return in->ok || in->select >= 0;
  if ((d > 0 && m->sel < places - 1) || (d < 0 && m->sel > 0)) {
    snd_menu(SND_CURSOR);
    m->tg.move = d;
    m->tg.move_t = 0;
    m->from = m->sel;
    m->sel += d;
  }
  return 0;
}

static void tg_select_step(menu_state *m, const menu_in *in) {
  if (!tg_sel_move(m, TG_SEL_PLACES[m->tg.gem], in)) return;
  if (!tg_sel_open_ok(m, TG_SEL_KIND[m->tg.gem][m->sel])) {
    snd_menu(SND_CANCEL);
    return;
  }
  snd_menu(SND_SELECT);
  m->tg.decided = 1;
  m->deciding = TG_SEL_DECIDE + TG_FADE;
}

static void draw_tg_select(const menu_state *m, int bars, void (*draw3d)(void *), void *ctx) {
  anm_draw(ID.tg_bg1, 0, 1);
  anm_draw_xy(ID.tg_bg3, 0, 1, 20, 130, 1, 0.5f, 0, 0);
  if (draw3d) draw3d(ctx);
  anm_swap dodai = { anm_sprite(T_SEL_DODAI), -1 };
  anm_draw_ex(ID.tg_all[m->tg.gem], tg_sel_frame(m), 1, 0, 0, &dodai, 1);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_SEL_MES) };
  draw_topbar(bars, 1, &mes);
  draw_bottom(bars, 1);
}

/* the chosen kind's screen: the wheel maker select (the gem cars' bar at once), the aero
 * set's, the bonnet's, wing's or mirror's change; Refuse to change: the game mode select */
static void tg_select_done(menu_state *m) {
  int kind = TG_SEL_KIND[m->tg.gem][m->sel];
  m->dress_kind = kind;
  m->tg.kind = kind;
  if (kind == 9) {
    tg_free_leave(m);
  } else if (kind == 0) {
    if (m->tg.gem) tg_wheel_open_at(m, 4);
    else tg_maker_open(m);
  } else if (kind == 1) {
    tg_aero_open(m);
  } else if (kind == 5) {
    tg_vinyl_open(m);
  } else {
    tg_part_open(m);
  }
}

/* ---- the wheel maker select (seqTgWheelMakerSelect, clSeqTgWheelMakerThread: stPimpl
 * 0x88a3df4, run 0x8884da4, yield 0x8884b50), the dress-up menu's wheel (not the gem
 * cars'): menu_g_shogo_bg_1 and _2, the maker's wheel picture (menu_layout_wheelmaker_bg)
 * under the places (menu_layout_wheelmaker_sel: Enkei, Rays, Watanabe, Yokohama, the
 * bar's makers 3, 2, 1, 0: 0x940a2c8) and the top bar's message 0x1ccf; no car. 100 frames
 * a place: resting on 100 sel + 1 .. + 59 in a loop; a move right plays the places 100 new
 * - 10.. for 5 frames (the picture 100 new - 9.. for 4), left 100 new + 90.. backwards (the
 * picture + 91..), SE 0x19, no wrap; decided 100 sel + 61..70 (SE 0x1a), 30 frames, then
 * the fade. It opens on the maker last chosen (wk_s + 0x10: at first Enkei's); the back key
 * (the cabinet's cancel, SE 0x1b) goes back to the dress-up menu. ---- */
#define WM_DECIDE 10
#define WM_WAIT 30 /* 0x9771198 */
static const signed char WM_MAKER[4] = { 3, 2, 1, 0 };
static const short WM_ICON[4] = { JID(0x171a), JID(0x13c2), JID(0x1afe), JID(0x1aff) };

static void tg_maker_open(menu_state *m) {
  go(m, MENU_TG_MAKER);
  for (int i = 0; i < 4; ++i)
    if (WM_MAKER[i] == m->tg.maker) m->sel = i;
  m->from = m->sel;
  m->tg.move = m->tg.move_t = m->tg.decided = m->tg.rest_t = 0;
  m->card_t = 0;
  m->bars = 1;
  m->music = 1;
}

static int tg_maker_frame(const menu_state *m, int bg) {
  int t = m->tg.move_t, s = 100 * m->sel;
  if (m->tg.move && t < (bg ? 4 : TG_SEL_SLIDE)) return m->tg.move > 0 ? s - 10 + bg + t : s + 94 - t;
  if (m->tg.move) return s + 1 + t - 4;
  if (m->tg.decided && !bg) {
    int d = WM_DECIDE + WM_WAIT + TG_FADE - m->deciding;
    return s + 61 + (d < WM_DECIDE ? d : WM_DECIDE - 1);
  }
  return s + 1 + (m->card_t - m->tg.rest_t) % 59;
}

static int tg_maker_hit(const menu_state *m) {
  int f = tg_maker_frame(m, 0), hit = -1;
  for (int i = 0; i < 4; ++i) {
    float r[4];
    if (anm_sprite_rect(ID.wmaker_sel, f, anm_sprite(WM_ICON[i]), r) && near_rect(m, r, 0, 6.0f) && (hit < 0 || i != m->sel))
      hit = i;
  }
  return hit;
}

static void tg_maker_step(menu_state *m, const menu_in *in) {
  if (!tg_sel_move(m, 4, in)) return;
  snd_menu(SND_SELECT);
  m->tg.decided = 1;
  m->deciding = WM_DECIDE + WM_WAIT + TG_FADE;
}

static void draw_tg_maker(const menu_state *m, int bars) {
  anm_draw(ID.shogo_bg1, 0, 1);
  anm_draw(ID.shogo_bg2, 0, 1);
  anm_draw(ID.wmaker_bg, tg_maker_frame(m, 1), 1);
  anm_draw(ID.wmaker_sel, tg_maker_frame(m, 0), 1);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_MAKER_MES) };
  draw_topbar(bars, 1, &mes);
  draw_bottom(bars, 1);
}

/* the maker's newest wheel won (where its bar opens), 0 none */
static int tg_maker_newest(const menu_state *m, int maker) {
  int lv = m->dress_lv[m->maker][menu_slot(m)], gem = tg_gem(m), w = 0;
  for (int l = 1; l <= lv && l < 64; ++l) {
    const dress_wheel_set *s = DRESS_LADDER[gem][l][0] == 0 ? wheel_set(DRESS_LADDER[gem][l][1] + 1, NULL) : NULL;
    if (s && s->maker == maker) w = DRESS_LADDER[gem][l][1] + 1;
  }
  return w;
}

/* ---- the part worn (seqTgWearParts, clSeqTgWearPartsAeroThread: run 0x887b714, yield
 * 0x887bf00), after the change: menu_g_tg_bg_1, the stand menu_g_tg_bg_3 moved by the kind
 * (table 0x9409f00: the aero set's (20, 120), the bonnet's (20, 87), the wing's (20, 268) and
 * the mirror's (20, 248) at half height), the top bar without time,
 * menu_layout_tg_tune_get_title_no_lv and the car (TG_WEAR_POSE). The yield's states: a
 * bonnet's, wing's or mirror's starts at 0 with the one worn before: 60 frames on the white
 * comes up (menu_p_whiteout 0..29, SE 0x41 parts_equip), 45 on it goes (69..98) with the new
 * one under it (SE 0x1e tuning), 60 on (state 3) the banner comes down: the frame
 * menu_layout_tg_tune_install_wheel_logo at (0, 23) (its wheel and font templates off) and
 * the part's name and what was done (get_parts_data + 0, + 0x20: font 4 datrums 4, centred
 * on (320, 140)), but not for the part kept (wk_s +0x42); 40 on, state 5. The aero set's
 * starts at state 2 with both SEs, the car turning. The screen fades in over 10 frames (the
 * aero set's 20); then 8 s (skip_enable_stop: no tap), at state 5 30 frames more and the fade
 * out over 10. The port takes a tap once the banner is down. The level 63 part's COMPLETE
 * (menu_layout_tg_tune_complete, SE 0x48) comes with a sticker or a wheel, which the port
 * does not wear yet. ---- */
#define WEAR_TEXT 101800
#define WEAR_HOLD 480 /* the run's 8 s */
#define WEAR_WHEEL_HOLD 240 /* clSeqTgWearPartsWheelThread::run's 4 s */
#define WEAR_WAIT 30  /* 0x9771198 */
#define WEAR_COMPLETE 180 /* its COMPLETE's hold */
/* tools/wear_texts.txt's rows of the wheel's banner: 装着完了！ (font 4 datrums 11, at the
 * banner's font template), 純正ホイールに交換完了！ (datrums 4, centred on (330, 118)) */
enum { WEAR_WHEEL = 16, WEAR_WHEEL_STOCK = 17, WEAR_NEON = 18, WEAR_STICKER = 31 };
enum { T_WEAR_WHEEL = JID(0x1ed4), T_WEAR_FONT = JID(0x49), S_WEAR_WAKU = JID(0x1f61), S_WHITE = JID(0x1c6) };

/* tools/wear_texts.txt's row of the part (weartexts.py) */
static int wear_text(int gem, int kind, int num) {
  static const signed char FIRST[5] = { -1, 0, 5, 9, 14 }, N[5] = { 0, 4, 4, 5, 2 };
  /* the neons': 18 none (取り外し完了！), 19.. the straight neons 1..6, the spot neons 7..12 */
  if (kind == 6 || kind == 8) return num >= 0 && num <= 12 ? WEAR_NEON + num : -1;
  /* the stickers': none's 18 (取り外し完了！), 31.. by pattern (get_parts_data(5, n)'s names) */
  if (kind == 5) {
    static const signed char FROM[10] = { 1, 11, 21, 31, 41, 42, 43, 48, 58, 68 };
    if (num == 0) return WEAR_NEON;
    for (int k = 9; k >= 0; --k)
      if (num >= FROM[k]) return num <= 77 ? WEAR_STICKER + k : -1;
    return -1;
  }
  if (kind < 1 || kind > 4 || num < 0 || num >= N[kind]) return -1;
  if (kind == 1 && gem && num == 1) return 4;
  return FIRST[kind] + num;
}

static int tg_wear_in(const menu_state *m) { return m->tg.kind == 1 ? 2 * TG_FADE : TG_FADE; }

static void tg_wear_open(menu_state *m, int old, int chosen) {
  go(m, MENU_TG_WEAR);
  m->tg.old = old;
  m->tg.chosen = chosen;
  m->tg.same = old == chosen;
  m->tg.state = m->tg.kind == 1 ? 2 : 0;
  m->tg.timer = 60;
  m->tg.left = WEAR_HOLD;
  m->tg.ry = TG_WEAR_POSE[tg_wear_k(m->tg.kind)][1];
  m->tg.white_from = -1;
  m->card_t = 0;
  m->bars = 0;
  m->music = 1;
  if (m->tg.kind == 0) {
    /* the wheel's: the yield's states from -5, the jack down, the body's bounce 0.5 */
    m->tg.state = -5;
    m->tg.timer = 30;
    m->tg.left = WEAR_WHEEL_HOLD;
    m->tg.lift = m->tg.body = m->tg.bounce = m->tg.phase = 0;
    m->tg.amp = 0.5f;
    m->tg.complete_t = 0;
  }
  if (m->tg.kind == 1) {
    snd_effect("parts_equip", 1);
    snd_effect("tuning", 1);
  }
}

/* clSeqTgWearPartsWheelThread::yield's states (0x9409f5c): -5, 30 frames then the jack up
 * (SE 0x47), the body to 0.6 over the tyres and then the car 1.2 up on a sine (0.2 a frame
 * to 1.57); -2, 20 frames to the white (0..29, SE 0x22 cutin2); -1, 70 more (SE 0x4a 40 on)
 * and the white goes (69..98) with the new wheel on; 0, 60 frames; 1, the jack down (SE
 * 0x46); 2, the car down 0.1 a frame; 3, the body down 0.1 a frame and its bounce (-sin of
 * a phase 0.1 a frame to 9.4, the size 0.5 by 0.93 a frame), then the banner (SE 0x4b); 4,
 * 40 frames; 5 for the run */
static void tg_wear_wheel_yield(menu_state *m) {
  switch (m->tg.state) {
  case -5:
    if (--m->tg.timer == 0) snd_effect("jack_up", 1);
    if (m->tg.timer < 0) {
      float y = sinf(m->tg.phase) * 1.2f;
      if (m->tg.phase >= 1.57f) {
        m->tg.phase = 0;
        m->tg.state = -2;
        m->tg.timer = 20;
        m->tg.body = m->tg.lift = 0.6f;
      } else {
        m->tg.phase += 0.2f;
        m->tg.body = y >= 0.6f ? 0.6f : y;
        m->tg.lift = y >= 0.6f ? y - 0.6f : 0;
      }
    }
    break;
  case -2:
    if (--m->tg.timer < 0) {
      m->tg.white_from = 0, m->tg.white_at = m->card_t;
      snd_menu(SND_CUTIN2);
      m->tg.state = -1;
      m->tg.timer = 70;
    }
    break;
  case -1:
    if (--m->tg.timer == 30) snd_effect("tuning1", 1);
    if (m->tg.timer < 0) {
      m->tg.white_from = 69, m->tg.white_at = m->card_t;
      m->tg.state = 0;
      m->tg.timer = 60;
    }
    break;
  case 0:
    if (--m->tg.timer < 0) m->tg.state = 1;
    break;
  case 1:
    m->tg.state = 2;
    snd_effect("jack_down", 1);
    break;
  case 2:
    if (m->tg.lift > 0.1f) m->tg.lift -= 0.1f;
    else m->tg.lift = 0, m->tg.state = 3;
    break;
  case 3: {
    if (m->tg.body > 0) m->tg.body = (float)((double)m->tg.body - 0.1);
    float x = m->tg.phase;
    m->tg.bounce = -sinf(x) * m->tg.amp;
    m->tg.phase = x + 0.1f;
    m->tg.amp *= 0.93f;
    if (m->tg.phase > 9.4f) {
      m->tg.state = 4;
      m->tg.timer = 40;
      snd_effect("tuning2", 1);
    }
    break;
  }
  case 4:
    if (--m->tg.timer < 0) m->tg.state = 5;
    break;
  }
}

static void tg_wear_step(menu_state *m, const menu_in *in) {
  m->card_t++;
  if (m->tg.kind == 0) {
    tg_wear_wheel_yield(m);
    if (m->card_t < TG_FADE) return;
    if (m->tg.left > 0 && in->any && m->tg.state == 5) m->tg.left = 0;
    /* the level 63 part's COMPLETE (wk_s + 0x40: SE 0x48, 180 frames) before the wait */
    if (m->tg.lv == 63 && m->tg.state == 5 && m->tg.left <= 0) {
      if (!m->tg.complete_t) {
        m->tg.complete_t = m->card_t;
        snd_effect("parts_comp", 1);
      }
      if (m->card_t - m->tg.complete_t < WEAR_COMPLETE) return;
    }
    if (m->tg.left > 0 || m->tg.state == 5) m->tg.left--;
    if (m->tg.left <= -(WEAR_WAIT + TG_FADE)) m->deciding = 1;
    return;
  }
  /* yield */
  if (m->tg.kind == 1 && m->tg.state > 0) m->tg.ry -= 0.3f;
  switch (m->tg.state) {
  case 0:
    if (--m->tg.timer < 0) {
      m->tg.white_from = 0, m->tg.white_at = m->card_t;
      snd_effect("parts_equip", 1);
      m->tg.timer = 45;
      m->tg.state = 1;
    }
    break;
  case 1:
    if (--m->tg.timer < 0) {
      m->tg.white_from = 69, m->tg.white_at = m->card_t;
      m->tg.state = 2;
      m->tg.timer = 60;
      snd_effect("tuning", 1);
    }
    break;
  case 2:
    if (--m->tg.timer < 0) m->tg.state = 3;
    break;
  case 3:
    m->tg.state = 4;
    m->tg.timer = 40;
    break;
  case 4:
    if (--m->tg.timer < 0) m->tg.state = 5;
    break;
  }
  /* run: the 8 s once the screen is in, then the wait and the fade out (left below 0) */
  if (m->card_t < tg_wear_in(m)) return;
  if (m->tg.left > 0 && in->any && m->tg.state == 5) m->tg.left = 0;
  if (m->tg.left > 0 || m->tg.state == 5) m->tg.left--;
  if (m->tg.left <= -(WEAR_WAIT + TG_FADE)) m->deciding = 1;
}

/* the dress-up screens' top bar without time (clMenuBarStd ctor: loop 0..119, in 190..199,
 * out 160..169; past 170 it is up off the screen): in as the screen opens, then the loop,
 * out over the `out` frames of its leaving (-1 not leaving) */
static int topbar_notime_frame(int t, int out) {
  if (out >= 0) return 160 + (out < 9 ? out : 9);
  return t < 10 ? 190 + t : (t - 10) % 120;
}

/* ---- the class-up screen (seqLvUp 0x8781e10, clSeqLvUpThread), after a race whose card's
 * class rose (mseqStory, mseqTen, mseqTimeAttack: GetClass of the card before the race and
 * after): menu_g_shogo_bg_1 and _2 under menu_layout_lvup, its class before (the small
 * plate, template 0x185e) and after (the large one, 0x185d) from the table at 0x93f09c0
 * (class k: menu_p_lvup_s_(47 - k), menu_p_lvup_l_(47 - k)). It fades in over 10 frames,
 * plays its intro (1..124: levelup, SE 0x34, at frame 101), then loops 201..259; a tap in
 * the intro goes to the loop, one in the loop fades out over 10 frames (the arcade's 10 s
 * countdown is the port's infinity). Then the rest of the way back from the race. ---- */
enum { T_LVUP_S = JID(0x185e), T_LVUP_L = JID(0x185d) };
#define LVUP_INTRO 124
#define LVUP_LOOP 201
#define LVUP_LOOP_LEN 59
#define LVUP_SE 101

static int lvup_small(int k) { return k < 38 ? 1555 - k : 1261 - k; }
static int lvup_large(int k) { return k < 38 ? 1517 - k : 1252 - k; }

void menu_lvup_check(menu_state *m) {
  menu_own_store(m);
  int mk = m->maker, c = m->car, sl = menu_slot(m), cls = menu_class(m, mk, sl);
  if (cls <= m->class_car[mk][sl]) return;
  m->lvup_due = 1;
  m->lvup_from = m->class_car[mk][sl];
  m->lvup_to = cls;
  m->class_car[mk][sl] = (signed char)cls;
  m->garage_changed = 1;
}

static void lvup_open(menu_state *m) {
  go(m, MENU_LVUP);
  m->lvup_due = 0;
  m->lvup_loop = 0;
  m->card_t = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static void lvup_step(menu_state *m, const menu_in *in) {
  if (m->t == LVUP_SE) snd_effect("levelup", 1);
  if (!m->lvup_loop && m->t >= LVUP_INTRO) m->lvup_loop = LVUP_INTRO;
  if (!in->any || m->t < TG_FADE) return;
  if (!m->lvup_loop) m->lvup_loop = m->t;
  else m->deciding = TG_FADE;
}

static void draw_lvup(const menu_state *m) {
  int t = m->t, from = m->lvup_from, to = m->lvup_to;
  int f = !m->lvup_loop ? (t < 1 ? 1 : t) : LVUP_LOOP + (t - m->lvup_loop) % LVUP_LOOP_LEN;
  float fade = m->deciding ? (float)m->deciding / TG_FADE : t < TG_FADE ? (float)(t + 1) / TG_FADE : 1.0f;
  anm_draw(ID.shogo_bg1, 0, 1);
  anm_draw(ID.shogo_bg2, 0, 1);
  anm_swap sw[2] = { { anm_sprite(T_LVUP_S), from >= 0 && from < 47 ? anm_sprite(lvup_small(from)) : -1 },
                     { anm_sprite(T_LVUP_L), to >= 0 && to < 47 ? anm_sprite(lvup_large(to)) : -1 } };
  anm_draw_ex(ID.lvup, f, 1, 0, 0, sw, 2);
  anm_fill(0, 0, 0, 1.0f - fade);
}

/* ---- the versus stars (seqJoStarGet 0x882bdd0, clSeqJoStarGetThread): after a versus whose
 * stars changed (clRaceThread::finiJoin: the table 0x94128e0, or 0x9412940 above 2000 stars,
 * by machines and place), on the way back to the lobby. The screen (ctor 0x882a8b4, yield
 * 0x88285c0): menu_g_vs_star_bg with the emblem of the stars held swapped into 0x18e4, the
 * car turning (clMenuCar about y, 0.3 degrees a frame back, environment 0, eye (0, 11, 50) at
 * (0, 4, 0)), menu_layout_vs_star_logo, _car (the car's logo into 0x1595), _title (the name,
 * the class and title, the power) and _wins (the versus wins in menu_g_st_result_L digits);
 * the stars held (drawHaveStar 0x8827d1c): by 1 (the stars % 100), 100, 1000 and 10000 in
 * menu_use_vs_star's four frames, the biggest first, 20 a row from the tables at 0x93faf60.
 * runAddStar (0x882c490): menu_g_vs_star_<n> (220 frames, SE 0x2c win at each star's
 * landing, frames 20 / 30 / 40), or with a carry into a bigger star keta_a_<n> (130) then
 * keta_b_<the ones left + 1> (90, its 100 star swapped for 1000 / 10000 by the carry); at
 * 130 an emblem changed by the new stars swaps in (SE 0x2d approach3); at 220 the new stars
 * fade in over 30 frames into the stars held, a new big star with its finish_<100..10000>
 * (160, SE 0x38 win_bigstar); then a tap, or frame 600, fades out. runSubStar (0x882df04):
 * minus1 / minus2 (200 frames). ---- */
enum { T_STAR_EMBLEM = JID(0x18e4), T_STAR_LOGO = JID(0x1595), T_STAR_L100 = JID(0xd44), T_STAR_NAME = JID(0x58b), T_STAR_CLASS = JID(0x37d),
       T_STAR_TITLE = JID(0x476), T_STAR_POWER = JID(0x477), T_STAR_WIN_D1 = JID(2657), T_STAR_WIN_D2 = JID(2666), T_STAR_WIN_D3 = JID(2665),
       S_STAR_DIGIT = JID(2657) };
#define STAR_FADE 15
#define STAR_ADD_LEN 220
#define STAR_KETA_A 130
#define STAR_EMBLEM_AT 130
#define STAR_IN_LEN 30
#define STAR_FINISH_LEN 160
#define STAR_SUB_LEN 200
#define STAR_MAX 600
static const int STAR_EMBLEMS[10] = { -1, JID(0x18e5), JID(0x1706), JID(0x1ab8), JID(0x1381), JID(0x1382), JID(0x1707), JID(0x1708), JID(0x18e6), JID(0x18e4) };
static const int STAR_TIERS[9] = { 1000, 2000, 4000, 7000, 10000, 15000, 20000, 30000, 45000 };
static const float STAR_XSTEP[4] = { 30, 68, 68, 102 }, STAR_YSTEP[4] = { 32, 60, 60, 104 };
static const float STAR_X0[4] = { 21, 14, 14, 13 }, STAR_Y0[4] = { 72, 82, 82, 68 };
static const int STAR_BIG[4] = { 0, T_STAR_L100, JID(0x1081), JID(0x1380) };

static int star_tier(int s) {
  int k = 0;
  while (k < 9 && s >= STAR_TIERS[k]) k++;
  return k;
}

static void star_counts(int s, int n[4]) {
  n[0] = s % 100, n[1] = s / 100 % 10, n[2] = s / 1000 % 10, n[3] = s / 10000 % 10;
}

/* the carry of adding d to s: 0 none, 1 a new 100 star, 2 a 1000, 3 a 10000 (the stars past
 * 0..99 of ones turn into one bigger star; the screen only shows one) */
static int star_carry(int s, int d) {
  if (d <= 0 || s % 100 + d <= 99) return 0;
  int t = s + d;
  if (t / 100 % 10) return 1;
  if (t / 1000 % 10) return 2;
  return t / 10000 % 10 ? 3 : 0;
}

int menu_vs_stars(menu_state *m, int place, int n) {
  static const signed char LOW[3][4] = { { 1, 0 }, { 2, 1, 0 }, { 3, 2, 1, 0 } };
  static const signed char HIGH[3][4] = { { 1, -1 }, { 2, 1, -1 }, { 3, 2, 1, -1 } };
  int mk = m->maker, c = m->car, sl = menu_slot(m);
  n = n < 2 ? 2 : n > 4 ? 4 : n;
  int r = place - 1 < 0 ? 0 : place - 1 >= n ? n - 1 : place - 1;
  int *s = &m->vs_stars_car[mk][sl];
  int d = (*s > 2000 ? HIGH : LOW)[n - 2][r];
  m->star_from = *s;
  *s = *s + d < 0 ? 0 : *s + d > 999999 ? 999999 : *s + d;
  m->vs_count_car[mk][sl] = m->vs_count_car[mk][sl] < 999999 ? m->vs_count_car[mk][sl] + 1 : 999999;
  /* seqJoStarGet: the screen for a change of -2..3 */
  d = *s - m->star_from;
  m->star_delta = d >= -2 && d <= 3 ? d : 0;
  m->garage_changed = 1;
  return d;
}

void menu_vs_star_open(menu_state *m) {
  go(m, MENU_VS_STAR);
  m->star_t = 0;
  m->star_skip = 0;
  m->star_spin = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

/* the frames the stars' part takes (from the fade in), till a tap may leave */
static int star_len(const menu_state *m) {
  int d = m->star_delta;
  if (d < 0) return STAR_SUB_LEN;
  return STAR_ADD_LEN + STAR_IN_LEN + (star_carry(m->star_from, d) ? STAR_FINISH_LEN : 0);
}

static void star_to_lobby(menu_state *m);

static void star_step(menu_state *m, const menu_in *in) {
  int t = ++m->star_t, f = t - STAR_FADE, d = m->star_delta;
  m->star_spin -= 0.3f;
  if (m->star_skip) {
    if (++m->star_skip > STAR_FADE) star_to_lobby(m);
    return;
  }
  if (d > 0 && f >= 0) {
    int carry = star_carry(m->star_from, d);
    if ((f == 30) || (f == 20 && d >= 2) || (f == 40 && d == 3)) snd_effect("win", 1);
    if (f == STAR_EMBLEM_AT && star_tier(m->star_from + d) != star_tier(m->star_from)) snd_menu(SND_APPROACH3);
    if (carry && f == STAR_ADD_LEN + STAR_IN_LEN) snd_effect("win_bigstar", 1);
  }
  if (f >= STAR_MAX || (in->any && f >= star_len(m))) m->star_skip = 1;
}

/* the centre of star k (0 the first) of kind `kind` with the stars held s; 0 if none */
static int star_slot(int s, int kind, int k, float *x, float *y) {
  int n[4];
  star_counts(s, n);
  float yy = 0;
  for (int j = 3; j >= 0; --j) {
    if (n[j] <= 0) continue;
    if (yy == 0) yy = STAR_Y0[j];
    float xx = STAR_X0[j];
    for (int i = 0; i < n[j]; ++i) {
      if (i && i % 20 == 0) yy += STAR_YSTEP[j], xx = STAR_X0[j];
      if (j == kind && i == k) {
        *x = xx + 0.5f * STAR_XSTEP[j], *y = yy + 0.5f * STAR_YSTEP[j];
        return 1;
      }
      xx += STAR_XSTEP[j];
    }
    yy += STAR_YSTEP[j];
  }
  return 0;
}

/* the stars held s, the last `fresh` of the ones at alpha af, the last of kind `hide` (1..3)
 * left out (its finish draws it) */
static void star_held(int s, int fresh, float af, int hide) {
  int n[4];
  star_counts(s, n);
  for (int j = 3; j >= 0; --j)
    for (int i = 0; i < n[j]; ++i) {
      float x, y, al = j == 0 && i >= n[0] - fresh ? af : 1.0f;
      if (j && j == hide && i == n[j] - 1) continue;
      if (al > 0.0f && star_slot(s, j, i, &x, &y)) anm_draw_ex(ID.star_use, j, al, x, y, NULL, 0);
    }
}

static void draw_vs_star(const menu_state *m, void (*draw3d)(void *), void *ctx) {
  int t = m->star_t, f = t - STAR_FADE, d = m->star_delta, from = m->star_from, to = from + d;
  int carry = star_carry(from, d), mk = m->maker, cr = m->car, sl = menu_slot(m);
  const cardb_car *c = &CARDB_CARS[mk][cr];
  float a = m->star_skip ? 1.0f - (float)m->star_skip / STAR_FADE : t < STAR_FADE ? (float)t / STAR_FADE : 1.0f;
  /* the emblem by the stars: the new one from frame 130 */
  int now = d > 0 && f >= STAR_EMBLEM_AT ? to : d < 0 && f >= STAR_SUB_LEN ? to : from;
  int em = STAR_EMBLEMS[star_tier(now)];
  anm_swap bg = { anm_sprite(T_STAR_EMBLEM), em >= 0 ? anm_sprite(em) : -1 };
  anm_draw_ex(ID.star_bg, 0, 1, 0, 0, &bg, 1);
  if (draw3d) draw3d(ctx);
  anm_draw(ID.star_logo, 0, 1);
  anm_swap logo = { anm_sprite(T_STAR_LOGO), anm_sprite(c->logo) };
  anm_draw_ex(ID.star_car, 0, 1, 0, 0, &logo, 1);
  /* the title row: the name, the class and title, the power */
  anm_swap tsw[4] = { { anm_sprite(T_STAR_NAME), -1 }, { anm_sprite(T_STAR_CLASS), -1 }, { anm_sprite(T_STAR_TITLE), -1 },
                      { anm_sprite(T_STAR_POWER), -1 } };
  int tf = anm_frames(ID.star_title) - 1;
  anm_draw_ex(ID.star_title, tf < 0 ? 0 : tf, 1, 0, 0, tsw, 4);
  float r[4];
  if (anm_sprite_rect(ID.star_title, 0, anm_sprite(T_STAR_NAME), r) && anm_sprite(NAME_GLYPH5) >= 0)
    for (int i = 0; i < 5 && m->name[i] >= 0; ++i) {
      int s = anm_sprite(NAME_GLYPH5 + m->name[i]);
      float src[4] = { 0, 0, anm_sprite_width(s), anm_sprite_height(s) };
      float dst[4] = { r[0] + 22.0f * (float)i, r[1], r[0] + 22.0f * (float)i + 0.8f * src[2], r[1] + 0.8f * src[3] };
      if (s >= 0) anm_draw_part(s, src, dst, 1);
    }
  if (anm_sprite_rect(ID.star_title, 0, anm_sprite(T_STAR_CLASS), r)) {
    int pre = anm_sprite(CARD_CLASSP + m->class_car[mk][sl]);
    if (pre >= 0) {
      float src[4] = { 0, 0, anm_sprite_width(pre), anm_sprite_height(pre) };
      float dst[4] = { r[0], r[1], r[0] + 0.72f * src[2], r[1] + 0.72f * src[3] };
      anm_draw_part(pre, src, dst, 1);
    }
  }
  if (anm_sprite_rect(ID.star_title, 0, anm_sprite(T_STAR_TITLE), r)) {
    int title = m->title_car[mk][sl] > 0 ? m->title_car[mk][sl] : TITLE_FIRST;
    const unsigned short *g;
    int n = title_glyphs(title, &g);
    float x = r[0];
    for (int i = 0; i < n; ++i) {
      int s = title_glyph_slash(g[i]) ? -1 : anm_sprite(TITLE_GLYPH1 + g[i]);
      float src[4] = { 0, 0, 20, 20 }, dst[4] = { x, r[1], x + 20, r[1] + 20 };
      if (s >= 0) anm_draw_part(s, src, dst, 1);
      x += title_glyph_half(g[i]) ? 8.0f : 16.0f;
    }
  }
  if (anm_sprite_rect(ID.star_title, 0, anm_sprite(T_STAR_POWER), r)) {
    char text[16];
    int power = m->tune_full[mk][sl] ? m->tune_power[mk][sl] : m->st_power[mk][sl];
    snprintf(text, sizeof text, "%d", car_tune_hp(c->hp, power));
    float x = card_glyphs(CARD_GLYPH2, "-,", 8.0f, text, r[0], r[1], 1);
    anm_draw_sprite(anm_sprite(CARD_TEXT + C_HP), x, r[1], 1);
  }
  /* the wins: frame 1 one digit, 0 two, 2 three */
  int w = m->vs_wins_car[mk][sl] < 999 ? m->vs_wins_car[mk][sl] : 999;
  int nd = w >= 100 ? 3 : w >= 10 ? 2 : 1;
  anm_swap wsw[3];
  if (nd == 1) {
    wsw[0] = (anm_swap){ anm_sprite(T_STAR_WIN_D1), anm_sprite(S_STAR_DIGIT + w) };
  } else if (nd == 2) {
    wsw[0] = (anm_swap){ anm_sprite(T_STAR_WIN_D1), anm_sprite(S_STAR_DIGIT + w / 10) };
    wsw[1] = (anm_swap){ anm_sprite(T_STAR_WIN_D2), anm_sprite(S_STAR_DIGIT + w % 10) };
  } else {
    wsw[0] = (anm_swap){ anm_sprite(T_STAR_WIN_D1), anm_sprite(S_STAR_DIGIT + w / 100) };
    wsw[1] = (anm_swap){ anm_sprite(T_STAR_WIN_D2), anm_sprite(S_STAR_DIGIT + w / 10 % 10) };
    wsw[2] = (anm_swap){ anm_sprite(T_STAR_WIN_D3), anm_sprite(S_STAR_DIGIT + w % 10) };
  }
  anm_draw_ex(ID.star_wins, nd == 1 ? 1 : nd == 2 ? 0 : 2, 1, 0, 0, wsw, nd);
  /* the stars held, and the race's */
  if (d > 0) {
    int done = f >= STAR_ADD_LEN;
    float in = done ? (f - STAR_ADD_LEN >= STAR_IN_LEN ? 1.0f : (float)(f - STAR_ADD_LEN) / STAR_IN_LEN) : 0.0f;
    if (!done) star_held(from, 0, 1, 0);
    else if (!carry) star_held(to, d, in, 0);
    else {
      /* the new big star with its finish (held from its end), the ones left fading in */
      int k = carry, n[4], g = f - STAR_ADD_LEN - STAR_IN_LEN;
      star_counts(to, n);
      star_held(to, to % 100, in, g < STAR_FINISH_LEN ? k : 0);
      float x, y;
      if (g >= 0 && g < STAR_FINISH_LEN && star_slot(to, k, n[k] - 1, &x, &y)) anm_draw_ex(ID.star_finish[k], g, 1, x, y, NULL, 0);
    }
    if (f >= 0 && f < STAR_ADD_LEN) {
      if (!carry) anm_draw(ID.star_add[d - 1], f, 1);
      else if (f < STAR_KETA_A) anm_draw(ID.star_keta_a[d - 1], f, 1);
      else {
        int left = to % 100;
        anm_swap big = { anm_sprite(T_STAR_L100), anm_sprite(STAR_BIG[carry]) };
        anm_draw_ex(ID.star_keta_b[left < 3 ? left : 2], f - STAR_KETA_A, 1, 0, 0, &big, 1);
      }
    }
  } else if (d < 0) {
    star_held(f >= STAR_SUB_LEN ? to : from, 0, 1, 0);
    if (f >= 0 && f < STAR_SUB_LEN) anm_draw(ID.star_minus[-d - 1], f, 1);
  } else star_held(from, 0, 1, 0);
  anm_fill(0, 0, 0, 1.0f - a);
}

static void star_to_lobby(menu_state *m) {
  m->star_delta = m->star_skip = 0;
  go(m, MENU_VS);
  m->bars = 1;
  m->flash = 0;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

/* ---- the title select (seqTitleSelect 0x8785e08, clSeqTitleSelectThread), after the
 * class-up: the titles judgeTitle found for the race (src/title.h; menu_title_due).
 * stPimpl: menu_g_shogo_bg_1 and _2 (bg_line in place of _2 on REWRITE) and
 * menu_g_shogo_bg_moji, the bars (clMenuBarStd: the top bar's message 0x1e59 "rewrite the
 * title?"), the spin box, the GET part (clMenuGet: menu_g_shogo_shogo_<n> with the titles
 * in font 7 datrums 9 on templates 1450..1452, menu_g_shogo_title looped) and REWRITE's
 * (menu_layout_shogo_rewrite, the title on 0x1bb0). run:
 * - runGet: fades in over 10 frames (1/6 s); its frame f counts to 200: at 60 the frames play
 *   1..139 once, SE 0x30 (rank) at 65 and, with a second and a third title, 90 and 120; at
 *   200 they loop 141..199. A tap before 200 goes to the loop, one after it fades out (the
 *   arcade's 10 s timer is the port's infinity). bg_moji stays on its first frame.
 * - runSelect for each new title: the spin box (clMenuSpinBoxStd on menu_layout_yn_rank, its
 *   templates' backlight 0x18ea, light 0xd4b and frame 0x3b6; YES with the new title on
 *   0x581, NO with the card's on 0x583, font 1 datrums 9), YES first; the bars come in,
 *   bg_moji plays 1..59 then loops 61..659. Decided: the card's 10 frames, 30 more, then the
 *   fade and the bars out. YES makes it the card's title.
 * - runRewrite once one was taken: the layout plays 1..179 once (SE 0x30 at 40), then loops
 *   200..258; a tap before 180 goes to the loop, one after it fades out.
 * Then the rest of the way back from the race. ---- */
enum { T_SHOGO_FONT = JID(1450), T_RW_FONT = JID(0x1bb0), T_RW_YES = JID(6922), T_RW_NO = JID(6683), T_YN_FONT = JID(0x580),
       T_YN_BACKLIGHT = JID(0x1f38), T_YN_LIGHT = JID(0x1dc0), T_YN_WAKU = JID(0x1721), T_SHOGO_MES = JID(0x1e59) };
#define SHOGO_FADE 10
#define SHOGO_IN 60     /* runGet: the frames start */
#define SHOGO_ONCE 139  /* 1..139 */
#define SHOGO_LOOP 200  /* then 141..199 */
#define SHOGO_SE1 65
#define SHOGO_SE2 90
#define SHOGO_SE3 120
#define SHOGO_DECIDED 10 /* the spin box's decided frames, */
#define SHOGO_WAIT 30    /* then 0x9771198's */
#define RW_ONCE 179      /* runRewrite: 1..179, */
#define RW_LOOP 180      /* then 200..258 */
#define RW_SE 40

void menu_title_due(menu_state *m, const int titles[3]) {
  /* (a versus' titles wait for the next race's way back: both race's kept, three at most) */
  for (int i = 0; i < 3; ++i) {
    int id = titles[i], dup = 0;
    for (int j = 0; j < m->title_n; ++j) dup |= m->title_new[j] == id;
    if (id > 0 && !dup && m->title_n < 3) m->title_new[m->title_n++] = id;
  }
}

int menu_title(const menu_state *m) {
  int t = m->title_car[m->maker][menu_slot(m)];
  return t > 0 ? t : TITLE_FIRST;
}

static void shogo_open(menu_state *m) {
  go(m, MENU_SHOGO);
  m->title_k = m->title_phase = m->title_taken = m->title_f = m->title_dec = m->title_out = 0;
  m->card_t = m->slide = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

/* the next phase, out of the fade: the spin box for title k, REWRITE, or the way on */
static void shogo_next(menu_state *m) {
  m->title_out = m->title_f = m->title_dec = 0;
  if (m->title_phase == 1) m->title_k++;
  if (m->title_phase <= 1 && m->title_k < m->title_n) {
    m->title_phase = 1;
    m->sel = 0; /* clArg +4: YES */
    m->slide = m->card_t = 0;
    m->bars = 1;
    return;
  }
  if (m->title_phase <= 1 && m->title_taken) {
    m->title_phase = 2;
    return;
  }
  m->title_n = 0;
  m->deciding = 1;
}

/* whether the tap is on card k (0 YES, 1 NO) of the spin box at rest */
static int shogo_on_card(const menu_state *m, int k) {
  float r[4];
  if (!anm_sprite_rect(ID.yn_rank, 100 * m->sel + m->card_t % 60, anm_sprite(k ? T_RW_NO : T_RW_YES), r)) return 0;
  return m->tap_x >= r[0] && m->tap_x < r[2] && m->tap_y >= r[1] && m->tap_y < r[3];
}

static int shogo_on_arrow(const menu_state *m) {
  float r[4];
  if (!anm_sprite_rect(ID.yn_rank, 100 * m->sel + m->card_t % 60, anm_sprite(T_ARROW), r)) return 0;
  return m->tap_x >= r[0] - 40.0f && m->tap_x < r[2] + 40.0f && m->tap_y >= r[1] - 30.0f && m->tap_y < r[3] + 30.0f;
}

static void shogo_step(menu_state *m, const menu_in *in) {
  if (m->title_out) {
    if (++m->title_out > SHOGO_FADE) shogo_next(m);
    return;
  }
  int f = ++m->title_f;
  if (m->title_phase == 0) {
    int n = m->title_n;
    if ((f == SHOGO_SE1) || (f == SHOGO_SE2 && n > 1) || (f == SHOGO_SE3 && n > 2)) snd_effect("rank", 1);
    if (f > SHOGO_LOOP) m->title_f = f = SHOGO_LOOP + 1 + (f - SHOGO_LOOP - 1) % 59;
    if (!in->any || f <= SHOGO_FADE) return;
    if (f < SHOGO_LOOP) m->title_f = SHOGO_LOOP;
    else m->title_out = 1;
    return;
  }
  if (m->title_phase == 2) {
    if (f == RW_SE) snd_effect("rank", 1);
    if (f > RW_LOOP + 58) m->title_f = f = RW_LOOP + 1;
    if (!in->any || f <= SHOGO_FADE) return;
    if (f < RW_LOOP) m->title_f = RW_LOOP;
    else m->title_out = 1;
    return;
  }
  /* the spin box: decided, sliding, or at rest */
  if (m->title_dec) {
    if (++m->title_dec > SHOGO_DECIDED + SHOGO_WAIT) m->title_out = 1;
    return;
  }
  if (m->slide) {
    if (--m->slide == 0) {
      m->sel ^= 1;
      m->card_t = 0;
    }
    return;
  }
  m->card_t++;
  if (f <= SHOGO_FADE) return;
  if (in->dx) {
    snd_menu(SND_CURSOR);
    m->slide = SLIDE;
    return;
  }
  if (in->ok) {
    snd_menu(m->sel ? SND_CANCEL : SND_SELECT);
    if (!m->sel) {
      m->title_car[m->maker][menu_slot(m)] = (short)m->title_new[m->title_k];
      m->title_taken = 1;
      m->garage_changed = 1;
    }
    m->title_dec = 1;
  }
}

/* a title in a font's glyphs where a frame has template tmpl (clSpriteFont datrums 9: the
 * advances' sum W centred on the template's x + full / 2, at its top; scaled with it), at
 * the template's opacity */
/* (the three title places of the frames: the build's own ids, not the first one's plus 1 and 2) */
static const int SHOGO_FONT[3] = { JID(1450), JID(1451), JID(1452) };
static const int SHOGO_FONT28[3] = { JID(1439), JID(1440), JID(1441) }; /* (the export build's frames hold these instead) */

/* the place of title i in the frame: the f_40 template, or the f_28 one the export build's frames hold */
static int shogo_font(int anim, int frame, int i) {
  float r[4], a;
  int t = anm_sprite(SHOGO_FONT[i]);
  return t >= 0 && anm_sprite_rect_at(anim, frame, t, 0, r, &a) ? SHOGO_FONT[i] : SHOGO_FONT28[i];
}

static void shogo_text(int anim, int frame, int tmpl, int base, int half, int full, int cell, int id, float alpha, int fit) {
  float r[4], a;
  int t = anm_sprite(tmpl);
  float tw = anm_sprite_width(t), th = anm_sprite_height(t);
  if (id <= 0 || t < 0 || tw <= 0.0f || th <= 0.0f) return;
  const unsigned short *g;
  int n = title_glyphs(id, &g);
  /* the English titles break their lines at '/' (a title's two lines, each centred): line starts and widths */
  int ls[4] = { 0 }, le[4], nl = 1;
  for (int i = 0; i < n && nl < 3; ++i)
    if (title_glyph_slash(g[i])) { le[nl - 1] = i; ls[nl++] = i + 1; }
  le[nl - 1] = n;
  float lw[3], wmax = 0.0f;
  for (int l = 0; l < nl; ++l) {
    lw[l] = 0.0f;
    for (int i = ls[l]; i < le[l]; ++i) lw[l] += (float)(title_glyph_half(g[i]) ? half : full);
    if (lw[l] > wmax) wmax = lw[l];
  }
  float zoom = fit ? th / (float)cell : 1.0f; /* (fit: a smaller place than the font's: the text scaled to it) */
  if (nl > 1 && wmax * zoom > 8.5f * (float)cell) zoom = 8.5f * (float)cell / wmax;
  if (nl > 1) zoom *= 0.85f; /* (two lines in a place for one) */
  float pitch = (float)full * zoom;
  /* each of the frame's records of the template (a decided card has a hidden one too) */
  for (int k = 0; anm_sprite_rect_at(anim, frame, t, k, r, &a); ++k) {
    if ((a *= alpha) <= 0.0f) continue;
    float sx = (r[2] - r[0]) / tw, sy = (r[3] - r[1]) / th;
    for (int l = 0; l < nl; ++l) {
      float x = ((fit ? tw : (float)full) - lw[l] * zoom) / 2.0f, y = pitch * ((float)l - 0.1f * (float)(nl - 1));
      for (int i = ls[l]; i < le[l]; ++i) {
        int s = anm_sprite(base + g[i]);
        float src[4] = { 0, 0, (float)cell, (float)cell };
        float dst[4] = { r[0] + sx * x, r[1] + sy * y, r[0] + sx * (x + (float)cell * zoom), r[1] + sy * (y + (float)cell * zoom) };
        if (s >= 0) anm_draw_part(s, src, dst, a);
        x += (float)(title_glyph_half(g[i]) ? half : full) * zoom;
      }
    }
  }
}

static void draw_shogo(const menu_state *m, int clock) {
  int f = m->title_f, phase = m->title_phase;
  float fade = m->title_out ? 1.0f - (float)m->title_out / SHOGO_FADE
               : m->deciding ? 0.0f
               : f < SHOGO_FADE ? (float)(f + 1) / SHOGO_FADE : 1.0f;
  anm_draw(ID.shogo_bg1, 0, 1);
  if (phase == 2) anm_draw(ID.line, clock % anm_frames(ID.line), 1);
  else anm_draw(ID.shogo_bg2, clock % anm_frames(ID.shogo_bg2), 1);
  int moji = phase != 1 ? 0 : f < 59 ? 1 + f : 61 + (f - 59) % 599;
  anm_draw(ID.shogo_moji, moji, 1);
  if (phase == 0) {
    int n = m->title_n < 1 ? 1 : m->title_n > 3 ? 3 : m->title_n, anim = ID.shogo_n[n - 1];
    int sf = f < SHOGO_IN ? 0 : f < SHOGO_LOOP ? 1 + (f - SHOGO_IN < SHOGO_ONCE - 1 ? f - SHOGO_IN : SHOGO_ONCE - 1)
                                               : 141 + (f - SHOGO_LOOP) % 59;
    anm_swap off[7] = { { anm_sprite(SHOGO_FONT[0]), -1 }, { anm_sprite(SHOGO_FONT[1]), -1 }, { anm_sprite(SHOGO_FONT[2]), -1 },
                        { anm_sprite(SHOGO_FONT28[0]), -1 }, { anm_sprite(SHOGO_FONT28[1]), -1 }, { anm_sprite(SHOGO_FONT28[2]), -1 },
                        { anm_sprite(T_RW_FONT), -1 } };
    anm_draw_ex(anim, sf, 1, 0, 0, off, 7);
    anm_draw(ID.shogo_title, clock % anm_frames(ID.shogo_title), 1);
    for (int i = 0; i < n; ++i) { int tf = shogo_font(anim, sf, i); shogo_text(anim, sf, tf, TITLE_GLYPH7, 18, 36, 40, m->title_new[i], 1, tf != SHOGO_FONT[i]); }
  } else if (phase == 2) {
    int rf = f < RW_LOOP ? (f < 1 ? 1 : f > RW_ONCE ? RW_ONCE : f) : 200 + (f - RW_LOOP) % 59;
    anm_swap off = { anm_sprite(T_RW_FONT), -1 };
    anm_draw_ex(ID.shogo_rw, rf, 1, 0, 0, &off, 1);
    shogo_text(ID.shogo_rw, rf, T_RW_FONT, TITLE_GLYPH7, 18, 36, 40, menu_title(m), 1, 0);
  } else {
    /* the spin box: at rest, sliding, or decided */
    int z = 100 * m->sel, sf = z + m->card_t % 60;
    if (m->slide) sf = z + 90 + SLIDE - m->slide;
    int d = m->title_dec - 1;
    if (m->title_dec) sf = z + 60 + (d < SHOGO_DECIDED ? d : SHOGO_DECIDED - 1);
    anm_swap sw[7] = { { anm_sprite(T_YN_BACKLIGHT), anm_sprite(JID(0x18ea)) }, { anm_sprite(T_YN_LIGHT), anm_sprite(JID(0xd4b)) },
                       { anm_sprite(T_YN_WAKU), anm_sprite(JID(0x3b6)) }, { anm_sprite(T_YN_FONT), -1 },
                       { anm_sprite(T_YN_FONT + 1), -1 }, { anm_sprite(T_YN_FONT + 2), -1 },
                       { anm_sprite(T_YN_FONT + 3), -1 } };
    anm_draw_ex(ID.yn_rank, sf, 1, 0, 0, sw, 7);
    shogo_text(ID.yn_rank, sf, T_YN_FONT + 1, TITLE_GLYPH1, 8, 16, 20, m->title_new[m->title_k], 1, 0);
    shogo_text(ID.yn_rank, sf, T_YN_FONT + 3, TITLE_GLYPH1, 8, 16, 20, menu_title(m), 1, 0);
    if (!m->slide && !m->title_dec) anm_draw(ID.day_ring, z + (m->card_t < 59 ? m->card_t : 59), 1);
    /* the bars: in as the spin box opens, out as it fades */
    int bars = m->title_out ? 160 + (m->title_out < 10 ? m->title_out : 10) : m->bars ? 188 + m->bars : clock % 160;
    anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(T_SHOGO_MES) };
    draw_topbar(bars, 1, &mes);
    draw_bottom(bars, 1);
  }
  anm_fill(0, 0, 0, 1.0f - fade);
}

/* ---- the tuning work (seqStTuneEff 0x883f7c0, clSeqStTuneEffThread), after the story's
 * tuning screen: the part going onto the car. Its row (src/steff.h, clArg 0x8846640) by T
 * (power + handling after) and the side tuned: the top bar's title, the part's picture, the
 * install anime and the car's pose, two lines of text; the arcade's overrides by the car
 * (the jump tables at 0x94081e0 by power, 0x940820c by T - 21) after it.
 * - stPimpl: menu_g_shogo_bg_1 and menu_g_bg_line, the bar without time, the top bar
 *   menu_g_topbar_st_install (by power or handling and T <= 9 or over: from 0, 200, 400 or
 *   600; in at +190 over 10 frames, then +0..119 looped, out at +160; T's digits in 0x479
 *   and 0x47a, the title in 0xa81), the install anime (the part's picture in 0x1ef9), the car
 *   (the card's, scaled 0.9, turned and placed by its pose, environment 8, 20 degrees across
 *   from (0, 28.6, -143) at (0, 5, 0)).
 * - run: in over 10 frames; the install anime (90 frames), SE 0x1e tuning, out to black over
 *   20, the car rebuilt (clMenu3DCar::change), 40 frames on the car's pose's second set and
 *   it turns 0.3 degrees a frame, in over 40; the text box (clMenuText: power
 *   menu_layout_st_instal_result 1..420 with the horsepower before (0xa6c..0xa6e) and after
 *   (0xa61..0xa63), handling menu_layout_st_instal_result_handle 1..299; the lines in font 1
 *   centred on the template 0x580, with it) and SE 0x34 levelup 150 frames on (power) or 0x22
 *   cutin2 75 on (handling). Its 10 s: a step that ends a stage (T 20, 30, 34) shows its
 *   complete picture 3 s before the end (menu_layout_st_install_complete, the tuners 14..16's
 *   _complete2, their picture in 0x1d6f / 0x1e21 from 0x93fd5e0; SE 0x36 result_pass, the
 *   text gone, 3 s more). Then 30 frames, the top bar out, out over 10.
 * The port's: a tap after the text box is on shows the complete picture now (when there is
 * one), else ends it (the arcade's runs its 10 s, and with no time limit never shows the
 * picture). The R35's own (step20, stepup) are not ported: it has the others' rows, as on
 * the tuning screen. ---- */
enum { T_EFF_PARTS = JID(0x1ef9), T_EFF_TENS = JID(0x479), T_EFF_ONES = JID(0x47a), T_EFF_TITLE = JID(0xa81), T_EFF_FONT = JID(0x580),
       T_EFF_HP0 = JID(0xa6c), T_EFF_HP1 = JID(0xa61), T_EFF_FACE = JID(0x1d6f), T_EFF_FACE2 = JID(0x1e21) };
#define EFF_IN 10              /* the fade in, then the install anime */
#define EFF_INSTALL 90
#define EFF_DARK (EFF_IN + EFF_INSTALL) /* SE tuning, out to black over 20 */
#define EFF_CHANGE (EFF_DARK + 20 + 40) /* the new car, in over 40 */
#define EFF_MAIN (EFF_CHANGE + 40)      /* the text box on */
#define EFF_TIME 600
#define EFF_COMPLETE 119
#define EFF_QUIT 30

static void eff_open(menu_state *m, int power, int p0) {
  int mk = m->maker, c = m->car, sl = menu_slot(m), p = m->st_power[mk][sl], h = m->st_handling[mk][sl], t = p + h;
  const steff_car *f = &STEFF_CAR[mk][c];
  const steff_row *r;
  int chara = 0;
  if (t <= 20) {
    if (!power) r = &(f->ucf10 ? STEFF_HAND_UCF10 : STEFF_HAND)[h - 1 < 0 ? 0 : h - 1];
    else if (f->r2 && p == 1) r = &STEFF_POWER_R2[0];
    else if (f->na && p == 1) r = &STEFF_POWER_NA[0];
    else r = &STEFF_POWER[p - 1 < 0 ? 0 : p - 1 > 9 ? 9 : p - 1];
  } else if (t <= 30) {
    r = &(f->ucf10 ? STEFF_MID_UCF10 : STEFF_MID)[t - 21];
    chara = f->u30;
  } else {
    r = &STEFF_TOP[0];
    chara = f->u33;
  }
  memset(&m->eff, 0, sizeof m->eff);
  m->eff.title = r->title, m->eff.parts = r->parts, m->eff.anim = r->anim, m->eff.pose = r->pose;
  m->eff.text = r->text;
  /* the overrides: by power after when power was tuned, by handling after otherwise */
  int *a = &m->eff.anim, *pose = &m->eff.pose;
  if (power) {
    switch (p) {
    case 1:
      if (f->gem) *pose = 1, *a = f->na ? 0x238 : 0x239;
      else if (f->r2) m->eff.parts = TUNE_PARTS_SWAP, *pose = 3, *a = 0x23b;
      else if (f->hiace) *pose = 1, *a = 0x23c;
      break;
    case 2:
      if (f->gem) m->eff.parts = TUNE_PARTS_ENGINE;
      break;
    case 4: case 10:
      if (f->gem) *pose = 7, *a = 0x23d;
      break;
    case 3: case 5: case 6: case 7: case 8: case 9:
      if (f->m12) *pose = 1, *a = 0x238;
      if (f->m11) *pose = 1, *a = 0x23a;
      if (f->hiace) *a = 0x23c;
      break;
    }
  } else {
    if (h == 4 && f->hilift) *a = 0x237;
    if (h == 7 && f->m0f) *pose = 3, *a = 0x23b;
  }
  /* and by T: 21, 25..27, 29 */
  if (t == 21 || t == 25 || t == 26 || t == 27 || t == 29) {
    if (f->m11) *pose = 1, *a = 0x23a;
    if (f->m12) *pose = 1, *a = 0x238;
  }
  m->eff.power = power;
  m->eff.total = t;
  m->eff.milestone = t == 20 || t == 30 || t == 34;
  m->eff.chara = chara;
  int hp = chosen_car(m, c)->hp;
  m->eff.hp0 = car_tune_hp(hp, p0);
  m->eff.hp1 = car_tune_hp(hp, p);
  m->eff.timer = EFF_TIME;
  go(m, MENU_ST_EFF);
  m->card_t = 0;
  m->bars = 0;
  m->music = 1;
  m->tap = m->key = m->key_held = m->back = m->leaving = 0;
}

static void eff_complete(menu_state *m) {
  m->eff.complete_t = m->t;
  snd_menu(SND_RESULT_PASS);
  m->eff.timer += 180;
}

static void eff_step(menu_state *m, const menu_in *in) {
  int t = m->t;
  if (t == EFF_DARK) snd_effect("tuning", 1);
  if (t == EFF_CHANGE) m->eff.after = 1;
  if (m->eff.after) m->eff.angle -= 0.3f;
  if (t == EFF_MAIN) m->eff.main_t = t;
  if (!m->eff.main_t) return;
  int k = t - m->eff.main_t;
  if (m->eff.power && k == 150) snd_effect("levelup", 1);
  if (!m->eff.power && k == 75) snd_menu(SND_CUTIN2);
  if (m->eff.quit) {
    if (--m->eff.quit == 0) m->deciding = TG_FADE;
    return;
  }
  /* the arcade's: the complete picture 3 s before the end, once the text box is in */
  int text_end = m->eff.power ? 420 : 299;
  if (m->eff.milestone && !m->eff.complete_t && m->eff.timer <= 180 && k >= text_end) eff_complete(m);
  if (--m->eff.timer <= 0 && (!m->eff.complete_t || t - m->eff.complete_t >= EFF_COMPLETE)) m->eff.quit = EFF_QUIT;
  if (!in->any) return;
  if (m->eff.milestone && !m->eff.complete_t) eff_complete(m);
  else m->eff.quit = EFF_QUIT;
}

/* the horsepower's digits: the hundreds' none below 100 */
static void eff_hp(anm_swap *sw, int tmpl, int hp) {
  sw[0] = (anm_swap){ anm_sprite(tmpl), hp >= 100 ? anm_sprite(tmpl + hp / 100 % 10) : -1 };
  sw[1] = (anm_swap){ anm_sprite(tmpl + 1), anm_sprite(tmpl + hp / 10 % 10) };
  sw[2] = (anm_swap){ anm_sprite(tmpl + 2), anm_sprite(tmpl + hp % 10) };
}

static void draw_st_eff(const menu_state *m, void (*draw3d)(void *), void *ctx) {
  int t = m->t, T = m->eff.total;
  anm_draw(ID.shogo_bg1, 0, 1);
  anm_draw(ID.line, 0, 1);
  if (draw3d) draw3d(ctx);
  int out = m->deciding ? TG_FADE - m->deciding : -1;
  anm_draw(ID.top_notime, topbar_notime_frame(t, out), 1);
  int base = m->eff.power ? (T <= 9 ? 0 : 200) : (T <= 9 ? 400 : 600);
  int tf = out >= 0 ? base + 160 + (out < 9 ? out : 9) : t < 10 ? base + 190 + t : base + (t - 10) % 120;
  anm_swap top[3] = { { anm_sprite(T_EFF_TENS), anm_sprite(T_EFF_TENS + T / 10 % 10) },
                      { anm_sprite(T_EFF_ONES), anm_sprite(T_EFF_TENS + T % 10) },
                      { anm_sprite(T_EFF_TITLE), m->eff.title > 1 ? anm_sprite(m->eff.title) : -1 } };
  anm_draw_ex(ID.eff_top, tf, 1, 0, 0, top, 3);
  /* the text box and its lines, till the complete picture */
  if (m->eff.main_t && !m->eff.complete_t) {
    int box = m->eff.power ? ID.eff_result : ID.eff_handle, end = m->eff.power ? 420 : 299;
    int f = t - m->eff.main_t + 1;
    if (f > end) f = end;
    anm_swap sw[7];
    int n = 0;
    sw[n++] = (anm_swap){ anm_sprite(T_EFF_FONT), -1 };
    if (m->eff.power) {
      eff_hp(sw + n, T_EFF_HP0, m->eff.hp0);
      eff_hp(sw + n + 3, T_EFF_HP1, m->eff.hp1);
      n += 6;
    }
    anm_draw_ex(box, f, 1, 0, 0, sw, n);
    float r[4], a = anm_sprite_alpha(box, f, anm_sprite(T_EFF_FONT));
    int text = m->eff.text >= 0 ? anm_sprite(STEFF_TEXT + m->eff.text) : -1;
    /* clSpriteFont datrums 1: the lines' widest advance centred on the template (the
     * sprite is a cell wider) */
    if (text >= 0 && a > 0 && anm_sprite_rect(box, f, anm_sprite(T_EFF_FONT), r))
      anm_draw_sprite(text, r[0] - (anm_sprite_width(text) - 20.0f) / 2.0f, r[1], a);
  }
  int ia = m->eff.anim - STEFF_ANIM;
  if (ia >= 0 && ia < 13) {
    anm_swap part = { anm_sprite(T_EFF_PARTS), m->eff.parts > 1 ? anm_sprite(m->eff.parts) : -1 };
    int f = t < EFF_IN ? 0 : t - EFF_IN < EFF_INSTALL ? t - EFF_IN : EFF_INSTALL - 1;
    anm_draw_ex(ID.eff_install[ia], f, 1, 0, 0, &part, 1);
  }
  if (m->eff.complete_t) {
    int c = m->eff.chara, two = c >= 14 && c <= 16, f = t - m->eff.complete_t + 1;
    anm_swap face = { anm_sprite(two ? T_EFF_FACE2 : T_EFF_FACE),
                      two || T > 20 ? anm_sprite(STEFF_COMPLETE[c]) : anm_sprite(T_EFF_FACE) };
    anm_draw_ex(two ? ID.eff_complete2 : ID.eff_complete, f < EFF_COMPLETE ? f : EFF_COMPLETE, 1, 0, 0, &face, 1);
  }
  /* clMenuFade: in over 10, out to black over 20 after the install, in over 40, out over 10 */
  float black = t < EFF_IN ? 1.0f - (float)(t + 1) / EFF_IN
                : t >= EFF_DARK && t < EFF_DARK + 20 ? (float)(t - EFF_DARK + 1) / 20.0f
                : t >= EFF_DARK + 20 && t < EFF_CHANGE ? 1.0f
                : t >= EFF_CHANGE && t < EFF_MAIN ? 1.0f - (float)(t - EFF_CHANGE + 1) / 40.0f : 0.0f;
  if (m->deciding) black = 1.0f - (float)m->deciding / TG_FADE;
  anm_fill(0, 0, 0, black);
}

static void draw_tg_wear(const menu_state *m, int clock, void (*draw3d)(void *), void *ctx) {
  /* the stand by the part (the wheel's: clSeqTgWearPartsWheelThread::yield setPosition) */
  static const float STAND[5][3] = { { 20, 118, 1 }, { 20, 120, 1 }, { 20, 87, 1 }, { 20, 268, 0.5f }, { 20, 248, 0.5f } };
  int k = m->tg.kind >= 0 && m->tg.kind <= 4 ? m->tg.kind : 1;
  const dress_wheel_set *w = k == 0 ? wheel_set(m->tg.chosen, NULL) : NULL;
  int text = k == 0 ? (w ? WEAR_WHEEL : WEAR_WHEEL_STOCK) : wear_text(m->tg.gem, m->tg.kind, m->tg.chosen);
  int name = text < 0 ? -1 : anm_sprite(WEAR_TEXT + text);
  /* the banner's pictures up before it comes down, one a frame */
  if (m->card_t < 3) anm_warm_sprite(m->card_t == 0 ? name : anm_sprite(m->card_t == 1 ? S_WEAR_WAKU : S_WHITE), 1);
  anm_draw(ID.tg_bg1, 0, 1);
  anm_draw_xy(ID.tg_bg3, 0, 1, STAND[k][0], STAND[k][1], 1, STAND[k][2], 0, 0);
  if (draw3d) draw3d(ctx);
  int out = -m->tg.left - WEAR_WAIT;
  anm_draw(ID.top_notime, topbar_notime_frame(m->card_t, out >= 0 ? out : -1), 1);
  anm_draw(ID.tg_wear_title, 0, 1);
  if (k == 0) {
    /* the wheel's banner from state 4 (setPos(0, 0); up off the screen before): the name
     * logo (+ 0x1c) and 装着完了！ at the font template, the stock's text alone centred */
    if (m->tg.state >= 4 && !m->tg.same) {
      int f = clock % anm_frames(ID.tg_wear_logo);
      anm_swap sw[2] = { { anm_sprite(T_WEAR_WHEEL), w ? anm_sprite(w->logo) : -1 }, { anm_sprite(T_WEAR_FONT), -1 } };
      anm_draw_ex(ID.tg_wear_logo, f, 1, 0, 0, sw, 2);
      float r[4];
      if (name >= 0 && w && anm_sprite_rect(ID.tg_wear_logo, f, anm_sprite(T_WEAR_FONT), r))
        anm_draw_sprite(name, r[0], r[1], 1);
      else if (name >= 0 && !w)
        anm_draw_sprite(name, 330.0f - anm_sprite_width(name) / 2.0f, 118.0f - 14.0f - 2.0f, 1);
    }
    if (m->tg.complete_t) {
      /* the COMPLETE over the second fade (to 0.5 at 1/6 a frame) */
      int t = m->card_t - m->tg.complete_t;
      anm_fill(0, 0, 0, t < 3 ? (float)(t + 1) / 6.0f : 0.5f);
      anm_draw(ID.tg_complete, t < anm_frames(ID.tg_complete) ? t : anm_frames(ID.tg_complete) - 1, 1);
    }
    return;
  }
  if (m->tg.state >= 3 && !m->tg.same) {
    anm_swap off[2] = { { anm_sprite(T_WEAR_WHEEL), -1 }, { anm_sprite(T_WEAR_FONT), -1 } };
    anm_draw_ex(ID.tg_wear_logo, clock % anm_frames(ID.tg_wear_logo), 1, 0, 23.0f, off, 2);
    /* clSpriteFont::draw datrums 4: the advances' sum W and the line's 28 centred on the
     * place, each 32-pixel cell 2 up and left of its advance (the sprite is W + 4 wide) */
    if (name >= 0) anm_draw_sprite(name, 320.0f - anm_sprite_width(name) / 2.0f, 140.0f - 14.0f - 2.0f, 1);
  }
}

/* the GET screens' stand (tg_bg_3) moved by the part (clSeqTgPartsGetWheelThread::yield,
 * clSeqTgPartsGetWingThread::yield setPosition): the wheel's (20, 111), the bonnet's (20, -12),
 * the wing's (20, 15), the mirror's (20, -6); the car's where it is */
static void tg_get_stand(const menu_state *m, float *dx, float *dy) {
  static const float STAND[5][2] = { { 20, 111 }, { 0, 0 }, { 20, -12 }, { 20, 15 }, { 20, -6 } };
  int k = m->screen == MENU_TG_GET && m->tg.kind >= 0 && m->tg.kind <= 4 ? m->tg.kind : 1;
  *dx = STAND[k][0], *dy = STAND[k][1];
}

static void draw_tg_bg(const menu_state *m, float fade) {
  /* the neon parts' screens have the black stripe (setImageReplace(0x5af, 0x111f)) */
  int black = m->screen == MENU_TG_GET && (m->tg.kind == 6 || m->tg.kind == 8);
  anm_swap sw = { anm_sprite(T_TG_BG), anm_sprite(S_TG_BG_BLACK) };
  anm_draw_ex(ID.tg_bg1, 0, fade, 0, 0, &sw, black);
  float dx, dy;
  tg_get_stand(m, &dx, &dy);
  anm_draw_ex(ID.tg_bg3, 0, fade, dx, dy, 0, 0);
}

static void draw_tg_get(const menu_state *m, int clock, float fade, void (*draw3d)(void *), void *ctx) {
  draw_tg_bg(m, fade);
  if (draw3d) draw3d(ctx);
  (void)clock;
  anm_draw(ID.top_notime, topbar_notime_frame(m->card_t, m->deciding ? TG_FADE - m->deciding : -1), fade);
  int lv = m->tg.lv;
  anm_swap title[3] = {
    { anm_sprite(T_TG_TENS), lv >= 10 ? anm_sprite(DRESS_DIGIT[lv / 10 % 10]) : -1 },
    { anm_sprite(T_TG_ONES), anm_sprite(DRESS_DIGIT[lv % 10]) },
    { anm_sprite(T_TG_KIND), m->tg.kind >= 0 && m->tg.kind < 9 ? anm_sprite(DRESS_KIND_TITLE[m->tg.kind]) : -1 },
  };
  anm_draw_ex(ID.tg_get_title, 0, fade, 0, 0, title, 3);
  int type = tg_get_type(m);
  if (type == TG_GET_WHEEL) {
    /* get_wheel from 101 on, its maker's logo in 0x1e5f (the wheel turning behind it) */
    anm_swap name = { anm_sprite(T_TG_WHEEL_NAME), anm_sprite(m->tg.name) };
    int f = TG_WHEEL_FROM + m->card_t;
    anm_draw_ex(ID.tg_get_wheel, f < anm_frames(ID.tg_get_wheel) ? f : anm_frames(ID.tg_get_wheel) - 1, fade, 0, 0, &name, 1);
    return;
  }
  if (type == TG_GET_PICTURE && m->card_t >= TG_FADE) {
    /* the part's picture: get_parts_image 80..99 from the end of the fade in, then held,
     * moved by the part (the bonnet's (-10, 10), the wing's (0, 5) or the per-car wing's
     * (50, 16), the mirror's (20, 10)), under the name */
    int t = m->card_t - TG_FADE;
    float dx = m->tg.kind == 2 ? -10.0f : m->tg.kind == 4 ? 20.0f : m->tg.num == 1 ? 50.0f : 0.0f;
    float dy = m->tg.kind == 3 ? (m->tg.num == 1 ? 16.0f : 5.0f) : 10.0f;
    anm_swap img = { anm_sprite(T_TG_IMAGE), anm_sprite(m->tg.image) };
    anm_draw_ex(ID.tg_get_image, TG_IMAGE_FROM + (t < TG_IMAGE_LEN ? t : TG_IMAGE_LEN - 1), fade, dx, dy, &img, 1);
  }
  anm_swap part = { anm_sprite(T_TG_PART), anm_sprite(m->tg.name) };
  anm_draw_ex(ID.tg_get_parts, m->card_t % anm_frames(ID.tg_get_parts), fade, 0, 0, &part, 1);
}

static void draw_tg_part(const menu_state *m, int clock, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  /* the stand's place and scale across, down; the neons' (setScale(0.95, 0.75), setPos(16,
   * 60)) on the black stripe (setImageReplace(0x5af, 0x111f)) */
  static const float STAND[5][4] = { { 20, -36, 1, 1 }, { 20, 130, 1, 0.5f }, { 20, 130, 1, 0.5f }, { 20, 130, 1, 0.6f },
                                     { 16, 60, 0.95f, 0.75f } };
  int l = tg_part_layout(m), spot = m->tg.kind == 8;
  const tg_layout_def *d = &TG_PART_LAYOUTS[l];
  anm_swap black = { anm_sprite(T_TG_BG), anm_sprite(S_TG_BG_BLACK) };
  anm_draw_ex(ID.tg_bg1, 0, fade, 0, 0, &black, l == 4);
  anm_draw_xy(ID.tg_bg3, 0, fade, STAND[l][0], STAND[l][1], STAND[l][2], STAND[l][3], 0, 0);
  if (draw3d) draw3d(ctx);
  anm_swap sw[32];
  int n = 0;
  for (int k = 0; k < 4; ++k) sw[n++] = (anm_swap){ anm_sprite(T_TG_DOT + k), -1 };
  for (int k = 0; k < d->slots; ++k) {
    if (d->new_t[k]) sw[n++] = (anm_swap){ anm_sprite(d->new_t[k]), k == m->tg.newest && !m->dress_free ? anm_sprite(T_TG_NEW) : -1 };
    if (d->lock[k] && k >= m->tg.n) sw[n++] = (anm_swap){ anm_sprite(d->lock[k]), anm_sprite(spot ? TG_SPOT_LOCK : d->lock_to) };
    else if (l == 4 && spot && k >= 2) sw[n++] = (anm_swap){ anm_sprite(d->icon[k]), anm_sprite(TG_SPOT_ICON + k - 2) };
  }
  /* the neons' bar white (setImageReplace(0xb82, 0x19dc)) */
  if (l == 4) sw[n++] = (anm_swap){ anm_sprite(JID(0xb82)), anm_sprite(JID(0x19dc)) };
  anm_draw_ex(tg_part_anim(m, 0), tg_part_frame(m), fade, 0, 0, sw, n);
  anm_draw(tg_part_anim(m, 1), tg_part_arrow_frame(m), fade);
  (void)clock;
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(l == 4 && spot ? TG_SPOT_TOPBAR : d->topbar) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

static void draw_tg_aero(const menu_state *m, int clock, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  draw_tg_bg(m, fade);
  if (draw3d) draw3d(ctx);
  int lay = tg_layout(m);
  for (int i = 0; i < m->tg.n; ++i) {
    int o = m->tg.opt[i];
    int name = o < 0 ? S_TG_REFUSE : o == 0 ? S_TG_STOCK : m->tg.gem ? S_TG_AERO_GEM : T_TG_NAME + o - 1;
    anm_swap sw[5] = {
      { anm_sprite(T_TG_PLATE), anm_sprite(S_TG_PLATE) },
      { anm_sprite(T_TG_NAME), anm_sprite(name) },
      { anm_sprite(T_TG_NAME2), anm_sprite(name) },
      { anm_sprite(T_TG_NEW), i == m->tg.newest ? anm_sprite(T_TG_NEW) : -1 },
      { anm_sprite(T_TG_DOT), -1 },
    };
    anm_draw_ex(lay, tg_frame(m, i), fade, 0, 0, sw, 5);
  }
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_TG_TOPBAR) };
  draw_topbar(bars, fade, &mes);
  draw_bottom(bars, fade);
}

/* ---- the versus (MENU_VS, the port's own screen, no arcade sprites but its top bar's title:
 * hud.c's panels and 5x7 text), for the kind the versus select chose (MENU_VS_SEL). Left over
 * the LAN the games found nearby (a tap joins) and HOST A GAME; over a server its
 * address with a keypad and CONNECT. Right the lobby's players, then the host's start ramp
 * (the arcade's join ramps, clRaceThread::initJoin's table) and distance, and START. Taps in
 * the menus' points (1024 x 768). */
static const char *const VS_RAMPS[MENU_VS_RAMPS] = {
  "C1 INNER - KANDABASHI", "C1 OUTER - KANDABASHI", "C1 INNER - SHIODOME", "C1 OUTER - SHIBAKOEN",
  "NEW BELT IN - ARIAKE", "NEW BELT OUT - KIBA", "WANGAN OUT - 13GOCHI", "WANGAN IN - DAIKOKU",
  "WANGAN OUT - AIRPORT", "WANGAN IN - AIRPORT", "YOKOHANE DOWN - SHIBAURA", "YOKOHANE UP - SHIOIRI",
  "YOKOHANE UP - HANEDA", "YOKOHANE DOWN - HANEDA", "OSAKA - TONBORI", "HAKONE OUTWARD",
  "HAKONE HOMEWARD", "NAGOYA - MARUNOUCHI", "FUKUOKA - MEINOHAMA", "FUKUOKA - NISHIKOEN",
  "FUKUOKA - HAKATA EKI", "FUKUOKA - HANMICHIBASHI",
};

const char *menu_vs_ramp_name(int ramp) { return ramp >= 0 && ramp < MENU_VS_RAMPS ? VS_RAMPS[ramp] : ""; }

/* the kana as romaji (hiragana from U+3041; katakana are 0x60 on) */
static const char *const KANA[83] = {
  "A", "A", "I", "I", "U", "U", "E", "E", "O", "O", "KA", "GA", "KI", "GI", "KU", "GU", "KE", "GE", "KO", "GO",
  "SA", "ZA", "SHI", "JI", "SU", "ZU", "SE", "ZE", "SO", "ZO", "TA", "DA", "CHI", "DI", "", "TSU", "DU", "TE", "DE",
  "TO", "DO", "NA", "NI", "NU", "NE", "NO", "HA", "BA", "PA", "HI", "BI", "PI", "FU", "BU", "PU", "HE", "BE", "PE",
  "HO", "BO", "PO", "MA", "MI", "MU", "ME", "MO", "YA", "YA", "YU", "YU", "YO", "YO", "RA", "RI", "RU", "RE", "RO",
  "WA", "WA", "WI", "WE", "WO", "N",
};

void menu_name_glyphs(short g[5], const char *ascii) {
  char s[8] = { 0 };
  int n = 0;
  /* vs_name's PLAYER n (a save from before the names): nP, as the arcade's sides */
  if (ascii[0] == 'P' && ascii[1] == 'L' && ascii[2] == 'A' && ascii[3] == 'Y' && ascii[4] == 'E' && ascii[5] == 'R' &&
      ascii[6] == ' ' && ascii[7] >= '1' && ascii[7] <= '9')
    s[0] = ascii[7], s[1] = 'P';
  else
    for (int i = 0; ascii[i] && n < 5; ++i)
      if (ascii[i] != ' ') s[n++] = ascii[i];
  n = 0;
  for (int i = 0; s[i] && n < 5; ++i) {
    char c = s[i] >= 'a' && s[i] <= 'z' ? (char)(s[i] - 32) : s[i];
    for (int k = 0; k < NAMEDB_N; ++k)
      if (NAMEDB_WCHAR[k] == (unsigned)(unsigned char)c + 0xfee0) {
        g[n++] = (short)k;
        break;
      }
  }
  for (int i = n; i < 5; ++i) g[i] = -1;
}

void menu_name_ascii(const menu_state *m, char *out, int n) {
  int k = 0;
  for (int i = 0; i < 5 && m->name[i] >= 0 && m->name[i] < NAMEDB_N; ++i) {
    unsigned c = NAMEDB_WCHAR[m->name[i]];
    char one[2] = { 0, 0 };
    const char *s = one;
    if (c >= 0xff01 && c <= 0xff5e) one[0] = (char)(c - 0xfee0);
    else if (c == 0x3000) one[0] = ' ';
    else if (c == 0x30fc) one[0] = '-';
    else if (c == 0x30f4) s = "VU";
    else if (c >= 0x30a1 && c <= 0x30f3) s = KANA[c - 0x30a1];
    else if (c >= 0x3041 && c <= 0x3093) s = KANA[c - 0x3041];
    else one[0] = '*';
    for (; *s && k < n - 1; ++s) out[k++] = *s >= 'a' && *s <= 'z' ? (char)(*s - 32) : *s;
  }
  if (!k && n > 6) memcpy(out, "PLAYER", 6), k = 6;
  out[k < n ? k : n - 1] = 0;
}

/* menupack.py's VS_TEXT_BASE (the top bar's VERSUS, then BLUETOOTH, LAN and ONLINE VERSUS),
 * VS_ICON_BASE and VS_LINE_BASE (the versus select's discs, tools/vsdiscs.py: BLUETOOTH, LAN, ONLINE; the first is no longer used, so a kind's picture is at
 * its number + 1) */
#define VS_TEXT 107000
#define VS_ICON 107010
#define VS_LINE 107020
enum { VS_KEYS = 12, VS_CONNECT = VS_KEYS, VS_RAMP_L, VS_RAMP_R, VS_KM_L, VS_KM_R, VS_START, VS_HOSTNET /* (online: host a game here) */, VS_BUTTONS };
#define VS_LEFT 60.0f
#define VS_RIGHT 520.0f
#define VS_RIGHT_W 460.0f
#define VS_ROW_Y 165.0f
#define VS_ROW_H 54.0f

/* a button's place: under VS_KEYS the server's keypad, or the games found nearby's rows */
static void vs_button(const menu_state *m, int b, float r[4]) {
  if (b < VS_KEYS && m->vs.kind != MENU_VS_NET) {
    r[0] = VS_LEFT, r[1] = 186.0f + (float)b * 64.0f, r[2] = 410.0f, r[3] = 56.0f;
    return;
  }
  if (b < VS_KEYS) {
    r[0] = VS_LEFT + (float)(b % 3) * 140.0f, r[1] = 250.0f + (float)(b / 3) * 64.0f, r[2] = 130.0f, r[3] = 56.0f;
    return;
  }
  static const float R[VS_BUTTONS - VS_KEYS][4] = {
    { VS_LEFT, 512, 410, 56 },        /* CONNECT */
    { VS_RIGHT, 436, 64, 52 }, { VS_RIGHT + VS_RIGHT_W - 64, 436, 64, 52 }, /* the ramp */
    { VS_RIGHT, 500, 64, 52 }, { VS_RIGHT + VS_RIGHT_W - 64, 500, 64, 52 }, /* the distance */
    { VS_RIGHT, 568, VS_RIGHT_W, 58 }, /* START */
    { VS_LEFT + 210, 512, 200, 56 },   /* HOST ONLINE */
  };
  memcpy(r, R[b - VS_KEYS], sizeof R[0]);
  if (b == VS_CONNECT && m->vs.kind == MENU_VS_NET && m->vs.state == MENU_VS_OFF) r[2] = 200; /* (the address's CONNECT leaves room for HOST ONLINE) */
}

/* a lobby row's name in the arcade's glyphs (from a build that sends them, a menu.bin with them) */
#define VS_GLYPH_W 28.0f /* font 5's advance (the card's) */
#define VS_GLYPH_H 30.0f
static int vs_glyphs(const menu_state *m, int i) { return m->vs.glyph[i][0] >= 0 && anm_sprite(NAME_GLYPH5) >= 0; }

static int vs_host(const menu_state *m) { return m->vs.state == MENU_VS_LOBBY && m->vs.me == m->vs.host; }

/* which of the screen's buttons is at (x, y) and on now: its number, -1 none */
static int vs_hit(const menu_state *m, float x, float y) {
  for (int b = 0; b < VS_BUTTONS; ++b) {
    if (b < VS_KEYS && m->vs.state != MENU_VS_OFF) continue;
    if (b < VS_KEYS && m->vs.kind != MENU_VS_NET && b >= m->vs.n_found) continue;
    if (b == VS_HOSTNET && (m->vs.kind != MENU_VS_NET || m->vs.state != MENU_VS_OFF)) continue;
    if (b > VS_CONNECT && b != VS_HOSTNET && !vs_host(m)) continue;
    if (b == VS_RAMP_L || b == VS_RAMP_R) continue; /* the area is voted (MENU_VS_AREA) */
    float r[4];
    vs_button(m, b, r);
    if (x >= r[0] - 6 && x < r[0] + r[2] + 6 && y >= r[1] - 4 && y < r[1] + r[3] + 4) return b;
  }
  return -1;
}

/* whether button b is there and on now */
static int vs_visible(const menu_state *m, int b) {
  if (b < VS_KEYS && m->vs.state != MENU_VS_OFF) return 0;
  if (b < VS_KEYS && m->vs.kind != MENU_VS_NET && b >= m->vs.n_found) return 0;
  if (b == VS_HOSTNET) return m->vs.kind == MENU_VS_NET && m->vs.state == MENU_VS_OFF;
  if (b > VS_CONNECT && !vs_host(m)) return 0;
  return b != VS_RAMP_L && b != VS_RAMP_R; /* the area is voted (MENU_VS_AREA) */
}

/* the controls over the buttons: left and right the neighbour, up and down the row (the keypad's: three along) */
static void vs_move(menu_state *m, const menu_in *in) {
  int f = m->vs.focus - 1;
  if (f < 0 || !vs_visible(m, f)) {
    for (f = 0; f < VS_BUTTONS && !vs_visible(m, f); ++f) {}
    if (f < VS_BUTTONS) snd_menu(SND_CURSOR), m->vs.focus = f + 1;
    return;
  }
  int step = in->dx ? in->dx : in->dy * (f < VS_KEYS && m->vs.kind == MENU_VS_NET ? 3 : 1);
  for (int b = f + step; b >= 0 && b < VS_BUTTONS; b += step > 0 ? 1 : -1) {
    if (!vs_visible(m, b)) continue;
    snd_menu(SND_CURSOR);
    m->vs.focus = b + 1;
    return;
  }
}

static void vs_step(menu_state *m, const menu_in *in) {
  if (m->vs.state >= MENU_VS_LOADING) return; /* the race is on its way */
  if (in->back) {
    snd_menu(SND_CANCEL);
    m->vs.connect = -1;
    m->vs.focus = 0;
    go(m, MENU_VS_SEL);
    return;
  }
  if (in->select >= 0 && vs_visible(m, in->select)) m->vs.focus = in->select + 1; /* a button chosen outright */
  else if (in->select < 0 && (in->dx || in->dy)) {
    vs_move(m, in);
    return;
  }
  if (!in->ok) return;
  int b = m->vs.focus - 1;
  if (b < 0 || !vs_visible(m, b)) {
    vs_move(m, in);
    return;
  }
  size_t len = strlen(m->vs.ip);
  if (b < VS_KEYS && m->vs.kind != MENU_VS_NET) {
    /* a game found nearby: join it */
    m->vs.join = b + 1;
    snd_menu(SND_SELECT);
  } else if (b < VS_KEYS) {
    static const char KEY[VS_KEYS] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '0', 0 };
    if (!KEY[b]) {
      if (len) m->vs.ip[len - 1] = 0;
    } else if (len < sizeof m->vs.ip - 1) {
      m->vs.ip[len] = KEY[b];
      m->vs.ip[len + 1] = 0;
    }
    snd_menu(SND_CURSOR);
  } else if (b == VS_HOSTNET) { /* host a game here, the router asked to open its port */
    m->vs.connect = 2;
    snd_menu(SND_SELECT);
  } else if (b == VS_CONNECT) {
    if (m->vs.state == MENU_VS_OFF && !len && m->vs.kind == MENU_VS_NET) return;
    m->vs.connect = m->vs.state == MENU_VS_OFF ? 1 : -1;
    snd_menu(m->vs.connect > 0 ? SND_SELECT : SND_CANCEL);
  } else if (b == VS_RAMP_L || b == VS_RAMP_R) {
    m->vs.ramp = (m->vs.ramp + (b == VS_RAMP_L ? MENU_VS_RAMPS - 1 : 1)) % MENU_VS_RAMPS;
    snd_menu(SND_CURSOR);
  } else if (b == VS_KM_L || b == VS_KM_R) {
    m->vs.km += b == VS_KM_L ? -1 : 1;
    m->vs.km = m->vs.km < 1 ? 1 : m->vs.km > 50 ? 50 : m->vs.km;
    snd_menu(SND_CURSOR);
  } else if (b == VS_START) {
    int all = m->vs.n >= 2;
    for (int i = 0; i < m->vs.n; ++i) all &= !m->vs.racing[i] && !m->vs.mismatch[i];
    if (!all) {
      snd_menu(SND_CANCEL);
      return;
    }
    m->vs.start = 1;
    snd_menu(SND_SELECT);
  }
}

/* a button shown pressed: under the finger, or the one the controls are on */
static int vs_lit(const menu_state *m, int b) { return m->vs.held == b + 1 || m->vs.focus == b + 1; }

static void vs_label(const char *s, float x, float y, float px, float a) { hud_ui_text(s, x, y, px, 1, 1, 1, a); }
static void vs_centred(const char *s, const float r[4], float px, float cr, float cg, float cb, float a) {
  hud_ui_text(s, r[0] + 0.5f * (r[2] - hud_ui_text_width(s, px)), r[1] + 0.5f * (r[3] - 7.0f * px), px, cr, cg, cb, a);
}

static const char *vs_place_name(int place) {
  static char s[8];
  snprintf(s, sizeof s, "%d%s", place, place == 1 ? "ST" : place == 2 ? "ND" : place == 3 ? "RD" : "TH");
  return s;
}

/* The race's loading after the votes (the arcade goes from them to the race's loading, the
 * lobby not again): the race's title page as the time attack's (the course, the time zone,
 * this player's song), NOW LOADING bottom left; loaded first, waiting for the others' */
static void draw_vs_loading(const menu_state *m, int clock, float screen_w, float screen_h) {
  /* (black while the cars flying in first come in) */
  if (m->vs.match_n >= 2 && !m->vs.match_played) {
    anm_fill(0, 0, 0, 1);
    draw_wait(1, m->t, 1);
    return;
  }
  float fade = m->t < FADE ? (float)(m->t + 1) / FADE : 1.0f;
  draw_pre_as(m, fade, 1);
  if (!m->vs.loaded) {
    int f = m->t + 1;
    float a = f > LOADING_POP ? 0.725f + 0.275f * cosf((float)(f - LOADING_POP) * (2.0f * (float)M_PI / 60.0f)) : 1.0f;
    anm_draw_xf(ID.loading, f < LOADING_POP ? f : LOADING_POP, a * fade, LOADING_PRE_DX, 0, 1, NULL, 0);
  } else if (m->vs.wait_others && ID.wait1 >= 0) {
    draw_wait(0, m->t, fade); /* (the arcade's: Please wait! / Waiting on other player(s)) */
  } else if (m->vs.wait_others) {
    static const char *const WAIT[4] = { "WAITING FOR THE OTHER PLAYER", "WAITING FOR THE OTHER PLAYER.",
                                         "WAITING FOR THE OTHER PLAYER..", "WAITING FOR THE OTHER PLAYER..." };
    hud_ui_begin(screen_w, screen_h);
    float px = 3.0f, y = screen_h * 0.5f + 60.0f;
    hud_ui_rect(0, y - 14.0f, hud_ui_text_width(WAIT[3], px) + 72.0f, 7.0f * px + 28.0f, 0, 0, 0, 0.6f);
    hud_ui_text(WAIT[(clock / 20) % 4], 36.0f, y, px, 1.0f, 0.55f, 0.1f, fade);
    hud_ui_end();
  }
}

static void draw_vs(const menu_state *m, int clock, int bars, float screen_w, float screen_h) {
  if (m->vs.state == MENU_VS_LOADING) {
    draw_vs_loading(m, clock, screen_w, screen_h);
    return;
  }
  float fade = m->t < FADE ? (float)(m->t + 1) / FADE : 1.0f;
  draw_back(ID.bg, clock % 600, 1);
  anm_draw(ID.line, 0, 1);
  anm_swap title = { anm_sprite(T_TOPBAR_MES), anm_sprite(VS_TEXT + 2 + m->vs.kind) }; /* (LAN: "Local Battle", ONLINE: "Online Battle": the baked list still has BLUETOOTH VERSUS second, which is gone) */
  draw_topbar(bars, 1, &title);
  draw_bottom(bars, 1);
  hud_ui_begin(screen_w, screen_h);
  const float O[3] = { 1.0f, 0.55f, 0.1f }; /* the arcade's orange */
  float r[4];
  int off = m->vs.state == MENU_VS_OFF;
  if (m->vs.kind == MENU_VS_NET && m->vs.hosting) {
    /* hosting online: the room code to give the others */
    vs_label("ROOM CODE", VS_LEFT, 160, 2.5f, 0.85f);
    hud_ui_rect(VS_LEFT, 182, 410, 56, 0, 0, 0, 0.55f);
    const char *rc = m->vs.invite[0] ? m->vs.invite : (clock / 20) % 4 == 0 ? "WORKING" : (clock / 20) % 4 == 1 ? "WORKING." : (clock / 20) % 4 == 2 ? "WORKING.." : "WORKING...";
    float rpx = fminf(4.0f, (410.0f - 28.0f) / hud_ui_text_width(rc, 1.0f));
    hud_ui_text(rc, VS_LEFT + 14, 182.0f + (56.0f - 7.0f * rpx) * 0.5f, rpx, 1, m->vs.invite[0] ? 1.0f : 0.55f, m->vs.invite[0] ? 1.0f : 0.15f, 1);
    if (m->vs.addr[0]) hud_ui_text(m->vs.addr, VS_LEFT, 256, 2.5f, 1, 1, 1, 0.8f);
    if (m->vs.natnote[0]) { /* (as large as fits the left column) */
      float npx = fminf(2.2f, 410.0f / hud_ui_text_width(m->vs.natnote, 1.0f));
      hud_ui_text(m->vs.natnote, VS_LEFT, 290, npx, 1, 0.85f, 0.4f, 1);
    }
    hud_ui_text("GIVE THE ROOM CODE TO THE OTHERS:", VS_LEFT, 340, 2.0f, 1, 1, 1, 0.6f);
    hud_ui_text("THEY TYPE IT UNDER CONNECT", VS_LEFT, 366, 2.0f, 1, 1, 1, 0.6f);
  } else if (m->vs.kind == MENU_VS_NET) {
    /* the server */
    vs_label("SERVER ADDRESS OR ROOM CODE", VS_LEFT, 160, 2.5f, 0.85f);
    hud_ui_rect(VS_LEFT, 182, 410, 56, 0, 0, 0, 0.55f);
    char ip[24];
    int caret = off && (clock / 30) % 2;
    snprintf(ip, sizeof ip, "%s_", m->vs.ip);
    /* as big as fits the box, the caret's room kept (a long address overflowed it) */
    float px = fminf(5.0f, (410.0f - 28.0f) / hud_ui_text_width(ip, 1.0f));
    if (!caret) ip[strlen(ip) - 1] = 0;
    hud_ui_text(ip[0] ? ip : " ", VS_LEFT + 14, 182.0f + (56.0f - 7.0f * px) * 0.5f, px, 1, 1, 1, off ? 1.0f : 0.7f);
    for (int b = 0; b < VS_KEYS && off; ++b) {
      static const char *const LABEL[VS_KEYS] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "DEL" };
      vs_button(m, b, r);
      hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, b));
      vs_centred(LABEL[b], r, b == 11 ? 3.5f : 4.5f, 1, 1, 1, 0.95f);
    }
  } else {
    /* the games nearby, a tap joins; or host one */
    vs_label(off ? "GAMES NEARBY" : m->vs.hosting ? "HOSTING A GAME" : "JOINED A GAME", VS_LEFT, 160, 2.5f, 0.85f);
    for (int b = 0; b < MENU_VS_FOUND && off; ++b) {
      vs_button(m, b, r);
      if (b < m->vs.n_found) {
        hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, b));
        hud_ui_text(m->vs.found[b], r[0] + 16, r[1] + 17, 3.0f, 1, 1, 1, 0.95f);
      } else {
        hud_ui_rect(r[0], r[1], r[2], r[3], 0, 0, 0, 0.3f);
        if (b == m->vs.n_found)
          hud_ui_text((clock / 20) % 4 == 0 ? "SEARCHING" : (clock / 20) % 4 == 1 ? "SEARCHING." : (clock / 20) % 4 == 2 ? "SEARCHING.." : "SEARCHING...",
                      r[0] + 16, r[1] + 20, 2.5f, 1, 1, 1, 0.45f);
      }
    }
    if (off)
      hud_ui_text("THE SAME WI-FI ON EVERY DEVICE", VS_LEFT, 448, 2.0f, 1, 1, 1, 0.6f);
  }
  vs_button(m, VS_CONNECT, r);
  hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, VS_CONNECT));
  const char *act = m->vs.kind == MENU_VS_NET ? (off ? "CONNECT" : m->vs.hosting ? "STOP HOSTING" : "DISCONNECT")
                    : off                     ? "HOST A GAME"
                    : m->vs.hosting           ? "STOP HOSTING"
                                              : "LEAVE";
  vs_centred(act, r, m->vs.kind == MENU_VS_NET && off ? 3.0f : 4.0f, 1, 1, 1, 0.95f);
  if (m->vs.kind == MENU_VS_NET && off) {
    vs_button(m, VS_HOSTNET, r);
    hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, VS_HOSTNET));
    vs_centred("HOST ONLINE", r, 2.6f, 1, 1, 1, 0.95f);
  }
  if (m->vs.note[0]) hud_ui_text(m->vs.note, VS_LEFT, 584, 2.5f, 1, 0.85f, 0.4f, 1);
  if (m->vs.last_place) {
    char s[48];
    snprintf(s, sizeof s, "LAST RACE: %s OF %d", vs_place_name(m->vs.last_place), m->vs.last_n);
    hud_ui_text(s, VS_LEFT, 612, 2.5f, 1, 1, 1, 0.9f);
  }
  /* the lobby */
  vs_label("PLAYERS", VS_RIGHT, 140, 2.5f, 0.85f);
  for (int i = 0; i < MENU_VS_PLAYERS; ++i) {
    float y = VS_ROW_Y + (float)i * (VS_ROW_H + 8);
    int on = i < m->vs.n && m->vs.state >= MENU_VS_LOBBY;
    hud_ui_rect(VS_RIGHT, y, VS_RIGHT_W, VS_ROW_H, i == m->vs.me && on ? 0.35f : 0, i == m->vs.me && on ? 0.18f : 0, 0, 0.55f);
    char k[4];
    snprintf(k, sizeof k, "%d", i + 1);
    hud_ui_text(k, VS_RIGHT + 12, y + 13, 4.0f, O[0], O[1], O[2], on ? 1.0f : 0.4f);
    if (!on) {
      hud_ui_text("---", VS_RIGHT + 60, y + 18, 3.0f, 1, 1, 1, 0.35f);
      continue;
    }
    if (!vs_glyphs(m, i)) hud_ui_text(m->vs.name[i], VS_RIGHT + 60, y + 7, 3.5f, 1, 1, 1, 1);
    hud_ui_text(m->vs.car[i], VS_RIGHT + 60, y + 35, 2.0f, 0.8f, 0.85f, 0.95f, 0.9f);
    const char *tag = m->vs.racing[i] ? "RACING" : i == m->vs.host ? "HOST" : "READY";
    hud_ui_text(tag, VS_RIGHT + VS_RIGHT_W - 12 - hud_ui_text_width(tag, 2.5f), y + 19, 2.5f,
                m->vs.racing[i] ? 1.0f : 0.5f, m->vs.racing[i] ? 0.5f : 1.0f, 0.4f, 1);
  }
  /* the race: the host chooses */
  int host = vs_host(m);
  vs_label("COURSE / DISTANCE", VS_RIGHT, 414, 2.5f, 0.85f);
  float row[2][4] = { { VS_RIGHT, 436, VS_RIGHT_W, 52 }, { VS_RIGHT, 500, VS_RIGHT_W, 52 } };
  char km[16];
  snprintf(km, sizeof km, "%d KM", m->vs.km);
  for (int k = 0; k < 2; ++k) {
    hud_ui_rect(row[k][0], row[k][1], row[k][2], row[k][3], 0, 0, 0, 0.55f);
    vs_centred(k ? km : "AREA CHOSEN BY MAJORITY", row[k], k ? 3.5f : 2.5f, 1, 1, 1, host || m->vs.state == MENU_VS_LOBBY ? 1 : 0.5f);
    for (int s = 0; host && k && s < 2; ++s) {
      int b = VS_RAMP_L + 2 * k + s;
      vs_button(m, b, r);
      hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, b));
      vs_centred(s ? ">" : "<", r, 4.0f, 1, 1, 1, 0.95f);
    }
  }
  vs_button(m, VS_START, r);
  if (host) {
    int ready = m->vs.n >= 2;
    for (int i = 0; i < m->vs.n; ++i) ready &= !m->vs.racing[i];
    hud_ui_button(r[0], r[1], r[2], r[3], vs_lit(m, VS_START));
    vs_centred(ready ? "START" : m->vs.n < 2 ? "WAITING FOR PLAYERS" : "WAITING FOR THE RACE", r, ready ? 5.0f : 3.0f, 1, 1, 1,
               ready ? 0.95f : 0.5f);
  } else if (m->vs.state == MENU_VS_LOBBY) {
    vs_centred("WAITING FOR THE HOST", r, 3.0f, 1, 1, 1, 0.7f);
  }
  hud_ui_end();
  /* the names as the card has them, in font 5's glyphs (over the rows the UI drew; the UI's
   * units are the screen's, the sprites' the 640x480 layout's), VS_GLYPH_H tall */
  float kx = 640.0f / screen_w, ky = 480.0f / screen_h;
  for (int i = 0; i < m->vs.n && m->vs.state >= MENU_VS_LOBBY; ++i) {
    if (!vs_glyphs(m, i)) continue;
    float y = VS_ROW_Y + (float)i * (VS_ROW_H + 8) + 3.0f, x = VS_RIGHT + 60.0f;
    for (int j = 0; j < 5 && m->vs.glyph[i][j] >= 0; ++j) {
      int sp = anm_sprite(NAME_GLYPH5 + m->vs.glyph[i][j]);
      float w = anm_sprite_width(sp), h = anm_sprite_height(sp);
      if (h <= 0) continue;
      float k = VS_GLYPH_H / h; /* screen px per sprite px */
      const float src[4] = { 0, 0, w, h };
      float dst[4] = { x * kx, y * ky, (x + w * k) * kx, (y + VS_GLYPH_H) * ky };
      anm_draw_part(sp, src, dst, 1);
      x += VS_GLYPH_W * k;
    }
  }
  hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
  anm_fill(0, 0, 0, 1.0f - fade);
  if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
}

/* ---- the area vote (MENU_VS_AREA): seqJoCourseSelect's clseqJoCourseSelectThread, the
 * versus' "Please select the Driving area / Area shall be chosen by majority" after the
 * lobby's START.
 * - menu_layout_vs_course1, the strip of nine cards (0 omakase, then the areas by menu order:
 *   C1, Shinkanjo, Wangan, Yokohane, Nagoya, Osaka, Fukuoka, Hakone), 100 frames a card k:
 *   k*100..58 the card chosen at rest (the arrows' pulse), k*100+60.. decided, k*100+90..94
 *   the 5 frames on to card k + 1 (back: the same, the other way). The run's init sets the
 *   cards' pictures to the whole areas' and hides every "Can not select" and the distances.
 * - The card's place on screen (0..5) and the strip's scroll: going right it stays at 3 from
 *   the fourth card on while the card was 5 or less, going left at 2 from the third on, the
 *   strip sliding 17.6 px a frame for the move's 5 frames (0x93fba54).
 * - The map: clMiniatureCourse with the versus' table (0x93fa400: Tokyo's cards on the 3D
 *   miniature) or mapChange's city picture (Nagoya, Osaka, Fukuoka, Hakone), as the time
 *   attack's.
 * - 20 s on the top bar (clMenuBarStd::setTime: menu_g_timer_x, ti_count each second from 9);
 *   at 0 the card under the cursor is the vote. Omakase votes a random area (the timer's
 *   bits: Hakone, 6, at half the odds), which the cursor walks to.
 * - dispOther: the other players' marks (menu_p_vs_4p_N) under the card each has; three a
 *   card, 32 px apart; a mark off the screen goes to its edge with an arrow (menu_g_vs_icon,
 *   or its omakase pair after an omakase vote).
 * - When all have voted the area with the most votes wins, a tie by a random pick among them
 *   (clNet::getJoinArea); the strip goes to it and plays its decided frames.
 * The vote rides each player's lobby status (menu_vs_status); the host decides and starts the
 * race on a random ramp of the area (app.c vs_menu). */
#define AREA_TIME (20 * 60)
#define AREA_MOVE 5
#define AREA_SCROLL 17.6f
#define AREA_DECIDE 10 /* the decided frames the result plays (k*100+60..69) */
#define AREA_VOTED 16  /* this player's own vote: k*100+60..75, then held on 80 (the strip, no arrows) */
enum { A_ICON = JID(0x39e), A_ARROW_R = JID(7377), A_ARROW_L = JID(7771), A_ARROW = JID(2535), A_ARROW_L_IMG = JID(3393),
       A_TOPBAR = JID(7149), A_TIME_DIGIT = JID(693), A_TIME_LAST = JID(2524) };
/* the cards' pictures (the run's init: the areas' own) and what it hides */
static const int AREA_PICTURE[8][2] = { { JID(4646), JID(5545) }, { JID(4648), JID(5546) }, { JID(4650), JID(5550) }, { JID(4652), JID(5557) },
                                        { JID(4658), JID(4659) }, { JID(4654), JID(4657) }, { JID(4655), JID(5559) }, { JID(4661), JID(4661) } };
static const int AREA_HIDE[] = { JID(6524), JID(7009), JID(7010), JID(7011), JID(7012), JID(7013), JID(7014), JID(7015), JID(2777), JID(3168),
                                 JID(2239), JID(2240), JID(2241), JID(2242), JID(2243), JID(2244), JID(2245), JID(2246), JID(2247), JID(2248),
                                 JID(7031), JID(7032), JID(7033), JID(7034), JID(7035), JID(7036), JID(7037), JID(7038), JID(7039), JID(7040),
                                 JID(7226), JID(7227), JID(7228), JID(7229), JID(7230), JID(7231), JID(7232), JID(7233), JID(7234), JID(7235), JID(7236), JID(7237), JID(7238), JID(7493) };
#define AREA_NSWAP (8 + (int)(sizeof AREA_HIDE / sizeof AREA_HIDE[0]))
/* dispOther's places (0x93f9fe0): a mark under a card left of this player's, under it, right
 * of it; rows 32 px down. Off the screen (0x93f9f80): the left or right edge, from the bottom */
static const float AREA_MARK_X0[3] = { 60, 100, 140 }, AREA_MARK_Y0[3] = { 210, 260, 210 };
static const float AREA_EDGE_X[2] = { 69, 570 }, AREA_EDGE_Y[3] = { 316, 348, 380 };
/* menu_g_vs_icon's frames by player (0x93f9f5c) */
static const int AREA_ICON_FROM[MENU_VS_PLAYERS] = { 1, 101, 201, 301 };
/* areaIDFromMenuOrder: the area of card k (1..8) */
static const int CARD_AREA[MENU_VS_CARDS] = { -1, 0, 1, 2, 3, 4, 5, 7, 6 };

int menu_vs_card_area(int card) { return card > 0 && card < MENU_VS_CARDS ? CARD_AREA[card] : -1; }

int menu_vs_status(const menu_state *m) {
  if (m->vs.tuning) return (m->vs.tuned ? MENU_VS_TUNED : MENU_VS_TUNE) + (m->vs.day_final > 0);
  if (m->screen == MENU_BGM && m->vs.day) {
    if (m->vs.day_final >= 0 && m->vs.me == m->vs.host) return MENU_VS_DAY_FINAL + m->vs.day_final;
    return (m->vs.day_voted == 2 ? MENU_VS_DAY_OMAKASE : m->vs.day_voted ? MENU_VS_DAY_VOTED : MENU_VS_DAY) + m->zone;
  }
  /* (on the stars screen as if still racing: the host's START waits for it) */
  if (m->screen == MENU_VS_STAR) return 1;
  if (m->screen != MENU_VS_AREA) return m->vs.state < MENU_VS_LOADING ? 0 : m->vs.loaded ? MENU_VS_LOADED : 1;
  if (m->vs.area_final >= 0 && m->vs.me == m->vs.host) return MENU_VS_FINAL + m->vs.area_final;
  if (m->vs.voted) return (m->vs.voted == 2 ? MENU_VS_OMAKASE : MENU_VS_VOTED) + m->vs.vote;
  return MENU_VS_CHOOSING + m->vs.item;
}

/* the HP screen after the course and the zone are decided (the arcade's: the power is set per course): the car setting with DONE, then waiting for the
 * others; the host's START (all done, or the time out) takes everyone on to the race */
void menu_vs_tune_open(menu_state *m) {
  m->vs.tuning = 1;
  m->vs.tuned = 0;
  m->vs.tune_timer = 15 * 60; /* (the arcade's HP screen counts 15 seconds) */
  tune_confirm_enter(m, MENU_VS);
}

/* the game it was in gone while the HP screen was up: back to the versus screen */
void menu_vs_tune_close(menu_state *m) {
  m->vs.tuning = m->vs.tuned = 0;
  m->tune_confirm = 0;
  go(m, MENU_VS);
  m->bars = 1;
}

void menu_vs_loading(menu_state *m) {
  if (m->screen == MENU_VS_AREA || m->screen == MENU_VS_MATCH || (m->screen == MENU_BGM && m->vs.day) || (m->screen == MENU_SETTING && m->vs.tuning)) {
    m->vs.tuning = m->vs.tuned = 0;
    m->tune_confirm = 0;
    go(m, MENU_VS);
  }
  m->vs.state = MENU_VS_LOADING;
  m->vs.day = m->vs.loaded = m->vs.wait_others = m->vs.match_played = 0;
}

static void vs_area_enter(menu_state *m) {
  go(m, MENU_VS_AREA);
  m->vs.area_go = 0;
  m->vs.item = m->vs.pos = 0;
  m->vs.item_from = -1;
  m->vs.move_t = 0;
  m->vs.scroll = m->vs.scroll_from = 0;
  m->vs.timer = AREA_TIME;
  m->vs.voted = m->vs.vote = m->vs.vote_t = 0;
  m->vs.area_final = -1;
  m->vs.final_t = 0;
  m->music = 1;
}

/* one card on (d = +1) or back (-1), as the arcade's moves */
static void area_move(menu_state *m, int d) {
  int to = m->vs.item + d;
  if (to < 0 || to >= MENU_VS_CARDS) return;
  snd_menu(SND_CURSOR);
  int scroll = d > 0 ? m->vs.pos > 2 && m->vs.item <= 5 : m->vs.pos <= 2 && m->vs.item > 2;
  if (!scroll) m->vs.pos += d;
  m->vs.scroll_from = m->vs.scroll;
  if (scroll) m->vs.scroll -= AREA_SCROLL * AREA_MOVE * (float)d;
  m->vs.item_from = m->vs.item;
  m->vs.item = to;
  m->vs.move_t = 0;
}

void menu_vs_song(menu_state *m, int pick) {
  int song = (pick & 31) - 2, set = pick >> 5 & 3;
  if (set > 2) set = 0;
  m->vs.song_set = set + 1;
  m->song = song > 0 && song < BGM_SETS[set].lines - 1 ? song : -1;
  m->bgm = m->song > 0 ? m->song : bgm_lines(m) - 1;
}

/* this machine's song choice as the profile sends it: its sound source (the car's OST) and the line + 2 (1: no music) */
int menu_vs_pick(const menu_state *m) { return bgm_set(m) << 5 | (m->song > 0 ? m->song + 2 : 1); }

/* the arcade's omakase: an area from the timer's bits, Hakone (6) at half the odds */
int menu_vs_random_card(void) {
  int a = rand() & 7;
  if (a == 6 && (rand() & 1)) a = (rand() >> 2) & 7;
  for (int k = 1; k < MENU_VS_CARDS; ++k)
    if (CARD_AREA[k] == a) return k;
  return 1;
}

static void area_vote(menu_state *m) {
  snd_menu(SND_SELECT);
  if (m->vs.item == 0) {
    /* the arcade's omakase is a random area; here it leaves the choice to the others (the
     * host counts it so: random only when every vote is omakase) */
    m->vs.voted = 2;
    m->vs.vote = 0;
  } else {
    m->vs.voted = 1;
    m->vs.vote = m->vs.item;
  }
  m->vs.vote_t = 0;
}

/* the strip's card under a tap: -1 left of this player's, +1 right, 0 on it */
static int area_tap_dir(const menu_state *m) {
  float left = 88.0f * (float)m->vs.item - 10.0f + m->vs.scroll;
  if (m->tap_y > 290.0f) return 2; /* below the strip: nothing */
  return m->tap_x < left ? -1 : m->tap_x > left + 164.0f ? 1 : 0;
}

static void vs_area_step(menu_state *m, const menu_in *in) {
  /* the vote lost its lobby (the host gone, the connection lost): back to the versus screen */
  if (m->vs.state != MENU_VS_LOBBY) {
    go(m, MENU_VS);
    return;
  }
  if (m->vs.move_t < AREA_MOVE && m->vs.item_from >= 0 && ++m->vs.move_t >= AREA_MOVE) m->vs.item_from = -1;
  if (m->vs.area_final >= 0) {
    if (m->vs.final_t == 0 && m->vs.item != m->vs.area_final) {
      /* the strip jumps to the result: its place and scroll as the cursor's walk there */
      m->vs.item = m->vs.area_final;
      m->vs.item_from = -1;
      m->vs.pos = 0;
      m->vs.scroll = 0;
      for (int k = 0; k < m->vs.item; ++k) {
        if (m->vs.pos <= 2 || k > 5) m->vs.pos++;
        else m->vs.scroll -= 88.0f;
      }
    }
    m->vs.final_t++;
    m->vs.timer = 0; /* (decided: the bar's timer at its 0) */
    return;
  }
  /* the 20 s (debug_time_keep aside): the shown second's ti_count from 9 down */
  if (m->vs.timer > 0) {
    int was = (m->vs.timer + 59) / 60;
    m->vs.timer--;
    int now = (m->vs.timer + 59) / 60;
    if (now != was && now >= 1 && now <= 9) snd_menu(SND_TIME_COUNT);
    if (!m->vs.timer && !m->vs.voted) area_vote(m);
  }
  if (m->vs.voted) {
    /* omakase's walk to its area, a card at a time; there, the decided frames */
    if (m->vs.item_from >= 0) return;
    if (m->vs.item != m->vs.vote) area_move(m, m->vs.vote > m->vs.item ? 1 : -1);
    else m->vs.vote_t++;
    return;
  }
  if (m->vs.item_from >= 0) return;
  if (in->dx) area_move(m, in->dx);
  else if (in->ok) area_vote(m);
}

/* the strip's frame: at rest, moving, decided (own vote, the result) */
static int area_frame(const menu_state *m) {
  int k = m->vs.item;
  if (m->vs.area_final >= 0) return k * 100 + 60 + (m->vs.final_t < AREA_DECIDE ? m->vs.final_t : AREA_DECIDE - 1);
  if (m->vs.item_from >= 0) {
    int t = m->vs.move_t < AREA_MOVE ? m->vs.move_t : AREA_MOVE - 1;
    return m->vs.item_from < k ? m->vs.item_from * 100 + 90 + t : k * 100 + 94 - t;
  }
  if (m->vs.voted && m->vs.item == m->vs.vote) {
    /* the walk's last step done: decided, then the card held without its arrows */
    return m->vs.vote_t < AREA_VOTED ? k * 100 + 60 + m->vs.vote_t : k * 100 + 80;
  }
  return k * 100 + m->t % 59;
}

/* the strip's shift: the scroll, sliding over a move */
static float area_dx(const menu_state *m) {
  if (m->vs.item_from < 0 || m->vs.move_t >= AREA_MOVE) return m->vs.scroll;
  return m->vs.scroll_from + (m->vs.scroll - m->vs.scroll_from) * (float)m->vs.move_t / AREA_MOVE;
}

/* clMenuBarStd::setTime's countdown (menu_g_timer_x) where the bar has its third dot: the
 * second shown, then at the second's end its scale-out into the next */
static void vs_timer(int timer, int bars, float fade) {
  float r[4];
  if (ID.timer < 0 || !anm_sprite_rect(ID.top, bars, anm_sprite(T_TIME_DOT3), r)) return;
  int s = (timer + 59) / 60, p = s * 60 - timer; /* p: frames into the second shown */
  if (p > 59) p = 59;
  anm_swap sw[8];
  int n = 0;
  for (int set = 0; set < 2; ++set) {
    int v = set ? s - 1 : s;
    if (v < 0) v = 0;
    /* the templates: tens, ones, one digit, the last seconds' red */
    int tens = A_TIME_DIGIT + set, ones = A_TIME_DIGIT + 4 + set, one = A_TIME_DIGIT + 2 + set, last = A_TIME_LAST + set;
    sw[n++] = (anm_swap){ anm_sprite(tens), v >= 10 ? anm_sprite(A_TIME_DIGIT + v / 10) : -1 };
    sw[n++] = (anm_swap){ anm_sprite(ones), v >= 10 ? anm_sprite(A_TIME_DIGIT + v % 10) : -1 };
    sw[n++] = (anm_swap){ anm_sprite(one), -1 };
    sw[n++] = (anm_swap){ anm_sprite(last), v < 10 ? anm_sprite(A_TIME_LAST + v) : -1 };
  }
  /* (a record takes one swap: the templates are digits too) */
  anm_draw_ex(ID.timer, p, fade, r[0], r[1], sw, n);
}

/* dispOther: the others' marks */
static void area_marks(const menu_state *m, int clock, float dx, float fade) {
  int under[MENU_VS_CARDS] = { 0 }, edge[2] = { 0 };
  for (int i = 0; i < m->vs.n && i < MENU_VS_PLAYERS; ++i) {
    if (i == m->vs.me || m->vs.other_vote[i] < 0) continue;
    int card = m->vs.other[i], omakase = m->vs.other_vote[i] == 2;
    if (card < 0 || card >= MENU_VS_CARDS) continue;
    int set = card < m->vs.item ? 0 : card == m->vs.item ? 1 : 2;
    int row = under[card]++;
    if (row > 2) continue;
    float x = AREA_MARK_X0[set] + 88.0f * (float)card + dx, y = AREA_MARK_Y0[set] + 32.0f * (float)row;
    int f = clock % 59;
    if (x >= 0.0f && x <= 640.0f) {
      anm_swap sw = { anm_sprite(A_ICON), anm_sprite(A_ICON + i) };
      anm_draw_ex(omakase ? ID.vs_icon_omakase : ID.vs_ex_player, 1 + f, fade, x, y, &sw, 1);
      continue;
    }
    int side = set == 0 ? 0 : 1, c = edge[side]++;
    if (c > 2) continue;
    x = AREA_EDGE_X[side] + (omakase ? (side ? -18.0f : 19.0f) : 0.0f);
    y = AREA_EDGE_Y[c < 2 ? 2 - c : 0];
    anm_swap sw[2] = { { anm_sprite(A_ARROW_R + i), side ? -1 : anm_sprite(A_ARROW) },
                       { anm_sprite(A_ARROW_L + i), side ? anm_sprite(A_ARROW_L_IMG) : -1 } };
    anm_draw_ex(omakase ? ID.vs_icon_omakase2 : ID.vs_icon, AREA_ICON_FROM[i] + f, fade, x, y, sw, 2);
  }
}

static void draw_vs_area(const menu_state *m, int clock, int bars, float screen_w, float screen_h,
                         int (*mini3d)(void *, float)) {
  float fade = m->t < FADE ? (float)(m->t + 1) / FADE : 1.0f;
  draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  draw_bg_logo(chosen_car(m, m->car), clock % 240, fade);
  /* the cities' picture (Japan's map with the city's name) for Nagoya, Osaka, Fukuoka, Hakone, and the route's 3D miniature for every city, over it
   * (the arcade's mapChange shows only the picture for those four, the newer builds both: issue 12) */
  static const int CITY_MAP[4] = { JID(0x1832), JID(0x15b9), JID(0x19a4), JID(0x1831) };
  int k = m->vs.item, city = k >= 5 && k <= 8;
  if (city && m->vs.item_from < 0 && ID.course_map >= 0) {
    anm_swap map = { anm_sprite(JID(0x15b9)), anm_sprite(CITY_MAP[k - 5]) };
    anm_draw_ex(ID.course_map, 0, fade, 0, 0, &map, 1);
  }
  if (mini3d && k >= 1) mini3d((void *)m, fade);
  anm_swap sw[AREA_NSWAP];
  int n = 0;
  for (int i = 0; i < 8; ++i) sw[n++] = (anm_swap){ anm_sprite(AREA_PICTURE[i][0]), anm_sprite(AREA_PICTURE[i][1]) };
  for (int i = 0; i < AREA_NSWAP - 8; ++i) sw[n++] = (anm_swap){ anm_sprite(AREA_HIDE[i]), -1 };
  float dx = area_dx(m);
  anm_draw_ex(ID.vs_course, area_frame(m), fade, dx, 0, sw, n);
  area_marks(m, clock, dx, fade);
  anm_swap mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(A_TOPBAR) };
  draw_topbar_timed(bars, fade, &mes);
  vs_timer(m->vs.timer, bars, fade);
  draw_bottom(bars, fade);
  if (!m->vs.voted && m->vs.area_final < 0) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 0);
  draw_vs_omakase(m, screen_w, screen_h);
  if (m->vs.timer == 0 && m->vs.area_final < 0) draw_wait(0, m->t, fade); /* (the time up: waiting on the others, dimmed) */
}

/* ---- the cars flying in before a versus race (MENU_VS_MATCH): the arcade's
 * clRaceThread::JoMatchingInformationMove (with clSeqJoMatchingInformation's set-up). The
 * race's machines' cars (clMenuCar, environment 0) follow data/menu3d/demo/Battle_anm.igb's
 * car (battle.h: its 3D camera too, makeProjection(55, -1, 4/3, 5, 20000)), each placed by
 * the tables for its count (0x9412500..): scaled, turned about x and moved; the second set of
 * two (slot 1) mirrored across x, coming from the other side. Over menu_g_shogo_bg_1 behind
 * them; frame f from the screen's start:
 *   f 40     the cars show and the animation runs (a frame of it each frame, held at 200);
 *            the engine (SNDEnginePlay, accel on): rpm 3500 + 116.7 a frame to f 70, then
 *            brakes and slip till f 135, the rpm down 233.3 a frame to f 100
 *   f 80     menu_p_pre_vs_rival_<n>_shadow (frames 1..39, then 101..159 round)
 *   f 120    _car: each machine's name plate (clNamePlate2, at its dot sprite 1716 + PCB)
 *   f 160    _vs: VS (cutin2, SE 0x22)
 *   f 260    over (the fade out); the race */
#define MATCH_CARS 40
#define MATCH_SHADOW 80
#define MATCH_PLATES 120
#define MATCH_VS 160
#define MATCH_END 260
#define MATCH_FADE 10
/* by machine count (2..4) and machine: the set, scale, turn about x (degrees), x and y */
static const signed char MATCH_SLOT[3][4] = { { 0, 1 }, { 0, 1, 0 }, { 0, 0, 0, 0 } };
static const float MATCH_SCALE[3][4] = { { 1, 1 }, { 0.7f, 0.7f, 0.7f }, { 0.6f, 0.6f, 0.6f, 0.6f } };
static const float MATCH_ROT[3][4] = { { 17, -6 }, { 19, 6, -7 }, { 18, 7, -2, -11.5f } };
static const float MATCH_X[3][4] = { { -1.5f, 1.5f }, { -1.5f, 1.5f, -1.5f }, { -4.5f, -4.5f, -4.5f, -4.5f } };
static const float MATCH_Y[3][4] = { { 20, -9.5f }, { 21.5f, 4, -14 }, { 23.5f, 10, -2.5f, -15.5f } };
/* clNamePlate2's templates: the car's name plate, the power and the handling (its two
 * fonts' places), the class, the title and the name */
enum { T_PLATE_CAR = JID(7450), T_PLATE_POWER = JID(1391), T_PLATE_HANDLING = JID(1392), T_PLATE_CLASS = JID(1389), T_PLATE_TITLE = JID(1390),
       T_PLATE_NAME = JID(1439), T_PLATE_DOT = JID(1716) };

void menu_vs_match(menu_state *m) {
  go(m, MENU_VS_MATCH);
  m->vs.match_t = 0;
}

int menu_vs_match_done(const menu_state *m) {
  return m->screen == MENU_VS_MATCH && m->vs.match_t >= MATCH_END + MATCH_FADE;
}

void menu_vs_match_back(menu_state *m) {
  go(m, MENU_VS);
  m->vs.match_played = 1;
}

/* the animation's frame at screen frame f (the arcade's counter runs from the cars' showing) */
static int match_anim(int f) {
  int a = f - MATCH_CARS + 1;
  return a < 0 ? 0 : a >= BATTLE_FRAMES ? BATTLE_FRAMES - 1 : a;
}

/* row-vector matrices (Alchemy's, GL's floats): out = a b, a first */
static void match_mul(float *out, const float *a, const float *b) {
  float r[16];
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      r[4 * i + j] = a[4 * i] * b[j] + a[4 * i + 1] * b[4 + j] + a[4 * i + 2] * b[8 + j] + a[4 * i + 3] * b[12 + j];
  memcpy(out, r, sizeof r);
}

static int match_set(const menu_state *m) {
  int n = m->vs.match_n;
  return n < 2 ? 0 : n > 4 ? 2 : n - 2;
}

/* car i's matrix: the animation's body (mirrored for slot 1) * scale * turn about x * move */
static void match_car_matrix(const menu_state *m, int i, const float *body, float *out) {
  int k = match_set(m);
  float b[16];
  memcpy(b, body, sizeof b);
  if (MATCH_SLOT[k][i]) /* F b F, F = (-1, 1, 1) */
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        if ((r == 0) != (c == 0)) b[4 * r + c] = -b[4 * r + c];
  float sc = MATCH_SCALE[k][i], a = MATCH_ROT[k][i] * DEG, cs = cosf(a), sn = sinf(a);
  const float S[16] = { sc, 0, 0, 0, 0, sc, 0, 0, 0, 0, sc, 0, 0, 0, 0, 1 };
  const float R[16] = { 1, 0, 0, 0, 0, cs, sn, 0, 0, -sn, cs, 0, 0, 0, 0, 1 };
  const float T[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, MATCH_X[k][i], MATCH_Y[k][i], 0, 1 };
  match_mul(out, b, S);
  match_mul(out, out, R);
  match_mul(out, out, T);
}

static int match_scene3d(const menu_state *m, menu_scene *s) {
  const battle_frame *bf = &BATTLE[match_anim(m->vs.match_t)];
  s->env = 0;
  s->fov_x = 55.0f * DEG;
  memcpy(s->eye, bf->eye, sizeof s->eye);
  memcpy(s->at, bf->at, sizeof s->at);
  for (int i = 0; i < m->vs.match_n && i < MENU_VS_PLAYERS; ++i) {
    int mk = m->vs.match[i].maker, cr = m->vs.match[i].car, co = m->vs.match[i].color;
    if (mk < 0 || mk >= CARDB_ALL || cr < 0 || cr >= CARDB_NCARS[mk]) mk = m->maker, cr = m->car, co = m->color;
    const cardb_car *c = &CARDB_CARS[mk][cr];
    const cardb_color *col = &c->colors[co >= 0 && co < c->ncolors ? co : 0];
    menu_car *mc = &s->cars[s->n++];
    mc->pack = col->pack;
    mc->paint = col->paint;
    int cc = m->vs.match[i].ccolor;
    if (cc > 0 && cc <= c->ncustom) mc->paint = c->custom[cc - 1].paint; /* (the special colour its card wears) */
    mc->tuned = 1, mc->tune_set = 1, mc->tune_pw = m->vs.match[i].power, mc->tune_hd = m->vs.match[i].handling; /* (its tune's exhaust, roll cage and the like) */
    mc->aero = m->vs.match[i].aero, mc->mirror = m->vs.match[i].mirror, mc->wing = m->vs.match[i].wing;
    mc->bonnet = m->vs.match[i].bonnet, mc->wheel = m->vs.match[i].wheel, mc->neon = m->vs.match[i].neon;
    mc->vinyl = m->vs.match[i].vinyl;
    match_car_matrix(m, i, bf->body, mc->model);
    /* the corners' spin and steer (TIRE_ROT_k * TIRE_k), mirrored with the car */
    mc->turned = 1;
    for (int w = 0; w < 4; ++w) {
      float *t = mc->tyre[w];
      memset(t, 0, sizeof mc->tyre[w]);
      for (int col3 = 0; col3 < 3; ++col3)
        for (int r = 0; r < 3; ++r) {
          float v = bf->wheel[w][3 * col3 + r];
          if (MATCH_SLOT[match_set(m)][i] && (col3 == 0) != (r == 0)) v = -v;
          t[4 * col3 + r] = v;
        }
      t[15] = 1;
    }
  }
  return 1;
}

/* the overlays' frames: once 1..39 from their start, then 101..159 round */
static int match_overlay_frame(int since) {
  return since < 39 ? 1 + since : 101 + (since - 39) % 59;
}

static int match_r35(int maker, int car) {
  return maker >= 0 && maker < CARDB_ALL && car >= 0 && car < CARDB_NCARS[maker] && has(CARDB_CARS[maker][car].label, "[R35]");
}

/* clNamePlate2 of machine i at (x, y), sy its height's scale (the 3 machines' plates
 * unfold): the plate of an R35 (this one's or the player's) shows the tune's power and
 * handling, the other its horsepower; the car's name plate, the player's name */
static void match_plate(const menu_state *m, int i, float x, float y, float sy, float a) {
  int mk = m->vs.match[i].maker, cr = m->vs.match[i].car;
  if (mk < 0 || mk >= CARDB_ALL || cr < 0 || cr >= CARDB_NCARS[mk]) return;
  const cardb_car *c = &CARDB_CARS[mk][cr];
  int me = m->vs.match_me >= 0 && m->vs.match_me < m->vs.match_n ? m->vs.match_me : 0;
  int ph = match_r35(mk, cr) || match_r35(m->vs.match[me].maker, m->vs.match[me].car);
  int anim = ph ? ID.plate_ph : ID.plate;
  if (anim < 0) return;
  anm_swap sw[] = {
    { anm_sprite(T_PLATE_CAR), anm_sprite(c->plate) }, { anm_sprite(T_PLATE_POWER), -1 },
    { anm_sprite(T_PLATE_HANDLING), -1 }, { anm_sprite(T_PLATE_CLASS), -1 },
    { anm_sprite(T_PLATE_TITLE), -1 }, { anm_sprite(T_PLATE_NAME), -1 },
  };
  anm_draw_xy(anim, 0, a, x, y, 1, sy, sw, (int)(sizeof sw / sizeof sw[0]));
  if (sy < 0.95f) return; /* the texts once it is open */
  float r[4];
  char text[16];
  if (anm_sprite_rect(anim, 0, anm_sprite(T_PLATE_POWER), r)) {
    if (ph) snprintf(text, sizeof text, "%d", m->vs.match[i].power);
    else snprintf(text, sizeof text, "%d", car_tune_hp(c->hp, m->vs.match[i].power));
    card_glyphs(CARD_GLYPH2, "-,", 8.0f, text, x + r[0], y + r[1], a);
  }
  if (ph && anm_sprite_rect(anim, 0, anm_sprite(T_PLATE_HANDLING), r)) {
    snprintf(text, sizeof text, "%d", m->vs.match[i].handling);
    card_glyphs(CARD_GLYPH2, "-,", 8.0f, text, x + r[0], y + r[1], a);
  }
  /* the name centred on its template (the plate's middle: a clSpriteFont's), 28 px a glyph:
   * from the template's left edge a long one ran off the plate */
  if (anm_sprite_rect(anim, 0, anm_sprite(T_PLATE_NAME), r) && anm_sprite(NAME_GLYPH5) >= 0) {
    int n = 0;
    while (n < 5 && m->vs.match[i].glyph[n] >= 0) n++;
    float x0 = x + 0.5f * (r[0] + r[2]) - 14.0f * (float)n;
    for (int j = 0; j < n; ++j)
      anm_draw_sprite(anm_sprite(NAME_GLYPH5 + m->vs.match[i].glyph[j]), x0 + 28.0f * (float)j, y + r[1], a);
  }
}

/* the cars' engine as they fly in (SNDEnginePlay, accel on): the rpm 3500 + 116.7 a frame to f 70, then the brakes and slip till f 135 with the
 * rpm down 233.3 a frame (to the idle by f 100). Driven through the race's own engine voices (snd_update) with a car made for it. */
static void match_engine(const menu_state *m, int f) {
  if (f < MATCH_CARS || f > 135 + 30) {
    if (f == 135 + 31) snd_quiet();
    return;
  }
  static car_state cs;
  static car_input in;
  if (f == MATCH_CARS) {
    int me = m->vs.match_me >= 0 && m->vs.match_me < m->vs.match_n ? m->vs.match_me : 0;
    int mk = m->vs.match[me].maker, cr = m->vs.match[me].car, co = m->vs.match[me].color;
    if (mk < 0 || mk >= CARDB_ALL || cr < 0 || cr >= CARDB_NCARS[mk]) mk = m->maker, cr = m->car, co = m->color;
    const cardb_car *c = &CARDB_CARS[mk][cr];
    snd_set_car((unsigned)c->colors[co >= 0 && co < c->ncolors ? co : 0].type, m->vs.match[me].power);
  }
  memset(&cs, 0, sizeof cs);
  memset(&in, 0, sizeof in);
  float rpm = f <= 70 ? 3500.0f + 116.7f * (float)(f - MATCH_CARS) : 7000.0f - 233.3f * (float)(f - 70);
  cs.rpm = rpm < 1000.0f ? 1000.0f : rpm;
  in.throttle = f < 70 ? 1.0f : 0.0f;
  cs.slip = f >= 70 && f <= 135 ? 0.8f * (1.0f - (float)(f - 70) / 65.0f) : 0.0f;
  snd_update(&cs, &in, 1.0f / 60.0f);
}

static void match_step(menu_state *m) {
  int f = ++m->vs.match_t;
  if (f == MATCH_VS + 1) snd_menu(SND_CUTIN2);
  match_engine(m, f);
}

static void draw_vs_match(const menu_state *m, void (*draw3d)(void *), void *ctx) {
  int f = m->vs.match_t, k = match_set(m);
  float fade = f < MATCH_FADE ? (float)(f + 1) / MATCH_FADE
               : f > MATCH_END ? 1.0f - (float)(f - MATCH_END) / MATCH_FADE : 1.0f;
  if (fade < 0) fade = 0;
  anm_fill(0, 0, 0, 1);
  if (ID.shogo_bg >= 0) anm_draw(ID.shogo_bg, 0, 1);
  /* (the discs under the cars: drawn after they covered them) */
  if (f > MATCH_SHADOW && ID.pre_shadow[k] >= 0) anm_draw(ID.pre_shadow[k], match_overlay_frame(f - MATCH_SHADOW - 1), 1);
  if (draw3d && f >= MATCH_CARS) draw3d(ctx);
  if (f > MATCH_PLATES && ID.pre_cars[k] >= 0) {
    int pf = match_overlay_frame(f - MATCH_PLATES - 1);
    for (int i = 0; i < m->vs.match_n && i < MENU_VS_PLAYERS; ++i) {
      int dot = anm_sprite(T_PLATE_DOT + i);
      float r[4];
      if (!anm_sprite_rect(ID.pre_cars[k], pf, dot, r)) continue;
      float h = anm_sprite_height(dot), sy = h > 0 ? (r[3] - r[1]) / h : 1;
      match_plate(m, i, r[0], r[1], sy, anm_sprite_alpha(ID.pre_cars[k], pf, dot));
    }
  }
  if (f > MATCH_VS && ID.pre_vs[k] >= 0) anm_draw(ID.pre_vs[k], match_overlay_frame(f - MATCH_VS - 1), 1);
  anm_fill(0, 0, 0, 1.0f - fade);
}

/* The pointer (a tap of the touch screen or a click) as intents (menu_in): the screens that read only menu_in have their adapter
 * here, and menu_pointer returns 1 for them. A maker or a colour under it is chosen outright, the selected one decides; on the
 * car carousel its two sides step one car and the middle decides; the spin boxes (game mode, mode, course) step one place
 * towards what is tapped, and decide on the selected one; the course's TRAFFIC key is the view button. */
static int menu_pointer(const menu_state *m, menu_in *in) {
  float x = m->tap_x, y = m->tap_y;
  if (m->screen == MENU_MAKER) {
    int k = (int)floorf((x - MAKER_X0) / MAKER_STEP);
    float off = x - MAKER_X0 - MAKER_STEP * (float)k;
    if (k < 0 || k >= MAKER_SLOTS || off > MAKER_BW || y < MAKER_Y0 || y > MAKER_Y1) return 1; /* (a tap on nothing) */
    if (k == m->sel) in->ok = 1;
    else in->select = k;
    return 1;
  }
  if (m->screen == MENU_CAR) {
    if (x < 200.0f) in->dx = -1;
    else if (x > 440.0f) in->dx = 1;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_COLOR) {
    int k = color_at(m, x, y);
    if (k == m->sel) in->ok = 1;
    else if (k >= 0) in->select = k;
    return 1;
  }
    if (m->screen == MENU_GAME || m->screen == MENU_VS_SEL || m->screen == MENU_MODE || m->screen == MENU_COURSE ||
      m->screen == MENU_TEN_COURSE) {
    if (m->screen == MENU_COURSE && ch_hit(m, traffic_key())) {
      in->view = 1;
      return 1;
    }
    if (m->screen == MENU_GAME || m->screen == MENU_VS_SEL) { /* the discs: the one tapped is chosen (on the one already on, decided); between them, nothing */
      int h = disc_hit(m);
      if (h >= 0) in->select = h;
      return 1;
    }
    int d = tap_dir(m);
    if (d) in->dx = d;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_VS) { /* a button: chosen and pressed */
    int b = vs_hit(m, x * 1024.0f / 640.0f, y * 768.0f / 480.0f);
    if (b >= 0) in->select = b, in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_VS_AREA) { /* the strip's card: left of this player's, right of it, or on it */
    int d = area_tap_dir(m);
    if (d == 0) in->ok = 1;
    else if (d != 2) in->dx = d;
    return 1;
  }
  if (m->screen == MENU_VOLUME) { /* a row, and a level on its bar */
    int row = (int)floorf((y - (VOL_Y0 - 20.0f)) / VOL_DY);
    if (row < 0 || row >= VOL_ROWS) return 1;
    in->select = row;
    if (x >= VOL_BAR_X - 24.0f && x < VOL_BAR_X + VOL_CELL * 10.0f + 4.0f) {
      int v = x < VOL_BAR_X ? 0 : (int)floorf((x - VOL_BAR_X) / VOL_CELL) + 1;
      in->tab = v > 10 ? 10 : v;
    }
    return 1;
  }
  if (m->screen == MENU_HUDOPT) { /* a row, and a chip on it */
    orow r[4];
    optl_pointer(m, in, r, debug_rows((menu_state *)m, r), OPTL_DY_DEBUG);
    return 1;
  }
  if (m->screen == MENU_RUMBLE) {
    orow r[4];
    optl_pointer(m, in, r, rumble_rows((menu_state *)m, r), OPTL_DY_DEBUG);
    return 1;
  }
  if (m->screen == MENU_BINDINGS && bind_touch(m)) {
    touchctl_pointer(m, in);
    return 1;
  }
  if (m->screen == MENU_BINDINGS) { /* a list tab, a cell (chosen and waited on), the defaults row */
    const float k = 1.0f;
    for (int c = 0; c < BIND_TABS; ++c) {
      float tx = BIND_X_NAME + 96.0f * (float)c;
      if (x >= tx && x < tx + 90.0f && y >= BIND_Y_TAB && y < BIND_Y_TAB + 20.0f) {
        in->tab = c;
        return 1;
      }
    }
    int row = m->bind_ctx == BIND_FFB ? (int)floorf((y - FR_Y0) / FR_DY * k) : (int)floorf((y - BIND_Y_ROW) / BIND_ROW_DY * k), n = bind_rows(m->bind_ctx);
    if (row < 0 || row > n) return 1;
    if (row == n) {
      in->select = row * BIND_SLOTS, in->ok = 1;
      return 1;
    }
    if (m->bind_ctx == BIND_FFB) { /* the wheel list: the left third of the value cell steps down, the right third up, the middle decides */
      in->select = row * BIND_SLOTS;
      if (x >= AX_X[0] && x < AX_X[0] + FR_CELL_W / 3.0f) in->dx = -1;
      else if (x >= AX_X[0] + FR_CELL_W * 2.0f / 3.0f && x < AX_X[0] + FR_CELL_W) in->dx = 1;
      else if (x >= AX_X[0] && x < AX_X[0] + FR_CELL_W) in->ok = 1;
      return 1;
    }
    if (m->bind_ctx == BIND_CONTEXTS && row == BA_COUNT) {
      in->select = row * BIND_SLOTS, in->ok = 1;
      return 1;
    }
    if (bind_is_speed(m->bind_ctx, row)) { /* the buttons' steering speed: the left third of its cell steps it down, the right third up */
      float cx, cw;
      bindings_cell(0, &cx, &cw);
      in->select = row * BIND_SLOTS;
      if (x >= cx && x < cx + cw / 3.0f) in->dx = -1;
      else if (x >= cx + cw * 2.0f / 3.0f && x < cx + cw) in->dx = 1;
      return 1;
    }
    if (m->bind_ctx == BIND_CONTEXTS && row > BA_COUNT && row < BA_COUNT + 3) { /* a number of the filter: the left third of its cell steps it down, the right third up */
      in->select = row * BIND_SLOTS;
      if (x >= AX_X[0] && x < AX_X[0] + AX_W[0] / 3.0f) in->dx = -1;
      else if (x >= AX_X[0] + AX_W[0] * 2.0f / 3.0f && x < AX_X[0] + AX_W[0]) in->dx = 1;
      return 1;
    }
    if (m->bind_ctx == BIND_CONTEXTS) {
      for (int s = 0; s < 4; ++s)
        if (x >= AX_X[s] && x < AX_X[s] + AX_W[s]) in->select = row * BIND_SLOTS + s, in->ok = 1;
    } else {
      for (int s = 0; s < BIND_SLOTS; ++s) {
        float cx, cw;
        bindings_cell(s, &cx, &cw);
        if (x >= cx && x < cx + cw + 4.0f) in->select = row * BIND_SLOTS + s, in->ok = 1;
      }
    }
    if (m->bind_wait) in->aux = 1; /* (a tap while it waits: the wait ends, then the tap's cell) */
    return 1;
  }
  if (m->screen == MENU_OPTION) { /* a row and the chip tapped on it */
    orow r[12];
    { int on_ = option_rows((menu_state *)m, r); optl_pointer(m, in, r, on_, optl_dy(on_)); }
    return 1;
  }
  if (m->screen == MENU_TG_GET || m->screen == MENU_TG_AERO) { /* either side of the middle steps, the middle decides */
    if (x < 200.0f) in->dx = -1;
    else if (x > 440.0f) in->dx = 1;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_TG_PART) { /* a place: stepped towards, or decided (on nothing too) */
    int k = tg_part_hit(m);
    if (k >= 0) in->select = k;
    in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_TG_SELECT || m->screen == MENU_TG_MAKER) {
    int k = m->screen == MENU_TG_SELECT ? tg_sel_hit(m) : tg_maker_hit(m);
    if (k >= 0) in->select = k;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_TG_WHEEL) { /* a colour, a place, the bar's arrows, else the decision */
    int col = tg_wheel_colour_hit(m), k = tg_wheel_hit(m);
    if (col >= 0) in->tab = col;
    else if (k >= 0) in->select = k;
    else if (y >= 270.0f && y < 400.0f) in->dx = x < 320.0f ? -1 : 1;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_TG_VINYL) {
    int col = tg_vinyl_colour_hit(m), k = tg_vinyl_hit(m);
    if (col >= 0) in->tab = col;
    else if (k >= 0) in->select = k;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_SHOGO) { /* the title's YES / NO: the arrows and the other card turn, the card decides */
    if (shogo_on_arrow(m) || shogo_on_card(m, !m->sel)) in->dx = 1;
    else if (shogo_on_card(m, m->sel)) in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_TEN_LEVEL) { /* a tap on the chosen box (x 192..448 on the arcade's screen) decides, one aside moves */
    if (x < 192.0f) in->dx = -1;
    else if (x > 448.0f) in->dx = 1;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_ST_TUNE) { /* the Power row, the Handling row, anywhere else decides */
    if (y >= 312.0f && y < 352.0f) in->dx = -1;
    else if (y >= 352.0f && y < 395.0f) in->dx = 1;
    else if (y >= 40.0f) in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_ST_LEVEL || m->screen == MENU_ST_RIVAL) {
    int d = st_tap(m);
    if (d == ST_NONE) return 1;
    if (d) in->dx = d;
    else in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_BGM) { /* the time zone: the arrows and the other card turn, the BGM line steps, the card decides */
    if (on_card(m, T_ARROW) || on_card(m, T_ARROW2) || on_card(m, m->zone ? T_MORNING : T_NIGHT)) in->dx = 1;
    else if (x >= 170.0f && x < 560.0f && y >= 326.0f && y < 392.0f) in->view = 1;
    else if (on_card(m, 0)) in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_HAVE && !m->keep_ask) {
    int k = grid_hit(m, x, y);
    if (k <= -2) in->tab = -2 - k;
    else if (k >= 0) in->select = k, in->ok = k == m->grid_sel;
    return 1;
  }
  if (m->screen == MENU_HAVE) {
    if (have_on(m, T_ARROW) || have_on(m, T_ARROW2) || have_on(m, m->have_sel ? T_MORNING : T_NIGHT)) in->dx = 1;
    else if (have_on(m, 0)) in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_GARAGE) { /* either side of the car goes round the cars, the car takes it */
    if (x < SET_CAR_X0) in->dx = -1;
    else if (x >= SET_CAR_X1) in->dx = 1;
    else if (y >= SET_CAR_Y0 - 40.0f && y < SET_CAR_Y1 + 60.0f) in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_CARD) {
    /* the arrows at either side turn the card's pages (what the view change and the steering do); a tap elsewhere confirms */
    if (card_arrows_on(m) && y >= CARD_ARROW_Y0 && y < CARD_ARROW_Y1) {
      if (x < CARD_ARROW_W) return in->dx = -1, 1;
      if (x >= 640.0f - CARD_ARROW_W) return in->dx = 1, 1;
    }
    in->ok = 1;
    return 1;
  }
  if (m->screen == MENU_CARDSET) {
    if (bar_keys() && x >= CS_DONE[0] && x < CS_DONE[0] + CS_DONE[2] && y >= CS_DONE[1] - 6 && y < CS_DONE[1] + CS_DONE[3] + 6) {
      in->ok = 1;
      return 1;
    }
    for (int row = 0; row < 2; ++row) {
      float cy = row ? CS_ROW1 : CS_ROW0;
      if (y < cy - 40.0f || y >= cy + 40.0f) continue;
      for (int k = 0; k < 3; ++k) {
        float cx = CS_X0 + CS_DX * (float)k;
        if (x >= cx - CS_DX * 0.5f && x < cx + CS_DX * 0.5f) in->select = row * 3 + k;
      }
    }
    return 1;
  }
  if (m->screen == MENU_SETTING) {
    int car = x >= SET_CAR_X0 && x < SET_CAR_X1 && y >= SET_CAR_Y0 && y < SET_CAR_Y1;
    float xc[2];
    setting_chips(xc);
    int row = y >= SET_ROW_Y - 16.0f && y < SET_ROW_Y + 46.0f;
    if (m->tune_confirm && (tune_confirm_done_hit(m) || car)) {
      in->ok = 1;
    } else if (row) {
      for (int k = 0; k < 2; ++k)
        if (x >= xc[k] - 14.0f && x < xc[k] + anm_sprite_width(anm_sprite(SET_TEXT + 2 + k)) + (k ? 14.0f : SET_CHIP_GAP * 0.5f)) in->tab = 2 + k;
    } else if (car) {
      in->ok = 1;
    }
    return 1;
  }
  if (m->screen == MENU_SAVE) {
    const float *ik = INFO_KEY_RECT;
    if (anm_sprite(INFO_KEY) >= 0 && x >= ik[0] - 8 && x < ik[0] + ik[2] + 8 && y >= ik[1] - 8 && y < ik[1] + ik[3] + 8) {
      in->aux = 1;
    } else if (y >= SET_ROW_Y - 16.0f && y < SET_ROW_Y + 46.0f) {
      float xs[3];
      save_chips(xs);
      for (int k = 2; k >= 0; --k) {
        if (x < xs[k] - SET_CHIP_GAP * 0.5f) continue;
        if (x <= xs[k] + anm_sprite_width(save_chip_text(k)) + SET_CHIP_GAP * 0.5f) in->tab = k;
        break;
      }
    } else {
      int k = (int)floorf((y - SAVE_Y0) / SAVE_DY);
      if (k >= 0 && k < MENU_SAVES && y - SAVE_Y0 - SAVE_DY * (float)k < SAVE_H) in->select = k, in->ok = 1;
    }
    return 1;
  }
  if (m->screen == MENU_NAME) { /* a tab, or a character of the grid: chosen and pressed */
    for (int k = 0; k < 5; ++k) {
      if (fabsf(x - (NAME_TAB_X + NAME_TAB_DX * (float)k)) >= 32.0f || fabsf(y - NAME_TAB_Y) >= 18.0f) continue;
      in->tab = k, in->ok = 1;
      return 1;
    }
    int row = (int)floorf((y - NAME_ROW_Y(0) + 4.0f) / 32.0f), col = 0;
    if (row < 0 || row > 2) return 1;
    for (int c = 1; c < 20; ++c)
      if (fabsf(x - 12.0f - (float)NAMEDB_GRID_X[c]) < fabsf(x - 12.0f - (float)NAMEDB_GRID_X[col])) col = c;
    if (fabsf(x - 12.0f - (float)NAMEDB_GRID_X[col]) >= 16.0f) return 1;
    if (NAMEDB_GRID[m->name_page][row * 20 + col] != -1) in->select = row * 20 + col, in->ok = 1;
    return 1;
  }
  return 0;
}

static void step(menu_state *m) {
  m->t++;
  m->clock++;
#ifdef WMMT_EGL
  static int tg_now;
  if (m->screen == MENU_GAME && !tg_now && getenv("WMMT_TG_NOW")) { /* test: these dress-up wins now */
    tg_now = 1;
    menu_dress_win(m, atoi(getenv("WMMT_TG_NOW")));
    tg_due(m);
  }
  static int title_now;
  if (m->screen == MENU_GAME && !title_now && getenv("WMMT_TITLE_NOW")) { /* test: these new titles now */
    title_now = 1;
    int t[3] = { 0, 0, 0 };
    sscanf(getenv("WMMT_TITLE_NOW"), "%d %d %d", &t[0], &t[1], &t[2]);
    menu_title_due(m, t);
    shogo_open(m);
    return;
  }
#endif
  if (m->screen == MENU_PRE) {
    if (m->loading) m->load_shown = m->loading, m->load_out = 0;
    else if (m->load_shown && m->load_out < LOADING_OUT) m->load_out++;
  }
  int tap = m->tap, key = m->key, back = m->back, option = m->option;
  int hidden = m->hidden;
  m->tap = m->key = m->back = m->option = m->hidden = 0;
  int ok = m->act_ok, up = m->act_up, down = m->act_down, view = m->act_view, left = m->act_left, right = m->act_right;
  m->act_ok = m->act_up = m->act_down = m->act_view = m->act_left = m->act_right = 0; /* (this step's controls) */
  /* the input as the migrated screens read it: the controls, and the pointer turned into the same (menu_pointer) */
  menu_in in = { .dx = key, .dy = down - up, .select = -1, .tab = -1, .ok = ok, .back = back, .view = view,
                 .any = tap || key };
  if (m->screen == MENU_NAME) { /* the steering moves the cursor; the bottom keys (touch) take the neighbouring page */
    in.dx = right - left;
    if (key) in.tab = (m->name_page + key + 4) % 4, in.ok = 1;
  }
  if (tap && !ok) menu_pointer(m, &in);
  if (m->flash) m->flash--;
  if (m->bars && ++m->bars > 11) m->bars = 0;
  if (in_attract(m) && (tap || key) && menu_booting(m) && m->t >= 10) { /* the boot (the maker logos, the prologue): a press or a click skips it, to the title */
    m->demo = 0;
    go(m, MENU_TITLE);
    m->tap = m->key = 0;
    return;
  }
  if (in_attract(m)) {
    if ((tap || key) && !menu_booting(m)) {
      menu_start(m);
      return;
    }
    if (m->screen == MENU_TITLE && m->t >= TITLE_LEN + FADE) go(m, MENU_RANKING);
    else if (m->screen == MENU_RANKING && m->t >= RANKING_LEN + FADE) go(m, MENU_LOGO);
#if WMMT_LANG_EN
    /* the prologue has two parts: the English one stays, the second (the 20XX text, in Japanese) is skipped */
    else if (m->screen == MENU_LOGO && m->t >= 3 * LOGO_LEN) go(m, MENU_PROLOGUE);
    else if (m->screen == MENU_PROLOGUE && m->t >= PROLOGUE_LEN) {
      m->demo++;
      go(m, MENU_MOVIE);
    }
#else
    else if (m->screen == MENU_LOGO && m->t >= 3 * LOGO_LEN) go(m, MENU_PROLOGUE);
    else if (m->screen == MENU_PROLOGUE && m->t >= 2 * PROLOGUE_LEN) {
      m->demo++;
      go(m, MENU_MOVIE);
    }
#endif
    return;
  }
  if (m->screen == MENU_VS && m->vs.area_go) {
    vs_area_enter(m);
    return;
  }
  if (m->screen == MENU_VS) {
    vs_step(m, &in);
    return;
  }
  if (m->screen == MENU_VS_AREA) {
    vs_area_step(m, &in);
    return;
  }
  if (m->screen == MENU_VS_MATCH) {
    match_step(m);
    return;
  }
  if (m->screen == MENU_START) {
    if (m->t >= START_LEN) {
      if (m->save_skip) car_select_enter(m, MENU_MAKER);
      else save_enter(m);
      m->flash = FLASH_LEN;
      m->bars = 1;
    }
    return;
  }
  if (m->deciding) {
    m->deciding--;
    return;
  }
  if (m->screen == MENU_INFO) {
    if (in.back || in.any) {
      snd_menu(SND_CANCEL);
      go(m, MENU_SAVE);
    }
    return;
  }
  if (m->screen == MENU_SAVE && m->save_auto) {
    if (m->save_pick) return; /* (the app loads the card first) */
    m->save_auto = 0;
    if (back) {
      m->leaving = 1;
      m->deciding = FADE;
    } else {
      main_menu_enter(m); /* (PLAY goes on to the car select) */
    }
    return;
  }
  if (m->screen == MENU_SAVE) {
    if (back) {
      snd_menu(SND_CANCEL);
      m->leaving = 1;
      m->deciding = FADE;
      return;
    }
    if ((option || in.view) && !m->save_confirm) { /* the gear key, or the view button */
      snd_menu(SND_SELECT);
      m->opt_from = MENU_SAVE;
      option_enter(m);
      return;
    }
    if (save_step(m, &in)) m->deciding = FADE;
    return;
  }
  if (m->screen == MENU_NAME) {
    if (m->name_pop) m->name_pop--;
    if (m->name_char_pop) m->name_char_pop--;
    if (back) {
      if (name_len(m)) name_erase(m);
      else if (m->name_car == 2) { /* (the new car's name, asked right after its colour: back to the colours) */
        snd_menu(SND_CANCEL);
        m->name_car = 0;
        if (m->own_sel >= 0 && m->own_sel < m->n_own) memcpy(m->name, m->own[m->own_sel].name, sizeof m->name); /* (the car driven keeps its name) */
        car_select_enter(m, MENU_COLOR);
      } else if (m->name_car) snd_menu(SND_CANCEL); /* (a car's name is asked for before it is kept) */
      else {
        snd_menu(SND_CANCEL);
        save_enter(m);
      }
      return;
    }
    if (in.dx || in.dy || in.view) {
      name_pad(m, &in);
      return;
    }
    if (in.tab >= 0) { /* a tab chosen outright (the pointer) */
      m->focus = in.tab;
      m->name_y = 3;
    } else if (in.select >= 0) { /* a character of the grid */
      m->name_y = in.select / 20;
      m->name_x = in.select % 20;
    } else if (in.ok) {
      m->name_ctl = 1; /* (the controls' OK: the cursor shows) */
    }
    if (in.ok && name_press(m)) m->deciding = FADE;
    return;
  }
  if (m->screen == MENU_HAVE) {
    if (back && !m->keep_ask && m->erase_armed) { /* (the bin armed: back takes that back first) */
      snd_menu(SND_CANCEL);
      m->erase_armed = 0;
      return;
    }
    if (back && !m->slide) {
      snd_menu(SND_CANCEL);
      if (m->keep_ask) {
        card_enter(m, MENU_SETTING);
      } else {
        main_menu_enter(m);
      }
      return;
    }
    if (!m->keep_ask && option && !m->deciding) { /* the gear: the settings, back to the car select */
      snd_menu(SND_SELECT);
      m->opt_from = MENU_HAVE;
      m->opt_sel = 0;
      option_enter(m);
      return;
    }
    if (m->keep_ask) {
      if (have_step(m, &in)) m->deciding = DECIDE_DAY + FADE;
    } else if (grid_step(m, &in)) {
      m->deciding = FADE;
    }
    return;
  }
  if (m->screen == MENU_GARAGE) {
    if (back) {
      snd_menu(SND_CANCEL);
      have_enter(m);
      return;
    }
    if (garage_step(m, &in)) m->deciding = FADE;
    return;
  }
  if (m->screen == MENU_CARD) {
    m->spin -= 0.3f;
    if (back) {
      snd_menu(SND_CANCEL);
      if (m->card_back == MENU_SETTING) {
        /* a new car's card: not kept yet, its setting again (its choice there kept) */
        int mk = m->maker, c = m->car, sl = menu_slot(m);
        signed char full = m->tune_full[mk][sl], power = m->tune_power[mk][sl];
        keep_drop(m);
        m->tune_full[mk][sl] = full, m->tune_power[mk][sl] = power;
        m->new_card = 1;
        m->tune_locked = 0;
      }
      if (m->card_back == MENU_COLOR && m->card_color_was >= 0) {
        /* an owned car's new colour only kept by the card's tap: back, its own again */
        int k = menu_own_find(m, m->maker, m->car, m->dup);
        if (k >= 0) {
          m->own[k].color = (signed char)m->card_color_was;
          m->color = m->card_color_was;
          m->garage_changed = 1;
        }
        m->card_color_was = -1;
      }
      if (m->card_back == MENU_HAVE) have_enter(m);
      else car_select_enter(m, m->card_back == MENU_SETTING ? MENU_SETTING : MENU_COLOR);
      return;
    }
    /* the card's pages (dress-up, special colours and METER / BGM are on the Car / Settings discs now) */
    if (m->card_back != MENU_SETTING && !m->deciding) {
      int pages = cardpages_ok();
      if (pages && m->card_page == CP_TA && in.aux) { /* (the time attack's challenge button: the list's scrolling stopped or going on) */
        snd_menu(SND_CURSOR);
        m->card_row ^= 1;
        return;
      }
      if (pages && (in.view || in.dx)) { /* (the view change and the steering: the next page, or the one before; the ghost battles' two are hidden) */
        int step = in.dx < 0 ? CP_N - 1 : 1, p = m->card_page;
        do p = (p + step) % CP_N; while (p == CP_GHOST || p == CP_GHOST_REC);
        snd_menu(SND_CURSOR);
        m->card_page = p;
        m->card_row = 0;
        return;
      }
      if (in.aux) return; /* (the METER / BGM key it was is gone) */
    }
    if (in.any || in.ok) {
      snd_menu(SND_SELECT);
      m->deciding = FADE;
    }
    return;
  }
  if (m->screen == MENU_LAYOUT) {
    layout_step(m, &in);
    return;
  }
  if (m->screen == MENU_OPTION) {
    if (back) {
      snd_menu(SND_CANCEL);
      if (m->opt_from == MENU_VS_SEL) {
        selmenu_enter(m, SELMENU_SETTINGS, 0);
      } else if (m->opt_from == MENU_HAVE) {
        have_enter(m);
      } else if (m->opt_from == MENU_SAVE) {
        save_enter(m);
      } else {
        car_select_enter(m, MENU_MAKER);
        m->sel = m->opt_sel;
      }
      return;
    }
    option_step(m, &in);
    return;
  }
  if ((option || in.view) && m->screen == MENU_MAKER && m->from < 0) { /* the gear key, or the view button */
    snd_menu(SND_SELECT);
    m->opt_sel = m->sel;
    m->opt_from = MENU_MAKER;
    option_enter(m);
    return;
  }
  /* "Special Vehicles": the hidden cars' select, as a maker's (the port's own) */
  if (hidden && m->screen == MENU_MAKER && m->from < 0) {
    snd_menu(SND_SELECT);
    m->hidden_go = 1;
    m->deciding = FADE;
    m->t = 0;
    return;
  }
  if (back && m->screen == MENU_MAKER && m->from < 0) {
    snd_menu(SND_CANCEL);
    /* back to the save's cars, or the saves (a test without them: the title screen) */
    if (m->save_skip) {
      m->leaving = 1;
      m->deciding = FADE;
    } else if (m->n_own) {
      have_enter(m);
    } else {
      main_menu_enter(m);
    }
    return;
  }
  /* the car setting goes back to the save's cars: the car is one of them by now */
  if (back && m->screen == MENU_SETTING && m->from < 0 && !m->save_skip && !m->new_card && !m->tune_confirm && m->own_sel >= 0) {
    snd_menu(SND_CANCEL);
    have_enter(m);
    return;
  }
  if (back && (m->screen == MENU_ST_LEVEL || m->screen == MENU_ST_RIVAL) && m->from < 0) {
    snd_menu(SND_CANCEL);
    if (m->screen == MENU_ST_RIVAL) {
      go(m, MENU_ST_LEVEL);
      m->sel = m->st_level;
      m->card_t = FADE;
    } else {
      car_select_enter(m, MENU_GAME);
    }
    return;
  }
  if (back && m->screen == MENU_VS_SEL && m->from < 0) {
    snd_menu(SND_CANCEL);
    if (m->sel_menu == SELMENU_MAIN) {
      m->leaving = 1; /* (the main menu is the first screen: back is the title) */
      m->deciding = FADE;
    } else if (m->sel_menu == SELMENU_SETTINGS) {
      selmenu_enter(m, SELMENU_MAIN, 1);
    } else if (m->sel_menu == SELMENU_TRANS && m->trans_edit) {
      m->trans_edit = 0;
      selmenu_enter(m, SELMENU_CAR, 0);
    } else if (m->sel_menu == SELMENU_TRANS) {
      selmenu_enter(m, SELMENU_TUNE, m->trans_full);
    } else if (m->sel_menu == SELMENU_TUNE) {
      car_select_enter(m, MENU_COLOR);
    } else if (m->sel_menu == SELMENU_MODE) {
      if (m->save_skip || m->card_back == MENU_SETTING || m->own_sel < 0) car_select_enter(m, MENU_SETTING);
      else card_enter(m, m->card_back); /* (a car of the save: back to its card) */
    } else if (m->sel_menu == SELMENU_CAR) {
      mode_menu_enter(m, 2);
    } else if (m->sel_menu == SELMENU_DEL) {
      selmenu_enter(m, SELMENU_CAR, 3);
    } else if (m->sel_menu == SELMENU_CUSTOM) {
      selmenu_enter(m, SELMENU_CAR, 1);
    } else {
      mode_menu_enter(m, 1); /* (the versus kinds: back to the mode select) */
    }
    return;
  }
  if (back && (m->screen == MENU_TEN_COURSE || m->screen == MENU_TEN_LEVEL) && m->from < 0) {
    snd_menu(SND_CANCEL); /* SE 0x1b */
    if (m->screen == MENU_TEN_LEVEL) ten_course_enter(m);
    else car_select_enter(m, MENU_GAME);
    return;
  }
  if (back && m->screen == MENU_SETTING && m->vs.tuning) return; /* (no way out of a race that is starting) */
  if (back && m->screen == MENU_SETTING && m->tune_confirm) {
    /* the pre-race check's back: the screen it came from */
    int from = m->tune_confirm;
    m->tune_confirm = 0;
    snd_menu(SND_CANCEL);
    if (from == MENU_ST_RIVAL) st_rival_enter(m);
    else if (from == MENU_TEN_LEVEL) ten_level_enter(m);
    else if (from == MENU_VS_SEL) go(m, MENU_VS_SEL), m->sel = m->vs.kind;
    else go(m, MENU_BGM), m->sel = m->bgm, m->slide = m->card_t = 0, m->bars = 1;
    return;
  }
  if (back && m->screen == MENU_GAME && m->from < 0 && !m->slide) {
    snd_menu(SND_CANCEL); /* (the solo games: back to the mode select's discs) */
    mode_menu_enter(m, 0);
    return;
  }
  if (back && m->screen == MENU_COLOR && m->cc_mode && m->from < 0 && !m->deciding) {
    snd_menu(SND_CANCEL); /* (the special colours: back to the card, or the Customize discs they were opened from) */
    m->cc_mode = 0;
    if (m->carmenu == 2) m->carmenu = 0, selmenu_enter(m, SELMENU_CUSTOM, 1);
    else card_enter(m, m->cc_back);
    return;
  }
  if (back && m->screen == MENU_BGM && m->game == MENU_GAME_TEN && m->from < 0 && !m->slide && !m->vs.day) {
    snd_menu(SND_CANCEL); /* (the 10 out run's time zone and music: back to its level select) */
    ten_level_enter(m);
    m->sel = m->ten_face_lv = m->ten_level < m->ten_levels ? m->ten_level : m->sel;
    return;
  }
  if (back && m->screen > MENU_MAKER && m->screen <= MENU_BGM && m->from < 0 && m->rot_speed == 0.0f && !m->slide && !m->vs.day) {
    snd_menu(SND_CANCEL);
    car_select_enter(m, m->screen == MENU_COURSE ? MENU_GAME : m->screen - 1); /* (the course select is the time attack's first) */
    return;
  }
  if (m->screen <= MENU_COLOR) {
    if (car_select_step(m, &in)) {
      m->deciding = FADE;
      m->t = 0;
    }
    return;
  }
  /* (the card check's wheel key, the dress-up menu: its back key, and on a change screen it opened (the cabinet's cancel there), back to
   * it, from the wheel's bar to the makers': above, in the card's step) */
  if (m->screen == MENU_CARDSET) {
    cardset_step(m, &in);
    return;
  }
  if (m->screen == MENU_BINDINGS) {
    bindings_step(m, &in);
    return;
  }
  if (m->screen == MENU_VOLUME) {
    volume_step(m, &in);
    return;
  }
  if (m->screen == MENU_HUDOPT) {
    hudopt_step(m, &in);
    return;
  }
  if (m->screen == MENU_PORTINFO) {
    portinfo_step(m, &in);
    return;
  }
  if (m->screen == MENU_RUMBLE) {
    rumble_step(m, &in);
    return;
  }
  if (m->screen == MENU_SETTING) {
    if (m->vs.tuning && m->vs.tune_timer > 0) m->vs.tune_timer--;
    if (setting_step(m, &in)) m->deciding = FADE;
    return;
  }
  if (m->screen == MENU_BGM && m->vs.day) {
    vs_day_step(m, &in);
    return;
  }
  if (m->screen == MENU_BGM) {
    if (day_select_step(m, &in)) m->deciding = DECIDE_DAY + FADE;
    return;
  }
  if (m->screen == MENU_PRE) {
    if (m->pre_quiet && m->ready && !m->deciding && (m->ending != 2 || m->t >= 60)) m->deciding = 1;
    if (m->t >= PRE_LEN && m->ready && !m->deciding) m->deciding = FADE;
    return;
  }
  if (m->screen == MENU_ST_LEVEL || m->screen == MENU_ST_RIVAL) {
    st_step(m, &in);
    return;
  }
  if (m->screen == MENU_ST_TUNE) {
    tune_step(m, &in);
    return;
  }
  if (m->screen == MENU_LVUP) {
    lvup_step(m, &in);
    return;
  }
  if (m->screen == MENU_VS_STAR) {
    star_step(m, &in);
    return;
  }
  if (m->screen == MENU_SHOGO) {
    shogo_step(m, &in);
    return;
  }
  if (m->screen == MENU_POEM) {
    poem_step(m);
    return;
  }
  if (m->screen == MENU_MILAGE) {
    milage_step(m, &in);
    return;
  }
  if (m->screen == MENU_BONUS) {
    bonus_step(m, &in);
    return;
  }
  if (m->screen == MENU_ST_EFF) {
    eff_step(m, &in);
    return;
  }
  if (back && m->dress_free && !m->tg.move && m->card_t >= TG_FADE &&
      (m->screen == MENU_TG_SELECT || m->screen == MENU_TG_MAKER || m->screen == MENU_TG_AERO ||
       m->screen == MENU_TG_PART || m->screen == MENU_TG_WHEEL || m->screen == MENU_TG_VINYL)) {
    snd_menu(SND_CANCEL);
    if (m->screen == MENU_TG_SELECT) {
      tg_free_leave(m);
    } else if (m->screen == MENU_TG_WHEEL && !m->tg.gem) {
      tg_maker_open(m);
    } else {
      tg_select_open(m);
    }
    return;
  }
  if (m->screen == MENU_TG_SELECT) {
    tg_select_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_MAKER) {
    tg_maker_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_GET || m->screen == MENU_TG_AERO) {
    tg_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_PART) {
    tg_part_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_WHEEL) {
    tg_wheel_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_VINYL) {
    tg_vinyl_step(m, &in);
    return;
  }
  if (m->screen == MENU_TG_WEAR) {
    tg_wear_step(m, &in);
    return;
  }
  if (m->screen == MENU_TEN_LEVEL) {
    ten_level_step(m, &in);
    return;
  }
  if (m->screen == MENU_COURSE && in.view && m->from < 0) { /* the view button (or the TRAFFIC key): traffic on or off */
    snd_menu(SND_CURSOR);
    m->ta_traffic = !m->ta_traffic;
    m->garage_changed = 1;
    return;
  }
  if (m->screen == MENU_TEN_COURSE) m->card_t++;
  if (m->screen == MENU_VS_SEL) m->sel_age++;
  if (m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_TRANS) m->spin -= 0.3f; /* (the car on its turntable, as the colour select's) */
  int ten = m->screen == MENU_TEN_COURSE;
  int spin = m->screen == MENU_MODE || m->screen == MENU_GAME || m->screen == MENU_VS_SEL;
  int move_len = spin ? MOVE_MODE : MOVE_COURSE;
  int count = m->screen == MENU_GAME     ? GAME_SLOTS
              : m->screen == MENU_VS_SEL ? (SEL_TWO(m) ? 2 : m->sel_menu == SELMENU_CAR ? 4 : m->sel_menu == SELMENU_VS ? MENU_VS_KINDS : 3)
              : m->screen == MENU_MODE   ? 2
              : ten                      ? TEN_COURSES
                                         : N_COURSES;
  if (m->from >= 0) {
    if (m->t >= move_len) { m->from = -1; m->t = 0; }
    return;
  }
  if (in.select >= 0 && in.select < count && (m->screen == MENU_GAME || m->screen == MENU_VS_SEL)) { /* a disc tapped: on it, decided; another, moved to */
    if (in.select == m->sel) {
      in.ok = 1;
    } else {
      snd_menu(SND_CURSOR);
      m->from = m->sel;
      m->sel = in.select;
      m->t = 0;
      return;
    }
  }
  if (!in.dx && !in.ok) return;
  int d = in.dx;
  if (in.ok) {
    snd_menu(SND_SELECT);
    m->deciding = (m->screen == MENU_COURSE || ten ? DECIDE_COURSE : DECIDE_MODE) + FADE;
    m->t = 0;
  } else {
    int to = m->sel + (d > 0 ? 1 : -1);
    if (to < 0 || to >= count) return;
    snd_menu(SND_CURSOR);
    if (m->screen == MENU_COURSE || ten) {
      /* the spin box's place on screen: going right it stays at 3 from the fourth card on
       * till the ninth (the 10-in-a-row's: the seventh), going left at 2 from the third card on */
      m->course_scroll_from = m->sel - m->course_pos;
      if (d > 0 && !(m->course_pos > 2 && m->sel <= (ten ? TEN_COURSES - 4 : 8))) m->course_pos++;
      if (d < 0 && (m->course_pos > 2 || m->sel <= 2)) m->course_pos--;
    }
    m->from = m->sel;
    m->sel = to;
    m->t = 0;
  }
}

int menu_update(menu_state *m, float dt) {
  m->acc += dt;
  if (m->acc > 0.25f) m->acc = 0.25f;
  while (m->acc >= 1.0f / 60.0f) {
    m->acc -= 1.0f / 60.0f;
    int was_deciding = m->deciding;
    step(m);
    if (was_deciding == 1 && m->deciding == 0) {
      if (m->leaving) {
        /* the session ends: its mileage tally first when a card drove a km this time, then out of the white, the attract loop again from its title screen */
        m->leaving = 0;
        go(m, MENU_TITLE);
        m->flash = FLASH_LEN;
      } else if (m->screen == MENU_SAVE && m->save_pick_new) {
        /* a NEW GAME asks the player's name first */
        name_enter(m);
      } else if (m->screen == MENU_NAME && m->name_car == 2) {
        /* the new car named: STOCK or FULL TUNE next */
        m->name_car = 0;
        /* (the name is the new car's, not the car driven so far: kept apart until it is registered, the driven car's own back in m->name, as any store before it would put the new name on its card) */
        memcpy(m->new_name, m->name, sizeof m->new_name);
        m->new_named = 1;
        if (m->own_sel >= 0 && m->own_sel < m->n_own) memcpy(m->name, m->own[m->own_sel].name, sizeof m->name);
        else memset(m->name, -1, sizeof m->name);
        selmenu_enter(m, SELMENU_TUNE, 0);
      } else if (m->screen == MENU_NAME && m->name_car) {
        /* the new car named: kept on its card, whose KEEP question is next */
        m->name_car = 0;
        menu_own_store(m);
        m->garage_changed = 1;
        m->card_color_was = -1;
        card_enter(m, MENU_HAVE);
      } else if (m->screen == MENU_SAVE || m->screen == MENU_NAME) {
        /* a save with cars asks which; without, a new car */
        if (m->n_own) {
          have_enter(m);
        } else {
          car_select_enter(m, MENU_MAKER);
          m->bars = 1;
        }
      } else if (m->screen == MENU_HAVE && m->keep_ask) {
        /* the new car kept (its card already saved), or taken out and the car select again */
        m->keep_ask = 0;
        if (m->have_sel == 0) {
          m->card_back = MENU_HAVE; /* (the car is in the save now: back from the mode select shows its card, then the car select) */
          mode_menu_enter(m, 0);
        } else {
          keep_drop(m);
          car_select_enter(m, MENU_COLOR);
          m->bars = 1;
        }
      } else if (m->screen == MENU_HAVE) {
        if (m->have_sel == 0) {
          int k = m->grid_sel - 1;
          menu_own_store(m);
          menu_own_load(m, k);
          m->own_view = k;
          m->tune_locked = 1;
          m->new_card = 0;
          card_enter(m, MENU_HAVE);
        } else {
          car_select_enter(m, MENU_MAKER);
          m->bars = 1;
        }
      } else if (m->screen == MENU_GARAGE) {
        menu_own_store(m);
        menu_own_load(m, m->own_view);
        m->tune_locked = 1;
        m->new_card = 0;
        card_enter(m, MENU_GARAGE);
      } else if (m->screen == MENU_CARD) {
        m->card_color_was = -1;
        /* (a new card's setting came first: whether to keep it, then on) */
        if (m->card_back == MENU_SETTING) keep_enter(m);
        else mode_menu_enter(m, 0); /* (a car of the save: its setting is the pre-race check's) */
      } else if (m->screen == MENU_MAKER) {
        int hid = m->hidden_go || m->sel >= CARDB_MAKERS; /* (the key, or the button on the row) */
        int to = hid ? CARDB_HIDDEN : m->sel;
        if (hid) m->hidden_from = m->sel;
        m->hidden_go = 0;
        if (to != m->maker) m->car = 0, m->color = 0;
        m->maker = to;
        car_select_enter(m, MENU_CAR);
      } else if (m->screen == MENU_CAR) {
        if (m->sel != m->car) m->color = 0;
        m->car = m->sel;
        m->all_parts = 0;
        car_select_enter(m, MENU_COLOR);
      } else if (m->screen == MENU_COLOR && m->cc_mode) {
        /* the special colour chosen (0 its own): kept on the card */
        m->ccolor[m->maker][menu_slot(m)] = (signed char)m->sel;
        m->cc_mode = 0;
        m->garage_changed = 1;
        if (m->carmenu == 2) m->carmenu = 0, selmenu_enter(m, SELMENU_CUSTOM, 1);
        else card_enter(m, m->cc_back);
      } else if (m->screen == MENU_COLOR) {
        m->color = m->sel;
        /* the new car's card (a test without the saves: straight on) */
        if (m->save_skip) {
          own_register(m);
          car_select_enter(m, MENU_SETTING);
        } else if (menu_free_dup(m, m->maker, m->car) >= 0) {
          /* a new kind: its name first (name_car 2), then STOCK or FULL TUNE, which registers it */
          name_enter(m);
          m->name_car = 2;
        } else {
          int k = menu_own_find(m, m->maker, m->car, 0);
          m->card_color_was = k >= 0 ? m->own[k].color : -1;
          own_register(m);
          card_enter(m, MENU_COLOR);
        }
      } else if (m->screen == MENU_SETTING && m->vs.tuning) {
        /* a versus' HP set: done, waiting for the others (the host's START moves everyone on) */
        m->vs.tuned = 1;
        m->deciding = 0;
        m->t = FADE;
      } else if (m->screen == MENU_SETTING && m->tune_confirm) {
        /* the pre-race check done: on to the race as it was going */
        int from = m->tune_confirm;
        m->tune_confirm = 0;
        if (from == MENU_VS_SEL) {
          go(m, MENU_VS);
          m->bars = 1;
          m->vs.held = 0;
        } else {
          go(m, MENU_PRE);
        }
      } else if (m->screen == MENU_SETTING) {
        if (m->new_card) {
          own_register(m);
          if (m->save_skip) {
            card_enter(m, MENU_SETTING);
          } else {
            m->name_car = 1; /* (the car's name, as a Banapassport car is named, then its card) */
            name_enter(m);
          }
        } else {
          mode_menu_enter(m, 0);
        }
      } else if (m->screen == MENU_GAME) {
        m->game = GAME_AT[m->sel];
        if (m->game == MENU_GAME_STORY) {
          st_level_enter(m);
        } else if (m->game == MENU_GAME_TEN) {
          ten_course_enter(m);
        } else {
          car_select_enter(m, MENU_COURSE); /* (Personal Best and Official Machine race the same: no mode select) */
        }
      } else if (m->screen == MENU_VS_SEL && m->sel_menu) {
        selmenu_decide(m);
      } else if (m->screen == MENU_VS_SEL) {
        m->vs.kind = m->sel;
        go(m, MENU_VS); /* (the HP is set after the course and the zone are decided: menu_vs_tune_open) */
        m->bars = 1;
        m->vs.held = 0;
      } else if (m->screen == MENU_MODE) {
        m->official = m->sel;
        go(m, MENU_COURSE);
        /* the first card, at the strip's first place (the last visit's place left a gap of
         * empty places before it) */
        m->course_pos = 0;
      } else if (m->screen == MENU_COURSE) {
        m->course = m->sel;
        go(m, MENU_BGM);
        m->sel = m->bgm;
        m->slide = m->card_t = 0;
        m->bars = 1;
      } else if (m->screen == MENU_TEN_COURSE) {
        m->ten_course = m->sel;
        ten_level_enter(m);
      } else if (m->screen == MENU_TEN_LEVEL) {
        /* the level's time zone (clEffTenCourseType::getDayTime, as the title page's) to start on, then the
         * time zone and BGM select (the time attack's) to change it and pick the music */
        m->ten_level = m->sel;
        m->zone = m->zones > 1 ? ten_night(m->ten_course, m->ten_level) : 0;
        go(m, MENU_BGM);
        m->sel = ten_pick(m) ? m->ten_level % 5 + 1 : m->bgm; /* (the 10 out run's list starts on the level's own soundtrack) */
        m->slide = m->card_t = 0;
        m->bars = 1;
      } else if (m->screen == MENU_ST_LEVEL) {
        m->st_level = m->sel;
        st_rival_enter(m);
      } else if (m->screen == MENU_ST_RIVAL) {
        /* the episode's race: its start, its rivals; a song at random (there is no BGM
         * select on the way) */
        m->story_n = 20 * st_division(m) + 5 * m->st_level + m->sel;
        m->story_pick = st_index(st_loop(m), m->story_n);
        if (m->story_pick > 111) m->story_pick = 111;
        tune_confirm_enter(m, MENU_ST_RIVAL);
        m->song = 1 + rand() % (bgm_lines(m) - 2);
        menu_story_song(m, 0);
      } else if (m->screen == MENU_ST_TUNE) {
        /* the part going on (seqStTuneEff) */
        int power = m->tune.choice, p0 = m->st_power[m->maker][menu_slot(m)];
        tune_done(m);
        eff_open(m, power, p0);
      } else if (m->screen == MENU_ST_EFF) {
        /* on to the game mode select, as from a race (the select's poem, the dress-up screens
         * due first) */
        if (m->poem >= 0) {
          poem_open(m);
        } else if (m->ending == 1) {
          ending_open(m);
        } else if (!tg_due(m)) {
          go(m, MENU_GAME);
          m->sel = game_slot(MENU_GAME_STORY);
          m->slide = m->card_t = 0;
          m->bars = 1;
        }
      } else if (m->screen == MENU_LVUP || m->screen == MENU_SHOGO) {
        back_from_race(m, 0);
      } else if (m->screen == MENU_TG_GET) {
        /* a wheel, an aero set, a bonnet, wing or mirror: on to its change (with a choice to
         * make), else the next one due */
        if (m->tg.kind == 0) tg_wheel_open(m);
        else if (m->tg.kind == 1) tg_aero_open(m);
        else if ((m->tg.kind >= 2 && m->tg.kind <= 4) || tg_neon_kind(m->tg.kind)) tg_part_open(m);
        else if (m->tg.kind == 5) tg_vinyl_open(m);
        else if (!tg_due(m)) tg_leave(m);
      } else if (m->screen == MENU_TG_AERO) {
        /* seqTgWearParts after the change (the choice: Refuse to change keeps the one worn) */
        int old = m->aero[m->maker][menu_slot(m)];
        tg_aero_done(m);
        if (!tg_kept(m, old, m->aero[m->maker][menu_slot(m)])) tg_wear_open(m, old, m->aero[m->maker][menu_slot(m)]);
      } else if (m->screen == MENU_TG_WHEEL) {
        int old = m->wheel[m->maker][menu_slot(m)];
        tg_wheel_done(m);
        if (!tg_kept(m, old, m->wheel[m->maker][menu_slot(m)])) tg_wear_open(m, old, m->wheel[m->maker][menu_slot(m)]);
      } else if (m->screen == MENU_TG_VINYL) {
        int old = m->vinyl[m->maker][menu_slot(m)];
        tg_vinyl_done(m);
        if (!tg_kept(m, old, m->vinyl[m->maker][menu_slot(m)])) tg_wear_open(m, old, m->vinyl[m->maker][menu_slot(m)]);
      } else if (m->screen == MENU_TG_PART) {
        int old = *tg_part_value(m);
        tg_part_done(m);
        if (!tg_kept(m, old, *tg_part_value(m))) tg_wear_open(m, old, *tg_part_value(m));
      } else if (m->screen == MENU_TG_WEAR) {
        if (m->dress_free) tg_select_open(m);
        else if (!tg_due(m)) tg_leave(m);
      } else if (m->screen == MENU_TG_SELECT) {
        tg_select_done(m);
      } else if (m->screen == MENU_TG_MAKER) {
        m->tg.maker = WM_MAKER[m->sel];
        tg_wheel_open_at(m, m->tg.maker);
      } else if (m->screen == MENU_BGM) {
        tune_confirm_enter(m, MENU_BGM);
      } else if (m->screen == MENU_PRE) {
        return 1;
      }
    }
  }
  return 0;
}

/* frame of the layout animation for the current state */
static int layout_frame(const menu_state *m, int *arrow) {
  *arrow = -1;
  if (m->screen == MENU_MODE || m->screen == MENU_GAME || m->screen == MENU_VS_SEL) {
    if (m->from >= 0)
      return m->from < m->sel ? m->from * 100 + 91 + m->t : m->sel * 100 + 94 - m->t;
    if (m->deciding) {
      int t = DECIDE_MODE + FADE - m->deciding;
      return m->sel * 100 + 60 + (t < DECIDE_MODE ? t : DECIDE_MODE - 1);
    }
    return m->sel * 100 + m->t % 60;
  }
  int k = m->sel;
  if (m->from >= 0)
    return m->from < k ? m->from * 100 + 90 + m->t : k * 100 + 99 - m->t;
  if (m->deciding) {
    int t = DECIDE_COURSE + FADE - m->deciding;
    return k * 100 + 61 + (t < DECIDE_COURSE ? t : DECIDE_COURSE - 1);
  }
  /* the arrows by the card's place on screen (the first has none to its left, the last none
   * to its right) */
  static const int ARROW_AT[6] = { 0, 100, 200, 300, 400, 600 };
  *arrow = ARROW_AT[m->course_pos >= 0 && m->course_pos < 6 ? m->course_pos : 3] + 10 + m->t % 40;
  return k * 100 + (m->screen == MENU_TEN_COURSE); /* menu_layout_or_course rests on k*100 + 1 */
}

/* The versus select's discs, LAN and ONLINE VERSUS (MENU_VS_KINDS of them: two): centred, the one on large and the other small, changing places over the
 * move, each put together from its pieces (glow, disc, lit rim, picture: menu_layout_mode_sel_3's records) as the port's own two-disc selectors do, the
 * picture the extras' (tools/vsdiscs.py, after BLUETOOTH's, which is gone) */
static void draw_vs_discs(const menu_state *m, int clock, float fade) {
  const int GLOW = anm_sprite(JID(6378)), DISC = anm_sprite(JID(950)), RIM = anm_sprite(JID(3403));
  float rcx = 0, rcy = 0, edge[2][2]; /* (each disc's left and right edge, for the arrow between them) */
  int resting = 0;
  for (int i = 0; i < MENU_VS_KINDS; ++i) {
    const float BIG = 236.0f, SMALL = 118.0f, X0 = 320.0f - 118.0f, X1 = 320.0f + 118.0f;
    float p = m->from >= 0 ? (float)m->t / MOVE_MODE : 1.0f, so = i == m->sel ? 1.0f : 0.0f;
    if (m->from >= 0) so = i == m->sel ? p : (i == m->from ? 1.0f - p : 0.0f);
    float d = SMALL + (BIG - SMALL) * so, box = d * (256.0f / 236.0f);
    float cx = i == 0 ? X0 - 20.0f * so + 10.0f : X1 + 20.0f * so - 10.0f, cy = 190.6f;
    edge[i][0] = cx - d * 0.5f, edge[i][1] = cx + d * 0.5f;
    const float dst[4] = { cx - box * 0.5f, cy - box * 0.5f, cx + box * 0.5f, cy + box * 0.5f }, src[4] = { 0, 0, 256, 256 };
    const float dim = 0.5f + 0.5f * so, mul[4] = { dim, dim, dim, 1 }, add[4] = { 0, 0, 0, 0 };
    for (int q = 0; q < 3; ++q) {
      const int pc = q == 0 ? GLOW : q == 1 ? DISC : RIM;
      float xf[6], mu[4], ad[4];
      if (pc < 0 || (q != 1 && so <= 0.0f)) continue;
      if (q == 1 || !anm_sprite_xf(ID.vs_sel, clock % 60, pc, xf, mu, ad)) {
        anm_draw_part(pc, src, dst, (q == 1 ? 1.0f : so) * fade);
        continue;
      }
      float sc = box / 256.0f, t[6] = { 0, 0, xf[2] * sc, xf[3] * sc, xf[4] * sc, xf[5] * sc };
      t[0] = cx - 128.0f * (t[2] + t[4]), t[1] = cy - 128.0f * (t[3] + t[5]);
      anm_draw_sprite_xf(pc, t, so * mu[3] * fade);
    }
    if (i == m->sel && so >= 1.0f) rcx = cx, rcy = cy, resting = 1;
    int pic = anm_sprite(VS_ICON + 1 + i);
    if (pic >= 0) anm_draw_part_col(pic, src, dst, mul, add);
  }
  /* the red chevrons between the discs (the layout's own sprite: each of its records, with its pulse), the group centred in the gap and pointing to
   * the other disc */
  int lay[16], nl = anm_sprites(ID.vs_sel, lay, 16);
  if (nl > 6) {
    int ar = lay[6];
    float rs[4][4], al[4], x0 = 1e9f, x1 = -1e9f, w = anm_sprite_width(ar), h = anm_sprite_height(ar);
    int n = 0;
    while (n < 4 && ar >= 0 && anm_sprite_rect_at(ID.vs_sel, clock % 60, ar, n, rs[n], &al[n])) {
      x0 = rs[n][0] < x0 ? rs[n][0] : x0, x1 = rs[n][2] > x1 ? rs[n][2] : x1;
      ++n;
    }
    float gap = 0.5f * (edge[0][1] + edge[1][0]), mid = 0.5f * (x0 + x1);
    const float src[4] = { 0, 0, w, h };
    for (int k = 0; k < n; ++k) {
      float xa = rs[k][0] - mid, xb = rs[k][2] - mid; /* (about the group's middle) */
      if (m->sel != 0) { /* (the other way round: the group turned about its middle, each chevron drawn mirrored) */
        float t = xa;
        xa = -xb, xb = -t;
        const float xf[6] = { gap + xb, rs[k][1], -(xb - xa) / w, 0.0f, 0.0f, (rs[k][3] - rs[k][1]) / h };
        anm_draw_sprite_xf(ar, xf, al[k] * fade);
        continue;
      }
      const float dst[4] = { gap + xa, rs[k][1], gap + xb, rs[k][3] };
      anm_draw_part(ar, src, dst, al[k] * fade);
    }
  }
}

void menu_draw(const menu_state *m, float screen_w, float screen_h, void (*draw3d)(void *), void *ctx,
               int (*mini3d)(void *, float)) {
  photo_bg = 0;
  switch (m->screen) { /* (the key glyphs of the bottom bar: the arcade's, where its screens have them) */
  case MENU_HAVE: case MENU_MAKER: case MENU_SETTING: case MENU_GAME: case MENU_MODE: case MENU_ST_LEVEL: case MENU_ST_RIVAL:
    bottom_kind = BK_HS_GD;
    break;
  case MENU_VS_SEL: bottom_kind = m->sel_menu == SELMENU_MAIN || (m->sel_menu == SELMENU_TRANS && !m->trans_edit) ? BK_HS_GD : BK_HS_BB_GD; break; /* (the transmission select has no cancel key, as the arcade's) */
  case MENU_CARD: /* (the pages: view change and confirm; the settings pages and the time attack's their own keys too) */
    bottom_kind = m->card_back == MENU_SETTING ? BK_GC : !cardpages_ok() ? BK_GC : m->card_page == CP_TA ? BK_BP_GC_VS : BK_BP_GC;
    break;
  case MENU_CAR: case MENU_COLOR: case MENU_COURSE: case MENU_BGM: case MENU_TEN_COURSE: case MENU_TEN_LEVEL: case MENU_GARAGE: case MENU_NAME:
  case MENU_OPTION: case MENU_SAVE: case MENU_BINDINGS: case MENU_HUDOPT: case MENU_VOLUME: case MENU_PORTINFO: case MENU_RUMBLE:
    bottom_kind = BK_HS_BB_GD;
    break;
  default: bottom_kind = BK_NONE; break;
  }
  switch (m->screen) {
  case MENU_MOVIE: /* the movie view covers the screen */
    return;
  case MENU_TITLE: {
    int t = m->t;
    draw_title(t < TITLE_LEN ? t : TITLE_LEN - 1, t < TITLE_LEN ? 1.0f : 1.0f - (float)(t - TITLE_LEN) / FADE);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
    return;
  }
  case MENU_RANKING:
    draw_ranking(m);
    return;
  case MENU_LOGO:
    anm_draw(ID.maker[m->t / LOGO_LEN < 3 ? m->t / LOGO_LEN : 2], m->t % LOGO_LEN, 1.0f);
    return;
  case MENU_PROLOGUE:
    anm_draw(ID.prologue[m->t < PROLOGUE_LEN ? 0 : 1], m->t % PROLOGUE_LEN, 1.0f);
    return;
  case MENU_START:
    anm_fill(1, 1, 1, m->t < 6 ? (float)(m->t + 1) / 6.0f : 1.0f);
    return;
  }
  float fade = 1.0f, black = 0.0f;
  if (m->deciding && m->deciding < FADE) fade = (float)m->deciding / FADE;
  if (m->screen == MENU_VS_SEL && m->sel_menu) { /* (the port's own selectors fade in from black and out to it, over everything) */
    float fin = m->sel_age < FADE && !m->deciding ? (float)(m->sel_age + 1) / FADE : 1.0f; /* (in from black on opening, not on each move) */
    black = 1.0f - fin * fade;
    fade = 1.0f;
  }
  /* the backgrounds keep running across screens; the bars slide in when the menus open */
  const int clock = m->clock; /* (60 Hz steps, not draws: a 120 Hz display ran the backgrounds and glyphs twice as fast) */
  bottom_clock = clock;
  int bars = m->bars ? 188 + m->bars : 0;
  if (m->screen <= MENU_COLOR) {
    /* the car select fades as the arcade's clMenuFade does, to black over everything */
    draw_car_select(m, clock, draw3d, ctx);
    draw_topbar(bars, 1, NULL);
    if (!bars) anm_draw(m->screen == MENU_MAKER ? ID.mes_maker : m->screen == MENU_CAR ? ID.mes_car : ID.mes_color, 0, 1);
    draw_bottom(bars, 1);
    /* cars on show still loading: none of them yet, NOW LOADING (its pop-in, then held) */
    if (m->loading) anm_draw_xf(ID.loading, m->loading - 1, 1, LOADING_DX, LOADING_DY, 1, NULL, 0);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->screen == MENU_CAR, 1);
    if (!m->deciding && m->screen == MENU_MAKER) {
      hud_draw_menu_option(screen_w, screen_h, m->key_held == HUD_MENU_OPTION);
      /* the hidden cars key, its caption centred on it (the bottom bar's middle: 320, 425) */
      if (!HID_BTN) hud_draw_menu_hidden(screen_w, screen_h, m->key_held == HUD_MENU_HIDDEN);
      int cap = HID_BTN ? -1 : anm_sprite(OPT_TEXT + O_HIDDEN);
      if (cap >= 0) anm_draw_sprite(cap, 477.5f - 0.5f * anm_sprite_width(cap), 415.0f, 1); /* (centred on the key: hud.c MENU_HIDDEN) */
    }
    if (!m->deciding && all_parts_shown(m)) {
      /* the ALL PARTS key: held down while on */
      hud_draw_menu_hidden(screen_w, screen_h, m->all_parts || m->key_held == HUD_MENU_HIDDEN);
      int cap = anm_sprite(OPT_TEXT + (m->all_parts ? O_ALL_ON : O_ALL_OFF));
      if (cap >= 0) anm_draw_sprite(cap, 477.5f - 0.5f * anm_sprite_width(cap), 415.0f, 1); /* (centred on the key: hud.c MENU_HIDDEN) */
    }
    /* leaving for the title screen, it fades to white instead */
    float c = m->leaving ? 1.0f : 0.0f;
    anm_fill(c, c, c, 1.0f - fade);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
    if (m->screen == MENU_COLOR) setting_warm();
    return;
  }
  if (m->screen == MENU_HAVE) {
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    if (m->deciding) {
      int t = DECIDE_DAY + FADE - m->deciding;
      fade = t < DECIDE_DAY ? 1.0f : (float)m->deciding / FADE;
    }
    if (m->keep_ask) {
      draw_have(m, clock, fade, bars);
    } else {
      fade = m->deciding ? (float)m->deciding / FADE : fade;
      draw_grid(m, clock, fade, bars, draw3d, ctx, screen_w, screen_h);
      if (!m->deciding) hud_draw_menu_option(screen_w, screen_h, m->key_held == HUD_MENU_OPTION);
      anm_fill(0, 0, 0, 1.0f - fade);
    }
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    return;
  }
  if (m->screen == MENU_GARAGE) {
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    draw_garage(m, clock, draw3d, ctx, bars);
    /* its car still loading: NOW LOADING, as the car select's (a blank turntable before) */
    if (m->loading) anm_draw_xf(ID.loading, m->loading - 1, 1, LOADING_DX, LOADING_DY, 1, NULL, 0);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->n_own > 1, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_CARD) {
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    if (m->card_page > 0 && cardpages_ok()) draw_card_page(m, clock, 1, bars, draw3d, ctx);
    else draw_card(m, clock, 1, bars, draw3d, ctx);
    if (card_arrows_on(m)) arrows_pair(clock, 26.0f, 614.0f, 0.5f * (CARD_ARROW_Y0 + CARD_ARROW_Y1), fade, 1, 1); /* (the pages go round: both ways, always) */
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_NAME) {
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    draw_name(m, clock, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_INFO) {
    draw_info(clock, bars);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_SAVE) {
    /* out of START's white; to black for the maker select, to white for the title screen */
    draw_save(m, clock, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    if (!m->deciding && !m->save_confirm) hud_draw_menu_option(screen_w, screen_h, m->key_held == HUD_MENU_OPTION);
    float c = m->leaving ? 1.0f : 0.0f;
    anm_fill(c, c, c, 1.0f - fade);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
    return;
  }
  if (m->screen == MENU_LAYOUT) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_layout(m, clock, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_OPTION) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_option(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_SETTING) {
    /* out of the colour select's black, and back into it */
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    draw_setting(m, clock, draw3d, ctx, bars);
    /* (the balance keys whenever the tune can be split: FULL TUNE's, a story tune's of 20 or more) */
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->tune_full[m->maker][menu_slot(m)] || st_rebalance_ok(m), 1);
    if (m->tune_confirm && !m->deciding) draw_tune_confirm(m, screen_w, screen_h);
    if (m->vs.tuning && m->vs.tune_timer == 0) anm_fill(0, 0, 0, 1), draw_wait(1, m->t, 1); /* (the 15 seconds up: black, Please wait!, till the cars come in) */
    anm_fill(0, 0, 0, 1.0f - fade);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
    setting_warm();
    return;
  }
  if (m->screen == MENU_VOLUME) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_volume(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_HUDOPT) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_hudopt(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_PORTINFO) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_portinfo(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_RUMBLE) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_rumble(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_BINDINGS) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_bindings(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, bind_touch(m) && m->gyro == HUD_STEER_GYRO, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_CARDSET) {
    if (m->t < FADE) fade = (float)(m->t + 1) / FADE;
    draw_cardset(m, clock, bars, screen_w, screen_h);
    hud_draw_menu_keys(screen_w, screen_h, m->key_held, 0, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_BGM) {
    /* the screen fades in from the course select's black */
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    draw_day_select(m, clock, fade, bars);
    if (!m->deciding && !(m->vs.day && m->vs.day_voted)) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, !m->vs.day);
    if (m->vs.day) draw_vs_omakase(m, screen_w, screen_h);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN); /* back from the race's pause menu */
    return;
  }
  if (m->screen == MENU_ST_LEVEL || m->screen == MENU_ST_RIVAL) {
    /* both fade in from the black the screen before faded to (card_t: frames on the screen) */
    if (m->card_t < FADE && !m->deciding) fade = (float)(m->card_t + 1) / FADE;
    if (m->screen == MENU_ST_LEVEL) draw_st_level(m, clock, fade, bars);
    else draw_st_rival(m, clock, fade, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
    return;
  }
  if (m->screen == MENU_ST_TUNE) {
    if (m->card_t < FADE && !m->deciding) fade = (float)(m->card_t + 1) / FADE;
    draw_st_tune(m, clock, fade, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, !m->tune.fixed, 0);
    return;
  }
  if (m->screen == MENU_LVUP) {
    draw_lvup(m);
    return;
  }
  if (m->screen == MENU_VS_STAR) {
    draw_vs_star(m, draw3d, ctx);
    return;
  }
  if (m->screen == MENU_SHOGO) {
    draw_shogo(m, clock);
    return;
  }
  if (m->screen == MENU_POEM) {
    draw_poem(m);
    return;
  }
  if (m->screen == MENU_MILAGE) {
    draw_milage(m, clock, draw3d, ctx);
    return;
  }
  if (m->screen == MENU_BONUS) {
    draw_bonus(m, clock);
    return;
  }
  if (m->screen == MENU_ST_EFF) {
    draw_st_eff(m, draw3d, ctx);
    return;
  }
  if (m->screen == MENU_TG_GET) {
    /* clMenuFade: in and out over 10 frames */
    fade = m->deciding ? (float)m->deciding / TG_FADE : m->card_t < TG_FADE ? (float)(m->card_t + 1) / TG_FADE : 1.0f;
    draw_tg_get(m, clock, 1, draw3d, ctx);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TG_PART) {
    /* clMenuFade: in over 10 frames, out over 10 */
    fade = m->deciding ? (float)m->deciding / TG_FADE : m->card_t < TG_FADE ? (float)(m->card_t + 1) / TG_FADE : 1.0f;
    draw_tg_part(m, clock, 1, bars, draw3d, ctx);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->tg.n > 1, m->dress_free);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TG_WHEEL) {
    fade = m->deciding ? (float)m->deciding / TG_FADE : m->card_t < TG_FADE ? (float)(m->card_t + 1) / TG_FADE : 1.0f;
    draw_tg_wheel(m, 1, bars, draw3d, ctx);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->tg.n > 1, m->dress_free);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TG_VINYL) {
    fade = m->deciding ? (float)m->deciding / TG_FADE : m->card_t < TG_FADE ? (float)(m->card_t + 1) / TG_FADE : 1.0f;
    draw_tg_vinyl(m, 1, bars, draw3d, ctx);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->tg.n > 1, m->dress_free);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TG_WEAR) {
    /* clMenuFade: in over 10 frames (the aero set's 20), out over 10 */
    int in = tg_wear_in(m), out = -m->tg.left - WEAR_WAIT;
    fade = m->card_t < in ? (float)(m->card_t + 1) / (float)in : out > 0 ? 1.0f - (float)out / TG_FADE : 1.0f;
    draw_tg_wear(m, clock, draw3d, ctx);
    anm_fill(0, 0, 0, 1.0f - (fade < 0 ? 0 : fade));
    if (m->tg.white_from >= 0) {
      int f = m->tg.white_from + m->card_t - m->tg.white_at;
      if (f <= m->tg.white_from + 29) anm_draw(ID.whiteout, f, 1);
      else if (!m->tg.white_from) anm_draw(ID.whiteout, 29, 1);
    }
    return;
  }
  if (m->screen == MENU_TG_SELECT || m->screen == MENU_TG_MAKER) {
    /* clMenuFade: in over 10 frames; out over 10 after the decision's frames */
    fade = m->deciding ? (m->deciding < TG_FADE ? (float)m->deciding / TG_FADE : 1.0f)
                       : m->card_t < TG_FADE ? (float)(m->card_t + 1) / TG_FADE : 1.0f;
    if (m->screen == MENU_TG_SELECT) draw_tg_select(m, bars, draw3d, ctx);
    else draw_tg_maker(m, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TG_AERO) {
    if (m->card_t < TG_FADE && !m->deciding) fade = (float)(m->card_t + 1) / TG_FADE;
    draw_tg_aero(m, clock, 1, bars, draw3d, ctx);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, m->tg.n > 1, m->dress_free);
    anm_fill(0, 0, 0, 1.0f - fade);
    return;
  }
  if (m->screen == MENU_TEN_LEVEL) {
    if (m->card_t < FADE && !m->deciding) fade = (float)(m->card_t + 1) / FADE;
    draw_ten_level(m, clock, fade, bars);
    if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
    return;
  }
  if (m->screen == MENU_PRE) {
    if (m->t < FADE && !m->deciding) fade = (float)(m->t + 1) / FADE;
    if (m->pre_quiet) { /* the race's fade colour (black for none), no NOW LOADING */
      float g = m->pre_fade == 2 ? 1.0f : 0.0f;
      anm_fill(g, g, g, 1.0f);
      /* but the ending's clSeqNowLoading */
      if (m->ending == 2 && ID.loading >= 0) {
        int f = m->t + 1;
        float a = f > LOADING_POP ? 0.725f + 0.275f * cosf((float)(f - LOADING_POP) * (2.0f * (float)M_PI / 60.0f)) : 1.0f;
        anm_draw_xf(ID.loading, f < LOADING_POP ? f : LOADING_POP, a, LOADING_PRE_DX, 0, 1, NULL, 0);
      }
      return;
    }
    draw_pre(m, fade);
    /* the course and the car still loading: NOW LOADING bottom left (LOADING_PRE_DX) */
    int f = m->loading ? m->loading : m->load_shown ? m->load_shown + m->load_out : 0;
    if (f) {
      float a = m->loading ? 1.0f : 1.0f - (float)m->load_out / LOADING_OUT;
      if (f > LOADING_POP) a *= 0.725f + 0.275f * cosf((float)(f - LOADING_POP) * (2.0f * (float)M_PI / 60.0f));
      anm_draw_xf(ID.loading, f < LOADING_POP ? f : LOADING_POP, a * fade, LOADING_PRE_DX, 0, 1, NULL, 0);
    }
    return;
  }
  if (m->screen == MENU_VS) {
    draw_vs(m, clock, bars, screen_w, screen_h);
    return;
  }
  if (m->screen == MENU_VS_AREA) {
    draw_vs_area(m, clock, bars, screen_w, screen_h, mini3d);
    return;
  }
  if (m->screen == MENU_VS_MATCH) {
    draw_vs_match(m, draw3d, ctx);
    return;
  }
  int trans = m->screen == MENU_VS_SEL && m->sel_menu == SELMENU_TRANS;
  if (trans) draw_back(ID.bg_carsel, clock % 240, fade); /* (the car's, as the colour select's) */
  else draw_back(ID.bg, clock % 600, fade);
  anm_draw(ID.line, 0, fade);
  if (trans && draw3d) draw3d(ctx);
  if (!(m->screen == MENU_VS_SEL && m->sel_menu)) draw_bg_logo(chosen_car(m, m->car), clock % 240, fade); /* (the port's own selectors have none) */
  int arrow;
  int f = layout_frame(m, &arrow);
  if (m->screen == MENU_GAME) {
    /* story, 10 out run, time attack: the arcade's three-disc mode select (the card-less one) as it is */
    anm_draw(ID.vs_sel_image, f, fade);
    anm_draw(ID.vs_sel, f, fade);
    if (m->from < 0 && !m->deciding && m->t < 60) anm_draw(ID.vs_sel_ring, m->sel * 100 + m->t, fade);
  } else if (m->screen == MENU_VS_SEL && m->sel_menu) {
    draw_selmenu(m, clock, f, 1.0f, screen_w, screen_h);
  } else if (m->screen == MENU_VS_SEL) {
    /* the versus' discs in the story's, 10 out run's and time attack's places, the ghost
     * versus battle's picture behind each, the port's lines under it */
    /* the layout's picture and line behind (LAN's and ONLINE VERSUS's, the extras' after BLUETOOTH's which is gone), and its three discs cannot show
     * two, so the discs are put together from their pieces, as the port's two-disc selectors do */
    static const int PIC[3] = { JID(3224), JID(3223), JID(3225) }, LINE[3] = { JID(2317), JID(2316), JID(2318) };
    anm_swap sw[6];
    for (int k = 0; k < 3; ++k) {
      int on = k < MENU_VS_KINDS;
      sw[2 * k] = (anm_swap){ anm_sprite(PIC[k]), on ? anm_sprite(JID(3226)) : -1 };
      sw[2 * k + 1] = (anm_swap){ anm_sprite(LINE[k]), on ? anm_sprite(VS_LINE + 1 + k) : -1 };
    }
    anm_draw_ex(ID.vs_sel_image, f, fade, 0, 0, sw, 6);
    draw_vs_discs(m, clock, fade);
  } else if (m->screen == MENU_MODE) {
    anm_draw_ex(ID.mode, f, fade, 0, 0, mode_cars, 2);
    if (m->from < 0 && !m->deciding && m->t < 60) anm_draw(ID.mode_ring, m->sel * 100 + m->t, fade);
  } else {
    /* clThread_6_2_CourseSelect::mapChange (the 10-in-a-row's clThread_7_1_CourseSelect::
     * mapChange, by tenCourseIDFromMenuOrder's id: 8 Hakone, 9 Fukuoka, cards 9 and 8): the
     * city's map for cards 6..9, Nagoya, Osaka, Fukuoka, Hakone; the others' (Tokyo's, the time
     * attack's 10 and 11 too; flag 0x3c / 0x38 clear) clMiniatureCourse::update, the 3D map */
    static const int CITY_MAP[4] = { JID(0x1832), JID(0x15b9), JID(0x19a4), JID(0x1831) };
    int city = m->sel >= 6 && m->sel <= 9;
    /* (the cities' picture, Japan's map with the city's name, and over it their miniature too, as the newer builds show them: issue 12) */
    if (city && m->from < 0 && ID.course_map >= 0) {
      anm_swap map = { anm_sprite(JID(0x15b9)), anm_sprite(CITY_MAP[m->sel - 6]) };
      anm_draw_ex(ID.course_map, 0, fade, 0, 0, &map, 1);
    }
    if (mini3d) mini3d((void *)m, fade);
    if (m->screen == MENU_TEN_COURSE) anm_draw_ex(ID.or_course, f, fade, course_dx(m), 0, NULL, 0);
    else anm_draw_ex(ID.course, f, fade, course_dx(m), 0, km_swaps, km_nswaps);
  }
  if (arrow >= 0) anm_draw(ID.arrow, arrow, fade);
  anm_swap or_mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(S_MES_OR_COURSE) };
  anm_swap vs_mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(VS_TEXT) };
  int sel_title = m->screen == MENU_VS_SEL && m->sel_menu ? (trans ? anm_sprite(JID(0x1f99)) : anm_sprite(EX_TITLE + m->sel_menu - 1)) : -1;
  anm_swap game_mes = { anm_sprite(T_TOPBAR_MES), anm_sprite(EX_TITLE + 8) }; /* (the solo games' "Select game mode") */ /* (menu_extra.bin's title, if any) */
  anm_swap blank_mes = { anm_sprite(T_TOPBAR_MES), sel_title };
  draw_topbar(bars, fade, m->screen == MENU_TEN_COURSE ? &or_mes : m->screen == MENU_VS_SEL ? (m->sel_menu ? &blank_mes : &vs_mes) : m->screen == MENU_GAME && game_mes.to >= 0 ? &game_mes : NULL);
  if (m->screen == MENU_GAME && game_mes.to < 0) { /* (the three-disc layout has no title of its own) */
    hud_ui_begin(screen_w, screen_h);
    hud_ui_text("Select game mode", 24.0f * 1.6f, 22.0f * 1.6f, 4.0f, 1, 1, 1, fade);
    hud_ui_end();
  }
  if (m->screen == MENU_VS_SEL && m->sel_menu && sel_title < 0) { /* (the title: the port's text where there is none baked) */
    static const char *const TITLE[9] = { "", "PLEASE SELECT AN OPTION", "SETTINGS", "SELECT YOUR STARTING POINT", "SELECT TRANSMISSION", "Please select an option", "Car settings",
                                          "Delete this car?", "Customize" };
    hud_ui_begin(screen_w, screen_h);
    hud_ui_text(TITLE[m->sel_menu], 24.0f * 1.6f, 22.0f * 1.6f, 4.0f, 1, 1, 1, fade);
    hud_ui_end();
  }
  draw_bottom(bars, fade);
  if (!m->deciding) hud_draw_menu_keys(screen_w, screen_h, m->key_held, 1, 1);
  if (!m->deciding && m->screen == MENU_COURSE && m->from < 0) draw_traffic(m, screen_w, screen_h);
  if (black > 0.0f) anm_fill(0, 0, 0, black);
  if (m->flash) anm_fill(1, 1, 1, (float)m->flash / FLASH_LEN);
}
