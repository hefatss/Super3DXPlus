/* The versus's client: see net.h and tools/vs_server.py (the protocol). */
#include "net.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "car.h"
#include "netdiag.h"
#include "netsock.h"

enum { HELLO = 1, LOBBY, START, GO, RACE, BYE, DISCOVER, OFFER, CLOSED };
#define SCAN_EVERY 1.0
#define SCAN_GONE 4.0
#define HELLO_EVERY 0.5
#define PACKET_COUNT 0x34 /* SLxPacket: CLinx's frame count (older packets are dropped) */

static struct {
  net_fd fd; /* the server's UDP socket */
  uint32_t id;
  double last_hello, last_heard, now;
  int last_status;
  int heard;
  net_lobby lobby;
  net_go go;
  int go_new;
  int closed; /* the host's CLOSED came */
  uint32_t race; /* the race this machine is in (0 none) */
  /* the newest race packet from each machine, not yet handed over */
  unsigned char rx[CAR_LINK_MACHINES][CAR_LINK_PACKET];
  int rx_new[CAR_LINK_MACHINES];
  uint32_t rx_count[CAR_LINK_MACHINES];
  int rx_any[CAR_LINK_MACHINES];
  int rx_ping[CAR_LINK_MACHINES]; /* each machine's own round trip to the host (ms) as its packets carry it after the car link packet; -1 none */
  double rx_ping_t[CAR_LINK_MACHINES];
  int rx_flash[CAR_LINK_MACHINES]; /* each machine's headlights flashing (off for this step), after the ping in its packets */
} N = { .fd = NET_NOFD };

static void put32(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v, p[1] = (unsigned char)(v >> 8), p[2] = (unsigned char)(v >> 16), p[3] = (unsigned char)(v >> 24);
}
static uint32_t get32(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

static void head(unsigned char *p, int kind) { p[0] = 'W', p[1] = 'V', p[2] = 1, p[3] = (unsigned char)kind; }

static void tx(const unsigned char *b, int len) {
  if (net_fd_ok(N.fd)) send(N.fd, (const char *)b, len, 0);
}

/* ---- measuring (netdiag.h) ---- */
enum { PING = 10, PONG };
static double mono(void) {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (double)c.QuadPart / (double)f.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

/* the artificial bad connection: received packets wait in a queue for their time (MT3DX_NET_LAG) */
#define LAGQ 128
static struct {
  unsigned char b[2048];
  int n;
  double due;
} Q[LAGQ];
static int q_head, q_n;
static double lag_ms, lag_jit, lag_loss;
static int lag_init;
static void lag_setup(void) {
  if (lag_init) return;
  lag_init = 1;
  const char *e = getenv("MT3DX_NET_LAG");
  if (e) sscanf(e, "%lf,%lf,%lf", &lag_ms, &lag_jit, &lag_loss);
  if (lag_ms > 0 || lag_loss > 0) fprintf(stderr, "versus net: artificial lag %.0f ms +-%.0f, loss %.0f%% of what this machine receives\n", lag_ms, lag_jit, lag_loss);
}

static struct {
  int n, lost, late, any, age_last, age_min, age_max;
  double last_t, gap_sum, gap_max;
  long age_sum;
  uint32_t last_count;
} St[CAR_LINK_MACHINES];
static double rtt = -1, ping_at, log_at;
static long tot_n, tot_lost, tot_late; /* the last race's packets from the others, all machines */
static double tot_gap_max;
static int log_forced; /* NETWORK OPTIONS' DETAILED NET LOG */
static uint32_t tx_count;
static double ema[CAR_LINK_MACHINES]; /* the age (frames) of each machine's data, smoothed over some packets */
static int ema_n[CAR_LINK_MACHINES];

void net_log_force(int on) { log_forced = on != 0; }
int net_log_on(void) {
  static int on = -1;
  if (on < 0) on = getenv("MT3DX_NETLOG") != NULL && getenv("MT3DX_NETLOG")[0] != '0';
  return on || log_forced;
}

/* a line for the network options: the last race's packets from the others, those that never came and those that came late, and the longest wait between two */
void net_stats_text(char *out, int n) {
  if (!tot_n) snprintf(out, (size_t)n, "NO RACE YET, SO NO PACKET STATISTICS");
  else snprintf(out, (size_t)n, "LAST RACE: %ld PACKETS, %ld LOST, %ld LATE. LONGEST GAP: %.0f MS", tot_n, tot_lost, tot_late, tot_gap_max);
}

int net_age_frames(int pcb) { return pcb >= 0 && pcb < CAR_LINK_MACHINES && St[pcb].any ? St[pcb].age_last : -1000; }
double net_rtt_ms(void) { return rtt; }

/* machine pcb's round trip to the host in ms (its race packets carry it); -1 if not known or it stopped coming */
static int tx_flash; /* this machine's, set by the race (net_set_flash) */
void net_set_flash(int off) { tx_flash = off != 0; }
int net_flash_of(int pcb) { return pcb >= 0 && pcb < CAR_LINK_MACHINES && N.rx_flash[pcb] && mono() - N.rx_ping_t[pcb] < 1.0; }

int net_ping_ms(int pcb) {
  if (pcb < 0 || pcb >= CAR_LINK_MACHINES || N.rx_ping[pcb] < 0 || mono() - N.rx_ping_t[pcb] > 4.0) return -1;
  return N.rx_ping[pcb];
}

/* how far ahead of machine pcb's clock this machine's race clock is, in frames (positive: ahead; the data's delay on the way taken off) */
double net_clock_ahead_frames(int pcb, int *valid) {
  int ok = pcb >= 0 && pcb < CAR_LINK_MACHINES && ema_n[pcb] >= 30 && rtt >= 0;
  if (valid) *valid = ok;
  return ok ? ema[pcb] - 0.5 * rtt * 0.06 : 0.0;
}

static void stat_race(int pcb, uint32_t count, double t) {
  typeof(St[0]) *s = &St[pcb];
  int age = (int)(int32_t)(tx_count - count);
  if (s->any) {
    double gap = (t - s->last_t) * 1000.0;
    int d = (int)(int32_t)(count - s->last_count);
    s->gap_sum += gap;
    if (gap > s->gap_max) s->gap_max = gap;
    if (gap > tot_gap_max) tot_gap_max = gap;
    if (d > 1) s->lost += d - 1, tot_lost += d - 1;
    if (d <= 0) s->late++, tot_late++;
  }
  tot_n++;
  if (!s->n || age < s->age_min) s->age_min = age;
  if (!s->n || age > s->age_max) s->age_max = age;
  ema[pcb] = ema_n[pcb]++ ? ema[pcb] + ((double)age - ema[pcb]) * 0.15 : (double)age;
  s->age_sum += age, s->age_last = age, s->n++, s->any = 1, s->last_t = t, s->last_count = count;
}

/* every 2 s of a race: what the others' packets were like (then the window starts again) */
static void stat_log(double t) {
  if (!N.race || t - log_at < 2.0) return;
  log_at = t;
  for (int k = 0; k < CAR_LINK_MACHINES; ++k) {
    if (!St[k].n) continue;
    NETLOG("versus net: from pcb %d: %d packets, age %d..%d (avg %.1f) frames, gap avg %.1f max %.0f ms, lost %d, late %d, rtt %.0f ms\n", k, St[k].n,
            St[k].age_min, St[k].age_max, (double)St[k].age_sum / (double)St[k].n, St[k].n > 1 ? St[k].gap_sum / (double)(St[k].n - 1) : 0.0, St[k].gap_max,
            St[k].lost, St[k].late, rtt);
    St[k].n = St[k].lost = St[k].late = 0, St[k].age_sum = 0, St[k].gap_sum = St[k].gap_max = 0;
    St[k].any = 0; /* (the next window's first packet has no gap to measure) */
  }
}

static int rx_raw(unsigned char *b, int max) {
  net_ssize n = recv(N.fd, (char *)b, max, 0);
  return n > 0 ? (int)n : 0;
}

static int rx(unsigned char *b, int max) {
  if (!net_fd_ok(N.fd)) return 0;
  lag_setup();
  if (lag_ms <= 0 && lag_loss <= 0) return rx_raw(b, max);
  double t = mono();
  unsigned char tmp[2048];
  for (int n; (n = rx_raw(tmp, sizeof tmp)) > 0;) {
    if (lag_loss > 0 && (double)(rand() % 1000) < lag_loss * 10.0) continue;
    if (q_n >= LAGQ) continue;
    int at = (q_head + q_n++) % LAGQ;
    memcpy(Q[at].b, tmp, (size_t)n);
    Q[at].n = n;
    Q[at].due = t + (lag_ms + (lag_jit > 0 ? (double)(rand() % 2001 - 1000) / 1000.0 * lag_jit : 0.0)) * 1e-3;
  }
  for (int i = 0; i < q_n; ++i) { /* (the first that is due: a jitter can put a later one first) */
    int at = (q_head + i) % LAGQ;
    if (Q[at].due > t || Q[at].n > max) continue;
    int n = Q[at].n;
    memcpy(b, Q[at].b, (size_t)n);
    Q[at] = Q[q_head]; /* (the head's place filled by the one leaving: the order of the rest does not matter, each has its time) */
    q_head = (q_head + 1) % LAGQ, q_n--;
    return n;
  }
  return 0;
}

int net_open(const char *host, int port, uint32_t id) {
  net_close();
  memset(St, 0, sizeof St), memset(ema_n, 0, sizeof ema_n), q_n = q_head = 0, rtt = -1, ping_at = log_at = 0, tx_count = 0;
  if (net_sock_init() != 0) return -1;
  struct sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_port = htons((uint16_t)port);
  if (inet_pton(AF_INET, host, &a.sin_addr) != 1) return -1;
  net_fd fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!net_fd_ok(fd)) return -1;
  net_fd_nonblock(fd);
  if (connect(fd, (struct sockaddr *)&a, sizeof a) != 0) {
    net_fd_close(fd);
    return -1;
  }
  memset(&N, 0, sizeof N);
  N.fd = fd;
  N.id = id;
  N.last_hello = -1e9;
  return 0;
}

void net_close(void) {
  if (!net_is_open()) return;
  unsigned char b[8];
  head(b, BYE);
  put32(b + 4, N.id);
  tx(b, sizeof b);
  if (net_fd_ok(N.fd)) net_fd_close(N.fd);
  memset(&N, 0, sizeof N);
  N.fd = NET_NOFD;
}

int net_is_open(void) { return net_fd_ok(N.fd); }

const net_lobby *net_lobby_now(void) { return &N.lobby; }

double net_silence(double now) { return N.heard ? now - N.last_heard : 1e9; }

/* a player record: u32 id, u8 status (lobby only), char name[16], u8 n, profile[n] */
static int read_player(const unsigned char *p, int len, net_player *pl, int with_status) {
  int need = 4 + with_status + NET_NAME + 1;
  if (len < need) return -1;
  memset(pl, 0, sizeof *pl);
  pl->id = get32(p);
  pl->status = with_status ? p[4] : 1;
  memcpy(pl->name, p + 4 + with_status, NET_NAME);
  int n = p[need - 1];
  if (len < need + n) return -1;
  pl->profile_len = n > NET_PROFILE ? NET_PROFILE : n;
  memcpy(pl->profile, p + need, (size_t)pl->profile_len);
  return need + n;
}

static void got(const unsigned char *b, int len, double now) {
  if (len < 4 || b[0] != 'W' || b[1] != 'V' || b[2] != 1) return;
  const unsigned char *p = b + 4;
  len -= 4;
  if (b[3] == RACE) {
    if (len < 5 + CAR_LINK_PACKET) return;
    uint32_t race = get32(p);
    int pcb = p[4];
    if (race != N.race || pcb < 0 || pcb >= CAR_LINK_MACHINES) return;
    uint32_t count = get32(p + 5 + PACKET_COUNT);
    if (N.rx_any[pcb] && (int32_t)(count - N.rx_count[pcb]) < 0) return; /* came in late */
    memcpy(N.rx[pcb], p + 5, CAR_LINK_PACKET);
    if (len >= 5 + CAR_LINK_PACKET + 2) { /* (a newer machine's: its round trip after the packet; 0xffff not measured yet) */
      int ms = p[5 + CAR_LINK_PACKET] | p[6 + CAR_LINK_PACKET] << 8;
      N.rx_ping[pcb] = ms == 0xffff ? -1 : ms, N.rx_ping_t[pcb] = mono();
      N.rx_flash[pcb] = len >= 5 + CAR_LINK_PACKET + 3 && p[7 + CAR_LINK_PACKET] & 1;
    }
    N.rx_count[pcb] = count;
    stat_race(pcb, count, mono());
    N.rx_any[pcb] = N.rx_new[pcb] = 1;
    N.last_heard = now, N.heard = 1;
    return;
  }
  if (b[3] == PONG) { /* the host's answer to a PING: the time it carried */
    if (len >= 8) rtt = (double)(uint32_t)((uint32_t)(mono() * 1000.0) - get32(p + 4)) + (lag_ms > 0 ? lag_ms : 0.0); /* (the artificial lag delays what is received: the PING's way out had none, the other direction of a real link has it) */
    return;
  }
  if (b[3] == CLOSED) {
    N.closed = 1;
    return;
  }
  if (b[3] == LOBBY) {
    if (len < 9) return;
    net_lobby l;
    memset(&l, 0, sizeof l);
    l.connected = 1;
    l.n = p[0] > NET_PLAYERS ? NET_PLAYERS : p[0];
    l.me = p[1];
    l.host = p[2];
    if (l.n < 1 || l.me >= l.n || l.host >= l.n) return; /* (a lobby naming a place no one is in: from a bad host, thrown away) */
    int at = 9;
    for (int i = 0; i < l.n; ++i) {
      if (at + 1 > len) return;
      int k = read_player(p + at + 1, len - at - 1, &l.player[i], 1);
      if (k < 0) return;
      at += 1 + k;
    }
    N.lobby = l;
    N.last_heard = now, N.heard = 1;
    return;
  }
  if (b[3] == GO) {
    if (len < 12) return;
    net_go g;
    memset(&g, 0, sizeof g);
    g.race = get32(p);
    g.seed = get32(p + 4);
    g.ramp = p[8];
    g.km = p[9];
    g.n = p[10] > NET_PLAYERS ? NET_PLAYERS : p[10];
    g.pcb = p[11];
    if (g.n < 1 || g.pcb >= g.n || (g.ramp & 0x7f) >= NET_RAMPS || g.km < 1 || g.km > 99) return; /* (the numbers the race indexes with, checked here once; the ramp's top bit is the night zone) */
    if (g.race == N.go.race || g.race == N.race) return; /* said again */
    int at = 12;
    for (int i = 0; i < g.n; ++i) {
      if (at + 1 > len) return;
      int pcb = p[at];
      net_player pl;
      int k = read_player(p + at + 1, len - at - 1, &pl, 0);
      if (k < 0 || pcb >= NET_PLAYERS) return;
      g.player[pcb] = pl;
      at += 1 + k;
    }
    if (at < len) g.song = p[at]; /* after the machines: a newer host's */
    N.go = g;
    N.go_new = 1;
    N.last_heard = now, N.heard = 1;
  }
}

void net_poll(double now, int status, const char *name, const unsigned char *profile, int profile_len) {
  if (!net_is_open()) return;
  N.now = now;
  unsigned char b[2048];
  for (int n; (n = rx(b, sizeof b)) > 0;) got(b, n, now);
  double mt = mono();
  if (mt - ping_at >= 1.0) { /* (the round trip to the host, once a second) */
    unsigned char pg[12];
    ping_at = mt;
    head(pg, PING);
    put32(pg + 4, N.id);
    put32(pg + 8, (uint32_t)(mt * 1000.0));
    tx(pg, sizeof pg);
  }
  stat_log(mt);
  /* twice a second, and at once on a new status (the area vote's card) */
  if (now - N.last_hello < HELLO_EVERY && status == N.last_status) return;
  N.last_hello = now;
  N.last_status = status;
  int pl = profile_len < 0 ? 0 : profile_len > NET_PROFILE ? NET_PROFILE : profile_len;
  unsigned char h[4 + 4 + 1 + NET_NAME + 1 + NET_PROFILE];
  head(h, HELLO);
  put32(h + 4, N.id);
  h[8] = (unsigned char)status;
  memset(h + 9, 0, NET_NAME);
  if (name) memcpy(h + 9, name, strlen(name) < NET_NAME ? strlen(name) : NET_NAME);
  h[9 + NET_NAME] = (unsigned char)pl;
  if (pl) memcpy(h + 10 + NET_NAME, profile, (size_t)pl);
  tx(h, 10 + NET_NAME + pl);
}

void net_start(int ramp, int km, int song) {
  if (!net_is_open()) return;
  unsigned char b[7];
  head(b, START);
  b[4] = (unsigned char)ramp;
  b[5] = (unsigned char)km;
  b[6] = (unsigned char)song;
  tx(b, sizeof b);
}

int net_host_closed(void) { return N.closed; }

int net_take_go(net_go *go) {
  if (!N.go_new) return 0;
  N.go_new = 0;
  *go = N.go;
  /* the race's packets from now on */
  N.race = N.go.race;
  memset(N.rx_new, 0, sizeof N.rx_new);
  memset(N.rx_any, 0, sizeof N.rx_any);
  for (int k = 0; k < CAR_LINK_MACHINES; ++k) N.rx_ping[k] = -1;
  tot_n = tot_lost = tot_late = 0, tot_gap_max = 0; /* (the statistics of this race) */
  /* The clock comparison starts over with the race. It kept the last race's: this machine's last frame (tx_count, 12008 at the end of a 3-minute race) against the host's new
   * race's first packets (frame 0) made it "12007 frames ahead", the versus' clock correction dropped every step, and a machine that takes no step sends no packet that would
   * set tx_count right: the race stood still at its start on both machines, for ever (whenever the host's first packet came before this machine's own). */
  memset(St, 0, sizeof St), memset(ema_n, 0, sizeof ema_n), memset(ema, 0, sizeof ema), tx_count = 0;
  return 1;
}

void net_race_send(const unsigned char *packet, int len) {
  if (!net_is_open() || !N.race || len > CAR_LINK_PACKET) return;
  unsigned char b[4 + 5 + CAR_LINK_PACKET + 3];
  head(b, RACE);
  put32(b + 4, N.race);
  b[8] = (unsigned char)N.go.pcb;
  memcpy(b + 9, packet, (size_t)len);
  if (len >= PACKET_COUNT + 4) tx_count = get32(packet + PACKET_COUNT); /* (this machine's frame, to measure the others' packets' age by) */
  int ms = rtt < 0 ? 0xffff : rtt > 60000.0 ? 60000 : (int)(rtt + 0.5);
  b[9 + len] = (unsigned char)ms, b[10 + len] = (unsigned char)(ms >> 8); /* (this machine's round trip to the host, for the others' HUD) */
  b[11 + len] = (unsigned char)tx_flash; /* (and whether its headlights are flashing off) */
  tx(b, 12 + len);
}

void net_race_receive(void) {
  if (!net_is_open()) return;
  unsigned char b[2048];
  for (int n; (n = rx(b, sizeof b)) > 0;) got(b, n, N.now);
  stat_log(mono());
  for (int k = 0; k < CAR_LINK_MACHINES; ++k)
    if (N.rx_new[k]) {
      car_link_deliver(k, N.rx[k], CAR_LINK_PACKET);
      N.rx_new[k] = 0;
    }
}

/* ---- the LAN's games: a DISCOVER to the Wi-Fi's broadcast address (and the loopback, for a
 * host on this machine: the harness) every SCAN_EVERY s, each host's OFFER kept till it has
 * been quiet SCAN_GONE s */
static struct {
  net_fd fd;
  double last;
  net_found found[NET_FOUND];
  int n;
} S = { .fd = NET_NOFD };

void net_scan(double now, int on) {
  if (!on) {
    if (net_fd_ok(S.fd)) net_fd_close(S.fd);
    memset(&S, 0, sizeof S);
    S.fd = NET_NOFD;
    return;
  }
  if (!net_fd_ok(S.fd)) {
    if (net_sock_init() != 0) return;
    S.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!net_fd_ok(S.fd)) return;
    int one = 1;
    setsockopt(S.fd, SOL_SOCKET, SO_BROADCAST, NET_SOCKOPT(&one), sizeof one);
    net_fd_nonblock(S.fd);
    S.last = -1e9;
  }
  unsigned char b[256];
  for (;;) {
    struct sockaddr_in from;
    net_len al = sizeof from;
    net_ssize n = recvfrom(S.fd, (char *)b, sizeof b, 0, (struct sockaddr *)&from, &al);
    if (n <= 0) break;
    if (n < 9 + NET_NAME || b[0] != 'W' || b[1] != 'V' || b[2] != 1 || b[3] != OFFER) continue;
    char ip[16] = { 0 };
    if (!inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip)) continue;
    uint32_t id = get32(b + 5 + NET_NAME);
    int k = 0;
    while (k < S.n && S.found[k].id != id) k++;
    if (k == S.n) {
      if (S.n >= NET_FOUND) continue;
      S.n++;
      memset(&S.found[k], 0, sizeof S.found[k]);
    }
    net_found *f = &S.found[k];
    /* (the loopback's answer only till the Wi-Fi's comes) */
    int loop = from.sin_addr.s_addr == htonl(INADDR_LOOPBACK);
    if (!f->ip[0] || !loop) memcpy(f->ip, ip, sizeof f->ip);
    f->id = id;
    f->port = ntohs(from.sin_port); /* (the host answers from its own port, which may not be the usual one) */
    memcpy(f->name, b + 5, NET_NAME);
    f->name[NET_NAME] = 0;
    f->n = b[4];
    f->seen = now;
  }
  for (int k = S.n - 1; k >= 0; --k)
    if (now - S.found[k].seen > SCAN_GONE) memmove(&S.found[k], &S.found[k + 1], sizeof S.found[0] * (size_t)(--S.n - k));
  if (now - S.last < SCAN_EVERY) return;
  S.last = now;
  unsigned char d[4];
  head(d, DISCOVER);
  struct sockaddr_in to;
  memset(&to, 0, sizeof to);
  to.sin_family = AF_INET;
  for (int p = 0; p < NET_PORT_TRIES; ++p) { /* (a host on another port than the usual, its own taken: the ports it tries) */
    to.sin_port = htons((uint16_t)(NET_PORT + p));
    to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sendto(S.fd, (const char *)d, sizeof d, 0, (struct sockaddr *)&to, sizeof to);
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(S.fd, (const char *)d, sizeof d, 0, (struct sockaddr *)&to, sizeof to);
  }
}

int net_found_list(net_found *out, int max) {
  int n = S.n < max ? S.n : max;
  memcpy(out, S.found, sizeof out[0] * (size_t)n);
  return n;
}

/* ---- the room code: a game's address and port as one number (ip * 65536 + port, 15 digits), which the keypad can type and which keeps the
 * port; an address typed with its dots is the usual port's */
void net_room_code(const char *ip, int port, char out[24]) {
  struct in_addr a;
  if (inet_pton(AF_INET, ip, &a) != 1) {
    out[0] = 0;
    return;
  }
  unsigned long long v = ((unsigned long long)ntohl(a.s_addr) << 16) | (unsigned)(port & 0xffff);
  snprintf(out, 24, "%015llu", v);
}

int net_room_parse(const char *text, char ip[16], int *port) {
  if (strchr(text, '.')) {
    struct in_addr a;
    if (inet_pton(AF_INET, text, &a) != 1) return -1;
    snprintf(ip, 16, "%s", text);
    *port = NET_PORT;
    return 0;
  }
  int n = (int)strlen(text);
  if (n < 10 || n > 15) return -1;
  unsigned long long v = 0;
  for (int i = 0; i < n; ++i) {
    if (text[i] < '0' || text[i] > '9') return -1;
    v = v * 10 + (unsigned)(text[i] - '0');
  }
  if (v >> 48) return -1;
  struct in_addr a;
  a.s_addr = htonl((uint32_t)(v >> 16));
  if (!inet_ntop(AF_INET, &a, ip, 16)) return -1;
  *port = (int)(v & 0xffff);
  return *port > 0 ? 0 : -1;
}

/* a random number from the system (RtlGenRandom on Windows, /dev/urandom elsewhere); the clock and the process's id only if that fails */
#ifdef _WIN32
BOOLEAN WINAPI SystemFunction036(PVOID buffer, ULONG length);
#endif
uint32_t net_random32(void) {
  uint32_t v = 0;
  int ok = 0;
#ifdef _WIN32
  ok = SystemFunction036(&v, sizeof v) != 0;
#else
  FILE *f = fopen("/dev/urandom", "rb");
  if (f) {
    ok = fread(&v, sizeof v, 1, f) == 1;
    fclose(f);
  }
#endif
  if (!ok) v = (uint32_t)time(NULL) * 2654435761u ^ (uint32_t)clock();
  return v ? v : 1;
}

/* ---- the log's privacy: a log is attached to bug reports in public, so what could identify a player is taken out of it: public IPv4 addresses (a room code is a public
 * address in 15 digits), room codes, and the user's name in a path (C:\Users\name\...). Private LAN addresses (192.168.x.x, 10.x.x.x, 172.16-31.x.x, 169.254.x.x, 127.x.x.x) stay:
 * they say nothing about who the player is and are what a LAN problem is diagnosed from. The platforms scrub the last run's log file at the start (a crashed run too) and
 * their own at a clean exit (log_scrub_file); the log lines that name an address are masked where they are written (log_mask_ip) as well. ---- */
static int ip_is_private(int a, int b) {
  return a == 10 || a == 127 || a == 0 || a >= 224 || (a == 192 && b == 168) || (a == 172 && b >= 16 && b <= 31) || (a == 169 && b == 254) || (a == 100 && b >= 64 && b <= 127);
}

/* a dotted address at in[i..]: its length (0 if not one), its first two numbers */
static int ip_at(const char *in, int i, int *a, int *b) {
  int v[4], k = i;
  for (int part = 0; part < 4; ++part) {
    int d = 0, n = 0;
    while (in[k] >= '0' && in[k] <= '9' && d < 4) n = n * 10 + (in[k++] - '0'), ++d;
    if (d < 1 || d > 3 || n > 255) return 0;
    v[part] = n;
    if (part < 3) {
      if (in[k] != '.') return 0;
      ++k;
    }
  }
  if (in[k] >= '0' && in[k] <= '9') return 0;
  if (in[k] == '.' && in[k + 1] >= '0' && in[k + 1] <= '9') return 0; /* (a longer version number) */
  *a = v[0], *b = v[1];
  return k - i;
}

static int digits_at(const char *in, int i, int n) {
  for (int k = 0; k < n; ++k)
    if (in[i + k] < '0' || in[i + k] > '9') return 0;
  return 1;
}

/* one line scrubbed into out (at most cap - 1 bytes) */
static void scrub_line(const char *in, char *out, size_t cap) {
  size_t o = 0;
  int n = (int)strlen(in);
  for (int i = 0; i < n && o + 24 < cap;) {
    char prev = i ? in[i - 1] : ' ';
    int word_start = !((prev >= '0' && prev <= '9') || prev == '.' || (prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z'));
    if (word_start && in[i] >= '0' && in[i] <= '9') {
      /* a room code: 5-5-5 digits, or 15 */
      if (digits_at(in, i, 5) && in[i + 5] == '-' && digits_at(in, i + 6, 5) && in[i + 11] == '-' && digits_at(in, i + 12, 5) && !(in[i + 17] >= '0' && in[i + 17] <= '9')) {
        o += (size_t)snprintf(out + o, cap - o, "[room code hidden]");
        i += 17;
        continue;
      }
      if (digits_at(in, i, 15) && !(in[i + 15] >= '0' && in[i + 15] <= '9')) {
        o += (size_t)snprintf(out + o, cap - o, "[room code hidden]");
        i += 15;
        continue;
      }
      int a = 0, b = 0, len = ip_at(in, i, &a, &b);
      if (len && !ip_is_private(a, b)) {
        o += (size_t)snprintf(out + o, cap - o, "[public ip hidden]");
        i += len;
        continue;
      }
    }
    /* Users\name or Users/name: the name hidden */
    if ((in[i] == 'U' || in[i] == 'u') && !strncmp(in + i + 1, "sers", 4) && (in[i + 5] == '\\' || in[i + 5] == '/')) {
      out[o++] = in[i];
      memcpy(out + o, "sers", 4), o += 4;
      out[o++] = in[i + 5];
      int k = i + 6;
      while (in[k] && in[k] != '\\' && in[k] != '/' && in[k] != '"' && in[k] != '\'' && in[k] != ' ' && in[k] != '\n' && in[k] != '\r') ++k;
      if (k > i + 6) o += (size_t)snprintf(out + o, cap - o, "<user>");
      i = k;
      continue;
    }
    out[o++] = in[i++];
  }
  out[o] = 0;
}

/* a log file scrubbed in place (a missing one is left alone); 0 when done */
int log_scrub_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size <= 0 || size > 64L * 1024 * 1024) {
    fclose(f);
    return -1;
  }
  char *text = malloc((size_t)size + 1);
  if (!text || fread(text, 1, (size_t)size, f) != (size_t)size) {
    free(text);
    fclose(f);
    return -1;
  }
  fclose(f);
  text[size] = 0;
  char tmp[1300];
  snprintf(tmp, sizeof tmp, "%s.scrub", path);
  FILE *out = fopen(tmp, "wb");
  if (!out) {
    free(text);
    return -1;
  }
  char line[2048], clean[4096];
  for (char *p = text; *p;) {
    char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    if (len >= sizeof line) len = sizeof line - 1;
    memcpy(line, p, len);
    line[len] = 0;
    scrub_line(line, clean, sizeof clean);
    fputs(clean, out);
    if (e) fputc('\n', out);
    p = e ? e + 1 : p + strlen(p);
  }
  fclose(out);
  free(text);
  remove(path);
  return rename(tmp, path);
}

/* an address for a log line: kept if it is a private one, else hidden */
void log_mask_ip(const char *ip, char out[24]) {
  int a = 0, b = 0;
  if (ip && ip_at(ip, 0, &a, &b) && ip_is_private(a, b)) snprintf(out, 24, "%.20s", ip);
  else snprintf(out, 24, "[public ip hidden]");
}
