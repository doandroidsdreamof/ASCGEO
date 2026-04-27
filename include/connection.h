#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h>
#include <stdbool.h>

#define MAX_CONNECTIONS 256
#define CONN_NAME_LEN 256

typedef struct {
    int pid;
    char name[CONN_NAME_LEN];
    int proto;
    char remote_ip[INET6_ADDRSTRLEN];
    int remote_port;
    int tcp_state;
    char country[64];
    char city[128];
} ConnInfo;

void connection_update(void);
void connection_render(void);
void connection_mark_dirty(void);
int connection_count(void);
void connection_init(void);
void connection_cleanup(void);

#endif