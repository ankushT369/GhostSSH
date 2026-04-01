#ifndef GHOST_H
#define GHOST_H

#include "mongoose.h"

/* default ports */
#define SSH_PORT 22

#define HTTP_PORT 7777
#define TCP_PORT 8888

/* protocol */
#define TCP 0
#define UDP 1
#define HTTP 2
#define WS 3
#define LIMIT 4

/* mode */
#define SERVER 0
#define CLIENT 1

/* length */
#define MAX_LEN 1024

typedef struct ghost_config {
    /* server side config */
    int http_server_port;
    int sshd_port;


    /* client side config */
    int tcp_server_port;
    char ws_url[MAX_LEN];
} ghost_config;

extern int mode;
extern const char* proto_str[LIMIT];
extern char url_buffer[MAX_LEN];
extern int upgrade_done;

extern ghost_config config; // global configuration

/* API */
void ghost_http_handler(struct mg_connection *http, int ev, void *ev_data);
void ghost_ws_handler(struct mg_connection *ws, int ev, void *ev_data);
void ghost_tcp_handler(struct mg_connection *tcp, int ev, void *ev_data);
void ghost_tls_handshake(struct mg_connection *tcp);


/* util */
void convert_to_wss(const char* url);
void create_local_url(int protocol, int port);
void strip_https_for_tls_host(char *tls_host);

#endif // GHOST_H
