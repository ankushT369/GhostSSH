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
# MACOS (ARM64 - Apple Silicon)
# =========================================================
MAC_CC = clang
MAC_OPENSSL_DIR = /opt/homebrew/opt/openssl@3

MAC_CFLAGS = $(CFLAGS_BASE) $(TLS_OPENSSL) -I$(MAC_OPENSSL_DIR)/include
MAC_LDFLAGS = $(MAC_OPENSSL_DIR)/lib/libssl.a \
              $(MAC_OPENSSL_DIR)/lib/libcrypto.a \
              -ldl -lpthread

MAC_BIN = $(BIN_DIR)/ghost-macos-arm64

mac:
	$(MAKE) build \
	CC=$(MAC_CC) \
	BIN=$(MAC_BIN) \
	CFLAGS="$(MAC_CFLAGS)" \
	LDFLAGS="$(MAC_LDFLAGS)"

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
