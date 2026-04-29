#include "netmap.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

static NetDevice devices[MAX_DEVICES];
static int device_count = 0;
static bool dirty = true;
static char gateway_ip[INET_ADDRSTRLEN] = {0};
static char my_ip[INET_ADDRSTRLEN] = {0};

// instead use list from parsed oui.txt 
static struct
{
    unsigned char prefix[3];
    const char *vendor;
} oui_table[] = {
    {{0xAC, 0xDE, 0x48}, "Apple"},
    {{0x3C, 0x22, 0xFB}, "Apple"},
    {{0xA4, 0x83, 0xE7}, "Apple"},
    {{0x28, 0x6C, 0x07}, "Apple"},
    {{0xF0, 0x18, 0x98}, "Apple"},
    {{0x14, 0x7D, 0xDA}, "Apple"},
    {{0xDC, 0xA6, 0x32}, "Raspberry Pi"},
    {{0xB8, 0x27, 0xEB}, "Raspberry Pi"},
    {{0x00, 0x1A, 0x2B}, "HP"},
    {{0x00, 0x17, 0xA4}, "HP"},
    {{0x30, 0xE1, 0x71}, "HP"},
    {{0x00, 0x1E, 0x8F}, "Canon"},
    {{0x00, 0x1B, 0xA9}, "Brother"},
    {{0xC0, 0x25, 0xE9}, "TP-Link"},
    {{0x50, 0xC7, 0xBF}, "TP-Link"},
    {{0x14, 0xCC, 0x20}, "TP-Link"},
    {{0x20, 0xCF, 0x30}, "ASUS"},
    {{0x04, 0x92, 0x26}, "ASUS"},
    {{0xB0, 0x6E, 0xBF}, "Netgear"},
    {{0xC4, 0x3D, 0xC7}, "Netgear"},
    {{0x60, 0x38, 0xE0}, "Belkin"},
    {{0x00, 0x1C, 0xDF}, "Belkin"},
    {{0x98, 0xDA, 0xC4}, "TP-Link"},
    {{0x34, 0x97, 0xF6}, "ASUS"},
    {{0xCC, 0x50, 0xE3}, "Samsung"},
    {{0x8C, 0x79, 0xF5}, "Samsung"},
    {{0xBC, 0x54, 0x36}, "Samsung"},
    {{0x00, 0x50, 0x56}, "VMware"},
    {{0x08, 0x00, 0x27}, "VirtualBox"},
    {{0x00, 0x15, 0x5D}, "Hyper-V"},
    {{0xB4, 0xFB, 0xE4}, "Ubiquiti"},
    {{0x78, 0x8A, 0x20}, "Ubiquiti"},
    {{0x44, 0xD9, 0xE7}, "Ubiquiti"},
    {{0x64, 0xCC, 0x2E}, "Xiaomi"},
    {{0x9C, 0x99, 0xA0}, "Xiaomi"},
    {{0x28, 0x6C, 0x07}, "Xiaomi"},
    {{0x50, 0x64, 0x2B}, "Xiaomi"},
    {{0x7C, 0x1C, 0x4E}, "Xiaomi"},
    {{0xFC, 0x64, 0xBA}, "Xiaomi"},
    {{0x84, 0xD8, 0x1B}, "Xiaomi"},
    {{0x78, 0x02, 0xF8}, "Xiaomi"},
    {{0x00, 0x22, 0x07}, "ZTE"},
    {{0x54, 0x22, 0xF8}, "ZTE"},
    {{0xC8, 0x64, 0xC7}, "ZTE"},
    {{0x00, 0x19, 0xCB}, "ZTE"},
    {{0xE0, 0x19, 0x54}, "ZTE"},
    {{0x48, 0xA4, 0x72}, "ZTE"},
    {{0x00, 0x25, 0x9E}, "Huawei"},
    {{0x00, 0xE0, 0xFC}, "Huawei"},
    {{0x48, 0xDB, 0x50}, "Huawei"},
    {{0x88, 0x53, 0x95}, "Huawei"},
    {{0xCC, 0xA2, 0x23}, "Huawei"},
    {{0x4C, 0x8B, 0xEF}, "Huawei"},
    {{0x3C, 0x06, 0x30}, "MacBookPro"},
    {{0x4C, 0x32, 0x75}, "MacBookPro"},
    {{0x78, 0x4F, 0x43}, "MacBookPro"},
    {{0x40, 0x4E, 0x36}, "Xiaomi"},
    {{0x18, 0x59, 0x36}, "Xiaomi"},
    {{0x60, 0x83, 0xA3}, "TurkTelekom"},
    {{0x00, 0x25, 0x67}, "TurkTelekom"},
    {{0xD0, 0xC5, 0xF3}, "TurkTelekom"},
    {{0x00, 0x1F, 0xA3}, "TurkTelekom"},
};
static const int oui_table_size = sizeof(oui_table) / sizeof(oui_table[0]);

static const char *lookup_vendor(unsigned char mac[6])
{
    for (int i = 0; i < oui_table_size; i++)
    {
        if (mac[0] == oui_table[i].prefix[0] &&
            mac[1] == oui_table[i].prefix[1] &&
            mac[2] == oui_table[i].prefix[2])
        {
            return oui_table[i].vendor;
        }
    }
    return "Unknown";
}

static const char *guess_device_type(const char *vendor, bool is_gateway)
{
    if (is_gateway)
        return "Router";
    if (strcmp(vendor, "MacBookPro") == 0) 
        return "MacBook Pro"; 
    if (strcmp(vendor, "Apple") == 0)
        return "Apple Device";
    if (strcmp(vendor, "Samsung") == 0)
        return "Phone/TV";
    if (strcmp(vendor, "HP") == 0)
        return "Printer";
    if (strcmp(vendor, "Canon") == 0)
        return "Printer";
    if (strcmp(vendor, "Brother") == 0)
        return "Printer";
    if (strcmp(vendor, "Raspberry Pi") == 0)
        return "IoT";
    if (strcmp(vendor, "TP-Link") == 0)
        return "Network";
    if (strcmp(vendor, "ASUS") == 0)
        return "Network";
    if (strcmp(vendor, "Netgear") == 0)
        return "Network";
    if (strcmp(vendor, "Belkin") == 0)
        return "Network";
    if (strcmp(vendor, "Ubiquiti") == 0)
        return "Network";
    if (strcmp(vendor, "VMware") == 0)
        return "VM";
    if (strcmp(vendor, "VirtualBox") == 0)
        return "VM";
    if (strcmp(vendor, "Hyper-V") == 0)
        return "VM";
    if (strcmp(vendor, "Xiaomi") == 0)
        return "Phone"; // e.g. it can be another devices as well just keep it for testing
    if (strcmp(vendor, "ZTE") == 0)
        return "Modem";
    if (strcmp(vendor, "Huawei") == 0)
        return "Modem";
    if (strcmp(vendor, "TurkTelekom") == 0)
        return "Modem";
    return "Device";
}

// Get default gateway IP via sysctl route table
static void find_gateway(void)
{
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_GATEWAY};
    size_t len = 0;

    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return;

    char *buf = malloc(len);
    if (buf == NULL)
        return;

    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0)
    {
        free(buf);
        return;
    }

    char *ptr = buf;
    char *end = buf + len;

    while (ptr < end)
    {
        struct rt_msghdr *rtm = (struct rt_msghdr *)ptr;
        struct sockaddr_in *dst = (struct sockaddr_in *)(rtm + 1);
        struct sockaddr_in *gw = (struct sockaddr_in *)((char *)dst + dst->sin_len);

        // Default route has dst 0.0.0.0
        if (dst->sin_addr.s_addr == 0 && gw->sin_family == AF_INET)
        {
            inet_ntop(AF_INET, &gw->sin_addr, gateway_ip, sizeof(gateway_ip));
            break;
        }

        ptr += rtm->rtm_msglen;
    }

    free(buf);
}

// Get own IP address from en0 (ethernet or WIFI)
static void find_my_ip(void)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1)
        return;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (strcmp(ifa->ifa_name, "en0") != 0)
            continue;

        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &sa->sin_addr, my_ip, sizeof(my_ip));
        break;
    }

    freeifaddrs(ifaddr);
}

// Read ARP table via sysctl
static void read_arp_table(void)
{
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO};
    size_t len = 0;

    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        return;

    char *buf = malloc(len);
    if (buf == NULL)
        return;

    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0)
    {
        free(buf);
        return;
    }

    device_count = 0;
    char *ptr = buf;
    char *end = buf + len;

    while (ptr < end && device_count < MAX_DEVICES)
    {
        struct rt_msghdr *rtm = (struct rt_msghdr *)ptr;

        if (rtm->rtm_msglen == 0)
            break;

        struct sockaddr_in *sin = (struct sockaddr_in *)(rtm + 1);
        struct sockaddr_dl *sdl = (struct sockaddr_dl *)((char *)sin + sin->sin_len);

        // Skip incomplete entries (no MAC resolved)
        if (sdl->sdl_alen != 6)
        {
            ptr += rtm->rtm_msglen;
            continue;
        }

        NetDevice *d = &devices[device_count];

        // Extract IP
        inet_ntop(AF_INET, &sin->sin_addr, d->ip, sizeof(d->ip));

        // Skip own IP
        if (strcmp(d->ip, my_ip) == 0)
        {
            ptr += rtm->rtm_msglen;
            continue;
        }
        
        // Extract MAC
        unsigned char *mac = (unsigned char *)LLADDR(sdl);
        memcpy(d->mac, mac, 6);
        snprintf(d->mac_str, sizeof(d->mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Vendor lookup
        const char *vendor = lookup_vendor(d->mac);
        strncpy(d->vendor, vendor, sizeof(d->vendor) - 1);
        d->vendor[sizeof(d->vendor) - 1] = '\0';

        d->is_gateway = (strcmp(d->ip, gateway_ip) == 0);

        // Device type
        const char *dtype = guess_device_type(vendor, d->is_gateway);
        strncpy(d->device_type, dtype, sizeof(d->device_type) - 1);
        d->device_type[sizeof(d->device_type) - 1] = '\0';

        device_count++;
        ptr += rtm->rtm_msglen;
    }

    free(buf);
}

void netmap_init(void)
{
    find_gateway();
    find_my_ip();
}

void netmap_cleanup(void)
{
    // TODO implement
}

void netmap_update(void)
{
    read_arp_table();
    dirty = true;
}

// Draw a line from (x0,y0) to (x1,y1) using Bresenham's algorithm
static void draw_line(WINDOW *win, int y0, int x0, int y1, int x1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        // Skip start and end points (occupied by labels)
        if (!(x0 == x1 && y0 == y1))
        {
            char ch;
            if (dx == 0)
                ch = '|';
            else if (dy == 0)
                ch = '-';
            else if ((sx > 0 && sy > 0) || (sx < 0 && sy < 0))
                ch = '\\';
            else
                ch = '/';

            mvwaddch(win, y0, x0, ch);
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void netmap_render(void)
{
    if (!dirty || !win_map)
        return;

    werase(win_map);
    box(win_map, 0, 0);
    mvwprintw(win_map, 0, 0, " LAN MAP (%d devices) ", device_count);

    int win_rows, win_cols;
    getmaxyx(win_map, win_rows, win_cols);

    int inner_rows = win_rows - 2;
    int inner_cols = win_cols - 2;

    int center_x = inner_cols / 2;
    int center_y = inner_rows / 2;

    if (device_count == 0)
    {
        mvwprintw(win_map, center_y + 1, center_x - 10, "No devices found");
        wrefresh(win_map);
        dirty = false;
        return;
    }

    // Draw router/gateway at center
    wattron(win_map, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_map, center_y, center_x - 3, "[Router]");
    wattroff(win_map, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win_map, center_y + 1, center_x - (int)strlen(gateway_ip) / 2, "%s", gateway_ip);

    // Calculate radius based on window size
    int radius_x = (inner_cols / 3);
    int radius_y = (inner_rows / 3);

    // Place devices in a circle around the router
    double pi = 3.14159265358979;

    for (int i = 0; i < device_count; i++)
    {
        NetDevice *d = &devices[i];

        // Skip gateway — already drawn at center
        if (d->is_gateway)
            continue;

        double angle = (2.0 * pi * i) / device_count;
        int dev_x = center_x + (int)(radius_x * cos(angle));
        int dev_y = center_y + (int)(radius_y * sin(angle) * 0.5);

        // Bounds check
        if (dev_x < 2)
            dev_x = 2;
        if (dev_x > inner_cols - 16)
            dev_x = inner_cols - 16;
        if (dev_y < 2)
            dev_y = 2;
        if (dev_y > inner_rows - 3)
            dev_y = inner_rows - 3;

        wattron(win_map, COLOR_PAIR(4));
        draw_line(win_map, center_y, center_x + 1, dev_y, dev_x - 1);
        wattroff(win_map, COLOR_PAIR(4));


        wattron(win_map, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win_map, dev_y - 1, dev_x, "[%s]", d->device_type);
        wattroff(win_map, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win_map, dev_y, dev_x, "%s", d->ip);
        mvwprintw(win_map, dev_y + 1, dev_x, "%s", d->vendor);
    }

    wrefresh(win_map);
    dirty = false;
}

void netmap_mark_dirty(void)
{
    dirty = true;
}

int netmap_device_count(void)
{
    return device_count;
}