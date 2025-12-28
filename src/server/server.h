#ifndef NETPULSE_SERVER_H
#define NETPULSE_SERVER_H

#include "mongoose.h"
#include "core/config.h"
#include "core/scheduler.h"

/*
 * HTTP + WebSocket server using Mongoose
 */

typedef struct {
    struct mg_mgr mgr;
    struct mg_connection *listener;
    config_t *config;
    scheduler_t *scheduler;
    uint64_t start_time_ms;
} server_t;

// Initialize server
int server_init(server_t *srv, config_t *config, scheduler_t *scheduler);

// Free server resources
void server_free(server_t *srv);

// Poll server events (call from main loop)
// timeout_ms: how long to wait for events
void server_poll(server_t *srv, int timeout_ms);

// Broadcast message to all WebSocket clients
void server_broadcast_ws(server_t *srv, const char *msg, size_t len);

// Broadcast targets updated message to all WebSocket clients
void server_broadcast_targets_updated(server_t *srv);

// Wire up scheduler callbacks to broadcast via WebSocket
void server_setup_callbacks(server_t *srv, scheduler_t *sched);

#endif // NETPULSE_SERVER_H
