#include "core/scheduler.h"
#include "net/tcp_probe.h"
#include "net/icmp_probe.h"
#include "platform/platform.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    sched->icmp_available = false;

    // Initialize ICMP probe state if ICMP mode is requested
    if (config->probe_type == PROBE_TYPE_ICMP) {
        if (icmp_probe_init(&sched->icmp_state) == 0) {
            sched->icmp_available = true;
            printf("[scheduler] ICMP probing enabled\n");
        } else {
            printf("[scheduler] ICMP probing not available: %s\n",
                   icmp_probe_unavailable_reason());
            printf("[scheduler] Falling back to TCP probing\n");
            config->probe_type = PROBE_TYPE_TCP;
        }
    }

    if (event_log_init(&sched->event_log) != 0) {
        if (sched->icmp_available) {
            icmp_probe_cleanup(&sched->icmp_state);
            sched->icmp_available = false;
        }
        return -1;
    }

    int sync_result = scheduler_sync_targets(sched);
    if (sync_result != 0) {
        // Clean up on sync failure
        event_log_free(&sched->event_log);
        if (sched->icmp_available) {
            icmp_probe_cleanup(&sched->icmp_state);
            sched->icmp_available = false;
        }
    }
    return sync_result;
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

    // Clean up ICMP state
    if (sched->icmp_available) {
        icmp_probe_cleanup(&sched->icmp_state);
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
            // Clean up already-initialized targets on failure
            for (int j = 0; j < sched->target_count; j++) {
                ring_buffer_free(&sched->targets[j].samples);
            }
            sched->target_count = 0;
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

// Perform ICMP probe (blocking with internal timeout)
static void do_icmp_probe(scheduler_t *sched, target_state_t *ts) {
    double rtt = icmp_probe_ping(&sched->icmp_state, ts->config.host,
                                  (int)sched->config->probe_timeout_ms);
    if (rtt >= 0) {
        handle_probe_complete(sched, ts, true, rtt);
    } else {
        handle_probe_complete(sched, ts, false, 0.0);
    }
}

// Perform TCP probe start (non-blocking)
static void do_tcp_probe_start(scheduler_t *sched, target_state_t *ts) {
    int fd = tcp_probe_start(ts->config.host, ts->config.port);
    uint64_t now = now_ms();

    if (fd >= 0) {
        ts->probe_fd = fd;
        ts->probe_start_ms = now;
        ts->probe_state = PROBE_STATE_CONNECTING;
    } else {
        // DNS or socket error - record as failure
        handle_probe_complete(sched, ts, false, 0.0);
    }
}

int scheduler_tick(scheduler_t *sched) {
    if (sched == NULL || !sched->running) {
        return 1000;
    }

    uint64_t now = now_ms();
    int min_timeout = 1000; // Default 1 second
    bool use_icmp = (sched->config->probe_type == PROBE_TYPE_ICMP && sched->icmp_available);

    // Process each target
    for (int i = 0; i < sched->target_count; i++) {
        target_state_t *ts = &sched->targets[i];

        switch (ts->probe_state) {
            case PROBE_STATE_IDLE:
                // Check if it's time to start a new probe
                if (now >= ts->next_probe_ms) {
                    if (use_icmp) {
                        // ICMP probe is blocking
                        do_icmp_probe(sched, ts);
                    } else {
                        // TCP probe is non-blocking
                        do_tcp_probe_start(sched, ts);
                    }
                } else {
                    int wait = (int)(ts->next_probe_ms - now);
                    if (wait < min_timeout) {
                        min_timeout = wait;
                    }
                }
                break;

            case PROBE_STATE_CONNECTING: {
                // Only TCP probe uses this state
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

        // Debug: log resource usage every 10 seconds
        static int debug_counter = 0;
        if (++debug_counter >= 10) {
            debug_log_resources("scheduler_tick");
            debug_counter = 0;
        }

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
