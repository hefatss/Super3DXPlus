#include "spark.h"
#include "lights.h"
#include "sparkdb.h"
#include "backfiredb.h"
#include "cardb.h"
#include "anm.h"
#include "hud.h"
#include "spriteids.h" /* JID(n) */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* clCarSparkManager: 30 clCarSparkPrimitives a car (getMaxSparkPrimitive). A spark is a
 * ribbon of two quads through six points (A..F, relative to where it started from the car):
 * (A, C) the oldest cross-section, (B, D) the one after, (E, F) its head, F size above E.
 * Each frame (update 0x85f534c) the sections move back one (A, C <- B, D <- E, F), the head
 * goes on by the velocity, gravity takes 0.2 off its y (0x92f8258), and its life (13 at
 * birth) runs down; it is drawn while the life is 0 or more. */
#define SPARK_MAX 30
#define SPARK_LIFE 13
#define SPARK_GRAVITY 0.2f

typedef struct {
  float p[6][3]; /* A B C D E F */
  float v[3], rel[3];
  int life;      /* -1: free */
  int tex;       /* 0 car_spark03.tga (image 0x11a), 1 car_spark04.tga (0x11b) */
} spark;

/* clTireTrackManager (one a car, its four clTireTracks): each arcade frame (update 0x85f8368)
 * a wheel lays a cross-section of its mark under it, from its world matrix's translation down
 * the tyre's radius (+0x24), half the tread (+0x20) either side along the wheel's -x: the
 * left end u 1, the right u 0, both v (the distance run so far times 0.208333, 0x93dd0fc, from
 * the third section on) and the effect's strength for the wheel as their alpha. A track keeps
 * the last TIRE_TRACK_VERTEX_NUM (effect_config.lua: 30) ends, the oldest dropped. Drawn
 * (clTireTrack::draw 0x85f770c) as a strip in white with those alphas, sprite/tirepattern.tga,
 * blended over the road. */
#define TRACK_NODES 30
#define TRACK_V 0.208333f
#define TRACK_LIFT 0.05f /* (a hair above the road, off its depth) */
typedef struct { float p[3], u, v, a; } track_node;
typedef struct {
  track_node n[TRACK_NODES]; /* oldest first */
  int count;
  float last[2][3], v;
} track;

/* The tyre smoke (clRollUpManager, type 0: clRollUpObject::update 0x8627ec0, its primitive's draw 0x86274f0; effect_config.lua's TIRE_SMOKE_*). The arcade
 * does not leave it in the world: a particle's place is worked out again each frame from where the tyre is NOW, the tyre's contact point less its age times
 * the unit direction of the car's velocity, so a cloud hangs about the rear of the car, a metre or two behind each rear tyre however fast it goes.
 *   - clCarEffectManager::update hands it the two rear tyres' contact points (SInfoCarEffect +0x18, +0x24), the strength at each (+0x50, +0x54) and the
 *     velocity (+0x30). Every other frame (the odd ones: its counter % 2) a tyre whose strength is not 0 starts a particle, if fewer than 50 are about:
 *     alpha min(15 * strength, 1) falling to 0 over ROLLUP_LIFE frames, a size factor (((20 + int|v| % 40) / 40) * 0.5)^2 that grows by
 *     TIRE_SMOKE_SCALE_VALUE (1.2) a frame to TIRE_SMOKE_SCALE_VALUE_MAX (3), and a flag, int|v| % 40 odd: the particle follows the contact point, even: the
 *     point an eighth of the way in from it towards the other tyre.
 *   - drawn (sprite/tiresmoke_all.tga's smoke, white, alpha blended, depth tested and not written) as a camera-facing quad 2 * size across by size high,
 *     centred 2 units above the particle.
 * Only the player's car has any strength (CRivalCar::GetInfoCarEffect leaves it 0). */
#define SMOKE_MAX 50
#define ROLLUP_LIFE 25
#define ROLLUP_EVERY 2
#define ROLLUP_N 40
#define ROLLUP_GROW 1.2f
#define ROLLUP_GROW_MAX 3.0f
#define ROLLUP_ALPHA 15.0f
#define ROLLUP_SIZE 2.0f
/* The strength the sim gives (SInfoCarEffect +0x50, +0x54) is not 0 for nearly every frame the car is driven hard (a wheelspin ratio past a low threshold), and
 * on its own showed the smoke all the time. The tyre marks' strength for the same rear tyre (laid only while it slips, as before) gates it: a particle is
 * started only while that is past SMOKE_MIN, and starts at least as dense as that mark is dark. */
#define SMOKE_MIN 0.1f
typedef struct { float p[3], dir[3], alpha, decay, size; int age, emitter, flag, on; } puff;

/* clBackFireManager (src/backfiredb.h, tools/stage/pack_backfire.py): a fire at each of the
 * car's MF_NULLs; while SInfoCarEffect's flag is on, setup (0x85fe8b8) starts the idle ones
 * with SAMLL_A or MIDDLE_A by int(|velocity|) % 2 (B: SMALL_B, MIDDLE_B), then update (0x85fe694)
 * each frame shows each going one at frame 0..16 at the null, half size, under the car's matrix */
typedef struct { int state, group; } backfire;
#define BACKFIRE_TRIS_MAX 4096

static struct {
  spark s[SPARK_CARS][SPARK_MAX];
  backfire bf[SPARK_CARS][4];
  const backfire_car *bf_car[SPARK_CARS];
  float bf_body[SPARK_CARS][16];
  render_vertex bv[BACKFIRE_TEXTURES][4 * BACKFIRE_TRIS_MAX];
  track t[SPARK_CARS][4];
  puff pf[SPARK_CARS][SMOKE_MAX];
  unsigned pf_rand;
  int pf_n[SPARK_CARS];
  render_vertex pv[4 * SPARK_CARS * SMOKE_MAX];
  float radius[SPARK_CARS], tread[SPARK_CARS];
  render_vertex tv[4 * SPARK_CARS * 4 * (TRACK_NODES / 2)];
  float pos[SPARK_CARS][3]; /* the car's position (its matrix's translation) */
  int rand_at;              /* createSpark's running index into the tables (0x97fda24) */
  int ready;
  render_vertex v[2][4 * 2 * SPARK_CARS * SPARK_MAX];
} S;

static struct { float shift[SPARK_CARS][3]; float alpha; } L = { .alpha = 1.0f };

void spark_blend(const float (*shift)[3], float alpha) {
  memset(L.shift, 0, sizeof L.shift);
  if (shift && alpha < 1.0f) memcpy(L.shift, shift, sizeof L.shift);
  L.alpha = shift ? alpha : 1.0f;
}

void spark_reset(void) {
  S.ready = 1;
  for (int k = 0; k < SPARK_CARS; ++k) {
    for (int i = 0; i < SPARK_MAX; ++i) S.s[k][i].life = -1;
    memset(S.t[k], 0, sizeof S.t[k]);
    for (int i = 0; i < SMOKE_MAX; ++i) S.pf[k][i].on = 0;
    for (int i = 0; i < 4; ++i) S.bf[k][i].state = -1;
    if (S.radius[k] <= 0) S.radius[k] = 3.0f, S.tread[k] = 2.0f;
  }
}

void spark_backfire_car(int k, int pack, int body, int tuned_exhaust) {
  if (k < 0 || k >= SPARK_CARS) return;
  int n = (int)(sizeof BACKFIRE_CAR / sizeof BACKFIRE_CAR[0]);
  S.bf_car[k] = pack >= 0 && pack < n ? &BACKFIRE_CAR[pack][(tuned_exhaust ? 4 : 0) + (body & 3)] : NULL;
  for (int i = 0; i < 4; ++i) S.bf[k][i].state = -1;
  if (getenv("MT3DX_FXLOG") && S.bf_car[k]) fprintf(stderr, "backfire car %d: pack %d, body %d, tuned exhaust %d: %d pipes\n", k, pack, body & 3, tuned_exhaust != 0, S.bf_car[k]->n);
}

static void backfire_step(int k, const float *body, const car_effect *e) {
  const backfire_car *c = S.bf_car[k];
  if (!c) return;
  memcpy(S.bf_body[k], body, sizeof S.bf_body[k]);
  if (e->backfire) {
    int g = ((int)e->speed % 2) + (c->b ? 3 : 0);
    for (int i = 0; i < c->n; ++i)
      if (S.bf[k][i].state == -1) S.bf[k][i].state = 0, S.bf[k][i].group = g;
  }
}

/* after the frame's draw: on to the next frame, done past the last */
static void backfire_advance(int k) {
  for (int i = 0; i < 4; ++i)
    if (S.bf[k][i].state >= 0 && ++S.bf[k][i].state >= BACKFIRE_FRAMES) S.bf[k][i].state = -1;
}

void spark_tyre(int k, float radius, float tread) {
  if (k < 0 || k >= SPARK_CARS) return;
  S.radius[k] = radius > 0 ? radius : 3.0f;
  S.tread[k] = tread > 0 ? tread : 2.0f;
}

static void track_push(track *t, const float p[3], float u, float a) {
  if (t->count == TRACK_NODES) {
    memmove(t->n, t->n + 1, sizeof t->n[0] * (TRACK_NODES - 1));
    t->count--;
  }
  track_node *n = &t->n[t->count++];
  memcpy(n->p, p, sizeof n->p);
  n->u = u, n->v = t->v, n->a = a;
}

/* clTireTrackManager::update for car k's wheel w */
static void track_step(int k, int w, const float *m, float a) {
  track *t = &S.t[k][w];
  float c[3] = { m[12], m[13] - S.radius[k] + TRACK_LIFT, m[14] }, l[3], r[3];
  /* (-tread, 0, 0) through the wheel's matrix, halved */
  for (int j = 0; j < 3; ++j) {
    float h = -S.tread[k] * m[j] * 0.5f;
    l[j] = c[j] - h, r[j] = c[j] + h;
  }
  if (t->count > 2) {
    float d[3];
    for (int j = 0; j < 3; ++j) d[j] = ((l[j] - t->last[0][j]) + (r[j] - t->last[1][j])) * 0.5f;
    t->v += sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * TRACK_V;
  }
  memcpy(t->last[0], l, sizeof l);
  memcpy(t->last[1], r, sizeof r);
  track_push(t, l, 1.0f, a);
  track_push(t, r, 0.0f, a);
}

/* clCarSparkManager::createSpark (0x85f4eb0), clCarEffectManager::update's (strength * 30 *
 * 0.5) of them: each flies off along the effect's direction (unit length, doubled) plus a
 * random vector (doubled again across, y not), from the contact point; the faster, the wider
 * (((int)|v| / 5 + 10) * 0.1) and that number's odd or even picks its picture. A spark is
 * only made when one of the 30 is free. */
static void create(int k, const car_effect *e) {
  int n = (int)(e->spark * (float)SPARK_MAX * 0.5f);
#ifdef WMMT_EGL
  if (getenv("WMMT_SPARK_LOG"))
    fprintf(stderr, "spark car %d: %.3f at %.1f %.1f %.1f dir %.2f %.2f %.2f (car %.1f %.1f %.1f) n %d\n", k, e->spark, e->at[0], e->at[1],
            e->at[2], e->dir[0], e->dir[1], e->dir[2], S.pos[k][0], S.pos[k][1], S.pos[k][2], n);
#endif
  float len = sqrtf(e->dir[0] * e->dir[0] + e->dir[1] * e->dir[1] + e->dir[2] * e->dir[2]);
  float dn[3] = { 0, 0, 0 };
  if (len > 0)
    for (int j = 0; j < 3; ++j) dn[j] = e->dir[j] / len;
  for (int i = 0; i < n; ++i) {
    const float *r = SPARK_RAND[i % 4][S.rand_at];
    S.rand_at = S.rand_at + 1 > 99 ? 0 : S.rand_at + 1;
    float v[3] = { 2.0f * (2.0f * r[0] + dn[0]), 2.0f * r[1] + dn[1], 2.0f * (2.0f * r[2] + dn[2]) };
    spark *s = NULL;
    for (int j = 0; j < SPARK_MAX && !s; ++j)
      if (S.s[k][j].life == -1) s = &S.s[k][j];
    if (!s) continue;
    int w = (int)sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) / 5 + 10;
    s->tex = w & 1 ? 0 : 1;
    memset(s->p, 0, sizeof s->p);
    s->p[5][1] = (float)w * 0.1f;
    for (int j = 0; j < 3; ++j) {
      s->rel[j] = e->at[j] - S.pos[k][j];
      s->v[j] = v[j];
    }
    s->life = SPARK_LIFE;
  }
}

/* clRollUpManager::clRollUpObject::update, type 0 (see the comment at SMOKE_MAX) */
static float smoke_force(void) { /* tests: MT3DX_SMOKE_FORCE=0.5 gives the player's tyres that strength whatever the sim says */
  static float f = -1.0f;
  if (f < 0.0f) f = getenv("MT3DX_SMOKE_FORCE") ? (float)atof(getenv("MT3DX_SMOKE_FORCE")) : 0.0f;
  return f;
}

static void smoke_step(int k, const car_effect *e) {
  float q[2][3];
  for (int j = 0; j < 2; ++j)
    for (int c = 0; c < 3; ++c) q[j][c] = e->smoke_at[0][c] + (e->smoke_at[1][c] - e->smoke_at[0][c]) * (j ? 0.875f : 0.125f);
  int count = 0;
  for (int i = 0; i < SMOKE_MAX; ++i) {
    puff *p = &S.pf[k][i];
    if (!p->on) continue;
    const float *base = p->flag ? e->smoke_at[p->emitter] : q[p->emitter];
    for (int c = 0; c < 3; ++c) p->p[c] = base[c] - (float)p->age * p->dir[c];
    p->size *= ROLLUP_GROW;
    if (p->size > ROLLUP_GROW_MAX) p->size = ROLLUP_GROW_MAX;
    p->alpha -= p->decay;
    p->age++;
    if (p->alpha <= 0.0f || p->age >= ROLLUP_LIFE) p->on = 0;
    else count++;
  }
  if (++S.pf_n[k] % ROLLUP_EVERY == 0) return;
  float len = sqrtf(e->vel[0] * e->vel[0] + e->vel[1] * e->vel[1] + e->vel[2] * e->vel[2]);
  if (len <= 0.0f) return;
  int r = (int)len % ROLLUP_N;
  float seed = ((float)(ROLLUP_N / 2) + (float)r) / (float)ROLLUP_N * 0.5f;
  for (int j = 0; j < 2; ++j) {
    float sk = k == 0 && smoke_force() > 0.0f ? smoke_force() : e->smoke_k[j], mark = k == 0 && smoke_force() > 0.0f ? 1.0f : e->track[2 + j];
    if (sk == 0.0f || mark <= SMOKE_MIN || count >= SMOKE_MAX) continue;
    float a = fmaxf(sk * ROLLUP_ALPHA, mark > 1.0f ? 1.0f : mark);
    if (a > 1.0f) a = 1.0f;
    puff *p = NULL;
    for (int i = 0; i < SMOKE_MAX && !p; ++i)
      if (!S.pf[k][i].on) p = &S.pf[k][i];
    if (!p) continue;
    if (getenv("WMMT_SMOKE_LOG") && k == 0) fprintf(stderr, "smoke: tyre %d strength %.3f speed %.2f r %d count %d\n", j, (double)e->smoke_k[j], (double)len, r, count);
    p->flag = r & 1, p->emitter = j, p->age = 0, p->on = 1;
    p->alpha = a, p->decay = a / (float)ROLLUP_LIFE, p->size = seed * seed;
    for (int c = 0; c < 3; ++c) p->dir[c] = e->vel[c] / len, p->p[c] = p->flag ? e->smoke_at[j][c] : q[j][c];
    count++;
  }
}

void spark_step(int k, const float *body, const car_effect *e) {
  if (k < 0 || k >= SPARK_CARS) return;
  if (!S.ready) spark_reset();
  if (!e) {
    for (int i = 0; i < SPARK_MAX; ++i) S.s[k][i].life = -1;
    memset(S.t[k], 0, sizeof S.t[k]);
    for (int i = 0; i < SMOKE_MAX; ++i) S.pf[k][i].on = 0;
    for (int i = 0; i < 4; ++i) S.bf[k][i].state = -1;
    return;
  }
  backfire_advance(k);
  backfire_step(k, body, e);
  for (int w = 0; w < 4; ++w) {
    float a = e->track[w];
    track_step(k, w, e->wheel[w], a < 0 ? 0 : a > 1 ? 1 : a);
  }
  smoke_step(k, e);
  memcpy(S.pos[k], body + 12, sizeof S.pos[k]);
  if (e->spark != 0.0f) create(k, e);
  for (int i = 0; i < SPARK_MAX; ++i) {
    spark *s = &S.s[k][i];
    if (s->life == -1) continue;
    memcpy(s->p[0], s->p[1], sizeof s->p[0]);
    memcpy(s->p[2], s->p[3], sizeof s->p[0]);
    memcpy(s->p[1], s->p[4], sizeof s->p[0]);
    memcpy(s->p[3], s->p[5], sizeof s->p[0]);
    for (int j = 0; j < 3; ++j) {
      s->p[4][j] += s->v[j];
      s->p[5][j] += s->v[j];
    }
    s->v[1] -= SPARK_GRAVITY;
    if (--s->life < 0) s->life = -1;
  }
}

/* clCarSparkPrimitive::draw (0x8622d68): the triangles (A B C) (B C D) (B E D) (E D F), as
 * the quads B D C A and D B E F; u across (A B E 0, C D F 1), v along (A C 0, B D 0.5, E F
 * 1); all in clCarSparkPrimitive's colour (240, 212, 0). clCarSparkManager::draw: depth
 * tested, not written, glBlendFunc(GL_SRC_COLOR, GL_ONE). */
void spark_draw(const mat4 *proj, const mat4 *view, render_params *rp) {
  static const int QUAD[2][4] = { { 1, 3, 2, 0 }, { 3, 1, 4, 5 } };
  static const float U[6] = { 0, 0, 1, 1, 0, 1 }, V[6] = { 0, 0.5f, 0, 0.5f, 1, 1 };
  if (!S.ready) return;
  const float *m = view->m;
  int n[2] = { 0, 0 };
  for (int k = 0; k < SPARK_CARS; ++k)
    for (int i = 0; i < SPARK_MAX; ++i) {
      const spark *s = &S.s[k][i];
      if (s->life == -1) continue;
      float q[6][3];
      for (int c = 0; c < 6; ++c) {
        float p[3];
        for (int j = 0; j < 3; ++j) p[j] = S.pos[k][j] + L.shift[k][j] + s->rel[j] + s->p[c][j];
        for (int j = 0; j < 3; ++j) q[c][j] = m[j] * p[0] + m[4 + j] * p[1] + m[8 + j] * p[2] + m[12 + j];
      }
      for (int h = 0; h < 2; ++h) {
        render_vertex *v = S.v[s->tex] + 4 * n[s->tex]++;
        for (int c = 0; c < 4; ++c) {
          int at = QUAD[h][c];
          memcpy(v[c].pos, q[at], sizeof v[c].pos);
          v[c].uv[0] = U[at];
          v[c].uv[1] = V[at];
          v[c].rgba[0] = 240;
          v[c].rgba[1] = 212;
          v[c].rgba[2] = 0;
          v[c].rgba[3] = 255;
        }
      }
    }
  for (int t = 0; t < 2; ++t) {
    uint32_t tex = lights_spark_texture(t);
    if (n[t] && tex) render_sprites_by_colour(proj, S.v[t], n[t], tex, rp);
  }
}

void spark_draw_backfire(const mat4 *proj, const mat4 *view, render_params *rp) {
  if (!S.ready) return;
  const float *m = view->m;
  int n[BACKFIRE_TEXTURES] = { 0 };
  for (int k = 0; k < SPARK_CARS; ++k) {
    const backfire_car *c = S.bf_car[k];
    if (!c) continue;
    const float *b = S.bf_body[k];
    for (int i = 0; i < c->n; ++i) {
      const backfire *f = &S.bf[k][i];
      if (f->state < 0) continue;
      const backfire_frame *fr = &BACKFIRE_FRAME[f->group][f->state];
      for (int t = 0; t < fr->n; ++t) {
        const backfire_tri *tri = &BACKFIRE_TRI[fr->first + t];
        if (tri->tex >= BACKFIRE_TEXTURES || n[tri->tex] >= BACKFIRE_TRIS_MAX) continue;
        render_vertex *v = S.bv[tri->tex] + 4 * n[tri->tex]++;
        for (int q = 0; q < 4; ++q) {
          const float *src = tri->v[q < 3 ? q : 2]; /* (the triangle as a quad, its last corner twice) */
          float l[3], w[3];
          for (int j = 0; j < 3; ++j) l[j] = c->at[i][j] + 0.5f * src[j];
          for (int j = 0; j < 3; ++j) w[j] = b[j] * l[0] + b[4 + j] * l[1] + b[8 + j] * l[2] + b[12 + j] + L.shift[k][j];
          for (int j = 0; j < 3; ++j) v[q].pos[j] = m[j] * w[0] + m[4 + j] * w[1] + m[8 + j] * w[2] + m[12 + j];
          v[q].uv[0] = src[3];
          v[q].uv[1] = src[4];
          v[q].rgba[0] = v[q].rgba[1] = v[q].rgba[2] = v[q].rgba[3] = 255;
        }
      }
    }
  }
  for (int t = 0; t < BACKFIRE_TEXTURES; ++t) {
    uint32_t tex = lights_backfire_texture(t);
    if (n[t] && tex) render_sprites(proj, S.bv[t], n[t], tex, rp);
  }
}

/* the smoke: camera-facing quads, 2 * size across by size high, 2 units up from the particle (clPrimitive::draw), white with the particle's alpha, alpha blended */
void spark_draw_smoke(const mat4 *proj, const mat4 *view, render_params *rp) {
  uint32_t tex = lights_smoke_texture();
  if (!S.ready || !tex) return;
  const float *m = view->m;
  int n = 0;
  for (int k = 0; k < SPARK_CARS; ++k)
    for (int i = 0; i < SMOKE_MAX; ++i) {
      const puff *q = &S.pf[k][i];
      if (!q->on) continue;
      float c[3];
      const float pp[3] = { q->p[0] + L.shift[k][0], q->p[1] + L.shift[k][1], q->p[2] + L.shift[k][2] };
      for (int j = 0; j < 3; ++j) c[j] = m[j] * pp[0] + m[4 + j] * pp[1] + m[8 + j] * pp[2] + m[12 + j];
      if (c[2] > -1.0f) continue;
      float hw = ROLLUP_SIZE * q->size, hh = 0.5f * ROLLUP_SIZE * q->size, cy = c[1] + ROLLUP_SIZE;
      const float xs[4] = { c[0] - hw, c[0] + hw, c[0] + hw, c[0] - hw }, ys[4] = { cy + hh, cy + hh, cy - hh, cy - hh };
      static const float us[4] = { 0, 1, 1, 0 }, vs[4] = { 0, 0, 1, 1 };
      render_vertex *v = S.pv + 4 * n++;
      for (int j = 0; j < 4; ++j) {
        v[j].pos[0] = xs[j], v[j].pos[1] = ys[j], v[j].pos[2] = c[2];
        v[j].uv[0] = us[j], v[j].uv[1] = vs[j];
        v[j].rgba[0] = v[j].rgba[1] = v[j].rgba[2] = 255;
        v[j].rgba[3] = (uint8_t)(q->alpha < 0 ? 0 : q->alpha > 1 ? 255.0f : q->alpha * 255.0f);
      }
    }
  if (n) render_sprites_over(proj, S.pv, n, tex, rp);
}

#define SKID_SHADE 110 /* the tyre marks' colour (of 255): the picture is multiplied by it, lower is darker */
void spark_draw_tracks(const mat4 *proj, const mat4 *view, render_params *rp) {
  uint32_t tex = lights_track_texture();
  if (!S.ready || !tex) return;
  const float *m = view->m;
  int q = 0;
  for (int k = 0; k < SPARK_CARS; ++k)
    for (int w = 0; w < 4; ++w) {
      const track *t = &S.t[k][w];
      track_node eased[TRACK_NODES]; /* (smooth motion: the newest section not yet where the wheel is, but the part of the way the car's drawing has come) */
      if (L.alpha < 1.0f && t->count >= 4) {
        memcpy(eased, t->n, sizeof eased[0] * (size_t)t->count);
        for (int e = t->count - 2; e < t->count; ++e) {
          const track_node *old = &t->n[e - 2];
          float d[3] = { t->n[e].p[0] - old->p[0], t->n[e].p[1] - old->p[1], t->n[e].p[2] - old->p[2] };
          if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > 6400.0f) continue; /* (a jump: as it is) */
          for (int x = 0; x < 3; ++x) eased[e].p[x] = old->p[x] + d[x] * L.alpha;
          eased[e].v = old->v + (t->n[e].v - old->v) * L.alpha;
        }
      }
      const track_node *nodes = L.alpha < 1.0f && t->count >= 4 ? eased : t->n;
      /* each two sections a quad: (L0 R0 R1 L1), the strip's, skipped while all clear */
      for (int i = 0; i + 3 < t->count; i += 2) {
        const track_node *c[4] = { &nodes[i], &nodes[i + 1], &nodes[i + 3], &nodes[i + 2] };
        if (c[0]->a <= 0 && c[2]->a <= 0) continue;
        render_vertex *v = S.tv + 4 * q++;
        for (int j = 0; j < 4; ++j) {
          const float *p = c[j]->p;
          for (int x = 0; x < 3; ++x) v[j].pos[x] = m[x] * p[0] + m[4 + x] * p[1] + m[8 + x] * p[2] + m[12 + x];
          v[j].uv[0] = c[j]->u;
          v[j].uv[1] = c[j]->v;
          v[j].rgba[0] = v[j].rgba[1] = v[j].rgba[2] = SKID_SHADE; /* (the marks darker than the road under them) */
          v[j].rgba[3] = (uint8_t)(c[j]->a * 255.0f + 0.5f);
        }
      }
    }
  if (q) render_sprites_over(proj, S.tv, q, tex, rp);
}

/* ---- the route console's walls (clCourseJunctionManager, clCourseJunction): at a junction two boards stand across the road in front of the car, the
 * left's and the right's, each a frame (divergence_frame, its cell by side: u 0..0.25 the left's, 0.5..0.75 the right's, the next quarter
 * the same dimmed) with its arrow (divergence_arrow01 the left's, 02 the right's: 2 x 2 cells by the board's type). Made as the junction goes to
 * deciding (setup, on the junction's lines: yt::CSeq::GetInfoJctDecideLine), both pulse (0.5..1 over 30 frames); decided, the chosen one (state 2)
 * and the other (3, in its dimmed cell) fade out over 60 frames. Drawn depth tested, not written, glBlendFunc(GL_SRC_COLOR, GL_ONE), the colour
 * the pulse's grey. The sizes are effect_config.lua's JUNCTION_PLATE_SIZE_Y (85) and JUNCTION_ARROW_SIZE_X, _Y (70). The numbers here are what
 * the arcade's own setup and update leave (run under Unicorn: tools-wsl/jct/emu_jct*.py). ---- */
#define JCT_PLATE_H 85.0f
#define JCT_ARROW 70.0f
static struct {
  int prev, state[2], count[2], type[2];
  float p[2][3], d[2][3]; /* each board's two ends: setup (type, P, D): the left's are the left point and the span's, the right's the span's and the right point */
} J;

void junction_reset(void) { memset(&J, 0, sizeof J); }

/* one arcade frame: the junction's state (0 none, 1 deciding, 2 left chosen, 3 right chosen) and its areas and lines from the arcade */
void junction_step(const car_race *r, int on) {
  int s = on ? r->jct_state : 0;
  if (s != J.prev) {
    J.prev = s;
    if (s == 1) {
      const short *row = hud_jct_dest(r->jct_area[0], r->jct_area[1]);
      J.type[0] = row[2], J.type[1] = row[4];
      memcpy(J.p[0], r->jct_line[0], sizeof J.p[0]);
      memcpy(J.d[0], r->jct_line[2], sizeof J.d[0]);
      memcpy(J.p[1], r->jct_line[2], sizeof J.p[1]);
      memcpy(J.d[1], r->jct_line[1], sizeof J.d[1]);
      J.state[0] = J.state[1] = 1;
    } else if (s == 2 || s == 3) {
      J.state[0] = s == 2 ? 2 : 3;
      J.state[1] = s == 2 ? 3 : 2;
    } else {
      J.state[0] = J.state[1] = 0;
    }
    J.count[0] = J.count[1] = 0;
  }
  for (int k = 0; k < 2; ++k) J.count[k] += J.state[k] != 0 && !(J.state[k] > 1 && J.count[k] >= 60);
}

/* the cell of each picture: {uL, uR, vTop, vBot} (clCourseJunction::setup and ::update under Unicorn) */
static void junction_cell(int side, int state, int type, float plate[4], float arrow[4]) {
  static const float FRAME[2][2][4] = { { { 0, 0.25f, 1, 0 }, { 0.25f, 0.5f, 1, 0 } }, { { 0.5f, 0.75f, 1, 0 }, { 0.75f, 1, 1, 0 } } };
  static const float ARROW[2][2][2][4] = {
    { { { 0, 0.5f, 1, 0.5f }, { 0.5f, 1, 1, 0.5f } }, { { 0, 0.5f, 0.5f, 0 }, { 0.5f, 1, 0.5f, 0 } } },
    { { { 0.5f, 0, 1, 0.5f }, { 1, 0.5f, 1, 0.5f } }, { { 0, 0.5f, 0.5f, 0 }, { 0.5f, 1, 0.5f, 0 } } },
  };
  int dim = state == 3;
  memcpy(plate, FRAME[side][dim], 4 * sizeof(float));
  memcpy(arrow, ARROW[side][type & 1][dim], 4 * sizeof(float));
}

void junction_draw(const mat4 *proj, const mat4 *view, render_params *rp) {
  static int tex_frame = -2, tex_arrow[2];
  if (tex_frame == -2) {
    tex_frame = anm_sprite(JID(1385));
    tex_arrow[0] = anm_sprite(JID(2005));
    tex_arrow[1] = anm_sprite(JID(2006));
  }
  if (!J.state[0] && !J.state[1]) return;
  const float *m = view->m;
  for (int k = 0; k < 2; ++k) {
    if (!J.state[k] || (J.state[k] > 1 && J.count[k] >= 60)) continue;
    float level = J.state[k] == 1 ? 0.75f + 0.25f * sinf(6.2831853f * (float)(J.count[k] % 30 + 1) / 30.0f) : 1.0f - (float)J.count[k] / 60.0f;
    if (level <= 0.0f) continue;
    unsigned char c = (unsigned char)(255.0f * (level > 1.0f ? 1.0f : level) + 0.5f);
    float pc[4], ac[4];
    junction_cell(k, J.state[k], J.type[k], pc, ac);
    const float *P = J.p[k], *D = J.d[k];
    /* the frame: P and D, up by the plate's height; the arrow: the middle's span 70 across along the line, centred up the plate */
    float dir[3] = { P[0] - D[0], P[1] - D[1], P[2] - D[2] }, len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len < 1e-3f) continue;
    float mid[3] = { 0.5f * (P[0] + D[0]), 0.5f * (P[1] + D[1]), 0.5f * (P[2] + D[2]) }, u[3];
    for (int j = 0; j < 3; ++j) u[j] = dir[j] / len * 0.5f * JCT_ARROW;
    float wp[2][4][3] = {
      { { P[0], P[1] + JCT_PLATE_H, P[2] }, { D[0], D[1] + JCT_PLATE_H, D[2] }, { D[0], D[1], D[2] }, { P[0], P[1], P[2] } },
      { { mid[0] + u[0], mid[1] + u[1] + 0.5f * JCT_PLATE_H + 0.5f * JCT_ARROW, mid[2] + u[2] },
        { mid[0] - u[0], mid[1] - u[1] + 0.5f * JCT_PLATE_H + 0.5f * JCT_ARROW, mid[2] - u[2] },
        { mid[0] - u[0], mid[1] - u[1] + 0.5f * JCT_PLATE_H - 0.5f * JCT_ARROW, mid[2] - u[2] },
        { mid[0] + u[0], mid[1] + u[1] + 0.5f * JCT_PLATE_H - 0.5f * JCT_ARROW, mid[2] + u[2] } },
    };
    const float *cell[2] = { pc, ac };
    unsigned tex[2] = { tex_frame >= 0 ? anm_sprite_texture(tex_frame) : 0u, tex_arrow[k] >= 0 ? anm_sprite_texture(tex_arrow[k]) : 0u };
    for (int q = 0; q < 2; ++q) {
      if (!tex[q]) continue;
      render_vertex v[4];
      static const int UCOL[4] = { 0, 1, 1, 0 }, VROW[4] = { 0, 0, 1, 1 };
      for (int i = 0; i < 4; ++i) {
        const float *p = wp[q][i];
        for (int j = 0; j < 3; ++j) v[i].pos[j] = m[j] * p[0] + m[4 + j] * p[1] + m[8 + j] * p[2] + m[12 + j];
        v[i].uv[0] = cell[q][UCOL[i]], v[i].uv[1] = 1.0f - cell[q][2 + VROW[i]]; /* (the arcade's pictures are bottom up) */
        v[i].rgba[0] = v[i].rgba[1] = v[i].rgba[2] = c, v[i].rgba[3] = 255;
      }
      render_sprites_by_colour(proj, v, 1, tex[q], rp);
    }
  }
}
