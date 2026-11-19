# Versus: hosting and joining

The versus screen has two discs: **LAN** and **ONLINE VERSUS**. Up to 4 players. Everyone needs the same build number (a mismatched player shows
VERSION MISMATCH and holds the start). The Windows, Linux/Pi and Android players all speak the same protocol (`tools/vs_server.py` documents it).

## LAN

1. Everyone on the same Wi-Fi or wired network.
2. One player: LAN → **HOST A GAME**. The others: LAN; the game appears under GAMES NEARBY, tap it.
3. The host presses START in the lobby.

If nothing appears, allow the game through the host's firewall (UDP, private networks).

## Online

**Hosting.** ONLINE VERSUS → **HOST ONLINE**. The game opens a UDP port (27015, or the next free one up to 27024, or any free one), asks the
router to forward it (UPnP, nothing to set up on most home routers) and finds its public address. The screen then shows a **room code**
(15 digits, `ip * 65536 + port`) and the address it stands for. Give the code to the others. The line under it says how it went:

- *THE ROUTER OPENED THE PORT BY ITSELF*: done.
- *FORWARD UDP PORT n ON YOUR ROUTER TO THIS DEVICE*: the router has no UPnP (or it is off). Forward that UDP port to this device by hand; the code is
  still right.
- *YOUR PROVIDER SHARES ONE ADDRESS*: the connection is behind the provider's own NAT (CGNAT); no one outside can reach the host. Use a
  dedicated server (below), a VPN such as Tailscale or ZeroTier (then type the VPN address), or ask the provider for a public address.

The forwarding is a lease of two hours, renewed while the game is hosted and removed when hosting stops.

**Joining.** ONLINE VERSUS → type the host's **room code** or its IPv4 address (an address with dots uses port 27015) → **CONNECT**.

## A dedicated server

`python3 tools/vs_server.py [port]` on any computer the players can reach (a VPS, a Pi; forward the UDP port if it is at home). Everyone types
its address. It relays the race packets; nobody has to be the host's machine.

## Ports and "port in use"

The host never shares its port with another program: if 27015 is taken (a stale process, another game) it takes the next free one, and the
room code and the LAN list carry the port it got. Windows: the port is opened exclusively, so a second program cannot silently receive the
packets.

## What the network code does and does not do

- **Nothing runs until you ask.** No socket is opened at start-up. The LAN screen listens for answers to its search; HOST A GAME / HOST ONLINE
  opens the one UDP port; JOIN opens one socket to the host.
- **Who can join.** Anyone who can reach the host's port can appear in its lobby (up to 4). The online game takes a random port from 20000-59999
  (the room code carries it) instead of the well-known 27015, so a scan of the usual port finds nothing; the LAN game stays on 27015..27024.
  There is no password: share the room code only with the people you mean to race.
- **No taking over a player.** The host throws away a packet in a lobby player's name from another address while that player is active; the host's own
  player is only accepted from the host's own machine (so the "host" cannot be claimed over the network); one socket is one player. Player ids come
  from the system's random source.
- **The LAN search** is only answered for addresses on a home network (10/8, 172.16/12, 192.168/16, loopback, link-local), so a game hosted online is
  invisible to, and cannot be used to bounce traffic at, the internet.
- **Race relay**: only a machine of that race may send into it, and a packet longer than a race packet is dropped. A joining game checks what a host tells it
  (the place numbers, the ramp, the distance) before it indexes anything with them.
- **The router**: the UPnP reply is believed only if the router it names is on a home network address; the forwarding asks for one UDP port, with a
  lease of two hours, renewed while hosting and removed when hosting stops or the game closes.
- **What leaves your device**: your player name and car (to the host, in the lobby); your public address is found from the router, or, if the
  router will not say, by one request to a public STUN server (Google's, Cloudflare's); nothing else, no telemetry.
- **Known limits**: the packets are not encrypted (a name and a car are visible to anyone on the path), and a host cannot kick a player yet.

## How contact is kept sane over a delay (the netcode, as of baseline 2)

The arcade's cabinet link makes each machine drive its own car and show the other as a puppet from the packets it sends (CLxRival). The port keeps that and
makes the puppet right at the delay a real connection has:

1. **Clocks.** Each machine's sim runs a step per display refresh, so two machines drift (a CRT at 59.94 Hz against a 60 Hz monitor: 6 frames in a minute) and
   start a few frames apart. A client holds its frame counter to the host's (app.c `vs_pace`): a step dropped when ahead, one more when behind, fast while far off
   (the countdown takes the start's skew out), none within 1.5 frames.
2. **Position by the exact age.** A hook right after `CLinx::Compensate` (car.c `lx_override`; tools/recomp/recomp.py HOOKS) puts each other car where it is now:
   the packet's position plus (velocity + acceleration * t / 2) * t, t the packet's age in frames (the clocks being locked, that is the delay). The arcade's own
   estimate (half the round trip from acknowledgements) measured 1.1-1.4 m out at 70 ms each way; this is 4 cm out in free flight.
3. **Heading by its rate.** The packet's CEuler is three 16-bit angles (65536 to a turn); each angle's rate per frame from the last packets turns the predicted
   heading on by the age (damped, capped). `MT3DX_ANG=0` turns it off.
4. **The other car's reaction.** When this car's velocity changes like a hit with a close car (0.5 m/s a frame, pushed away from it), the other car's prediction is
   moved by the equal and opposite change, in position only, capped at 1.5 m, until packets sent after the contact arrive (car.c `rx_*`). `MT3DX_REACT=<factor>`
   scales it, 0 off. A velocity change handed to the hit code fed itself and was taken out.

What is left: the other machine's own reaction reaches this one an age later, so the first frames after a hit are about 0.2 m out, and each machine pushes
its own car the whole way out of an overlap (the arcade's design), a 0.2 m step in a frame during side-by-side contact. Rollback netcode (both machines
simulating both cars from the same inputs) would remove both; it is not built.

## Measuring (for the netcode work)

The log lines are off unless `MT3DX_NETLOG=1` is in the environment (a race writes hundreds of them); the behaviour they describe is always on. They go to
stderr: `log.txt` on Windows, `game.log` on the Pi, logcat on Android. On Windows the player writes `<save folder>/log.txt` itself when it is started by a
double-click or `play.bat` (the previous run's is kept as `log-prev.txt`, which is the one to send after a hang and a restart); started with its stderr sent
somewhere (`2> log.txt`) or with `MT3DX_STDERR=1` set it leaves it alone. The versus' own events (joined, race, loaded, left) are in it without `MT3DX_NETLOG`.

- `versus net: from pcb N: ...` every 2 s: the packets from that machine in the window, their **age in frames** (this machine's frame counter minus the packet's;
  with the clocks locked, the delay), the **gap** between arrivals, packets **lost** and **late**, and the **rtt** to the host (a PING each second, answered
  by the host's relay).
- `versus sync:` every 2 s on a client: its frames ahead of the host, the steps dropped and added.
- `versus predict:` every 2 s: the position error of what the game used, of the raw packet and of the plain formula, against where the car really was; and
  the error by the frames since a contact. `versus react`, `versus own jump`, `versus spike`, `versus calib`, `versus trace`, `versus contact`, `versus near`:
  the contact and turn events, each capped.
- `MT3DX_NET_LAG=<ms>[,<jitter ms>[,<loss %>]]` makes this machine delay (and lose) everything it receives, to try a bad connection on a LAN; set it on both
  machines for a lag each way. On the Pi: `a local dev script on|off`; the Pi's car can drive itself for a lone tester with `a local dev script on|off`
  (the arcade's pilot steers, `MT3DX_AUTODRIVE_KMH` caps the speed); `a local dev script KEY=VAL ...` sets other variables (`MT3DX_NETLOG=1`).

## The race as the arcade runs it (what the console shows)

- **Route choice.** The arcade's own `CBranch` decides where the field goes at a junction: once every machine has crossed the decision line, the
  machine furthest along (`GetPcbId_OfMaxOdo`) chooses by its lane across the road at that line (`DecideBranch(float)`); inside the last 500 m it is a
  random pick from the race's seed. The recompiled game does all of that; the port draws what the arcade's route console draws from
  `CSeq::Get2DInfoJctSelect` (state 1 deciding, 2 left, 3 right, and the two destinations): "Choose a direction" for 80 frames, the two destination
  boards (left orange, right blue, the names from `nsJunctionData`'s table), and on the decision the chosen board flashes and the screen's edges flare
  in its colour (`hud.c draw_junction`). Not drawn: the in-world arrow markers of `clCourseJunctionManager`.
- **The last stretch.** Once the front car is within 700 m of the goal: "Keep going!" with "until rival crosses the finish" for the machines behind
  (the metres are the front car's to go), "Don't get caught!" with "till goal" for the one in front (its own), the metres big beneath, and an alarm
  (`hud.c draw_vs_last`; the alarm is `approach4`, a stand-in: the sound heard on the real machine could not be matched to a file).
- **The end.** A place overlay (`menu_g_finish_vs`: "1st/4") for every machine's own place; the car's sound goes quiet; at TIME UP the clock's own
  screen comes first, then the places by who is ahead.
- **Leaving.** The host takes a machine out of the lobby on its BYE or once its race packets have been silent 5 s. The others see the lobby lose it:
  its car leaves the road (`car_link_drop`) and "(car name) HAS DISCONNECTED." shows at the top; with every other machine gone the round ends and the
  one left wins. The host closing the game (or its window) says so to everyone at once (`CLOSED`, sent on `app_vs_leave` too); a host that just vanishes is
  given up on after 5 s without its packets in a race (10 s in the lobby). The goal wait (`CState::Ck_GoalWait_2_Goal`) needs every machine's
  "finished" flag, so the one that left has it set for it (the others finish and are placed; it ranks last).

Test helpers: `tools/test/vs2.ps1` runs two players on one PC (host and client, both driving themselves, shots and logs per player);
`MT3DX_VSAUTO=host[:n]|join`, `MT3DX_GOTO=36` (the versus screen) and `MT3DX_VSKM=<km>` (a shorter race) are what it sets.

## Ping on the HUD

Every race packet carries its sender's round trip to the host after the car link packet (two bytes, ms; 0xffff not measured yet), then a flags byte (bit 0: its headlights are flashing off); a rival's headlights show as its packets say, the arcade's rival code shows them always. The HUD shows it as P: n under each rival's ADVANTAGE, coloured green below 75 ms, yellow to 120, orange to 200, red above. Hosts and servers relay the packet whole; vs_host.c accepts the three extra bytes (a host from before them drops the packets of a newer client).

## Protocol versions

`VS_BUILD` (src/app.c) is the versus's protocol version, sent in every player's profile. Players on different versions show VERSION MISMATCH (with both numbers) and the host cannot start. It changes only when a change makes two machines disagree: the wire (lobby, profile, statuses, race packet) or what both ends must compute alike to stay in sync (physics and car code, start, track and ramp choice). Looks, menus, options and fixes that stay on one machine leave it alone. Bump it by one in the commit that changes the wire and add a line here; static asserts beside it hold the wire's numbers so a quiet change to them fails the build.

| Version | Change |
|---|---|
| 169 | the first build with the mark |
| 170 | the race packet's trailing round trip (ping), the host's statuses carrying the area card after the vote, the ping on the HUD |
| 171 | a player's READY in the lobby (status 12), the host's START waiting for every other player's, AUTO READY; a race that stands still 30 s sends everyone back |

## Ready and the hang check

A player (not the host) clicks READY in the lobby before each round; the host's START waits for every other player's (the lobby shows NOT READY / READY). The tick goes after every round too, unless AUTO READY (the lobby's button under READY, saved in the options) is on, which is the old behaviour. A race whose clock does not move for 30 s (loading for a machine that never finishes, nothing heard from the host) sends every machine back to the versus screen with THE RACE TIMED OUT. PLEASE TRY AGAIN; `MT3DX_VSAUTO=join` test players ready themselves.

## Windows firewall

Windows asks "allow this program through the firewall?" once, when the game first listens, and keeps a No as a block rule with no second question: nobody can then join a hosted game. GAME OPTIONS > NETWORK OPTIONS (Windows player) has an ALLOW THROUGH FIREWALL button, next to OPEN LOG FOLDER, COPY DIAGNOSTICS, PING ON THE HUD and DETAILED NET LOG (the net log lines of `MT3DX_NETLOG=1`, on by default so a bug report's log has them): it runs `netsh advfirewall firewall` in an elevated command window (Windows shows its own administrator prompt) to delete the program's rules and add allow rules for it (UDP and TCP, every profile). The same by hand, in an administrator command prompt: `netsh advfirewall firewall delete rule name=all program="<path>\super3dx.exe"` and two `add rule ... dir=in action=allow program=... protocol=udp|tcp profile=any`.

## Logs and privacy

A log is attached to bug reports in public, so it must not name the player. The Windows player scrubs the last run's log when it starts (a crashed run too) and its own when it closes; Android scrubs the last run's at start. The scrub takes out public IPv4 addresses (a room code is one in 15 digits: `[public ip hidden]`), room codes (`[room code hidden]`) and the user name in paths (`Users\<user>`). Private LAN addresses (192.168.x.x, 10.x.x.x, 172.16-31.x.x) stay: a LAN problem is diagnosed from them. The log lines that name an address mask a public one where they are written, and the network options' connection lines and the copied diagnostics never carry the room code. A log of a run still going (or from a platform that does not scrub, like the Pi's `game.log`) is as it was written: check it before posting.
