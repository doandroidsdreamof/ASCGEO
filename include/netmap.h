#ifndef NETMAP_H
#define NETMAP_H

#include <stdbool.h>
#include <netinet/in.h>

#define MAX_DEVICES 64

typedef struct {
    char ip[INET_ADDRSTRLEN];
    unsigned char mac[6];
    char mac_str[18];       // "aa:bb:cc:dd:ee:ff"
    char vendor[64];
    char device_type[32];   // "Router", "Phone", "Laptop", "Printer", etc.
    bool is_gateway;
} NetDevice;

void netmap_init(void);
void netmap_cleanup(void);
void netmap_update(void);
void netmap_render(void);
void netmap_mark_dirty(void);
int netmap_device_count(void);

#endif