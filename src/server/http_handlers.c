#include "server/http_handlers.h"
#include "server/server.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Simple JSON parsing helpers (minimal implementation)
static bool json_get_string(const char *json, size_t json_len, const char *key, char *value, size_t value_size) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *p = strstr(json, pattern);
    if (p == NULL || p >= json + json_len) {
        return false;
    }

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;

    if (*p != '"') {
        return false;
    }
    p++;

    size_t i = 0;
    while (*p != '"' && *p != '\0' && i < value_size - 1) {
        value[i++] = *p++;
    }
    value[i] = '\0';

    return true;
}

static bool json_get_int(const char *json, size_t json_len, const char *key, int *value) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *p = strstr(json, pattern);
    if (p == NULL || p >= json + json_len) {
        return false;
    }

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;

    *value = atoi(p);
    return true;
}

static bool json_get_double(const char *json, size_t json_len, const char *key, double *value) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *p = strstr(json, pattern);
    if (p == NULL || p >= json + json_len) {
        return false;
    }

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;

    *value = atof(p);
    return true;
}

void http_handle_health(struct mg_connection *c, uint64_t start_time_ms) {
    uint64_t now = now_ms();
    uint64_t uptime_s = (now - start_time_ms) / 1000;

    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "{\"ok\":true,\"uptime_s\":%llu}\n",
                  (unsigned long long)uptime_s);
}

void http_handle_get_config(struct mg_connection *c, config_t *config) {
    // Build JSON response
    char buf[4096];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"probe_interval_ms\":%u,"
                    "\"probe_timeout_ms\":%u,"
                    "\"thresholds\":{"
                    "\"loss_pct\":%.1f,"
                    "\"p95_ms\":%.1f,"
                    "\"jitter_ms\":%.1f"
                    "},\"targets\":[",
                    config->probe_interval_ms,
                    config->probe_timeout_ms,
                    config->thresholds.loss_pct,
                    config->thresholds.p95_ms,
                    config->thresholds.jitter_ms);

    for (int i = 0; i < config->target_count; i++) {
        target_config_t *t = &config->targets[i];
        if (i > 0) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
        }
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        "{\"id\":\"%s\",\"host\":\"%s\",\"port\":%u,\"label\":\"%s\"}",
                        t->id, t->host, t->port, t->label);
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}\n");

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", buf);
}

void http_handle_post_config(struct mg_connection *c, struct mg_http_message *hm,
                             config_t *config, scheduler_t *scheduler) {
    const char *json = hm->body.buf;
    size_t json_len = hm->body.len;

    int val_int;
    double val_double;

    if (json_get_int(json, json_len, "probe_interval_ms", &val_int)) {
        if (val_int >= 100 && val_int <= 10000) {
            config->probe_interval_ms = (uint32_t)val_int;
        }
    }

    if (json_get_int(json, json_len, "probe_timeout_ms", &val_int)) {
        if (val_int >= 100 && val_int <= 30000) {
            config->probe_timeout_ms = (uint32_t)val_int;
        }
    }

    // Parse nested thresholds
    if (json_get_double(json, json_len, "loss_pct", &val_double)) {
        if (val_double >= 0 && val_double <= 100) {
            config->thresholds.loss_pct = val_double;
        }
    }

    if (json_get_double(json, json_len, "p95_ms", &val_double)) {
        if (val_double >= 0 && val_double <= 10000) {
            config->thresholds.p95_ms = val_double;
        }
    }

    if (json_get_double(json, json_len, "jitter_ms", &val_double)) {
        if (val_double >= 0 && val_double <= 10000) {
            config->thresholds.jitter_ms = val_double;
        }
    }

    (void)scheduler; // Config changes apply automatically on next cycle

    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                  "{\"ok\":true}\n");
}

void http_handle_post_targets(struct mg_connection *c, struct mg_http_message *hm,
                              config_t *config, scheduler_t *scheduler,
                              server_t *server) {
    const char *json = hm->body.buf;
    size_t json_len = hm->body.len;

    char action[32] = {0};
    json_get_string(json, json_len, "action", action, sizeof(action));

    if (strcmp(action, "add") == 0) {
        char host[MAX_HOST_LEN] = {0};
        char label[MAX_LABEL_LEN] = {0};
        int port = 443;

        json_get_string(json, json_len, "host", host, sizeof(host));
        json_get_string(json, json_len, "label", label, sizeof(label));
        json_get_int(json, json_len, "port", &port);

        if (host[0] == '\0' || label[0] == '\0') {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                          "{\"ok\":false,\"error\":\"host and label required\"}\n");
            return;
        }

        int idx = config_add_target(config, host, (uint16_t)port, label);
        if (idx < 0) {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                          "{\"ok\":false,\"error\":\"failed to add target\"}\n");
            return;
        }

        // Sync scheduler
        scheduler_sync_targets(scheduler);

        // Broadcast targets update to all WebSocket clients
        server_broadcast_targets_updated(server);

        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{\"ok\":true,\"target_id\":\"%s\"}\n",
                      config->targets[idx].id);

    } else if (strcmp(action, "remove") == 0) {
        char target_id[MAX_LABEL_LEN] = {0};
        json_get_string(json, json_len, "target_id", target_id, sizeof(target_id));

        if (target_id[0] == '\0') {
            mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                          "{\"ok\":false,\"error\":\"target_id required\"}\n");
            return;
        }

        if (config_remove_target(config, target_id) != 0) {
            mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                          "{\"ok\":false,\"error\":\"target not found\"}\n");
            return;
        }

        // Sync scheduler
        scheduler_sync_targets(scheduler);

        // Broadcast targets update to all WebSocket clients
        server_broadcast_targets_updated(server);

        mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                      "{\"ok\":true}\n");

    } else {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                      "{\"ok\":false,\"error\":\"action must be add or remove\"}\n");
    }
}

void http_handle_request(struct mg_connection *c, struct mg_http_message *hm,
                         config_t *config, scheduler_t *scheduler,
                         server_t *server, uint64_t start_time_ms) {

    if (mg_match(hm->uri, mg_str("/api/health"), NULL)) {
        http_handle_health(c, start_time_ms);
    } else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
        if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
            http_handle_get_config(c, config);
        } else if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
            http_handle_post_config(c, hm, config, scheduler);
        } else {
            mg_http_reply(c, 405, "", "Method not allowed\n");
        }
    } else if (mg_match(hm->uri, mg_str("/api/targets"), NULL)) {
        if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
            http_handle_post_targets(c, hm, config, scheduler, server);
        } else {
            mg_http_reply(c, 405, "", "Method not allowed\n");
        }
    } else {
        mg_http_reply(c, 404, "", "Not found\n");
    }
}
