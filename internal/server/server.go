package server

import (
	"context"
	"encoding/json"
	"fmt"
	"gossh/internal/log"
	"gossh/internal/tunnel"
	"io"
	"net"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"sync/atomic"
	"syscall"
	"time"

	"github.com/gorilla/websocket"
)

// TODO: do we want these to be configurable? Then add to config
// Or are they shared invariants between client/server? Move to a shared package for using in both client.go and server.go
const (
	ReadBufSize    = 32 * 1024
	WriteBufSize   = 32 * 1024
	sshUnavailable = "SSH_UNAVAILABLE"
)

type GoSSHServer struct {
	conf   GoSSHServerConfiguration
	logger log.Logger

	startedAt      time.Time
	activeSessions int32

	bytesIn  int64
	bytesOut int64
}

type GoSSHServerConfiguration struct {
	Port     int
	SSHPort  int
	Timeout  time.Duration
	MaxConns int
}

func NewGoSSHServer(config GoSSHServerConfiguration, logger log.Logger) *GoSSHServer {
	return &GoSSHServer{
		conf:      config,
		logger:    logger,
		startedAt: time.Now(),
	}
}

func (gossh *GoSSHServer) Run() error {
	gossh.startedAt = time.Now()
	addr := fmt.Sprintf(":%d", gossh.conf.Port)

	mux := http.NewServeMux()
	// '/ws' is the general endpoint for ssh connections
	mux.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		newSessionCounter := atomic.AddInt32(&gossh.activeSessions, 1)

		if int64(newSessionCounter) > int64(gossh.conf.MaxConns) {
			atomic.AddInt32(&gossh.activeSessions, -1)

			http.Error(w, "maximum connections reached", http.StatusServiceUnavailable)
			return
		}

		gossh.handleServerWebSocket(w, r)
	})

	// '/status' endpoint is for server status
	mux.HandleFunc("/status", func(w http.ResponseWriter, r *http.Request) {
		gossh.handleServerStatus(w, r)
	})

	server := &http.Server{
		Addr:    addr,
		Handler: mux,
	}

	gossh.logger.Info("gossh started in SERVER mode")
	gossh.logger.Info("HTTP/WebSocket port: %d", gossh.conf.Port)
	gossh.logger.Info("SSH port: %d", gossh.conf.SSHPort)
	gossh.logger.Info("Checking SSH connection")

	sshUrl := fmt.Sprintf("127.0.0.1:%d", gossh.conf.SSHPort)
	conn, err := net.DialTimeout("tcp", sshUrl, gossh.conf.Timeout)

	if err != nil {
		gossh.logger.Warn("SSH server is down or unreachable: %v", err)
	} else {
		gossh.logger.Info("SSH server port is open and reachable")
		conn.Close()
	}

	gossh.logger.Info("Listening on http://localhost:%d", gossh.conf.Port)

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt, syscall.SIGTERM)

	go func() {
		<-stop
		gossh.logger.Info("Shutting down server...")
		ctx, cancel := shutdownContext()
		defer cancel()
		_ = server.Shutdown(ctx)
	}()

	err = server.ListenAndServe()
	if err == http.ErrServerClosed {
		return nil
	}
	return err
}

func (gossh *GoSSHServer) handleServerWebSocket(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/ws" {
		http.NotFound(w, r)
		return
	}

	upgrader := websocket.Upgrader{
		ReadBufferSize:  ReadBufSize,
		WriteBufferSize: WriteBufSize,
		CheckOrigin: func(r *http.Request) bool {
			return true
		},
	}

	ws, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		gossh.logger.Error("WebSocket upgrade failed: %v", err)
		return
	}
	gossh.logger.Info("WebSocket connection established")

	sshAddr := fmt.Sprintf("localhost:%d", gossh.conf.SSHPort)
	tcp, err := net.Dial("tcp", sshAddr)
	if err != nil {
		gossh.logger.Warn("Cannot connect to sshd on %s: %v", sshAddr, err)
		_ = ws.WriteMessage(websocket.TextMessage, []byte(sshUnavailable))
		ws.Close()
		return
	}
	gossh.logger.Info("Connected to sshd successfully")

	session := tunnel.NewSession(ws, tcp)
	gossh.runServerSession(session)
}

func (gossh *GoSSHServer) handleServerStatus(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/status" {
		http.NotFound(w, r)
		return
	}

	resp := struct {
		Uptime         string `json:"uptime"`
		ActiveSessions int32  `json:"active_sessions"`
		In             int64  `json:"bytes_in"`
		Out            int64  `json:"bytes_out"`
	}{
		Uptime:         time.Since(gossh.startedAt).Round(time.Second).String(),
		ActiveSessions: atomic.LoadInt32(&gossh.activeSessions),
		In:             atomic.LoadInt64(&gossh.bytesIn),
		Out:            atomic.LoadInt64(&gossh.bytesOut),
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

func (gossh *GoSSHServer) runServerSession(s *tunnel.Session) {
	defer s.Close()
	defer atomic.AddInt32(&gossh.activeSessions, -1)

	var wg sync.WaitGroup
	wg.Add(2)

	// WebSocket -> SSH
	go func() {
		defer wg.Done()
		defer s.Close()

		for {
			messageType, data, err := s.ReadWS()
			if err != nil {
				gossh.logger.Debug("WebSocket read ended: %v", err)
				return
			}

			if messageType != websocket.BinaryMessage && messageType != websocket.TextMessage {
				continue
			}

			gossh.logger.Debug("Data from websocket in SERVER mode: %d bytes", len(data))

			if err := s.SendTCP(data); err != nil {
				gossh.logger.Debug("TCP write failed: %v", err)
				return
			}

			gossh.logger.Trace("Forwarded %d bytes WS -> TCP", len(data))
			atomic.AddInt64(&gossh.bytesIn, int64(len(data)))
		}
	}()

	// SSH -> WebSocket
	go func() {
		defer wg.Done()
		defer s.Close()

		buf := make([]byte, ReadBufSize)

		for {
			n, err := s.ReadTCP(buf)
			if n > 0 {
				gossh.logger.Debug("SSH read %d bytes", n)

				if err := s.SendWS(buf[:n]); err != nil {
					gossh.logger.Debug("WebSocket write failed: %v", err)
					return
				}

				gossh.logger.Trace("Forwarded %d bytes TCP -> WS", n)
				atomic.AddInt64(&gossh.bytesOut, int64(n))
			}

			if err != nil {
				if err != io.EOF {
					gossh.logger.Debug("TCP read ended: %v", err)
				}
				return
			}
		}
	}()

	wg.Wait()
	gossh.logger.Info("Client disconnected")
}

func shutdownContext() (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), 5*time.Second)
}
