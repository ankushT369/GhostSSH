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

// Verbosity levels
static int verbosity_level = 0;  // 0 = quiet, 1 = info, 2 = debug, 3 = verbose

ghost_config config = {
    .http_server_port = HTTP_PORT,
    .sshd_port = SSH_PORT,
    .tcp_server_port = TCP_PORT,
    .ws_url = "",
};

/* static declaration */
static void ghost_parse_args(int argc, char* arg[]);
static void ghost_print_usage();
static void ghost_set_verbosity(int level);
static void ghost_print_usage();

static void ghost_set_verbosity(int level) {
    verbosity_level = level;
    
    // Map to mongoose log levels
    switch (level) {
        case 0:  // Quiet - only errors
            mg_log_set(MG_LL_ERROR);
            break;
        case 1:  // Info - default
            mg_log_set(MG_LL_INFO);
            break;
        case 2:  // Debug
            mg_log_set(MG_LL_DEBUG);
            break;
        case 3:  // Verbose
            mg_log_set(MG_LL_VERBOSE);
            break;
        default:
            mg_log_set(MG_LL_INFO);
    }
}

static void ghost_parse_args(int argc, char *argv[]) {
    // First, check for verbosity flags before mode detection
    int verbosity_flag_count = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbosity_flag_count++;
        } else if (strcmp(argv[i], "-vv") == 0) {
            verbosity_flag_count = 2;
        } else if (strcmp(argv[i], "-vvv") == 0) {
            verbosity_flag_count = 3;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            verbosity_flag_count = -1;
        } else if (strncmp(argv[i], "--verbose", 9) == 0) {
            verbosity_flag_count = 1;
        }
    }
    
    // Set verbosity based on flags
    if (verbosity_flag_count >= 3) {
        ghost_set_verbosity(3);
    } else if (verbosity_flag_count == 2) {
        ghost_set_verbosity(2);
    } else if (verbosity_flag_count == 1) {
        ghost_set_verbosity(1);
    } else if (verbosity_flag_count == -1) {
        ghost_set_verbosity(0);
    } else {
        ghost_set_verbosity(1);  // Default to info level
    }
    
    // Detect mode
    if (argc < 2) {
        ghost_print_usage();
        exit(1);
    }
    
    if (strcmp(argv[1], "server") == 0) {
        mode = SERVER;
    } else if (strcmp(argv[1], "client") == 0) {
        mode = CLIENT;
    } else {
        ghost_print_usage();
        exit(1);
    }

    // Parse remaining arguments (skip verbosity flags)
    for (int i = 2; i < argc; i++) {
        // Skip verbosity flags
        if (strcmp(argv[i], "-v") == 0 || 
            strcmp(argv[i], "-vv") == 0 || 
            strcmp(argv[i], "-vvv") == 0 ||
            strcmp(argv[i], "-q") == 0 ||
            strcmp(argv[i], "--quiet") == 0 ||
            strcmp(argv[i], "--verbose") == 0) {
            continue;
        }
        
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
            config.ws_url[MAX_LEN - 1] = '\0';
        }
        else {
            printf("Unknown argument: %s\n", argv[i]);
            ghost_print_usage();
            exit(1);
        }
    }
    
    // Log configuration if verbosity is high enough
    if (verbosity_level >= 1) {
        MG_INFO(("Ghost started in %s mode", mode == SERVER ? "SERVER" : "CLIENT"));
        if (mode == SERVER) {
            MG_INFO(("HTTP port: %d, SSH port: %d", config.http_server_port, config.sshd_port));
        } else {
            MG_INFO(("Local port: %d, Remote URL: %s", config.tcp_server_port, config.ws_url));
        }
    }
}

static void ghost_print_usage() {
    printf("Usage: ghost <mode> [OPTIONS]\n\n");
    
    printf("Modes:\n");
    printf("  server                Run in server mode\n");
    printf("  client                Run in client mode\n\n");
    
    printf("Verbosity control:\n");
    printf("  -q, --quiet           Suppress all output except errors\n");
    printf("  -v                    Increase verbosity (info level)\n");
    printf("  -vv                   More verbose (debug level)\n");
    printf("  -vvv                  Most verbose (trace level)\n");
    printf("  --verbose             Same as -v\n\n");
    
    printf("Server mode options:\n");
    printf("  --port <port>         HTTP/WebSocket server port (required)\n");
    printf("  --ssh <port>          SSH daemon port (default: 22)\n\n");
    
    printf("Client mode options:\n");
    printf("  --connect <url>       Remote WebSocket/HTTP URL (required)\n");
    printf("  --port <port>         Local port to expose (required)\n\n");
    
    printf("Examples:\n");
    printf("  ghost server --port 7777 --ssh 22\n");
    printf("  ghost client --connect https://example.com --port 8888\n");
    printf("  ghost server -v --port 7777          # Info level logging\n");
    printf("  ghost client -vv --connect https://... --port 8888  # Debug level\n\n");
    
    printf("SSH connection:\n");
    printf("  ssh user@localhost -p 8888\n");
}

void ghost_tcp_handler(struct mg_connection *tcp, int ev, void *ev_data) {
    if (ev == MG_EV_ACCEPT) {
        MG_INFO(("Client accepted via TCP successfully"));
        ghost_tls_handshake(tcp);
    } else if (ev == MG_EV_CONNECT) {
        upgrade_done = 1;
        MG_INFO(("Connected to sshd successfully"));
    }
    else if (ev == MG_EV_CLOSE) {
        MG_INFO(("Client disconnected"));
    }
    else if (ev == MG_EV_READ) {
        struct mg_connection *ws = (struct mg_connection *) tcp->fn_data;
        
        // Only show data content in debug or verbose mode
        if (verbosity_level >= 2) {
            MG_DEBUG(("Client read %d bytes", (int)tcp->recv.len));
            if (verbosity_level >= 3) {
                // Show hex dump for binary data in verbose mode
                MG_VERBOSE(("Data: %.*s", tcp->recv.len, tcp->recv.buf));
            }
        }

        if (upgrade_done && ws && !ws->is_closing) {
            mg_ws_send(ws, tcp->recv.buf, tcp->recv.len, WEBSOCKET_OP_BINARY);
            mg_iobuf_del(&tcp->recv, 0, tcp->recv.len);
        }
    } else if (ev == MG_EV_ERROR) {
        MG_ERROR(("CLIENT error: %s", (char *) ev_data));
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

    struct mg_connection *conn = NULL;

    if (mode == SERVER) {
        /* create HTTP listener */
        create_local_url(HTTP, config.http_server_port);
        MG_INFO(("Starting server on %s", url_buffer));
        conn = mg_http_listen(&mgr, url_buffer, ghost_http_handler, NULL);

    } else {
        /* create TCP listener */
        create_local_url(TCP, config.tcp_server_port);
        MG_INFO(("Starting client, listening on %s", url_buffer));
        MG_INFO(("Connecting to remote: %s", config.ws_url));
        conn = mg_listen(&mgr, url_buffer, ghost_tcp_handler, NULL);
    }

    if (conn == NULL) {
        MG_ERROR(("Cannot create listener on %s", url_buffer));
        mg_mgr_free(&mgr);
        exit(1);
    }

    MG_INFO(("Ghost is running. Press Ctrl+C to stop"));
    
    /* blocking wait */
    for (;;) {
        mg_mgr_poll(&mgr, -1);
    }

    mg_mgr_free(&mgr);
    return 0;
}
