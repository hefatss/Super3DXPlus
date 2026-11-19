/* The versus host in the app (LAN versus): tools/vs_server.py's lobby and race
 * relay, the same protocol, on UDP NET_PORT on every interface (the host's own client comes in
 * through the loopback; the LAN's devices from the Wi-Fi, which find it by a DISCOVER
 * broadcast). */
#include "vs_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "car.h"
#include "net.h"
#include "netsock.h"

enum { HELLO = 1, LOBBY, START, GO, RACE, BYE, DISCOVER, OFFER, CLOSED, PING, PONG };
#define TIMEOUT 6.0
#define TIMEOUT_RACING 20.0 /* a player loading or racing (status 1): its device may stall a while at the start */
#define TIMEOUT_RACED 5.0 /* a machine whose race packets had begun: gone once they stop (the others' cars must not coast on 20 s) */
#define RACES 4

typedef struct {
  struct sockaddr_in a; /* a UDP address */
} addr;

typedef struct {
  uint32_t id;
  addr from;
  int status;
  unsigned char name[NET_NAME];
  unsigned char profile[NET_PROFILE];
  int profile_len;
  double seen;
  int raced; /* its race packets are coming: a silence of TIMEOUT_RACED is its leaving (not a slow start) */
} player;

static struct {
  net_fd fd;
  int open, port;
  uint32_t owner;
  player p[NET_PLAYERS];
  int n;
  int ramp, km, song;
  uint32_t race_id;
  /* the races' machines (by PCB) as the players' ids, the newest RACES */
  uint32_t race[RACES], machine[RACES][NET_PLAYERS];
  int machines[RACES];
} H = { .fd = NET_NOFD };

static void put32(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v, p[1] = (unsigned char)(v >> 8), p[2] = (unsigned char)(v >> 16), p[3] = (unsigned char)(v >> 24);
}
static uint32_t get32(const unsigned char *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void head(unsigned char *p, int kind) { p[0] = 'W', p[1] = 'V', p[2] = 1, p[3] = (unsigned char)kind; }

static int same(const addr *x, const addr *y) {
  return x->a.sin_addr.s_addr == y->a.sin_addr.s_addr && x->a.sin_port == y->a.sin_port;
}

static void send_to(const addr *to, const unsigned char *b, int len) {
  if (net_fd_ok(H.fd)) sendto(H.fd, (const char *)b, len, 0, (const struct sockaddr *)&to->a, sizeof to->a);
}

static int find(const addr *from) {
  for (int i = 0; i < H.n; ++i)
    if (same(&H.p[i].from, from)) return i;
  return -1;
}

static void lobby_to(int i) {
  unsigned char b[1024];
  head(b, LOBBY);
  b[4] = (unsigned char)H.n, b[5] = (unsigned char)i, b[6] = 0, b[7] = (unsigned char)H.ramp, b[8] = (unsigned char)H.km;
  put32(b + 9, H.race_id);
  int at = 13;
  for (int k = 0; k < H.n; ++k) {
    const player *q = &H.p[k];
    b[at] = (unsigned char)k;
    put32(b + at + 1, q->id);
    b[at + 5] = (unsigned char)q->status;
    memcpy(b + at + 6, q->name, NET_NAME);
    b[at + 6 + NET_NAME] = (unsigned char)q->profile_len;
    memcpy(b + at + 7 + NET_NAME, q->profile, (size_t)q->profile_len);
    at += 7 + NET_NAME + q->profile_len;
  }
  send_to(&H.p[i].from, b, at);
}

static void lobby_all(void) {
  for (int i = 0; i < H.n; ++i) lobby_to(i);
}

static void drop(int i) {
  fprintf(stderr, "versus host: %.16s left\n", (const char *)H.p[i].name);
  memmove(&H.p[i], &H.p[i + 1], sizeof H.p[0] * (size_t)(H.n - i - 1));
  H.n--;
  lobby_all();
}

/* an address of this machine or of a home network (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8) */
static int lan_addr(const addr *x) {
  uint32_t a = ntohl(x->a.sin_addr.s_addr);
  return a >> 24 == 127 || a >> 24 == 10 || a >> 20 == 0xac1 || a >> 16 == 0xc0a8 || a >> 16 == 0xa9fe;
}
static int loopback_addr(const addr *x) { return ntohl(x->a.sin_addr.s_addr) >> 24 == 127; }

/* a player silent for this long may be taken over by a new address (a phone's Wi-Fi changing hands); before it, a packet in its name from
 * another address is thrown away: the ids are in every lobby packet, and without this anyone who sees one could take that player's place */
#define TAKEOVER_AFTER 3.0
#define RACE_PACKET_MAX (5 + CAR_LINK_PACKET + 3) /* race id, pcb and the car link packet, and the sender's round trip and flags */

static void got(const unsigned char *b, int len, const addr *from, double now, const char *name) {
  if (len < 4 || b[0] != 'W' || b[1] != 'V' || b[2] != 1) return;
  const unsigned char *p = b + 4;
  len -= 4;
  int i = find(from);
  switch (b[3]) {
  case DISCOVER: {
    /* only the home network is told of the game (and a search from the internet, with a forged sender, cannot make this send it a larger
     * answer than it asked with) */
    if (!lan_addr(from)) return;
    /* OFFER: u8 players, char name[16], u32 the host's own id (one host by two ways in, once) */
    unsigned char o[4 + 1 + NET_NAME + 4];
    head(o, OFFER);
    o[4] = (unsigned char)H.n;
    memset(o + 5, 0, NET_NAME);
    memcpy(o + 5, name, strlen(name) < NET_NAME ? strlen(name) : NET_NAME);
    put32(o + 5 + NET_NAME, H.owner);
    send_to(from, o, sizeof o);
    return;
  }
  case RACE: {
    if (len < 5 || len > RACE_PACKET_MAX || i < 0) return;
    uint32_t race = get32(p);
    H.p[i].seen = now, H.p[i].raced = 1;
    for (int r = 0; r < RACES; ++r) {
      if (H.race[r] != race || !race) continue;
      int mine = 0; /* (only a machine of that race may send into it) */
      for (int k = 0; k < H.machines[r]; ++k) mine |= H.machine[r][k] == H.p[i].id;
      if (!mine) continue;
      for (int k = 0; k < H.machines[r]; ++k)
        for (int j = 0; j < H.n; ++j)
          if (j != i && H.p[j].id == H.machine[r][k]) send_to(&H.p[j].from, b, len + 4);
    }
    return;
  }
  case HELLO: {
    if (len < 22) return;
    uint32_t id = get32(p);
    int n = p[5 + NET_NAME];
    if (len < 6 + NET_NAME + n) return;
    if (n > NET_PROFILE) n = NET_PROFILE;
    int k = -1;
    for (int j = 0; j < H.n; ++j)
      if (H.p[j].id == id) k = j;
    int changed = 0;
    /* the host's own player comes from this machine only (its id is in every OFFER and lobby packet: no one else can be the host) */
    if (id == H.owner && !loopback_addr(from)) return;
    if (k >= 0 && !same(&H.p[k].from, from) && now - H.p[k].seen < TAKEOVER_AFTER) return;
    for (int j = 0; j < H.n; ++j) /* (one socket, one player) */
      if (j != k && same(&H.p[j].from, from)) return;
    if (k < 0) {
      if (H.n >= NET_PLAYERS) return;
      /* the host's own player first: it is the lobby's host (slot 0) */
      k = id == H.owner ? 0 : H.n;
      memmove(&H.p[k + 1], &H.p[k], sizeof H.p[0] * (size_t)(H.n - k));
      memset(&H.p[k], 0, sizeof H.p[k]);
      H.p[k].id = id;
      H.n++;
      changed = 1;
      fprintf(stderr, "versus host: %.16s joined (%d players)\n", (const char *)p + 5, H.n);
    }
    player *q = &H.p[k];
    changed |= !same(&q->from, from) || q->status != p[4] || memcmp(q->name, p + 5, NET_NAME) || q->profile_len != n ||
               memcmp(q->profile, p + 6 + NET_NAME, (size_t)n);
    q->from = *from;
    q->status = p[4];
    memcpy(q->name, p + 5, NET_NAME);
    q->profile_len = n;
    memcpy(q->profile, p + 6 + NET_NAME, (size_t)n);
    q->seen = now;
    if (changed) lobby_all();
    else lobby_to(k);
    return;
  }
  case START: {
    if (i != 0 || len < 2 || H.n < 2) return;
    H.ramp = p[0], H.km = p[1];
    H.song = len >= 3 ? p[2] : 0;
    H.race_id = H.race_id + 1 ? H.race_id + 1 : 1;
    uint32_t seed = (uint32_t)rand() & 0x7fffffff;
    int r = (int)(H.race_id % RACES);
    H.race[r] = H.race_id;
    H.machines[r] = H.n;
    for (int k = 0; k < H.n; ++k) H.machine[r][k] = H.p[k].id, H.p[k].status = 1, H.p[k].raced = 0;
    fprintf(stderr, "versus host: race %u, ramp %d, %d km, %d players\n", H.race_id, H.ramp, H.km, H.n);
    for (int k = 0; k < H.n; ++k) {
      unsigned char g[1024];
      head(g, GO);
      put32(g + 4, H.race_id);
      put32(g + 8, seed);
      g[12] = (unsigned char)H.ramp, g[13] = (unsigned char)H.km, g[14] = (unsigned char)H.n, g[15] = (unsigned char)k;
      int at = 16;
      for (int j = 0; j < H.n; ++j) {
        const player *q = &H.p[j];
        g[at] = (unsigned char)j;
        put32(g + at + 1, q->id);
        memcpy(g + at + 5, q->name, NET_NAME);
        g[at + 5 + NET_NAME] = (unsigned char)q->profile_len;
        memcpy(g + at + 6 + NET_NAME, q->profile, (size_t)q->profile_len);
        at += 6 + NET_NAME + q->profile_len;
      }
      g[at++] = (unsigned char)H.song; /* everyone's BGM (an older client stops at the machines) */
      for (int t = 0; t < 3; ++t) send_to(&H.p[k].from, g, at); /* said more than once: the first counts */
    }
    return;
  }
  case PING: { /* a client measuring its round trip: sent straight back */
    unsigned char r[64];
    if (len + 4 > (int)sizeof r || len < 8) return;
    memcpy(r, b, (size_t)(len + 4));
    r[3] = PONG;
    send_to(from, r, len + 4);
    return;
  }
  case BYE:
    if (i >= 0) drop(i);
    return;
  }
}

int vs_host_open(uint32_t owner, int online) {
  vs_host_close();
  if (net_sock_init() != 0) return -1;
  /* the first free port of the usual ones (a port a stale process holds is passed over), else any the system gives; the port is exclusive, so a
   * second socket can never share it and take the packets */
  net_fd fd = NET_NOFD;
  int port = 0;
  /* an online game takes a random high port first: scanners look at the usual one, and the room code carries the port anyway */
  int tries = NET_PORT_TRIES + (online ? 20 : 0);
  for (int t = 0; t <= tries; ++t) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!net_fd_ok(fd)) return -1;
    net_fd_exclusive(fd);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    int want = online && t < 20 ? 20000 + (int)(net_random32() % 40000) : t - (online ? 20 : 0) < NET_PORT_TRIES ? NET_PORT + t - (online ? 20 : 0) : 0;
    a.sin_port = htons((uint16_t)want);
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) == 0) {
      net_len al = sizeof a;
      getsockname(fd, (struct sockaddr *)&a, &al);
      port = ntohs(a.sin_port);
      break;
    }
    net_fd_close(fd);
    fd = NET_NOFD;
  }
  if (!net_fd_ok(fd)) return -1;
  net_fd_nonblock(fd);
  memset(&H, 0, sizeof H);
  H.port = port;
  H.fd = fd;
  H.owner = owner;
  H.km = 13;
  H.open = 1;
  return 0;
}

void vs_host_close(void) {
  /* the others told the game is closed (a race of theirs ends with it), said more than once */
  unsigned char b[8];
  head(b, CLOSED);
  put32(b + 4, H.owner);
  for (int i = 0; H.open && i < H.n; ++i)
    for (int t = 0; H.p[i].id != H.owner && t < 3; ++t) send_to(&H.p[i].from, b, sizeof b);
  if (net_fd_ok(H.fd)) net_fd_close(H.fd);
  memset(&H, 0, sizeof H);
  H.fd = NET_NOFD;
}

int vs_host_is_open(void) { return H.open; }
int vs_host_port(void) { return H.open ? H.port : 0; }

void vs_host_poll(double now, const char *name) {
  if (!H.open) return;
  unsigned char b[2048];
  for (;;) {
    addr from;
    memset(&from, 0, sizeof from);
    net_len al = sizeof from.a;
    net_ssize n = recvfrom(H.fd, (char *)b, sizeof b, 0, (struct sockaddr *)&from.a, &al);
    if (n <= 0) break;
    got(b, (int)n, &from, now, name);
  }
  int dropped = 0;
  for (int i = H.n - 1; i >= 0; --i)
    if (now - H.p[i].seen > (H.p[i].status == 1 ? (H.p[i].raced ? TIMEOUT_RACED : TIMEOUT_RACING) : TIMEOUT)) {
      fprintf(stderr, "versus host: %.16s timed out\n", (const char *)H.p[i].name);
      memmove(&H.p[i], &H.p[i + 1], sizeof H.p[0] * (size_t)(H.n - i - 1));
      H.n--;
      dropped = 1;
    }
  if (dropped) lobby_all();
}
