/* The versus's network (net.c, vs_host.c: UDP sockets) left out: the Windows player has no LAN play yet, so every call
 * answers "no network". The calls' meanings are in net.h and vs_host.h. */
#include "net.h"
#include "vs_host.h"

#include <stdio.h>
#include <string.h>

int net_open(const char *host, int port, uint32_t id) { return (void)host, (void)port, (void)id, -1; }
void net_close(void) {}
int net_is_open(void) { return 0; }
void net_poll(double now, int status, const char *name, const unsigned char *profile, int profile_len) {
  (void)now, (void)status, (void)name, (void)profile, (void)profile_len;
}
const net_lobby *net_lobby_now(void) {
  static net_lobby none;
  return &none;
}
double net_silence(double now) { return (void)now, 1e9; }
void net_start(int ramp, int km, int song) { (void)ramp, (void)km, (void)song; }
int net_host_closed(void) { return 0; }
int net_take_go(net_go *go) { return (void)go, 0; }
void net_race_send(const unsigned char *packet, int len) { (void)packet, (void)len; }
void net_race_receive(void) {}
void net_scan(double now, int on) { (void)now, (void)on; }
int net_found_list(net_found *out, int max) { return (void)out, (void)max, 0; }

int vs_host_open(uint32_t owner, int online) { return (void)owner, (void)online, -1; }
void vs_host_close(void) {}
int vs_host_is_open(void) { return 0; }
void vs_host_poll(double now, const char *name) { (void)now, (void)name; }
int net_ping_ms(int pcb) { return (void)pcb, -1; }
void net_set_flash(int off) { (void)off; }
int net_flash_of(int pcb) { return (void)pcb, 0; }
void net_log_force(int on) { (void)on; }
void net_stats_text(char *out, int n) { snprintf(out, (size_t)n, ""); }
