/* The sparks off a car scraping a wall: the arcade's clCarSparkManager (one a car, under its
 * clCarEffectManager), run each arcade frame and drawn additive after the scene. */
#ifndef SPARK_H
#define SPARK_H

#include "car.h"
#include "mathx.h"
#include "render.h"

#define SPARK_CARS (1 + CAR_RIVALS) /* the player's, then the rivals' */

/* none flying (a new race) */
void spark_reset(void);
/* One arcade frame of car k's sparks: new ones while e says it scrapes (e NULL: the car is
 * off the road, its sparks go), then all of them fly on; body is its model matrix (they keep
 * to its position) */
void spark_step(int k, const float *body, const car_effect *e);
/* Draw them all, as the lights (render_begin in effect) */
void spark_draw(const mat4 *proj, const mat4 *view, render_params *rp);
/* The cars' tyre marks (clTireTrackManager, laid in spark_step): car k's tyre radius and tread
 * (display units), and the marks drawn over the road, before the lights */
void spark_tyre(int k, float radius, float tread);
void spark_draw_tracks(const mat4 *proj, const mat4 *view, render_params *rp);
/* The tyre smoke of a sliding car (laid in spark_step) */
void spark_draw_smoke(const mat4 *proj, const mat4 *view, render_params *rp);
/* The cars' backfire (clBackFireManager, started and run in spark_step): car k's model (its
 * CARDB_PACKS pack, -1 none, body 0 stock, 1..3 aero A..C, and whether it has the tuned exhaust) for its exhausts; and the fires
 * drawn, as the sparks */
void spark_backfire_car(int k, int pack, int body, int tuned_exhaust);
void spark_draw_backfire(const mat4 *proj, const mat4 *view, render_params *rp);
/* Smooth motion (app.c car_pose): the cars are drawn alpha of the way from the last step's state to the current one, not at the current: what the effects keep to a car
 * (the sparks, the smoke, the backfire: shift[k], the car's place drawn less its place in the step) is drawn moved with it, and a tyre mark's newest section only that far
 * on its way. alpha 1 (or shift NULL): as it was. */
void spark_blend(const float (*shift)[3], float alpha);
/* The route console's walls at a junction (clCourseJunctionManager): none up (a new race), one arcade frame of them from the race's junction state (on: a versus racing), and drawn, as the sparks */
void junction_reset(void);
void junction_step(const car_race *r, int on);
void junction_draw(const mat4 *proj, const mat4 *view, render_params *rp);

#endif
