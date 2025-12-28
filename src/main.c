#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "platform/platform.h"
#include "core/config.h"
#include "core/scheduler.h"
#include "server/server.h"

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

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

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

        // Poll server with appropriate timeout
        // Use minimum of scheduler timeout and 100ms for responsiveness
        int poll_timeout = timeout < 100 ? timeout : 100;
        server_poll(&server, poll_timeout);
    }

    printf("\nShutting down...\n");

    // Cleanup
    server_free(&server);
    scheduler_free(&scheduler);

    printf("Goodbye!\n");
    return 0;
}
