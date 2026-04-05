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
        MG_INFO(("Shutting down server..."));
    }
}
