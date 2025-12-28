#ifndef NETPULSE_TCP_PROBE_H
#define NETPULSE_TCP_PROBE_H

#include <stdint.h>

/*
 * TCP connect timing probe using non-blocking sockets and poll()
 */

typedef enum {
    PROBE_PENDING,
    PROBE_SUCCESS,
    PROBE_ERROR
} probe_result_t;

// Start a non-blocking TCP connect probe.
// Returns socket fd on success, -1 on error.
int tcp_probe_start(const char *host, uint16_t port);

// Check probe status (non-blocking).
// Returns PROBE_PENDING if still connecting, PROBE_SUCCESS or PROBE_ERROR otherwise.
probe_result_t tcp_probe_check(int fd);

// Clean up probe socket
void tcp_probe_cleanup(int fd);

// Blocking probe with timeout (simpler API for testing)
// Returns RTT in milliseconds on success, -1.0 on error/timeout
double tcp_probe_blocking(const char *host, uint16_t port, int timeout_ms);

#endif // NETPULSE_TCP_PROBE_H
