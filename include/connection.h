#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h>
#include <stdbool.h>

#define MAX_CONNECTIONS 256
#define CONN_NAME_LEN 256

typedef struct {
    int pid;
    char name[CONN_NAME_LEN];
    int proto;              // IPPROTO_TCP or IPPROTO_UDP
    char remote_ip[INET6_ADDRSTRLEN];
    int remote_port;
    int tcp_state;         
} ConnInfo;

void connection_update(void);
void connection_render(void);
void connection_mark_dirty(void);
int connection_count(void);

#endif