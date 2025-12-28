#include "core/scheduler.h"
#include "net/tcp_probe.h"
#include "platform/platform.h"
#include <string.h>
#include <stdlib.h>

static sample_callback_t g_sample_cb = NULL;
static void *g_sample_ctx = NULL;
static metrics_callback_t g_metrics_cb = NULL;
static void *g_metrics_ctx = NULL;
static event_callback_t g_event_cb = NULL;
static void *g_event_ctx = NULL;

void scheduler_set_sample_callback(scheduler_t *sched, sample_callback_t cb, void *ctx) {
    (void)sched;
    g_sample_cb = cb;
    g_sample_ctx = ctx;
}

void scheduler_set_metrics_callback(scheduler_t *sched, metrics_callback_t cb, void *ctx) {
    (void)sched;
    g_metrics_cb = cb;
    g_metrics_ctx = ctx;
}

void scheduler_set_event_callback(scheduler_t *sched, event_callback_t cb, void *ctx) {
    (void)sched;
    g_event_cb = cb;
    g_event_ctx = ctx;
}

int scheduler_init(scheduler_t *sched, config_t *config) {
    if (sched == NULL || config == NULL) {
        return -1;
    }

    memset(sched, 0, sizeof(*sched));
    sched->config = config;
    sched->running = true;
    sched->start_time_ms = now_ms();
    sched->last_metrics_update_ms = 0;

    if (event_log_init(&sched->event_log) != 0) {
        return -1;
    }

    return scheduler_sync_targets(sched);
}

void scheduler_free(scheduler_t *sched) {
    if (sched == NULL) {
        return;
    }

    for (int i = 0; i < sched->target_count; i++) {
        ring_buffer_free(&sched->targets[i].samples);
        if (sched->targets[i].probe_fd >= 0) {
            tcp_probe_cleanup(sched->targets[i].probe_fd);
        }
    }

    event_log_free(&sched->event_log);
    sched->target_count = 0;
}

int scheduler_sync_targets(scheduler_t *sched) {
    if (sched == NULL || sched->config == NULL) {
        return -1;
    }

    uint64_t now = now_ms();

    // For MVP, just reinitialize all targets from config
    // TODO: Smarter sync that preserves existing sample history

    // Free existing targets
    for (int i = 0; i < sched->target_count; i++) {
        ring_buffer_free(&sched->targets[i].samples);
        if (sched->targets[i].probe_fd >= 0) {
            tcp_probe_cleanup(sched->targets[i].probe_fd);
        }
    }

    sched->target_count = 0;

    // Initialize targets from config
    for (int i = 0; i < sched->config->target_count; i++) {
        target_state_t *ts = &sched->targets[i];
        memset(ts, 0, sizeof(*ts));

        ts->config = sched->config->targets[i];

        if (ring_buffer_init(&ts->samples, sizeof(sample_t), DEFAULT_WINDOW_SIZE) != 0) {
            return -1;
        }

        ts->probe_state = PROBE_STATE_IDLE;
        ts->probe_fd = -1;
        ts->next_probe_ms = now; // Start probing immediately

        sched->target_count++;
    }

    return 0;
}

static void handle_probe_complete(scheduler_t *sched, target_state_t *ts, bool success, double rtt_ms) {
    uint64_t now = now_ms();

    sample_t sample = {
        .timestamp_ms = wall_clock_ms(),  // Use wall-clock time for display
        .rtt_ms = success ? rtt_ms : 0.0,
        .success = success
    };

    ring_buffer_push(&ts->samples, &sample);

    // Notify sample callback
    if (g_sample_cb != NULL) {
        g_sample_cb(ts->config.id, &sample, g_sample_ctx);
    }

    // Schedule next probe
    ts->probe_state = PROBE_STATE_IDLE;
    ts->next_probe_ms = now + sched->config->probe_interval_ms;
}

int scheduler_tick(scheduler_t *sched) {
    if (sched == NULL || !sched->running) {
        return 1000;
    }

    uint64_t now = now_ms();
    int min_timeout = 1000; // Default 1 second

    // Process each target
    for (int i = 0; i < sched->target_count; i++) {
        target_state_t *ts = &sched->targets[i];

        switch (ts->probe_state) {
            case PROBE_STATE_IDLE:
                // Check if it's time to start a new probe
                if (now >= ts->next_probe_ms) {
                    int fd = tcp_probe_start(ts->config.host, ts->config.port);
                    if (fd >= 0) {
                        ts->probe_fd = fd;
                        ts->probe_start_ms = now;
                        ts->probe_state = PROBE_STATE_CONNECTING;
                    } else {
                        // DNS or socket error - record as failure
                        handle_probe_complete(sched, ts, false, 0.0);
                    }
                } else {
                    int wait = (int)(ts->next_probe_ms - now);
                    if (wait < min_timeout) {
                        min_timeout = wait;
                    }
                }
                break;

            case PROBE_STATE_CONNECTING: {
                uint64_t elapsed = now - ts->probe_start_ms;
                int remaining = (int)(sched->config->probe_timeout_ms - elapsed);

                if (remaining <= 0) {
                    // Timeout
                    tcp_probe_cleanup(ts->probe_fd);
                    ts->probe_fd = -1;
                    handle_probe_complete(sched, ts, false, 0.0);
                } else {
                    // Check probe status
                    probe_result_t result = tcp_probe_check(ts->probe_fd);

                    if (result == PROBE_SUCCESS) {
                        double rtt = (double)(now - ts->probe_start_ms);
                        tcp_probe_cleanup(ts->probe_fd);
                        ts->probe_fd = -1;
                        handle_probe_complete(sched, ts, true, rtt);
                    } else if (result == PROBE_ERROR) {
                        tcp_probe_cleanup(ts->probe_fd);
                        ts->probe_fd = -1;
                        handle_probe_complete(sched, ts, false, 0.0);
                    } else {
                        // Still pending
                        if (remaining < min_timeout) {
                            min_timeout = remaining;
                        }
                    }
                }
                break;
            }

            case PROBE_STATE_DONE:
                // Should not happen - reset to idle
                ts->probe_state = PROBE_STATE_IDLE;
                break;
        }
    }

    // Update metrics once per second
    if (now - sched->last_metrics_update_ms >= 1000) {
        sched->last_metrics_update_ms = now;

        for (int i = 0; i < sched->target_count; i++) {
            target_state_t *ts = &sched->targets[i];

            stats_compute(&ts->samples, &ts->metrics, ts->scratch, DEFAULT_WINDOW_SIZE);

            // Check for events
            if (event_log_check(&sched->event_log, &ts->bad_state,
                               ts->config.id, &ts->metrics,
                               &sched->config->thresholds)) {
                // Event was emitted
                event_t *event = (event_t *)ring_buffer_newest(&sched->event_log.events);
                if (event != NULL && g_event_cb != NULL) {
                    g_event_cb(event, g_event_ctx);
                }
            }

            // Notify metrics callback
            if (g_metrics_cb != NULL) {
                g_metrics_cb(ts->config.id, &ts->metrics, g_metrics_ctx);
            }
        }
    }

    return min_timeout > 0 ? min_timeout : 1;
}

target_state_t *scheduler_get_target(scheduler_t *sched, const char *id) {
    if (sched == NULL || id == NULL) {
        return NULL;
    }

    for (int i = 0; i < sched->target_count; i++) {
        if (strcmp(sched->targets[i].config.id, id) == 0) {
            return &sched->targets[i];
        }
    }

    return NULL;
}
