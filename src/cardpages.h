/* ---- CARD PAGES (included by menu.c): the card check's seven pages, "Card data confirmed (n/7)". Page 1 is draw_card (the machine
 * registration); the others are drawn here from the arcade's own layouts (menu_extra.bin's animations: tools/extrapack.py EXTRA_ANIMS),
 * with the figures the port keeps and the arcade's fonts for them (EXF: the ASCII glyphs of fonts 1, 2 and 5). A page has no more than
 * what the port has to show; the rest is as an empty card's (zeros, dashes). View change steps through the pages, the gas confirms.
 * 2 story mode, 3 ghost versus battle, 4 its record, 5 10 out run, 6 versus player, 7 time attack (the settings are on the Car / Settings discs). ---- */
enum { CP_REG, CP_STORY, CP_GHOST, CP_GHOST_REC, CP_TEN, CP_VS, CP_TA, CP_N };

static const int CP_TITLE[CP_N] = { JID(0x1db2), JID(0x1db3), JID(0x1db4), JID(0x1db5), JID(0x1db6), JID(0x1db7), JID(0x1db8) };

/* the top bar's "Card data confirmed (n/7)" of page k (menu_extra.bin's titles 9 .. 15 after EX_TITLE_BASE 110100; the arcade's "(n/9)" without them) */
static int cp_title_sprite(int k) {
  int s = anm_sprite(110100 + 9 + k);
  return s >= 0 ? s : anm_sprite(CP_TITLE[k]);
}

static struct {
  int waku[CP_N];            /* each page's frame (its title tab and labels) */
  int chara, line, crown, lv, point, mask, time, stop;
} CP;

static void cardpages_init(void) {
  static const char *const WAKU[CP_N] = { NULL, "menu_g_cardcheck_2_waku", "menu_g_cardcheck_3_waku", "menu_layout_cardcheck_14_waku",
    "menu_g_cardcheck_4_waku", "menu_g_cardcheck_5_waku", "menu_g_cardcheck_6_waku" };
  for (int k = 1; k < CP_N; ++k) CP.waku[k] = anm_find(WAKU[k]);
  CP.chara = anm_find("menu_g_cardcheck_2_chara"), CP.line = anm_find("menu_g_cardcheck_2_line");
  CP.crown = anm_find("menu_layout_cardcheck_3_crown"), CP.lv = anm_find("menu_g_cardcheck_4_lv");
  CP.point = anm_find("menu_g_cardcheck_5_point"), CP.mask = anm_find("menu_g_cardcheck_6_mask"), CP.time = anm_find("menu_g_cardcheck_6_time");
  CP.stop = anm_find("menu_g_cardcheck_6_scrollstop");
}

/* a string in the arcade's font 1, 2 or 5 (menu_extra.bin): from the top left (x, y) of its cells on the 640 x 480 screen; returns where it ends */
#define EXF_BASE 120000
static const unsigned char EXF_ADV[8] = { 7, 8, 8, 12, 16, 16, 16, 18 };
static float ex_width(int font, const char *s) { return (float)strlen(s) * (float)EXF_ADV[font]; }
static float ex_text(int font, const char *s, float x, float y, float alpha) {
  for (; *s; ++s, x += (float)EXF_ADV[font]) {
    if (*s <= ' ' || *s > '~') continue;
    anm_draw_sprite(anm_sprite(EXF_BASE + 100 * font + (*s - ' ')), x, y, alpha);
  }
  return x;
}
static void ex_text_right(int font, const char *s, float xr, float y, float alpha) { ex_text(font, s, xr - ex_width(font, s), y, alpha); }

/* whether this build's menu_extra.bin has the pages' art */
static int cardpages_ok(void) { return CP.waku[CP_GHOST] >= 0 && CP.time >= 0 && anm_sprite(EXF_BASE + 500 + ('0' - ' ')) >= 0; }

static void cp_anim(int anim, int frame, float fade, const anm_swap *sw, int n) {
  if (anim >= 0) anm_draw_ex(anim, frame, fade, 0, 0, sw, n);
}
static void cp_sprite(int id, float x, float y, float a) { anm_draw_sprite(anm_sprite(id), x, y, a); }

/* the figures of the card shown (own[k]) */
typedef struct { int mk, sl; const menu_own *o; } cp_car;
static cp_car cp_of(const menu_state *m) {
  const menu_own *o = &m->own[m->own_sel >= 0 ? m->own_sel : 0];
  return (cp_car){ o->maker, menu_own_slot(o), o };
}

/* ---- 3: the ghost versus battle: the dress-up level, the results, the areas conquered (none kept) ---- */
static void cp_ghost(const menu_state *m, float fade) {
  cp_car c = cp_of(m);
  /* the layout is the older one's (with Maxi coins): its labels' places are taken over, the coins' left out */
  anm_swap sw[3] = { { anm_sprite(JID(0x0c55)), -1 }, { anm_sprite(JID(0x0c56)), -1 }, { anm_sprite(JID(0x0c57)), -1 } };
  cp_anim(CP.waku[CP_GHOST], 0, fade, sw, 3);
  if (CP.crown >= 0) anm_draw_ex(CP.crown, 0, fade, 0, -27.0f, sw, 3);
  cp_sprite(JID(0x0c55), -14.0f, 222.0f, fade);
  cp_sprite(JID(0x0c57), -14.0f, 285.0f, fade);
  char t[48];
  snprintf(t, sizeof t, "LV. %d", m->dress_lv[c.mk][c.sl]);
  ex_text(5, t, 222.0f, 157.0f, fade);
  snprintf(t, sizeof t, "%d races %d wins", m->vs_count_car[c.mk][c.sl], m->vs_wins_car[c.mk][c.sl]);
  ex_text(5, t, 220.0f, 222.0f, fade);
}

/* ---- 4: the ghost's win / loss record in this arcade: an empty one (the port has no ghosts' matches) ---- */
static const int CP_DIGITS[10] = { JID(0x0488), JID(0x0489), JID(0x048a), JID(0x048b), JID(0x048c), JID(0x048d), JID(0x048e), JID(0x048f), JID(0x0490), JID(0x0491) };
static const int CP_DOTS[8] = { JID(0x06b4), JID(0x06b5), JID(0x06b6), JID(0x06b7), JID(0x06b8), JID(0x06b9), JID(0x06ba), JID(0x06bb) };

static void cp_ghost_rec(const menu_state *m, float fade) {
  (void)m;
  /* the layout's digit sprites and placeholders are out; the figures are ours */
  anm_swap sw[22];
  int n = 0;
  for (int k = 0; k < 10; ++k) sw[n++] = (anm_swap){ anm_sprite(CP_DIGITS[k]), -1 };
  sw[n++] = (anm_swap){ anm_sprite(JID(0x17f5)), -1 };
  sw[n++] = (anm_swap){ anm_sprite(JID(0x17f6)), -1 };
  for (int k = 0; k < 8; ++k) sw[n++] = (anm_swap){ anm_sprite(CP_DOTS[k]), -1 };
  cp_anim(CP.waku[CP_GHOST_REC], 0, fade, sw, n);
  ex_text(5, "0", 410.0f, 130.0f, fade);
  ex_text_right(5, "0", 560.0f, 130.0f, fade);
  static const float COLX[5] = { 35.0f, 105.0f, 287.0f, 405.0f, 475.0f };
  static const char *const DASH[5] = { "----", "-----------", "-------", "----", "--------" };
  for (int r = 0; r < 8; ++r)
    for (int k = 0; k < 5; ++k) ex_text(5, DASH[k], COLX[k], 170.0f + 24.0f * (float)r, fade * 0.55f);
}


/* ---- 5: 10 out run: the medal of each level (1 to 10) of the courses the card page shows (the arcade's 8) ---- */
static const int CP_MEDAL[4] = { JID(0x0eda), JID(0x1586), JID(0x1587), JID(0x0ed9) }; /* none, bronze, silver, gold */
static const int CP_LV[10] = { JID(0x08ab), JID(0x08ac), JID(0x08ad), JID(0x08ae), JID(0x08af), JID(0x08b0), JID(0x08b1), JID(0x08b2), JID(0x08b3), JID(0x08b4) };
static const int CP_TEN_ID[8] = { 0, 1, 2, 3, 4, 5, 7, 8 }; /* the arcade's rows: Tokyo's six, Hanshin (Osaka), Hakone */
static void cp_ten(const menu_state *m, float fade) {
  cp_car c = cp_of(m);
  cp_anim(CP.waku[CP_TEN], 0, fade, NULL, 0);
  for (int r = 0; r < 8; ++r)
    for (int lv = 0; lv < 10; ++lv) {
      int v = m->ten_car[c.mk][c.sl][CP_TEN_ID[r]][lv];
      float x = 206.0f + 40.0f * (float)lv - 32.0f, y = 152.0f + 29.7f * (float)r - 16.0f;
      cp_sprite(CP_MEDAL[v < 0 ? 0 : v > 3 ? 3 : v], x, y, fade);
      cp_sprite(CP_LV[lv], x, y, fade);
    }
}

/* ---- 6: versus: the races and outruns, the driving style (the point stays at the middle: none kept) ---- */
static void cp_vs(const menu_state *m, float fade, int clock) {
  cp_car c = cp_of(m);
  cp_anim(CP.waku[CP_VS], 0, fade, NULL, 0);
  cp_sprite(JID(0x08b5), -15.0f, 141.0f, fade);
  cp_sprite(JID(0x0ace), 334.0f, 142.0f, fade);
  char t[48];
  snprintf(t, sizeof t, "%d Races", m->vs_count_car[c.mk][c.sl]);
  ex_text(5, t, 79.0f, 171.0f, fade);
  snprintf(t, sizeof t, "%d Outruns", m->vs_wins_car[c.mk][c.sl]);
  ex_text(5, t, 79.0f, 208.0f, fade);
  if (CP.point >= 0) anm_draw_ex(CP.point, clock % 60, fade, 459.0f, 267.0f, NULL, 0);
}

/* ---- 7: time attack: the player's best on every course, the list scrolling down by itself and round (the challenge button stops it); the
 * window is the frame's own: the rows run under the mask ---- */
static const int CP_STAGE[15] = { JID(0x0edb), JID(0x0edc), JID(0x0edd), JID(0x0ede), JID(0x0edf), JID(0x0ee0), JID(0x0ee1), JID(0x0ee2), JID(0x0ee3), JID(0x0ee4),
                                  JID(0x0ee5), JID(0x0ee6), JID(0x0ee7), JID(0x0ee8), JID(0x0ee9) };
#define CP_TA_PITCH 62.0f
static void cp_ta_row(int k, float y, float fade) {
  int id = car_course_id(k);
  cp_sprite(CP_STAGE[id < 15 ? id : 0], 39.0f, y - 2.0f, fade);
  cp_sprite(JID(0x06e8), 25.0f, y + 22.0f, fade);
  int ms = app_best_ms(k);
  char t[32];
  if (ms > 0) snprintf(t, sizeof t, "%d'%02d\"%03d", ms / 60000, ms / 1000 % 60, ms % 1000);
  else snprintf(t, sizeof t, "-'--\"---");
  ex_text(5, t, 182.0f, y, fade);
  if (ms > 0) ex_text(2, "[Your car]", 392.0f, y + 6.0f, fade);
}

static void cp_ta(const menu_state *m, int clock, float fade) {
  static float off;
  float total = CP_TA_PITCH * (float)N_COURSES;
  if (!m->card_row) off += 0.45f; /* (card_row: the scrolling stopped) */
  if (off >= total) off -= total;
  cp_anim(CP.waku[CP_TA], 0, fade, NULL, 0);
  for (int k = 0; k < N_COURSES; ++k)
    for (int lap = 0; lap < 2; ++lap) {
      float y = 141.0f + CP_TA_PITCH * (float)k - off + (lap ? total : 0.0f);
      if (y > 118.0f && y < 392.0f) cp_ta_row(k, y, fade);
    }
  cp_anim(CP.mask, 0, fade, NULL, 0);
  /* the scroll bar's knob, along the bar with how far the list has gone, its button glowing (the anim's 30 frames) */
  cp_sprite(JID(0x1b57), 580.0f, 135.0f, fade); /* the bar stays; the knob (the anim) goes along it */
  anm_swap bar = { anm_sprite(JID(0x1b57)), -1 };
  if (CP.stop >= 0) anm_draw_ex(CP.stop, clock % 30, fade, 0, 165.0f + 195.0f * off / total - 270.0f, &bar, 1);
}

/* ---- 2: story mode: the division's twenty episodes, the cleared ones with their rival and the ramp, and the rounds and chapters ---- */
static int st_index(int loop, int n);
/* the card's picture and ramp name of each story index (the arcade's switch at 0x88db000, tools-wsl/storyswitch2.py): 128 x 64 cut-outs, drawn on the panel */
static const int CP_ST[111] = {
  JID(0x0847), JID(0x0848), JID(0x0849), JID(0x084a), JID(0x084b), JID(0x084c),
  JID(0x084d), JID(0x084e), JID(0x084f), JID(0x0850), JID(0x0851), JID(0x0852),
  JID(0x0853), JID(0x0854), JID(0x0855), JID(0x0856), JID(0x0857), JID(0x0858),
  JID(0x0859), JID(0x085a), JID(0x085b), JID(0x085c), JID(0x085d), JID(0x085e),
  JID(0x085f), JID(0x0860), JID(0x0861), JID(0x0862), JID(0x0863), JID(0x0864),
  JID(0x0865), JID(0x0866), JID(0x0867), JID(0x0868), JID(0x0869), JID(0x086a),
  JID(0x086b), JID(0x086c), JID(0x086d), JID(0x086e), JID(0x086f), JID(0x0870),
  JID(0x0871), JID(0x0872), JID(0x0873), JID(0x0874), JID(0x0875), JID(0x0876),
  JID(0x0877), JID(0x0878), JID(0x0879), JID(0x087a), JID(0x087b), JID(0x087c),
  JID(0x087d), JID(0x087e), JID(0x087f), JID(0x0880), JID(0x0881), JID(0x0882),
  JID(0x0883), JID(0x0884), JID(0x0885), JID(0x0886), JID(0x0887), JID(0x0888),
  JID(0x0889), JID(0x088a), JID(0x088b), JID(0x088c), JID(0x088d), JID(0x088e),
  JID(0x088f), JID(0x0890), JID(0x0891), JID(0x0892), JID(0x0893), JID(0x0894),
  JID(0x0895), JID(0x0896), JID(0x0ac1), JID(0x0ac2), JID(0x0ac3), JID(0x0ac4),
  JID(0x0ac5), JID(0x0ac6), JID(0x0ac7), JID(0x0ac8), JID(0x0ac9), JID(0x0aca),
  JID(0x0897), JID(0x0898), JID(0x0899), JID(0x089a), JID(0x089b), JID(0x089c),
  JID(0x089d), JID(0x089e), JID(0x089f), JID(0x08a0), JID(0x0848), JID(0x08a2),
  JID(0x08a3), JID(0x08a4), JID(0x08a5), JID(0x08a6), JID(0x08a7), JID(0x08a8),
  JID(0x08a9), JID(0x0ac0), JID(0x1a51)
};
static const int CP_RAMP_OF[111] = {
  JID(0x1c22), JID(0x1c28), JID(0x1c1d), JID(0x1c2b), JID(0x1c23), JID(0x1b52),
  JID(0x1c18), JID(0x1c27), JID(0x1c24), JID(0x1c16), JID(0x1c16), JID(0x1c19),
  JID(0x1c22), JID(0x1c23), JID(0x1c25), JID(0x1c22), JID(0x1c20), JID(0x1c20),
  JID(0x1c18), JID(0x1c1a), JID(0x1c1d), JID(0x1c1a), JID(0x1c16), JID(0x1c18),
  JID(0x1c20), JID(0x1c1a), JID(0x1c21), JID(0x1c28), JID(0x1c18), JID(0x1c20),
  JID(0x1c17), JID(0x1c20), JID(0x1c24), JID(0x1c18), JID(0x1c23), JID(0x1b52),
  JID(0x1c1d), JID(0x1c1b), JID(0x1c23), JID(0x1c1f), JID(0x1b52), JID(0x1b51),
  JID(0x1b51), JID(0x1b51), JID(0x1c27), JID(0x1c1b), JID(0x1c1d), JID(0x1c20),
  JID(0x1c1b), JID(0x1c1e), JID(0x1b52), JID(0x1c27), JID(0x1c1b), JID(0x1c2a),
  JID(0x1c2b), JID(0x1c18), JID(0x1c24), JID(0x1c29), JID(0x1c1e), JID(0x1c1b),
  JID(0x1c17), JID(0x1c1c), JID(0x1c28), JID(0x1c1b), JID(0x1c26), JID(0x1c23),
  JID(0x1c22), JID(0x1c28), JID(0x1c1b), JID(0x1c1d), JID(0x1c17), JID(0x1c1b),
  JID(0x1c21), JID(0x1c18), JID(0x1c24), JID(0x1c28), JID(0x1c1b), JID(0x1c18),
  JID(0x1c16), JID(0x1c1f), JID(0x1c23), JID(0x1c27), JID(0x1c24), JID(0x1b52),
  JID(0x1c1f), JID(0x1b52), JID(0x1c26), JID(0x1c1b), JID(0x1c16), JID(0x1c1e),
  JID(0x1c16), JID(0x1c29), JID(0x1c25), JID(0x1c1e), JID(0x1c1d), JID(0x1c1f),
  JID(0x1c27), JID(0x1c20), JID(0x1c1e), JID(0x1c17), JID(0x1c27), JID(0x1c22),
  JID(0x1c16), JID(0x1b52), JID(0x1b51), JID(0x1c20), JID(0x1c2c), JID(0x1c18),
  JID(0x1c27), JID(0x1c17), JID(0x1c17)
};

static void cp_story(const menu_state *m, float fade) {
  cp_car c = cp_of(m);
  int div = m->st_div_car[c.mk][c.sl], bits = m->st_bits_car[c.mk][c.sl], loop = div / 5, dv = div % 5;
  if (getenv("MT3DX_CP_STREAK")) ((menu_state *)m)->st_streak_car[c.mk][c.sl] = (short)atoi(getenv("MT3DX_CP_STREAK")); /* test: this win streak */
  if (getenv("MT3DX_CP_BITS")) bits = atoi(getenv("MT3DX_CP_BITS")), div = getenv("MT3DX_CP_DIV") ? atoi(getenv("MT3DX_CP_DIV")) : 0, loop = div / 5, dv = div % 5; /* test: these episodes cleared */
  cp_anim(CP.waku[CP_STORY], 0, fade, NULL, 0);
  for (int k = 0; k < 20; ++k) {
    int n = 20 * dv + k, done = (bits >> k) & 1;
    float cx = 103.0f + 108.0f * (float)(k % 5), cy = 167.0f + 60.0f * (float)(k / 5);
    int row = st_index(loop, n);
    int pic = row >= 0 && row < 111 && CP_ST[row] >= 0 ? anm_sprite(CP_ST[row]) : -1;
    cp_sprite(JID(0x17ff), cx - 65.0f, cy - 34.0f, fade); /* the panel (its visible part is the middle 86 x 46) */
    if (done && pic >= 0) {
      anm_draw_sprite(pic, cx - 64.0f, cy - 32.0f, fade);
    } else {
      cp_sprite(JID(0x1a51), cx - 64.0f, cy - 32.0f, fade); /* the "?" */
    }
    char t[16];
    snprintf(t, sizeof t, "%d", loop * 100 + n + 1);
    ex_text(1, t, cx - 38.0f, cy + 2.0f, fade);
    if (done && row >= 0 && row < 111 && CP_RAMP_OF[row] >= 0) cp_sprite(CP_RAMP_OF[row], cx - 64.0f, cy + 22.0f, fade); /* (the ramp's name under the tile) */
  }
  int cleared = 20 * div + __builtin_popcount((unsigned)bits & 0xfffff);
  char t[24];
  /* the two boxes: the win streak (getStorySuccessiveVictoryCount: the episodes won in a row, the story's own count since the arcade's later
   * layout, issue 11; the rounds this page told before are not shown) and the chapters cleared */
  ex_text(2, "Win streak", 380.0f, 99.0f, fade);
  cp_sprite(JID(0x0abd), 380.0f, 115.0f, fade); /* "Chapters cleared" */
  ex_text(2, "wins", 566.0f, 99.0f, fade);
  cp_sprite(JID(0x11ef), 566.0f, 115.0f, fade); /* "Chapter" */
  snprintf(t, sizeof t, "%d", m->st_streak_car[c.mk][c.sl]);
  ex_text_right(2, t, 562.0f, 100.0f, fade);
  snprintf(t, sizeof t, "%d", cleared > 10000 ? 10000 : cleared);
  ex_text_right(2, t, 568.0f, 121.0f, fade);
}

/* the page shown: the common ground (as the registration's), its frame and figures, the top bar with its "(n/7)" and the bottom bar */
static void draw_card_page(const menu_state *m, int clock, float fade, int bars, void (*draw3d)(void *), void *ctx) {
  draw_back(ID.bg_carsel, clock % 240, fade);
  anm_draw(ID.line, clock % anm_frames(ID.line), fade);
  if (draw3d) draw3d(ctx); /* (the car turning behind the page's layer) */
  switch (m->card_page) {
  case CP_STORY: cp_story(m, fade); break;
  case CP_GHOST: cp_ghost(m, fade); break;
  case CP_GHOST_REC: cp_ghost_rec(m, fade); break;
  case CP_TEN: cp_ten(m, fade); break;
  case CP_VS: cp_vs(m, fade, clock); break;
  case CP_TA: cp_ta(m, clock, fade); break;
  default: break;
  }
  anm_swap title = { anm_sprite(T_TOPBAR_MES), cp_title_sprite(m->card_page) };
  draw_topbar(bars, fade, &title);
  draw_bottom(bars, fade);
}
