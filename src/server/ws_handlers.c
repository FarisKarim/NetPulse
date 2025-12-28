#include "server/ws_handlers.h"
#include <stdio.h>
#include <string.h>

void ws_handle_open(struct mg_connection *c, config_t *config, scheduler_t *scheduler) {
    // Mark connection as WebSocket
    c->data[0] = 'W';

    // Send snapshot
    ws_send_snapshot(c, config, scheduler);
}

void ws_handle_message(struct mg_connection *c, struct mg_ws_message *wm) {
    // For MVP, we don't handle incoming WS messages
    (void)c;
    (void)wm;
}

void ws_handle_close(struct mg_connection *c) {
    c->data[0] = '\0';
}

void ws_send_snapshot(struct mg_connection *c, config_t *config, scheduler_t *scheduler) {
    // Build snapshot JSON
    char buf[16384];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"type\":\"snapshot\",\"targets\":[");

    for (int i = 0; i < scheduler->target_count; i++) {
        target_state_t *ts = &scheduler->targets[i];

        if (i > 0) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
        }

        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        "{\"id\":\"%s\",\"host\":\"%s\",\"port\":%u,\"label\":\"%s\","
                        "\"metrics\":{"
                        "\"current_rtt_ms\":%.2f,"
                        "\"max_rtt_ms\":%.2f,"
                        "\"loss_pct\":%.2f,"
                        "\"jitter_ms\":%.2f,"
                        "\"p50_ms\":%.2f,"
                        "\"p95_ms\":%.2f"
                        "},\"samples\":[",
                        ts->config.id, ts->config.host, ts->config.port, ts->config.label,
                        ts->metrics.current_rtt_ms,
                        ts->metrics.max_rtt_ms,
                        ts->metrics.loss_pct,
                        ts->metrics.jitter_ms,
                        ts->metrics.p50_ms,
                        ts->metrics.p95_ms);

        // Add samples
        size_t sample_count = ring_buffer_count(&ts->samples);
        for (size_t j = 0; j < sample_count; j++) {
            sample_t *s = (sample_t *)ring_buffer_get(&ts->samples, j);
            if (s != NULL) {
                if (j > 0) {
                    pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
                }
                pos += snprintf(buf + pos, sizeof(buf) - pos,
                                "{\"ts\":%llu,\"rtt_ms\":%.2f,\"success\":%s}",
                                (unsigned long long)s->timestamp_ms,
                                s->rtt_ms,
                                s->success ? "true" : "false");
            }
        }

        pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "],\"config\":{"
                    "\"probe_interval_ms\":%u,"
                    "\"probe_timeout_ms\":%u,"
                    "\"thresholds\":{"
                    "\"loss_pct\":%.1f,"
                    "\"p95_ms\":%.1f,"
                    "\"jitter_ms\":%.1f"
                    "}}}",
                    config->probe_interval_ms,
                    config->probe_timeout_ms,
                    config->thresholds.loss_pct,
                    config->thresholds.p95_ms,
                    config->thresholds.jitter_ms);

    mg_ws_send(c, buf, (size_t)pos, WEBSOCKET_OP_TEXT);
}

int ws_build_sample_msg(char *buf, size_t buf_size, const char *target_id, const sample_t *sample) {
    return snprintf(buf, buf_size,
                    "{\"type\":\"sample\",\"target_id\":\"%s\","
                    "\"ts\":%llu,\"rtt_ms\":%.2f,\"success\":%s}",
                    target_id,
                    (unsigned long long)sample->timestamp_ms,
                    sample->rtt_ms,
                    sample->success ? "true" : "false");
}

int ws_build_metrics_msg(char *buf, size_t buf_size, const char *target_id, const metrics_t *metrics) {
    return snprintf(buf, buf_size,
                    "{\"type\":\"metrics\",\"target_id\":\"%s\","
                    "\"metrics\":{"
                    "\"current_rtt_ms\":%.2f,"
                    "\"max_rtt_ms\":%.2f,"
                    "\"loss_pct\":%.2f,"
                    "\"jitter_ms\":%.2f,"
                    "\"p50_ms\":%.2f,"
                    "\"p95_ms\":%.2f"
                    "}}",
                    target_id,
                    metrics->current_rtt_ms,
                    metrics->max_rtt_ms,
                    metrics->loss_pct,
                    metrics->jitter_ms,
                    metrics->p50_ms,
                    metrics->p95_ms);
}

int ws_build_event_msg(char *buf, size_t buf_size, const event_t *event) {
    const char *field;
    switch (event->type) {
        case EVENT_BAD_LOSS: field = "loss_pct"; break;
        case EVENT_BAD_P95: field = "p95_ms"; break;
        case EVENT_BAD_JITTER: field = "jitter_ms"; break;
        default: field = "unknown"; break;
    }

    return snprintf(buf, buf_size,
                    "{\"type\":\"event\",\"ts\":%llu,\"target_id\":\"%s\","
                    "\"reason\":\"%s\",\"details\":{"
                    "\"%s\":%.2f,\"threshold\":%.2f,\"duration_s\":%u"
                    "}}",
                    (unsigned long long)event->timestamp_ms,
                    event->target_id,
                    event->reason,
                    field,
                    event->value,
                    event->threshold,
                    event->duration_s);
}

int ws_build_targets_updated_msg(char *buf, size_t buf_size, config_t *config, scheduler_t *scheduler) {
    int pos = 0;

    pos += snprintf(buf + pos, buf_size - pos,
                    "{\"type\":\"targets_updated\",\"targets\":[");

    for (int i = 0; i < scheduler->target_count; i++) {
        target_state_t *ts = &scheduler->targets[i];

        if (i > 0) {
            pos += snprintf(buf + pos, buf_size - pos, ",");
        }

        pos += snprintf(buf + pos, buf_size - pos,
                        "{\"id\":\"%s\",\"host\":\"%s\",\"port\":%u,\"label\":\"%s\","
                        "\"metrics\":{"
                        "\"current_rtt_ms\":%.2f,"
                        "\"max_rtt_ms\":%.2f,"
                        "\"loss_pct\":%.2f,"
                        "\"jitter_ms\":%.2f,"
                        "\"p50_ms\":%.2f,"
                        "\"p95_ms\":%.2f"
                        "},\"samples\":[]}",
                        ts->config.id, ts->config.host, ts->config.port, ts->config.label,
                        ts->metrics.current_rtt_ms,
                        ts->metrics.max_rtt_ms,
                        ts->metrics.loss_pct,
                        ts->metrics.jitter_ms,
                        ts->metrics.p50_ms,
                        ts->metrics.p95_ms);
    }

    pos += snprintf(buf + pos, buf_size - pos,
                    "],\"config\":{"
                    "\"probe_interval_ms\":%u,"
                    "\"probe_timeout_ms\":%u,"
                    "\"thresholds\":{"
                    "\"loss_pct\":%.1f,"
                    "\"p95_ms\":%.1f,"
                    "\"jitter_ms\":%.1f"
                    "}}}",
                    config->probe_interval_ms,
                    config->probe_timeout_ms,
                    config->thresholds.loss_pct,
                    config->thresholds.p95_ms,
                    config->thresholds.jitter_ms);

    return pos;
}
