/* The versus's measuring (net.c): how old the other machines' race data is when it is used, the round trip to the host, and what an
 * artificial bad connection does to both. For the netcode work (docs/VERSUS.md "Measuring"); it only logs. MT3DX_NET_LAG=<ms>[,<jitter ms>[,<loss %>]]
 * delays (and loses) every packet this machine receives; set it on both ends for a two-way lag. */
#ifndef NETDIAG_H
#define NETDIAG_H
#include <stdio.h>

/* the frames behind this machine's own that the newest race packet from machine pcb was when it arrived (the clocks start together at the
 * GO: so this is the network's age of it, plus the start's difference); -1000 if none came yet */
int net_age_frames(int pcb);
/* the round trip to the host in ms (the last answer); -1 none yet */
double net_rtt_ms(void);
/* machine pcb's own round trip to the host in ms, as its race packets carry it (the HUD's P: n under each rival's card); -1 not known */
int net_ping_ms(int pcb);
/* the headlight flash: this machine's lights are off for the step (sent with its race packets), and whether machine pcb's are (1 for a second after its last packet) */
void net_set_flash(int off);
int net_flash_of(int pcb);

/* how many frames this machine's race clock is ahead of machine pcb's (the clients pace themselves to the host's with it, app.c vs_pace); valid is
 * 0 until enough packets and a round trip are in */
double net_clock_ahead_frames(int pcb, int *valid);

/* the measuring's log lines (versus net / sync / predict / react / spike / calib / trace / own jump / contact / near) only with MT3DX_NETLOG=1 in the environment: they are
 * for the netcode work, and a race writes hundreds of them. The behaviour they describe is always on. */
int net_log_on(void);
/* NETWORK OPTIONS' DETAILED NET LOG: the same log lines as MT3DX_NETLOG=1 */
void net_log_force(int on);
/* one line of the last race's packet statistics for the screen */
void net_stats_text(char *out, int n);
/* the log's privacy (net.c): log_scrub_file takes the public IP addresses, room codes and user names in paths out of a log file in place; log_mask_ip is an address for a log
 * line (a private LAN one as it is, any other hidden) */
int log_scrub_file(const char *path);
void log_mask_ip(const char *ip, char out[24]);
#define NETLOG(...) do { if (net_log_on()) fprintf(stderr, __VA_ARGS__); } while (0)

#endif
