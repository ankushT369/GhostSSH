#include "mongoose.h"
#include "ghost.h"

void ghost_http_handler(struct mg_connection *http, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
            mg_ws_upgrade(http, hm, NULL);   // Upgrade HTTP to WebSocket
            MG_INFO(("Client upgraded to WS"));
        }
    } else if (ev == MG_EV_WS_OPEN) {
        MG_INFO(("WS connection fully established"));
        create_local_url(TCP, SSH_PORT);
        struct mg_connection *tcp = mg_connect(http->mgr,
                url_buffer, ghost_tcp_handler, NULL);

        if (tcp) {
            http->fn_data = tcp;
            tcp->fn_data = http;
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        struct mg_connection *tcp = (struct mg_connection *) http->fn_data;

        MG_INFO(("Debug Data: %.*s", wm->data.len, wm->data.buf));
        if (tcp)
            mg_send(tcp, wm->data.buf, wm->data.len);
    }
}
