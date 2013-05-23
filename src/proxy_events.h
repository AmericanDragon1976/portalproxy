#ifndef PROXY_EVENTS_H
#define PROXY_EVENTS_H

void monitor_read_cb(struct bufferevent *bev, void *serv);
void client_connect_cb(struct evconnlistener *listener, evutil_socket_t fd, 
    struct sockaddr *address, int socklen, void *ctx);
void proxy_read_cb(struct bufferevent *bev, void *srv_pck);
void event_cb(struct bufferevent *bev, short what, void *ctx);
void signal_cb(evutil_socket_t sig, short events, void *user_data);

#endif