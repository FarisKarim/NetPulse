#ifndef NETPULSE_SCHEDULER_H
#define NETPULSE_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "core/config.h"
#include "core/stats.h"
#include "core/ring_buffer.h"
#include "core/event_log.h"

/*
 * Probe state for a single target
 */
typedef enum {
    PROBE_STATE_IDLE,
    PROBE_STATE_CONNECTING,
    PROBE_STATE_DONE
} probe_state_t;

/*
 * Runtime state for a target
 */
typedef struct {
    target_config_t config;
    ring_buffer_t samples;          // Ring buffer of sample_t
    metrics_t metrics;
    bad_state_t bad_state;
    probe_state_t probe_state;
    int probe_fd;                   // Socket fd during probe
    uint64_t probe_start_ms;        // When current probe started
    uint64_t next_probe_ms;         // When to start next probe
    double scratch[DEFAULT_WINDOW_SIZE]; // Scratch space for percentile calculation
} target_state_t;

/*
 * Scheduler state
 */
typedef struct {
    config_t *config;
    target_state_t targets[MAX_TARGETS];
    int target_count;
    event_log_t event_log;
    uint64_t last_metrics_update_ms;
    uint64_t start_time_ms;
    bool running;
} scheduler_t;

// Initialize scheduler
int scheduler_init(scheduler_t *sched, config_t *config);

// Free scheduler resources
void scheduler_free(scheduler_t *sched);

// Sync targets from config (call after config changes)
int scheduler_sync_targets(scheduler_t *sched);

// Main tick function - call from event loop
// Returns suggested timeout for next poll() in milliseconds
int scheduler_tick(scheduler_t *sched);

// Get target state by ID
target_state_t *scheduler_get_target(scheduler_t *sched, const char *id);

// Callback: called when a sample is recorded (for WebSocket broadcast)
typedef void (*sample_callback_t)(const char *target_id, const sample_t *sample, void *ctx);
void scheduler_set_sample_callback(scheduler_t *sched, sample_callback_t cb, void *ctx);

// Callback: called when metrics are updated (for WebSocket broadcast)
typedef void (*metrics_callback_t)(const char *target_id, const metrics_t *metrics, void *ctx);
void scheduler_set_metrics_callback(scheduler_t *sched, metrics_callback_t cb, void *ctx);

// Callback: called when an event is emitted
typedef void (*event_callback_t)(const event_t *event, void *ctx);
void scheduler_set_event_callback(scheduler_t *sched, event_callback_t cb, void *ctx);

#endif // NETPULSE_SCHEDULER_H
