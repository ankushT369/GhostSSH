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

#include <string.h>
#include <stdio.h>

void convert_to_wss(const char* url) {
    memset((void*)url_buffer, '\0', MAX_LEN); 
    const char *p = strstr(url, "://");
    if (p != NULL) {
        p += 3;
    } else {
        p = url;
    }

    size_t len = strlen(p);
    int has_ws = (len >= 3 && strcmp(p + len - 3, "/ws") == 0);

    if (has_ws) {
        snprintf(url_buffer, MAX_LEN, "wss://%s", p);
    } else {
        snprintf(url_buffer, MAX_LEN, "wss://%s/ws", p);
    }
}

void create_local_url(int protocol, int port) {
    memset((void*)url_buffer, '\0', MAX_LEN); 
    snprintf(url_buffer, MAX_LEN, "%s://localhost:%d", proto_str[protocol], port);
}

void strip_https_for_tls_host(char *tls_host) {
    memset(tls_host, 0, MAX_LEN);

    const char *p = config.ws_url;

    if (strncmp(p, "https://", 8) == 0) {
        p += 8;  // skip "https://"
    }

    snprintf(tls_host, MAX_LEN, "%s", p);
}
