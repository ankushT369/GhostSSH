#include "ghost.h"

#include <stdio.h>

/* global buffer for tls host */
char tls_host[MAX_LEN];

void ghost_ws_handler(struct mg_connection *ws, int ev, void *ev_data) {
    if (ev == MG_EV_WS_OPEN) {
        MG_INFO(("WebSocket connection is successfully established"));
        upgrade_done = 1;
        
        struct mg_connection *tcp = (struct mg_connection *) ws->fn_data;
        if (tcp && !tcp->is_closing) {
            mg_ws_send(ws, tcp->recv.buf, tcp->recv.len, WEBSOCKET_OP_BINARY);
            mg_iobuf_del(&tcp->recv, 0, tcp->recv.len);
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message* wm = (struct mg_ws_message *)ev_data;
        struct mg_connection* tcp = (struct mg_connection *)ws->fn_data;
        fwrite(wm->data.buf, 1, wm->data.len, stdout);
        printf("\n");

        if (wm) {
            mg_send(tcp, wm->data.buf, wm->data.len);
        }
    }
}

void ghost_tls_handshake(struct mg_connection *tcp) {
    /* configure TLS options */
    strip_https_for_tls_host(tls_host);

    struct mg_tls_opts tls_opts = {
        .name = mg_str(tls_host),
        .ca = mg_str(""),  // Use system CA certificates
        .cert = mg_str(""),
        .key = mg_str(""),
    };

    convert_to_wss(config.ws_url);
    struct mg_connection *ws = mg_ws_connect(tcp->mgr,
            url_buffer,
            ghost_ws_handler, NULL, NULL);

    if (ws) {
        /* Initialize TLS with SNI */
        mg_tls_init(ws, &tls_opts);
        tcp->fn_data = ws;
        ws->fn_data = tcp;
        MG_INFO(("WebSocket connection initiated with TLS/SNI"));
    } else {
        MG_INFO(("Failed to create WebSocket connection"));
        tcp->is_closing = 1;
    }
}
