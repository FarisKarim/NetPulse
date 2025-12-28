#ifndef NETPULSE_HTTP_HANDLERS_H
#define NETPULSE_HTTP_HANDLERS_H

#include "mongoose.h"
#include "core/config.h"
#include "core/scheduler.h"

/*
 * HTTP API handlers
 */

// Handle HTTP request routing
void http_handle_request(struct mg_connection *c, struct mg_http_message *hm,
                         config_t *config, scheduler_t *scheduler, uint64_t start_time_ms);

// GET /api/health
void http_handle_health(struct mg_connection *c, uint64_t start_time_ms);

// GET /api/config
void http_handle_get_config(struct mg_connection *c, config_t *config);

// POST /api/config
void http_handle_post_config(struct mg_connection *c, struct mg_http_message *hm,
                             config_t *config, scheduler_t *scheduler);

// POST /api/targets
void http_handle_post_targets(struct mg_connection *c, struct mg_http_message *hm,
                              config_t *config, scheduler_t *scheduler);

#endif // NETPULSE_HTTP_HANDLERS_H
