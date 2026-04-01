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
#include "ghost.h"

int mode = -1;
const char* proto_str[LIMIT] = { "tcp", "udp", "http", "ws" };

char url_buffer[MAX_LEN]; // global url buffer
int upgrade_done = 0;

ghost_config config = {
    .http_server_port = HTTP_PORT,
    .sshd_port = SSH_PORT,
    .tcp_server_port = TCP_PORT,
    .ws_url = "",
};

/* static declaration */
static void ghost_parse_args(int argc, char* arg[]);
static void ghost_print_usage();

static void ghost_parse_args(int argc, char *argv[]) {
    // Detect mode
    if (strcmp(argv[1], "server") == 0) {
        mode = SERVER;
    } else if (strcmp(argv[1], "client") == 0) {
        mode = CLIENT;
    } else {
        ghost_print_usage();
        exit(1);
    }

    // Parse remaining arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            int port = atoi(argv[++i]);

            if (mode == SERVER)
                config.http_server_port = port;
            else
                config.tcp_server_port = port;
        }
        else if (strcmp(argv[i], "--ssh") == 0 && i + 1 < argc) {
            config.sshd_port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
            strncpy(config.ws_url, argv[++i], MAX_LEN - 1);
            config.ws_url[MAX_LEN - 1] = '\0';  // safety null-termination
        }
        else {
            printf("Unknown argument: %s\n", argv[i]);
            ghost_print_usage();
            exit(1);
        }
    }
}

void ghost_print_usage() {
    printf("SERVER MODE:\n");
    printf("  ghost server --port <http_port> --ssh <ssh_port>\n");
    printf("    --port : Port where WebSocket/HTTP server runs (e.g. 7777)\n");
    printf("    --ssh  : SSH daemon port (default: 22)\n\n");

    printf("CLIENT MODE:\n");
    printf("  ghost client --connect <url> --port <local_port>\n");
    printf("    --connect : Remote WebSocket/HTTP URL (e.g. https://your_tunnel_url.com)\n");
    printf("    --port    : Local port to expose (e.g. 8888)\n\n");

    printf("EXAMPLES:\n");
    printf("  ghost server --port 7777 --ssh 22\n");
    printf("  ghost client --connect https://your_tunnel_url.com --port 8888\n\n");

    printf("USAGE WITH SSH:\n");
    printf("  ssh user@localhost -p 8888\n\n");
}

void ghost_tcp_handler(struct mg_connection *tcp, int ev, void *ev_data) {
    if (ev == MG_EV_ACCEPT) {
        MG_INFO(("Client accepted via TCP successfully"));
        ghost_tls_handshake(tcp);
    } else if (ev == MG_EV_CONNECT) {
        upgrade_done = 1;
        MG_INFO(("Client connected via TCP successfully"));
    }
    else if (ev == MG_EV_CLOSE) {
        MG_INFO(("Client disconnected"));
    }
    else if (ev == MG_EV_READ) {
        struct mg_connection *ws = (struct mg_connection *) tcp->fn_data;
        MG_INFO(("Client read data: %.*s", tcp->recv.len, tcp->recv.buf));

        if (upgrade_done && ws && !ws->is_closing) {
            mg_ws_send(ws, tcp->recv.buf, tcp->recv.len, WEBSOCKET_OP_BINARY);
            mg_iobuf_del(&tcp->recv, 0, tcp->recv.len);
        }
    } else if (ev == MG_EV_ERROR) {
        MG_INFO(("CLIENT error: %s", (char *) ev_data));
    }
}

/**
 * ghost server --port 7777 --ssh 9876 (by default its 22 because sshd runs on 22)
 * ghost client --connect https://your_tunnel_url.com --port 8888 (in this port your ssh will connect e.g.: ssh hostname@localost -p 8888)
 */
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        ghost_print_usage();
        exit(1);
    }

    ghost_parse_args(argc, argv);

    /* initialize the event manager */
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    if (mode == SERVER) {
        /* create HTTP listener */
        create_local_url(HTTP, config.http_server_port);
        struct mg_connection *conn = mg_http_listen(&mgr, url_buffer, ghost_http_handler, NULL);
        if (conn == NULL) MG_ERROR(("Cannot create listener"));

        /* blocking wait */
        for (;;) {
            mg_mgr_poll(&mgr, -1);
        }

    } else {
        /* create TCP listener */
        create_local_url(TCP, config.tcp_server_port);
        struct mg_connection *conn = mg_listen(&mgr, url_buffer, ghost_tcp_handler, NULL);
        if (conn == NULL) MG_ERROR(("Cannot create listener"));

        /* blocking wait */
        for (;;) {
            mg_mgr_poll(&mgr, -1);
        }

    }

    mg_mgr_free(&mgr);
    return 0;
}

