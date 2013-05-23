#ifndef AGENT_H
#define AGENT_H

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <event2/bufferevent_ssl.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define svc_nm_siz     30      // length of all char arrays holding service names
#define comp_add_len    22      // length of char arrays holding address a.d.c.e:portnum
#define file_nm_len     100     // length of file names including path to them
#define ip_len          16      // length of ip portion of address
#define port_len        6       // length of port number portion of address
#define hook_len        40      // largest length allowed for a hook name from agent config file
#define monitor_addr    "127.0.0.1:4000"
#define reply_len       500     // largest size for a reply from an executed file. 
#define no_such_command 404     // the int returned when attempting to execute a hook/command and there is no match to be used.

typedef struct hook_path_pair {
    struct hook_path_pair   *next;
    char                    hook[hook_len];
    char                    path[file_nm_len];
} hook_path_pair;

typedef struct service_list {
    struct service_list         *next;
    char                        name[svc_nm_siz];
    hook_path_pair              *cmd_lst;
} svc_lst;

typedef struct buffer_list {
    struct buffer_list      *next;
    struct bufferevent      *bev;
} buffer_list;

typedef struct {
    svc_lst    *s_list;
    buffer_list *b_list;
} list_heads;

void 
usage(); 

bool 
validate_args(int argc, char **argv);

svc_lst*
new_null_svc_lst();

svc_lst*
new_svc_lst(svc_lst *nxt, hook_path_pair *cmd_lst_head);

hook_path_pair*
new_null_hook_path_pair();

hook_path_pair*
new_hook_path_pair(hook_path_pair *nxt);

bool 
parse_address(char *addrToParse, char *ip_addr, char* port_num);

void
listen_for_monitors(struct event_base *base, struct evconnlistener *listner, list_heads *heads);

void
free_lists_memory(list_heads *heads);

void 
free_service_nodes(svc_lst *service_list);

void 
free_cmd_lst(hook_path_pair *cmds);

void 
free_buffers(buffer_list *bevs);
#endif
