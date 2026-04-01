CC = gcc

CFLAGS = -Wall -Wextra -g -DMG_TLS=MG_TLS_OPENSSL
LDFLAGS = -lssl -lcrypto

BIN_DIR = bin
OBJ_DIR = obj

SRC = mongoose.c ghost.c util.c ghost-server.c ghost-client.c
OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)

BIN = $(BIN_DIR)/ghost

all: $(BIN)

# Create directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compile .c → .o
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link all .o → single binary
$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

rebuild: clean all

.PHONY: all clean rebuild
