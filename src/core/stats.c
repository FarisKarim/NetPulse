#include "core/stats.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <math.h>

// Comparison function for qsort
static int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

double stats_compute_loss(ring_buffer_t *samples) {
    if (samples == NULL || ring_buffer_count(samples) == 0) {
        return 0.0;
    }

    size_t total = ring_buffer_count(samples);
    size_t failures = 0;

    for (size_t i = 0; i < total; i++) {
        sample_t *s = (sample_t *)ring_buffer_get(samples, i);
        if (s != NULL && !s->success) {
            failures++;
        }
    }

    return (double)failures / (double)total * 100.0;
}

double stats_compute_jitter(ring_buffer_t *samples) {
    if (samples == NULL || ring_buffer_count(samples) < 2) {
        return 0.0;
    }

    double total_delta = 0.0;
    size_t delta_count = 0;
    double prev_rtt = -1.0;

    for (size_t i = 0; i < ring_buffer_count(samples); i++) {
        sample_t *s = (sample_t *)ring_buffer_get(samples, i);
        if (s != NULL && s->success) {
            if (prev_rtt >= 0.0) {
                total_delta += fabs(s->rtt_ms - prev_rtt);
                delta_count++;
            }
            prev_rtt = s->rtt_ms;
        }
    }

    if (delta_count == 0) {
        return 0.0;
    }

    return total_delta / (double)delta_count;
}

static double stats_compute_max_rtt(ring_buffer_t *samples) {
    if (samples == NULL || ring_buffer_count(samples) == 0) {
        return 0.0;
    }

    double max_rtt = 0.0;
    for (size_t i = 0; i < ring_buffer_count(samples); i++) {
        sample_t *s = (sample_t *)ring_buffer_get(samples, i);
        if (s != NULL && s->success && s->rtt_ms > max_rtt) {
            max_rtt = s->rtt_ms;
        }
    }

    return max_rtt;
}

double stats_compute_percentile(ring_buffer_t *samples, double percentile, double *scratch, size_t scratch_size) {
    if (samples == NULL || scratch == NULL || scratch_size == 0) {
        return 0.0;
    }

    // Extract successful RTT values
    size_t count = 0;
    for (size_t i = 0; i < ring_buffer_count(samples) && count < scratch_size; i++) {
        sample_t *s = (sample_t *)ring_buffer_get(samples, i);
        if (s != NULL && s->success) {
            scratch[count++] = s->rtt_ms;
        }
    }

    if (count == 0) {
        return 0.0;
    }

    // Sort the values
    qsort(scratch, count, sizeof(double), compare_doubles);

    // Compute percentile index
    double idx = (percentile / 100.0) * (double)(count - 1);
    size_t lower = (size_t)idx;
    size_t upper = lower + 1;

    if (upper >= count) {
        return scratch[count - 1];
    }

    // Linear interpolation
    double frac = idx - (double)lower;
    return scratch[lower] * (1.0 - frac) + scratch[upper] * frac;
}

void stats_compute(ring_buffer_t *samples, metrics_t *metrics, double *scratch, size_t scratch_size) {
    if (samples == NULL || metrics == NULL) {
        return;
    }

    metrics->loss_pct = stats_compute_loss(samples);
    metrics->jitter_ms = stats_compute_jitter(samples);
    metrics->p50_ms = stats_compute_percentile(samples, 50.0, scratch, scratch_size);
    metrics->p95_ms = stats_compute_percentile(samples, 95.0, scratch, scratch_size);
    metrics->max_rtt_ms = stats_compute_max_rtt(samples);

    // Get current RTT from newest successful sample
    metrics->current_rtt_ms = 0.0;
    for (size_t i = ring_buffer_count(samples); i > 0; i--) {
        sample_t *s = (sample_t *)ring_buffer_get(samples, i - 1);
        if (s != NULL && s->success) {
            metrics->current_rtt_ms = s->rtt_ms;
            break;
        }
    }

    metrics->last_updated = now_ms();
}
