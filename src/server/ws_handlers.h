#ifndef NETPULSE_WS_HANDLERS_H
#define NETPULSE_WS_HANDLERS_H

#include "mongoose.h"
#include "core/config.h"
#include "core/scheduler.h"
#include "core/stats.h"
#include "core/event_log.h"

/*
 * WebSocket handlers
 */

// Handle WebSocket upgrade
void ws_handle_open(struct mg_connection *c, config_t *config, scheduler_t *scheduler);

// Handle WebSocket message (from client)
void ws_handle_message(struct mg_connection *c, struct mg_ws_message *wm);

// Handle WebSocket close
void ws_handle_close(struct mg_connection *c);

// Build and send snapshot message to a client
void ws_send_snapshot(struct mg_connection *c, config_t *config, scheduler_t *scheduler);

// Build sample message JSON
int ws_build_sample_msg(char *buf, size_t buf_size, const char *target_id, const sample_t *sample);

// Build metrics message JSON
int ws_build_metrics_msg(char *buf, size_t buf_size, const char *target_id, const metrics_t *metrics);

// Build event message JSON
int ws_build_event_msg(char *buf, size_t buf_size, const event_t *event);

#endif // NETPULSE_WS_HANDLERS_H
