#!/bin/bash

# If CI sets GHOST, use that. Otherwise use local path.
GHOST="${GHOST:-./bin/ghost-linux-amd64}"

SERVER_PORT=7777
CLIENT_PORT=8888
WS_URL="ws://127.0.0.1:$SERVER_PORT"

echo "[TEST] Using binary: $GHOST"
if [ ! -f "$GHOST" ]; then
    echo "[ERROR] Ghost binary not found at: $GHOST"
    exit 1
fi

echo "[TEST] Starting Ghost server..."
$GHOST server --port $SERVER_PORT --ssh 22 -vvv &
SERVER_PID=$!
sleep 1

if ps -p $SERVER_PID > /dev/null; then
    echo "[OK] Server started (PID=$SERVER_PID)"
else
    echo "[FAIL] Server failed to start"
    exit 1
fi

echo "[TEST] Starting Ghost client..."
$GHOST client --connect $WS_URL --port $CLIENT_PORT -vvv &
CLIENT_PID=$!
sleep 1

if ps -p $CLIENT_PID > /dev/null; then
    echo "[OK] Client started (PID=$CLIENT_PID)"
else
    echo "[FAIL] Client failed to start"
    kill $SERVER_PID
    exit 1
fi

echo ""
echo "[TEST] Both server and client launched successfully!"
sleep 2

echo "[TEST] Stopping processes..."
kill $SERVER_PID $CLIENT_PID

echo "[DONE] Test complete."
