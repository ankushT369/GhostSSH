# ---- COMPILERS ----
CC = gcc
MUSL_CC = musl-gcc
WIN_CC = x86_64-w64-mingw32-gcc

# ---- COMMON FLAGS ----
CFLAGS_BASE = -Wall -Wextra -g
TLS_OPENSSL = -DMG_TLS=MG_TLS_OPENSSL
TLS_NONE = -DMG_TLS=MG_TLS_NONE

# ---- DIRECTORIES ----
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/obj

# ---- SOURCES ----
SRC = mongoose.c ghost.c util.c ghost-server.c ghost-client.c
OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)

# ---- MUSL OPENSSL ----
MUSL_OPENSSL_DIR = /opt/musl-openssl
MUSL_CFLAGS = -I$(MUSL_OPENSSL_DIR)/include
MUSL_LDFLAGS = -L$(MUSL_OPENSSL_DIR)/lib64 -lssl -lcrypto

# ---- BIN NAMES ----
LINUX_BIN = $(BIN_DIR)/ghost-linux-amd64
STATIC_BIN = $(BIN_DIR)/ghost-linux-amd64-static
MUSL_BIN = $(BIN_DIR)/ghost-linux-amd64
WIN_BIN = $(BIN_DIR)/ghost-windows-amd64.exe

# =========================================================
# DEFAULT TARGET (Linux dynamic)
# =========================================================
all:
	$(MAKE) build \
	CC=$(CC) \
	BIN=$(LINUX_BIN) \
	CFLAGS="$(CFLAGS_BASE) $(TLS_OPENSSL)" \
	LDFLAGS="-lssl -lcrypto"

# =========================================================
# STATIC (glibc)
# =========================================================
static:
	$(MAKE) build \
	CC=$(CC) \
	BIN=$(STATIC_BIN) \
	CFLAGS="$(CFLAGS_BASE) $(TLS_OPENSSL) -static" \
	LDFLAGS="-lssl -lcrypto -static"

# =========================================================
# MUSL (portable static)
# =========================================================
musl:
	$(MAKE) build \
	CC=$(MUSL_CC) \
	BIN=$(MUSL_BIN) \
	CFLAGS="$(CFLAGS_BASE) $(TLS_OPENSSL) -static $(MUSL_CFLAGS)" \
	LDFLAGS="$(MUSL_LDFLAGS) -static"

# =========================================================
# WINDOWS
# =========================================================
win:
	$(MAKE) build \
	CC=$(WIN_CC) \
	BIN=$(WIN_BIN) \
	CFLAGS="$(CFLAGS_BASE) $(TLS_NONE)" \
	LDFLAGS="-lws2_32 -static -static-libgcc"

# =========================================================
# BUILD CORE
# =========================================================
build: clean $(BIN)

# ---- Directories ----
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ---- Compile ----
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Link ----
$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# =========================================================
# CLEAN
# =========================================================
clean:
	rm -rf $(BIN_DIR)

rebuild:
	$(MAKE) clean
	$(MAKE) all

.PHONY: all static musl win build clean rebuild

# CC = gcc
# MUSL_CC = musl-gcc
# WIN_CC = x86_64-w64-mingw32-gcc
#
# CFLAGS = -Wall -Wextra -g -DMG_TLS=MG_TLS_OPENSSL
# LDFLAGS = -lssl -lcrypto
#
# BIN_DIR = bin
# OBJ_DIR = $(BIN_DIR)/obj
#
# SRC = mongoose.c ghost.c util.c ghost-server.c ghost-client.c
# OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)
#
# # ---- MUSL OPENSSL ----
# MUSL_OPENSSL_DIR = /opt/musl-openssl
# MUSL_CFLAGS = -I$(MUSL_OPENSSL_DIR)/include
# MUSL_LDFLAGS = -L$(MUSL_OPENSSL_DIR)/lib64 -lssl -lcrypto
#
# # ---- BIN NAMES ----
# LINUX_BIN = $(BIN_DIR)/ghost-linux-amd64
# STATIC_BIN = $(BIN_DIR)/ghost-linux-amd64-static
# MUSL_BIN = $(BIN_DIR)/ghost-linux-amd64-musl
# WIN_BIN = $(BIN_DIR)/ghost-windows-amd64.exe
#
# # Default (Linux dynamic)
# # Default binary
# BIN = $(LINUX_BIN)
# # Default target
# all: $(BIN)
#
# # Static (glibc)
# static: BIN=$(STATIC_BIN)
# static: CFLAGS += -static
# static: LDFLAGS += -static
# static: clean $(BIN)
#
# # MUSL (portable)
# musl: CC=$(MUSL_CC)
# musl: BIN=$(MUSL_BIN)
# musl: CFLAGS = -Wall -Wextra -g -DMG_TLS=MG_TLS_OPENSSL -static $(MUSL_CFLAGS)
# musl: LDFLAGS = $(MUSL_LDFLAGS) -static
# musl: clean $(BIN)
#
# # Windows
# win: CC=$(WIN_CC)
# win: BIN=$(WIN_BIN)
# win: CFLAGS = -Wall -Wextra -g -DMG_TLS=MG_TLS_NONE
# win: LDFLAGS = -lws2_32
# win: clean $(BIN)
#
# # Directories
# $(BIN_DIR):
# 	mkdir -p $(BIN_DIR)
#
# $(OBJ_DIR):
# 	mkdir -p $(OBJ_DIR)
#
# # Compile
# $(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
#
# # Link
# $(BIN): $(OBJ) | $(BIN_DIR)
# 	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
#
# # Clean
# clean:
# 	rm -rf $(BIN_DIR)
#
# rebuild: clean all
#
# .PHONY: all clean rebuild static musl win
