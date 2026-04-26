#include "connection.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/proc_info.h>
#include <libproc.h>
#include <netinet/in.h>

static ConnInfo connections[MAX_CONNECTIONS]; // TODO => hasmap
static int conn_count = 0;
static bool dirty = true;

static const char *tcp_state_str(int state)
{
    switch (state)
    {
    case TSI_S_CLOSED:
        return "CLOSED";
    case TSI_S_LISTEN:
        return "LISTEN";
    case TSI_S_SYN_SENT:
        return "SYN_SENT";
    case TSI_S_SYN_RECEIVED:
        return "SYN_RECV";
    case TSI_S_ESTABLISHED:
        return "ESTABLISHED";
    case TSI_S__CLOSE_WAIT:
        return "CLOSE_WAIT";
    case TSI_S_FIN_WAIT_1:
        return "FIN_WAIT_1";
    case TSI_S_CLOSING:
        return "CLOSING";
    case TSI_S_LAST_ACK:
        return "LAST_ACK";
    case TSI_S_FIN_WAIT_2:
        return "FIN_WAIT_2";
    case TSI_S_TIME_WAIT:
        return "TIME_WAIT";
    default:
        return "UNKNOWN";
    }
}

static int is_remote_ip(const char *ip)
{
    if (strcmp(ip, "0.0.0.0") == 0)
        return 0;
    if (strcmp(ip, "127.0.0.1") == 0)
        return 0;
    if (strcmp(ip, "::") == 0)
        return 0;
    if (strcmp(ip, "::1") == 0)
        return 0;
    if (strncmp(ip, "192.168.", 8) == 0)
        return 0;
    if (strncmp(ip, "10.", 3) == 0)
        return 0;
    if (strncmp(ip, "fe80:", 5) == 0)
        return 0;
    return 1;
}

// Returns 1 if connection was added, 0 if skipped
static int extract_connection(int pid, struct socket_fdinfo *sinfo)
{
    if (conn_count >= MAX_CONNECTIONS)
        return 0;

    struct socket_info *si = &sinfo->psi;

    int family = si->soi_family;
    if (family != AF_INET && family != AF_INET6)
        return 0;

    int proto = si->soi_protocol;
    if (proto != IPPROTO_TCP && proto != IPPROTO_UDP)
        return 0;

    char remote_ip[INET6_ADDRSTRLEN];

    struct in_sockinfo *in;
    if (proto == IPPROTO_TCP)
    {
        in = &si->soi_proto.pri_tcp.tcpsi_ini;
    }
    else
    {
        in = &si->soi_proto.pri_in;
    }

    if (family == AF_INET)
    {
        inet_ntop(AF_INET, &in->insi_faddr.ina_46.i46a_addr4, remote_ip, sizeof(remote_ip));
    }
    else
    {
        inet_ntop(AF_INET6, &in->insi_faddr.ina_6, remote_ip, sizeof(remote_ip));
    }

    int local_port = ntohs(in->insi_lport);
    int remote_port = ntohs(in->insi_fport);

    if (local_port == 0 && remote_port == 0)
        return 0;
    if (!is_remote_ip(remote_ip))
        return 0;

    ConnInfo *c = &connections[conn_count];
    c->pid = pid;
    c->proto = proto;
    c->remote_port = remote_port;
    c->tcp_state = (proto == IPPROTO_TCP) ? si->soi_proto.pri_tcp.tcpsi_state : 0;
    strncpy(c->remote_ip, remote_ip, sizeof(c->remote_ip) - 1);
    c->remote_ip[sizeof(c->remote_ip) - 1] = '\0';

    proc_name(pid, c->name, sizeof(c->name));
    if (strlen(c->name) == 0)
    {
        snprintf(c->name, sizeof(c->name), "pid:%d", pid);
    }

    conn_count++;
    return 1;
}

void connection_update(void)
{
    int pid_count = proc_listallpids(NULL, 0);
    // TODO add proper logger
    if (pid_count <= 0)
        return;

    pid_t *pids = malloc(sizeof(pid_t) * pid_count);
    if (pids == NULL)
        return;

    pid_count = proc_listallpids(pids, sizeof(pid_t) * pid_count);

    conn_count = 0;

    for (int i = 0; i < pid_count; i++)
    {
        int pid = pids[i];

        int buf_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, NULL, 0);
        if (buf_size <= 0)
            continue;

        struct proc_fdinfo *fds = malloc(buf_size);
        if (fds == NULL)
            continue;

        int actual = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds, buf_size);
        int fd_count = actual / sizeof(struct proc_fdinfo);

        for (int j = 0; j < fd_count; j++)
        {
            if (fds[j].proc_fdtype != PROX_FDTYPE_SOCKET)
                continue;

            struct socket_fdinfo sinfo;
            int result = proc_pidfdinfo(
                pid, fds[j].proc_fd,
                PROC_PIDFDSOCKETINFO,
                &sinfo, sizeof(sinfo));
            if (result <= 0)
                continue;

            extract_connection(pid, &sinfo);
        }

        free(fds);
    }

    free(pids);
    dirty = true;
}

void connection_render(void)
{
    if (!dirty || !win_conn)
        return;

    werase(win_conn);
    box(win_conn, 0, 0);
    mvwprintw(win_conn, 0, 2, " CONNECTIONS (%d) ", conn_count);

    int win_rows, win_cols;
    getmaxyx(win_conn, win_rows, win_cols);
    (void)win_cols;

    // Header row
    mvwprintw(win_conn, 1, 2, "%-24s %-8s %-44s %-8s %-14s %-16s %-16s",
              "PROCESS", "PROTO", "REMOTE", "PORT", "STATE", "COUNTRY", "CITY");

    int inner_rows = win_rows - 3; // box top + header + box bottom
    int rows_to_draw = (conn_count < inner_rows) ? conn_count : inner_rows;

    for (int i = 0; i < rows_to_draw; i++)
    {
        ConnInfo *c = &connections[i];
        const char *proto_str = (c->proto == IPPROTO_TCP) ? "TCP" : "UDP";
        const char *state_str = (c->proto == IPPROTO_TCP) ? tcp_state_str(c->tcp_state) : "-";
        // TODO implement hashmap
        mvwprintw(win_conn, 2 + i, 2, "%-24s %-8s %-44s %-8d %-14s %-16s %-16s",
                  c->name, proto_str, c->remote_ip, c->remote_port, state_str, "-", "-");
    }

    wrefresh(win_conn);
    dirty = false;
}

void connection_mark_dirty(void)
{
    dirty = true;
}

int connection_count(void)
{
    return conn_count;
}