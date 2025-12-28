#ifndef NETPULSE_CONFIG_H
#define NETPULSE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Default configuration values
 */
#define DEFAULT_PROBE_INTERVAL_MS   500
#define DEFAULT_PROBE_TIMEOUT_MS    1500
#define DEFAULT_WINDOW_SIZE         120     // 60s at 500ms interval
#define DEFAULT_LOSS_THRESHOLD      5.0     // percent
#define DEFAULT_P95_THRESHOLD       100.0   // ms
#define DEFAULT_JITTER_THRESHOLD    20.0    // ms
#define BAD_CONDITION_DURATION_S    10      // seconds before emitting event
#define HTTP_WS_PORT                7331
#define MAX_TARGETS                 10
#define MAX_LABEL_LEN               64
#define MAX_HOST_LEN                256

/*
 * Threshold configuration for "bad minute" detection
 */
typedef struct {
    double loss_pct;
    double p95_ms;
    double jitter_ms;
} thresholds_t;

/*
 * Target definition
 */
typedef struct {
    char id[MAX_LABEL_LEN];         // Slugified label (e.g., "cloudflare")
    char host[MAX_HOST_LEN];        // Hostname or IP
    uint16_t port;                   // Port number
    char label[MAX_LABEL_LEN];      // Human-readable label
    bool enabled;                    // Whether target is active
} target_config_t;

/*
 * Global configuration
 */
typedef struct {
    uint32_t probe_interval_ms;
    uint32_t probe_timeout_ms;
    uint16_t http_port;
    thresholds_t thresholds;
    target_config_t targets[MAX_TARGETS];
    int target_count;
} config_t;

// Initialize config with defaults
void config_init(config_t *cfg);

// Add a target. Returns target index on success, -1 on error.
int config_add_target(config_t *cfg, const char *host, uint16_t port, const char *label);

// Remove a target by ID. Returns 0 on success, -1 if not found.
int config_remove_target(config_t *cfg, const char *id);

// Find target by ID. Returns pointer to target or NULL if not found.
target_config_t *config_find_target(config_t *cfg, const char *id);

// Generate slug ID from label (lowercase, spaces to dashes)
void config_slugify(const char *label, char *slug, size_t slug_size);

#endif // NETPULSE_CONFIG_H
