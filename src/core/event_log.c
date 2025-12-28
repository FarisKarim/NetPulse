#include "core/event_log.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int event_log_init(event_log_t *log) {
    if (log == NULL) {
        return -1;
    }

    memset(log, 0, sizeof(*log));

    if (ring_buffer_init(&log->events, sizeof(event_t), EVENT_BUFFER_SIZE) != 0) {
        return -1;
    }

    // Get events file path
    char *data_dir = get_data_dir();
    if (data_dir == NULL) {
        ring_buffer_free(&log->events);
        return -1;
    }

    size_t path_len = strlen(data_dir) + strlen("/events.jsonl") + 1;
    log->events_file_path = malloc(path_len);
    if (log->events_file_path == NULL) {
        free(data_dir);
        ring_buffer_free(&log->events);
        return -1;
    }

    snprintf(log->events_file_path, path_len, "%s/events.jsonl", data_dir);
    free(data_dir);

    return 0;
}

void event_log_free(event_log_t *log) {
    if (log != NULL) {
        ring_buffer_free(&log->events);
        free(log->events_file_path);
        log->events_file_path = NULL;
    }
}

static const char *event_type_to_string(event_type_t type) {
    switch (type) {
        case EVENT_BAD_LOSS: return "loss_pct exceeded threshold";
        case EVENT_BAD_P95: return "p95_ms exceeded threshold";
        case EVENT_BAD_JITTER: return "jitter_ms exceeded threshold";
        default: return "unknown";
    }
}

static const char *event_type_to_field(event_type_t type) {
    switch (type) {
        case EVENT_BAD_LOSS: return "loss_pct";
        case EVENT_BAD_P95: return "p95_ms";
        case EVENT_BAD_JITTER: return "jitter_ms";
        default: return "unknown";
    }
}

bool event_log_check(event_log_t *log, bad_state_t *state,
                     const char *target_id, const metrics_t *metrics,
                     const thresholds_t *thresholds) {
    if (log == NULL || state == NULL || target_id == NULL ||
        metrics == NULL || thresholds == NULL) {
        return false;
    }

    uint64_t now = now_ms();
    bool currently_bad = false;
    event_type_t bad_type = EVENT_BAD_LOSS;
    double bad_value = 0.0;
    double bad_threshold = 0.0;

    // Check thresholds
    if (metrics->loss_pct > thresholds->loss_pct) {
        currently_bad = true;
        bad_type = EVENT_BAD_LOSS;
        bad_value = metrics->loss_pct;
        bad_threshold = thresholds->loss_pct;
    } else if (metrics->p95_ms > thresholds->p95_ms) {
        currently_bad = true;
        bad_type = EVENT_BAD_P95;
        bad_value = metrics->p95_ms;
        bad_threshold = thresholds->p95_ms;
    } else if (metrics->jitter_ms > thresholds->jitter_ms) {
        currently_bad = true;
        bad_type = EVENT_BAD_JITTER;
        bad_value = metrics->jitter_ms;
        bad_threshold = thresholds->jitter_ms;
    }

    if (currently_bad) {
        if (!state->is_bad) {
            // Just became bad
            state->is_bad = true;
            state->bad_start_ms = now;
            state->event_emitted = false;
            state->last_bad_type = bad_type;
        }

        // Check if bad for >= BAD_CONDITION_DURATION_S
        uint64_t duration_ms = now - state->bad_start_ms;
        uint32_t duration_s = (uint32_t)(duration_ms / 1000);

        if (duration_s >= BAD_CONDITION_DURATION_S && !state->event_emitted) {
            // Emit event
            event_t event = {0};
            event.timestamp_ms = wall_clock_ms();  // Use wall-clock time for display
            strncpy(event.target_id, target_id, sizeof(event.target_id) - 1);
            event.type = bad_type;
            snprintf(event.reason, sizeof(event.reason), "%s", event_type_to_string(bad_type));
            event.value = bad_value;
            event.threshold = bad_threshold;
            event.duration_s = duration_s;

            ring_buffer_push(&log->events, &event);
            event_log_write_to_file(log, &event);

            state->event_emitted = true;
            return true;
        }
    } else {
        // Conditions cleared
        state->is_bad = false;
        state->event_emitted = false;
    }

    return false;
}

ring_buffer_t *event_log_get_events(event_log_t *log) {
    if (log == NULL) {
        return NULL;
    }
    return &log->events;
}

int event_log_write_to_file(event_log_t *log, const event_t *event) {
    if (log == NULL || log->events_file_path == NULL || event == NULL) {
        return -1;
    }

    FILE *f = fopen(log->events_file_path, "a");
    if (f == NULL) {
        return -1;
    }

    const char *field = event_type_to_field(event->type);

    fprintf(f, "{\"ts\":%llu,\"target_id\":\"%s\",\"reason\":\"%s\",\"details\":{\"%s\":%.2f,\"threshold\":%.2f,\"duration_s\":%u}}\n",
            (unsigned long long)event->timestamp_ms,
            event->target_id,
            event->reason,
            field,
            event->value,
            event->threshold,
            event->duration_s);

    fclose(f);
    return 0;
}
