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
#include "mongoose.h"
#include "ghost.h"
#include <signal.h>

static volatile sig_atomic_t shutdown_flag = 0;

void signal_handler(int sig) {
    (void)sig;
    shutdown_flag = 1;
    MG_INFO(("Received signal, initiating shutdown..."));
}

void ghost_http_handler(struct mg_connection *http, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
            mg_ws_upgrade(http, hm, NULL);   // Upgrade HTTP to WebSocket
            MG_INFO(("Upgraded to WebSocket"));
        }
    } else if (ev == MG_EV_WS_OPEN) {
        MG_INFO(("WebSocket connection is successfully established"));
        create_local_url(TCP, config.sshd_port);
        struct mg_connection *tcp = mg_connect(http->mgr,
                url_buffer, ghost_tcp_handler, NULL);

        if (tcp) {
            http->fn_data = tcp;
            tcp->fn_data = http;
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        struct mg_connection *tcp = (struct mg_connection *) http->fn_data;

        MG_DEBUG(("Data from websocket in SERVER mode: %.*s", wm->data.len, wm->data.buf));
        if (tcp) {
            mg_send(tcp, wm->data.buf, wm->data.len);
        }
    } else if (ev == MG_EV_CLOSE) {
        MG_INFO(("HTTP connection closed"));
        struct mg_connection *tcp = (struct mg_connection *) http->fn_data;
        if (tcp) {
            mg_close_connection(tcp);
            tcp->fn_data = NULL;
            http->fn_data = NULL;
        }
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    mg_mgr_init(&mgr, NULL);
    mg_log_set(MG_LOG_INFO);

    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    mg_http_listen(&mgr, mg_str("0.0.0.0"), 8080);

    while (!shutdown_flag) {
        mg_mgr_poll(&mgr, 100);
    }

    MG_INFO(("Shutting down..."));

    // Gracefully close all connections
    struct mg_connection *conn, *next_conn;
    mg_connection_iterate(&mgr, conn, next_conn) {
        if (conn->state == MG_CONN_OPEN) {
            mg_close_connection(conn);
        }
    }

    mg_mgr_free(&mgr);
    MG_INFO(("Shutdown complete."));

    return 0;
}