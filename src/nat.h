/* Opening the home router for a game hosted online (UPnP: the router's own "open this port for that computer"), and learning the address
 * the other players reach it at. Everything slow runs on a thread of its own: nat_open returns at once, nat_info says how it is going. */
#ifndef NAT_H
#define NAT_H

enum { NAT_IDLE, NAT_WORKING, NAT_READY, NAT_FAILED };
typedef struct {
  int state;      /* NAT_* */
  char ip[16];    /* the address the others reach this host at (NAT_READY; "" if it could not be found) */
  int port;       /* the UDP port asked for */
  int mapped;     /* the router opened the port by itself */
  int cgnat;      /* the address found is not a public one (the provider's own NAT: no one outside can reach this host) */
  char note[64];  /* a line for the screen */
} nat_info;

/* ask the router to forward UDP `port` to this machine, and find the public address; no-op while a call is under way */
void nat_open(int port);
/* take the forwarding away again and stop */
void nat_close(void);
/* this machine's address on its network (the one a UDP socket would send from; nothing is sent): 0 and the dotted text in out, -1 unknown */
int nat_local_ip(char out[16]);
const nat_info *nat_status(void);

#endif
