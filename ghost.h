/*
 * Copyright 2024-2026 Ankush Mondal
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
