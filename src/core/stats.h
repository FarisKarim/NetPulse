#ifndef NETPULSE_STATS_H
#define NETPULSE_STATS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "core/ring_buffer.h"

/*
 * Sample: a single probe result
 */
typedef struct {
    uint64_t timestamp_ms;  // Timestamp when probe completed
    double rtt_ms;          // Round-trip time in milliseconds (0 if failed)
    bool success;           // Whether probe succeeded
} sample_t;

/*
 * Computed metrics for a target
 */
typedef struct {
    double current_rtt_ms;  // Last successful RTT
    double max_rtt_ms;      // Maximum RTT in window
    double loss_pct;        // Packet loss percentage (0-100)
    double jitter_ms;       // Average absolute RTT delta
    double p50_ms;          // 50th percentile RTT
    double p95_ms;          // 95th percentile RTT
    uint64_t last_updated;  // Timestamp of last metrics update
} metrics_t;

// Compute metrics from a ring buffer of samples
// scratch must be an array of at least sample_count doubles for sorting
void stats_compute(ring_buffer_t *samples, metrics_t *metrics, double *scratch, size_t scratch_size);

// Compute loss percentage from samples
double stats_compute_loss(ring_buffer_t *samples);

// Compute jitter (avg absolute delta between consecutive successful RTTs)
double stats_compute_jitter(ring_buffer_t *samples);

// Compute percentile RTT (0-100). Requires scratch buffer for sorting.
double stats_compute_percentile(ring_buffer_t *samples, double percentile, double *scratch, size_t scratch_size);

#endif // NETPULSE_STATS_H
