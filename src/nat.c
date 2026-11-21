/* UPnP port forwarding and the public address, see nat.h. The router is found by SSDP (a multicast search), its description fetched over
 * HTTP, and AddPortMapping / GetExternalIPAddress / DeletePortMapping sent as SOAP. With no UPnP router the public address comes from a STUN
 * server instead (the port is then the player's to forward). Blocking calls with short timeouts, on one worker thread. */
#include "nat.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netsock.h"

#ifdef _WIN32
typedef HANDLE nat_thread;
#define NAT_THREAD_FN DWORD WINAPI
static int nat_thread_start(nat_thread *t, DWORD(WINAPI *fn)(void *)) {
  *t = CreateThread(NULL, 0, fn, NULL, 0, NULL);
  return *t ? 0 : -1;
}
static void nat_thread_join(nat_thread t) {
  WaitForSingleObject(t, 15000);
  CloseHandle(t);
}
#else
#include <pthread.h>
#include <sys/time.h>
typedef pthread_t nat_thread;
#define NAT_THREAD_FN void *
static int nat_thread_start(nat_thread *t, void *(*fn)(void *)) { return pthread_create(t, NULL, fn, NULL); }
static void nat_thread_join(nat_thread t) { pthread_join(t, NULL); }
#endif

#define IO_MS 2500
#define BIG 65536

static struct {
  nat_info info;
  nat_thread th;
  int running;
  volatile int cancel;
  /* the router's service, kept to take the forwarding away */
  char host[64];
  int port;
  char control[256], service[96], args[700];
  int have_map;
} T;

static void say(int state, const char *note) {
  snprintf(T.info.note, sizeof T.info.note, "%s", note);
  T.info.state = state; /* (last: the screen reads the state first) */
}

/* a TCP connection with timeouts; -1 on none */
static net_fd tcp_open(const char *ip, int port) {
  net_fd fd = socket(AF_INET, SOCK_STREAM, 0);
  if (!net_fd_ok(fd)) return NET_NOFD;
  struct sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_port = htons((uint16_t)port);
  if (inet_pton(AF_INET, ip, &a.sin_addr) != 1) {
    net_fd_close(fd);
    return NET_NOFD;
  }
  net_fd_nonblock(fd);
  connect(fd, (struct sockaddr *)&a, sizeof a); /* (in progress) */
  fd_set w;
  FD_ZERO(&w);
  FD_SET(fd, &w);
  struct timeval tv = { IO_MS / 1000, (IO_MS % 1000) * 1000 };
  if (select((int)fd + 1, NULL, &w, NULL, &tv) <= 0) {
    net_fd_close(fd);
    return NET_NOFD;
  }
  int err = 0;
  net_len el = sizeof err;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &el);
  if (err) {
    net_fd_close(fd);
    return NET_NOFD;
  }
  return fd;
}

/* send a request, read the whole answer (the connection closed or timed out) into out, NUL-terminated; its length, or -1 */
static int http(const char *ip, int port, const char *req, int req_len, char *out, int max) {
  net_fd fd = tcp_open(ip, port);
  if (!net_fd_ok(fd)) return -1;
  int sent = 0;
  while (sent < req_len) {
    fd_set w;
    FD_ZERO(&w);
    FD_SET(fd, &w);
    struct timeval tv = { IO_MS / 1000, (IO_MS % 1000) * 1000 };
    if (select((int)fd + 1, NULL, &w, NULL, &tv) <= 0) break;
    int k = (int)send(fd, req + sent, req_len - sent, 0);
    if (k <= 0) break;
    sent += k;
  }
  int n = 0;
  while (sent == req_len && n < max - 1) {
    fd_set r;
    FD_ZERO(&r);
    FD_SET(fd, &r);
    struct timeval tv = { IO_MS / 1000, (IO_MS % 1000) * 1000 };
    if (select((int)fd + 1, &r, NULL, NULL, &tv) <= 0) break;
    int k = (int)recv(fd, out + n, max - 1 - n, 0);
    if (k <= 0) break;
    n += k;
  }
  net_fd_close(fd);
  out[n] = 0;
  return sent == req_len && n > 0 ? n : -1;
}

/* the text between <tag> and </tag> after `from`, copied into out; 1 if found */
static int between(const char *from, const char *tag, char *out, int max) {
  char open[48], close[48];
  snprintf(open, sizeof open, "<%s>", tag);
  snprintf(close, sizeof close, "</%s>", tag);
  const char *a = strstr(from, open);
  if (!a) return 0;
  a += strlen(open);
  const char *b = strstr(a, close);
  if (!b) return 0;
  int n = (int)(b - a);
  if (n >= max) n = max - 1;
  memcpy(out, a, (size_t)n);
  out[n] = 0;
  return 1;
}

/* this machine's address on the network the router is on: the address a UDP socket would send from (nothing is sent) */
static int local_ip(char out[16]) {
  net_fd fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!net_fd_ok(fd)) return -1;
  struct sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_port = htons(53);
  inet_pton(AF_INET, "8.8.8.8", &a.sin_addr);
  int ok = connect(fd, (struct sockaddr *)&a, sizeof a) == 0;
  net_len al = sizeof a;
  if (ok) ok = getsockname(fd, (struct sockaddr *)&a, &al) == 0;
  if (ok) ok = inet_ntop(AF_INET, &a.sin_addr, out, 16) != NULL;
  net_fd_close(fd);
  return ok ? 0 : -1;
}

int nat_local_ip(char out[16]) { return net_sock_init() == 0 ? local_ip(out) : -1; }

/* SSDP: the first Internet Gateway Device to answer; its description's URL */
static int ssdp(char *location, int max, const char *mine) {
  net_fd fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!net_fd_ok(fd)) return -1;
  net_fd_nonblock(fd);
  /* the multicast out of the adapter the network is on (a PC with a VPN, WSL or virtual machines has more than one) */
  struct in_addr ifa;
  if (inet_pton(AF_INET, mine, &ifa) == 1 && ifa.s_addr) setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, NET_SOCKOPT(&ifa), sizeof ifa);
  /* and the router asked directly too (its address is usually the network's .1 or .254): a reply from the address asked is let through by a
   * firewall that drops a multicast search's answers */
  struct sockaddr_in gw[2];
  for (int g = 0; g < 2; ++g) {
    memset(&gw[g], 0, sizeof gw[g]);
    gw[g].sin_family = AF_INET;
    gw[g].sin_port = htons(1900);
    inet_pton(AF_INET, mine, &gw[g].sin_addr);
    ((unsigned char *)&gw[g].sin_addr)[3] = g ? 254 : 1;
  }
  static const char *const ST[] = { "urn:schemas-upnp-org:device:InternetGatewayDevice:1", "urn:schemas-upnp-org:service:WANIPConnection:1",
                                    "urn:schemas-upnp-org:service:WANPPPConnection:1" };
  struct sockaddr_in to;
  memset(&to, 0, sizeof to);
  to.sin_family = AF_INET;
  to.sin_port = htons(1900);
  inet_pton(AF_INET, "239.255.255.250", &to.sin_addr);
  int found = 0;
  for (int round = 0; round < 2 && !found && !T.cancel; ++round) {
    for (int s = 0; s < 3; ++s) {
      char q[300];
      int n = snprintf(q, sizeof q, "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: %s\r\n\r\n", ST[s]);
      sendto(fd, q, n, 0, (struct sockaddr *)&to, sizeof to);
      for (int g = 0; g < 2; ++g) sendto(fd, q, n, 0, (struct sockaddr *)&gw[g], sizeof gw[g]);
    }
    for (int t = 0; t < 12 && !found && !T.cancel; ++t) { /* about 1.2 s a round */
      char b[1500];
      int n = (int)recv(fd, b, sizeof b - 1, 0);
      if (n <= 0) {
#ifdef _WIN32
        Sleep(100);
#else
        struct timeval tv = { 0, 100000 };
        select(0, NULL, NULL, NULL, &tv);
#endif
        continue;
      }
      b[n] = 0;
      char lb[1500]; /* (the header's name found in a lower case copy, the address itself taken from the original: a path is case sensitive) */
      memcpy(lb, b, (size_t)n + 1);
      for (char *p = lb; *p; ++p) *p = (char)tolower((unsigned char)*p);
      char *l = strstr(lb, "\r\nlocation:");
      if (!l) continue;
      l = b + (l - lb) + 11;
      while (*l == ' ') l++;
      char *e = l;
      while (*e && *e != '\r' && *e != '\n') e++;
      int k = (int)(e - l);
      if (k <= 0 || k >= max) continue;
      memcpy(location, l, (size_t)k);
      location[k] = 0;
      found = 1;
    }
  }
  net_fd_close(fd);
  return found ? 0 : -1;
}

/* http://host:port/path -> host, port, path */
static int split_url(const char *url, char *host, int hmax, int *port, char *path, int pmax) {
  if (strncmp(url, "http://", 7) != 0) return -1;
  const char *h = url + 7, *slash = strchr(h, '/'), *colon = memchr(h, ':', slash ? (size_t)(slash - h) : strlen(h));
  int hl = (int)((colon ? colon : slash ? slash : h + strlen(h)) - h);
  if (hl <= 0 || hl >= hmax) return -1;
  memcpy(host, h, (size_t)hl);
  host[hl] = 0;
  *port = colon ? atoi(colon + 1) : 80;
  snprintf(path, (size_t)pmax, "%s", slash ? slash : "/");
  return 0;
}

static int soap(const char *action, const char *args, char *out, int max) {

  char body[1400], req[2200];
  int bl = snprintf(body, sizeof body,
                    "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:%s xmlns:u=\"%s\">%s</u:%s></s:Body></s:Envelope>",
                    action, T.service, args, action);
  int rl = snprintf(req, sizeof req,
                    "POST %s HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: %d\r\n"
                    "SOAPAction: \"%s#%s\"\r\nConnection: close\r\n\r\n%s",
                    T.control, T.host, T.port, bl, T.service, action, body);
  int n = http(T.host, T.port, req, rl, out, max);
  { char first[200], *c; snprintf(first, sizeof first, "%s", n > 0 ? out : "(no answer)"); for (c = first; *c; ++c) if (*c == 13 || *c == 10) { *c = 0; break; }
    fprintf(stderr, "nat: %s -> %s\n", action, first);
    if (n > 0 && strstr(out, " 500 ")) { const char *e = strstr(out, "<errorDescription>"); fprintf(stderr, "nat:   %.120s\n", e ? e : "error"); } }
  return n > 0 && strstr(out, " 200 ") != NULL && strstr(out, " 200 ") < out + 16 ? 0 : -1;
}

/* a STUN binding request (RFC 5389) for the public address; 0 and out filled in on an answer */
static int stun(char out[16]) {
  static const char *const SERVERS[] = { "stun.l.google.com", "stun1.l.google.com", "stun.cloudflare.com" };
  for (int s = 0; s < 3 && !T.cancel; ++s) {
    struct addrinfo hint, *res = NULL;
    memset(&hint, 0, sizeof hint);
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(SERVERS[s], s == 2 ? "3478" : "19302", &hint, &res) != 0 || !res) continue;
    net_fd fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!net_fd_ok(fd)) {
      freeaddrinfo(res);
      return -1;
    }
    unsigned char q[20] = { 0, 1, 0, 0, 0x21, 0x12, 0xa4, 0x42 };
    for (int i = 8; i < 20; ++i) q[i] = (unsigned char)(rand() >> 4);
    sendto(fd, (const char *)q, 20, 0, res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);
    fd_set r;
    FD_ZERO(&r);
    FD_SET(fd, &r);
    struct timeval tv = { 2, 0 };
    int ok = 0;
    if (select((int)fd + 1, &r, NULL, NULL, &tv) > 0) {
      unsigned char b[512];
      int n = (int)recv(fd, (char *)b, sizeof b, 0);
      for (int at = 20; n >= 20 && at + 4 <= n && !ok;) {
        int type = b[at] << 8 | b[at + 1], len = b[at + 2] << 8 | b[at + 3];
        if ((type == 0x0020 || type == 0x0001) && len >= 8 && b[at + 5] == 1 && at + 8 <= n) { /* XOR-MAPPED / MAPPED-ADDRESS, IPv4 */
          unsigned char a[4] = { b[at + 8], b[at + 9], b[at + 10], b[at + 11] };
          if (type == 0x0020) a[0] ^= 0x21, a[1] ^= 0x12, a[2] ^= 0xa4, a[3] ^= 0x42;
          snprintf(out, 16, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
          ok = 1;
        }
        at += 4 + ((len + 3) & ~3);
      }
    }
    net_fd_close(fd);
    if (ok) return 0;
  }
  return -1;
}

/* an address in a range the internet does not reach: private, the provider's shared (100.64/10), link-local */
static int private_ip(const char *ip) {
  int a, b;
  if (sscanf(ip, "%d.%d", &a, &b) != 2) return 0;
  return a == 10 || (a == 172 && b >= 16 && b < 32) || (a == 192 && b == 168) || (a == 100 && b >= 64 && b < 128) || a == 169 || a == 127;
}

/* an address on a home network (the router is one; a reply naming a server on the internet is not to be believed, or sent a request) */
static int lan_ip(const char *ip) {
  int a, b;
  if (sscanf(ip, "%d.%d", &a, &b) != 2) return 0;
  return a == 10 || (a == 172 && b >= 16 && b < 32) || (a == 192 && b == 168);
}

static NAT_THREAD_FN worker(void *arg) {
  (void)arg;
  char *buf = malloc(BIG);
  char loc[300], path[256], mine[16];
  int ok = 0;
  if (!buf || net_sock_init() != 0) {
    say(NAT_FAILED, "NOT CONNECTED TO A NETWORK");
    free(buf);
    return 0;
  }
  if (local_ip(mine) != 0) snprintf(mine, sizeof mine, "0.0.0.0");
  say(NAT_WORKING, "SETTING UP YOUR GAME...");
  int sr = ssdp(loc, sizeof loc, mine);
  fprintf(stderr, "nat: ssdp %s %s (local %s)\n", sr == 0 ? "found" : "found nothing", sr == 0 ? loc : "", mine);
  if (sr == 0 && split_url(loc, T.host, sizeof T.host, &T.port, path, sizeof path) == 0 && lan_ip(T.host)) {
    char req[400];
    int rl = snprintf(req, sizeof req, "GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\n\r\n", path, T.host, T.port);
    int gl = http(T.host, T.port, req, rl, buf, BIG);
    fprintf(stderr, "nat: description %d bytes\n", gl);
    if (gl > 0 && gl < 400) fprintf(stderr, "nat: %.300s\n", buf);
    if (gl > 0) {
      /* the WAN connection service: its type and control URL */
      static const char *const TYPES[] = { "urn:schemas-upnp-org:service:WANIPConnection:2", "urn:schemas-upnp-org:service:WANIPConnection:1",
                                           "urn:schemas-upnp-org:service:WANPPPConnection:1" };
      for (int t = 0; t < 3 && !ok; ++t) {
        const char *s = strstr(buf, TYPES[t]);
        char ctl[256];
        if (s && between(s, "controlURL", ctl, sizeof ctl)) {
          snprintf(T.service, sizeof T.service, "%s", TYPES[t]);
          if (ctl[0] == '/') snprintf(T.control, sizeof T.control, "%s", ctl);
          else snprintf(T.control, sizeof T.control, "/%s", ctl);
          ok = 1;
          fprintf(stderr, "nat: service %s control %s\n", T.service, T.control);
        }
      }
    }
  }
  if (ok && !T.cancel) {
    char args[700];
    /* a lease, not for ever: a mapping a crashed game leaves is gone by itself after 2 hours, and the worker renews it every 50 minutes while the game is hosted */
    snprintf(args, sizeof args,
               "<NewRemoteHost></NewRemoteHost><NewExternalPort>%d</NewExternalPort><NewProtocol>UDP</NewProtocol><NewInternalPort>%d</NewInternalPort>"
               "<NewInternalClient>%s</NewInternalClient><NewEnabled>1</NewEnabled><NewPortMappingDescription>Super3DX+ versus</NewPortMappingDescription>"
               "<NewLeaseDuration>%d</NewLeaseDuration>",
               T.info.port, T.info.port, mine, 7200);
    ok = soap("AddPortMapping", args, buf, BIG) == 0;
    snprintf(T.args, sizeof T.args, "%s", args);
    T.have_map = ok;
    T.info.mapped = ok;
    if (ok) {
      char ext[64];
      if (soap("GetExternalIPAddress", "", buf, BIG) == 0 && between(buf, "NewExternalIPAddress", ext, sizeof ext)) snprintf(T.info.ip, sizeof T.info.ip, "%s", ext);
    }
  }
  if (!T.info.ip[0] && !T.cancel) { /* (no UPnP router, or it would not say) */
    char ip[16];
    if (stun(ip) == 0) snprintf(T.info.ip, sizeof T.info.ip, "%s", ip);
  }
  T.info.cgnat = T.info.ip[0] && private_ip(T.info.ip);
  if (!T.info.ip[0]) say(NAT_FAILED, "NO INTERNET: ONLY PLAYERS ON YOUR WI-FI CAN JOIN");
  else if (T.info.cgnat) say(NAT_FAILED, "YOUR PROVIDER BLOCKS HOSTING: ONLY YOUR WI-FI CAN JOIN");
  else if (T.info.mapped) say(NAT_READY, "GAME HOSTED. SHARE THE ROOM CODE WITH YOUR FRIENDS");
  else {
    char s[64];
    snprintf(s, sizeof s, "ONE STEP LEFT: FORWARD UDP PORT %d IN YOUR ROUTER", T.info.port);
    say(NAT_READY, s);
  }
  /* (renewing the lease while the game is hosted) */
  for (int sec = 0; T.have_map && !T.cancel; ++sec) {
#ifdef _WIN32
    Sleep(1000);
#else
    struct timeval tv = { 1, 0 };
    select(0, NULL, NULL, NULL, &tv);
#endif
    if (sec >= 3000) {
      sec = 0;
      soap("AddPortMapping", T.args, buf, BIG);
    }
  }
  free(buf);
  return 0;
}

void nat_open(int port) {
  if (T.running) return;
  static int hooked;
  if (!hooked) hooked = 1, atexit(nat_close); /* (a game closed while hosting takes its forwarding away) */
  memset(&T.info, 0, sizeof T.info);
  T.info.port = port;
  T.cancel = 0;
  T.have_map = 0;
  say(NAT_WORKING, "SETTING UP YOUR GAME...");
  if (nat_thread_start(&T.th, worker) == 0) T.running = 1;
  else say(NAT_FAILED, "COULD NOT START HOSTING");
}

void nat_close(void) {
  if (!T.running) {
    memset(&T.info, 0, sizeof T.info);
    return;
  }
  T.cancel = 1;
  nat_thread_join(T.th);
  T.running = 0;
  if (T.have_map) {
    char *buf = malloc(4096), args[200];
    if (buf) {
      snprintf(args, sizeof args, "<NewRemoteHost></NewRemoteHost><NewExternalPort>%d</NewExternalPort><NewProtocol>UDP</NewProtocol>", T.info.port);
      soap("DeletePortMapping", args, buf, 4096);
      free(buf);
    }
  }
  T.have_map = 0;
  memset(&T.info, 0, sizeof T.info);
}

const nat_info *nat_status(void) { return &T.info; }
