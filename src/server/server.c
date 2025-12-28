#include "server/server.h"
#include "server/http_handlers.h"
#include "server/ws_handlers.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

// Forward declaration of event handler
static void server_event_handler(struct mg_connection *c, int ev, void *ev_data);

// Global server pointer for callbacks (mongoose doesn't have user context in event handler signature in older versions)
static server_t *g_server = NULL;

int server_init(server_t *srv, config_t *config, scheduler_t *scheduler) {
    if (srv == NULL || config == NULL || scheduler == NULL) {
        return -1;
    }

    memset(srv, 0, sizeof(*srv));
    srv->config = config;
    srv->scheduler = scheduler;
    srv->start_time_ms = now_ms();

    g_server = srv;

    mg_mgr_init(&srv->mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://0.0.0.0:%u", config->http_port);

    srv->listener = mg_http_listen(&srv->mgr, addr, server_event_handler, NULL);
    if (srv->listener == NULL) {
        fprintf(stderr, "Failed to listen on %s\n", addr);
        return -1;
    }

    printf("NetPulse daemon listening on %s\n", addr);
    printf("WebSocket endpoint: ws://localhost:%u/ws\n", config->http_port);

    return 0;
}

void server_free(server_t *srv) {
    if (srv != NULL) {
        mg_mgr_free(&srv->mgr);
        g_server = NULL;
    }
}

void server_poll(server_t *srv, int timeout_ms) {
    if (srv != NULL) {
        mg_mgr_poll(&srv->mgr, timeout_ms);
    }
}

void server_broadcast_ws(server_t *srv, const char *msg, size_t len) {
    if (srv == NULL || msg == NULL) {
        return;
    }

    for (struct mg_connection *c = srv->mgr.conns; c != NULL; c = c->next) {
        if (c->data[0] == 'W') {  // WebSocket connection
            mg_ws_send(c, msg, len, WEBSOCKET_OP_TEXT);
        }
    }
}

void server_broadcast_targets_updated(server_t *srv) {
    if (srv == NULL) {
        return;
    }

    char buf[8192];
    int len = ws_build_targets_updated_msg(buf, sizeof(buf), srv->config, srv->scheduler);
    if (len > 0) {
        server_broadcast_ws(srv, buf, (size_t)len);
    }
}

static void server_event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (g_server == NULL) {
        return;
    }

    switch (ev) {
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *)ev_data;

            // Check for WebSocket upgrade
            if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
                mg_ws_upgrade(c, hm, NULL);
            } else {
                // Handle HTTP request
                http_handle_request(c, hm, g_server->config, g_server->scheduler, g_server, g_server->start_time_ms);
            }
            break;
        }

        case MG_EV_WS_OPEN: {
            ws_handle_open(c, g_server->config, g_server->scheduler);
            break;
        }

        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
            ws_handle_message(c, wm);
            break;
        }

        case MG_EV_CLOSE: {
            if (c->data[0] == 'W') {
                ws_handle_close(c);
            }
            break;
        }

        default:
            break;
    }
}

// Callback wrappers for scheduler integration
static void on_sample(const char *target_id, const sample_t *sample, void *ctx) {
    server_t *srv = (server_t *)ctx;
    char buf[512];
    int len = ws_build_sample_msg(buf, sizeof(buf), target_id, sample);
    if (len > 0) {
        server_broadcast_ws(srv, buf, (size_t)len);
    }
}

static void on_metrics(const char *target_id, const metrics_t *metrics, void *ctx) {
    server_t *srv = (server_t *)ctx;
    char buf[512];
    int len = ws_build_metrics_msg(buf, sizeof(buf), target_id, metrics);
    if (len > 0) {
        server_broadcast_ws(srv, buf, (size_t)len);
    }
}

static void on_event(const event_t *event, void *ctx) {
    server_t *srv = (server_t *)ctx;
    char buf[512];
    int len = ws_build_event_msg(buf, sizeof(buf), event);
    if (len > 0) {
        server_broadcast_ws(srv, buf, (size_t)len);
    }
}

// Call this after server_init to wire up callbacks
void server_setup_callbacks(server_t *srv, scheduler_t *sched) {
    scheduler_set_sample_callback(sched, on_sample, srv);
    scheduler_set_metrics_callback(sched, on_metrics, srv);
    scheduler_set_event_callback(sched, on_event, srv);
}
