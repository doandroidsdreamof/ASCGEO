# ASCGEO

Terminal-based network visualization tool built in C with ncurses.

![ASCGEO](assets/cover.png)


## Modes

- **GEOMAP** — Live world map with active network connections, process info, remote IPs, ports, TCP state. *(in progress)*
- **ASCROUTE** — Traceroute implementation with ASCII path visualization. *(in progress)*
- **NETMAP** — Local network topology mapper using ARP and OUI vendor lookup. *(in progress)*

## Build

```bash
make
sudo ./ascgeo
```

## Dependencies

- ncurses
- macOS (libproc) — Linux support planned

## Structure

```
main.c          — mode switching, main loop
terminal.c      — ncurses window management, layout
map.c           — ASCII world map loading/rendering
connection.c    — live socket scanning via libproc
hashmap.c       — FNV-1a hashmap for IP caching
```