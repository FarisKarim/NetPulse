#include "core/config.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void config_slugify(const char *label, char *slug, size_t slug_size) {
    if (label == NULL || slug == NULL || slug_size == 0) {
        if (slug && slug_size > 0) {
            slug[0] = '\0';
        }
        return;
    }

    size_t j = 0;
    for (size_t i = 0; label[i] != '\0' && j < slug_size - 1; i++) {
        char c = label[i];
        if (isalnum((unsigned char)c)) {
            slug[j++] = (char)tolower((unsigned char)c);
        } else if (c == ' ' || c == '-' || c == '_') {
            // Avoid consecutive dashes
            if (j > 0 && slug[j - 1] != '-') {
                slug[j++] = '-';
            }
        }
    }

    // Remove trailing dash
    if (j > 0 && slug[j - 1] == '-') {
        j--;
    }

    slug[j] = '\0';
}

void config_init(config_t *cfg) {
    if (cfg == NULL) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));

    cfg->probe_interval_ms = DEFAULT_PROBE_INTERVAL_MS;
    cfg->probe_timeout_ms = DEFAULT_PROBE_TIMEOUT_MS;
    cfg->http_port = HTTP_WS_PORT;
    cfg->probe_type = PROBE_TYPE_TCP;  // Default to TCP (works everywhere)

    cfg->thresholds.loss_pct = DEFAULT_LOSS_THRESHOLD;
    cfg->thresholds.p95_ms = DEFAULT_P95_THRESHOLD;
    cfg->thresholds.jitter_ms = DEFAULT_JITTER_THRESHOLD;

    cfg->target_count = 0;

    // Add default targets
    config_add_target(cfg, "1.1.1.1", 443, "Cloudflare");
    config_add_target(cfg, "8.8.8.8", 443, "Google");
}

int config_add_target(config_t *cfg, const char *host, uint16_t port, const char *label) {
    if (cfg == NULL || host == NULL || label == NULL) {
        return -1;
    }

    if (cfg->target_count >= MAX_TARGETS) {
        return -1;
    }

    // Generate slug ID
    char slug[MAX_LABEL_LEN];
    config_slugify(label, slug, sizeof(slug));

    // Check for duplicate ID
    if (config_find_target(cfg, slug) != NULL) {
        return -1;
    }

    int idx = cfg->target_count;
    target_config_t *target = &cfg->targets[idx];

    strncpy(target->id, slug, sizeof(target->id) - 1);
    target->id[sizeof(target->id) - 1] = '\0';

    strncpy(target->host, host, sizeof(target->host) - 1);
    target->host[sizeof(target->host) - 1] = '\0';

    target->port = port;

    strncpy(target->label, label, sizeof(target->label) - 1);
    target->label[sizeof(target->label) - 1] = '\0';

    target->enabled = true;

    cfg->target_count++;

    return idx;
}

int config_remove_target(config_t *cfg, const char *id) {
    if (cfg == NULL || id == NULL) {
        return -1;
    }

    for (int i = 0; i < cfg->target_count; i++) {
        if (strcmp(cfg->targets[i].id, id) == 0) {
            // Shift remaining targets down
            for (int j = i; j < cfg->target_count - 1; j++) {
                cfg->targets[j] = cfg->targets[j + 1];
            }
            cfg->target_count--;

            // Clear the last slot
            memset(&cfg->targets[cfg->target_count], 0, sizeof(target_config_t));

            return 0;
        }
    }

    return -1;
}

target_config_t *config_find_target(config_t *cfg, const char *id) {
    if (cfg == NULL || id == NULL) {
        return NULL;
    }

    for (int i = 0; i < cfg->target_count; i++) {
        if (strcmp(cfg->targets[i].id, id) == 0) {
            return &cfg->targets[i];
        }
    }

    return NULL;
}
