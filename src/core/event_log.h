#ifndef NETPULSE_EVENT_LOG_H
#define NETPULSE_EVENT_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include "core/config.h"
#include "core/stats.h"
#include "core/ring_buffer.h"

#define MAX_REASON_LEN 128
#define EVENT_BUFFER_SIZE 100

/*
 * Event types
 */
typedef enum {
    EVENT_BAD_LOSS,
    EVENT_BAD_P95,
    EVENT_BAD_JITTER
} event_type_t;

/*
 * Event entry for "bad minute" detection
 */
typedef struct {
    uint64_t timestamp_ms;
    char target_id[MAX_LABEL_LEN];
    event_type_t type;
    char reason[MAX_REASON_LEN];
    double value;
    double threshold;
    uint32_t duration_s;
} event_t;

/*
 * Bad condition tracker for a single target
 */
typedef struct {
    bool is_bad;                    // Currently in bad state?
    uint64_t bad_start_ms;          // When bad state started
    bool event_emitted;             // Already emitted event for this period?
    event_type_t last_bad_type;     // What triggered bad state
} bad_state_t;

/*
 * Event log manager
 */
typedef struct {
    ring_buffer_t events;           // Ring buffer of event_t
    char *events_file_path;         // Path to events.jsonl
} event_log_t;

// Initialize event log
int event_log_init(event_log_t *log);

// Free event log resources
void event_log_free(event_log_t *log);

// Check metrics against thresholds and update bad state
// Returns true if a new event was emitted
bool event_log_check(event_log_t *log, bad_state_t *state,
                     const char *target_id, const metrics_t *metrics,
                     const thresholds_t *thresholds);

// Get recent events (for WebSocket snapshot)
ring_buffer_t *event_log_get_events(event_log_t *log);

// Append event to JSONL file
int event_log_write_to_file(event_log_t *log, const event_t *event);

#endif // NETPULSE_EVENT_LOG_H
