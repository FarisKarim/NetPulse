#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#include "platform/platform.h"
#include "core/config.h"
#include "core/scheduler.h"
#include "server/server.h"
#include "net/icmp_probe.h"

static volatile sig_atomic_t g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("\nOptions:\n");
    printf("  -p, --probe-type TYPE   Probe type: tcp (default) or icmp\n");
    printf("  -h, --help              Show this help message\n");
    printf("\nICMP mode:\n");
    printf("  On Linux, requires CAP_NET_RAW capability:\n");
    printf("    sudo setcap cap_net_raw+ep %s\n", prog);
    printf("  On macOS, ICMP is not supported (falls back to TCP).\n");
}

int main(int argc, char *argv[]) {
    probe_type_t probe_type = PROBE_TYPE_TCP;

    // Parse command-line options
    static struct option long_options[] = {
        {"probe-type", required_argument, 0, 'p'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                if (strcmp(optarg, "tcp") == 0) {
                    probe_type = PROBE_TYPE_TCP;
                } else if (strcmp(optarg, "icmp") == 0) {
                    probe_type = PROBE_TYPE_ICMP;
                } else {
                    fprintf(stderr, "Unknown probe type: %s\n", optarg);
                    fprintf(stderr, "Valid types: tcp, icmp\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("NetPulse v0.1.0\n");
    printf("Network Quality Monitor\n\n");

    // Set up signal handlers for graceful shutdown
    setup_signals();

    // Ensure data directory exists
    char *data_dir = get_data_dir();
    if (data_dir == NULL) {
        fprintf(stderr, "Failed to create data directory\n");
        return 1;
    }
    printf("Data directory: %s\n", data_dir);
    free(data_dir);

    // Initialize configuration
    config_t config;
    config_init(&config);
    config.probe_type = probe_type;

    // Print probe mode
    if (probe_type == PROBE_TYPE_ICMP) {
        if (icmp_probe_available()) {
            printf("Probe mode: ICMP (ping)\n");
        } else {
            printf("Probe mode: ICMP requested, but not available\n");
            printf("  Reason: %s\n", icmp_probe_unavailable_reason());
            printf("  Will fall back to TCP\n");
        }
    } else {
        printf("Probe mode: TCP (connect timing)\n");
    }
    printf("\n");

    printf("Default targets:\n");
    for (int i = 0; i < config.target_count; i++) {
        printf("  - %s (%s:%u)\n",
               config.targets[i].label,
               config.targets[i].host,
               config.targets[i].port);
    }
    printf("\n");

    // Initialize scheduler
    scheduler_t scheduler;
    if (scheduler_init(&scheduler, &config) != 0) {
        fprintf(stderr, "Failed to initialize scheduler\n");
        return 1;
    }

    // Initialize server
    server_t server;
    if (server_init(&server, &config, &scheduler) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        scheduler_free(&scheduler);
        return 1;
    }

    // Wire up callbacks
    server_setup_callbacks(&server, &scheduler);

    printf("\nStarting probes...\n\n");

    // Main event loop
    while (g_running) {
        // Run scheduler tick
        int timeout = scheduler_tick(&scheduler);

        // Poll server with appropriate timeout.
        // Cap at 2ms for accurate RTT measurement - longer timeouts add latency
        // since we only detect completed TCP connects on the next tick.
        int poll_timeout = timeout < 2 ? timeout : 2;
        server_poll(&server, poll_timeout);
    }

    printf("\nShutting down...\n");

    // Cleanup
    server_free(&server);
    scheduler_free(&scheduler);

    printf("Goodbye!\n");
    return 0;
}
