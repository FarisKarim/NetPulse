/*
 * ICMP Probe Linux Implementation
 *
 * Uses raw sockets to send ICMP Echo Requests and measure RTT.
 * Requires CAP_NET_RAW capability or root privileges.
 */

#include "net/icmp_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

// ICMP packet structure
typedef struct {
    struct icmphdr hdr;
    char data[56];  // Standard ping payload size
} icmp_packet_t;

// Get monotonic time in microseconds
static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

// Internet checksum (RFC 1071)
static uint16_t icmp_checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    // Handle odd byte
    if (len == 1) {
        sum += *(uint8_t *)ptr;
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

int icmp_probe_init(icmp_probe_state_t *state) {
    if (state == NULL) {
        return -1;
    }

    state->sock = -1;
    state->identifier = (uint16_t)getpid();
    state->sequence = 0;

    // Create raw socket for ICMP
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        if (errno == EPERM || errno == EACCES) {
            // No permission - caller should fall back to TCP
            return -1;
        }
        return -1;
    }

    // Set receive timeout as backup (poll is primary timeout mechanism)
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    state->sock = sock;
    return 0;
}

void icmp_probe_cleanup(icmp_probe_state_t *state) {
    if (state != NULL && state->sock >= 0) {
        close(state->sock);
        state->sock = -1;
    }
}

double icmp_probe_ping(icmp_probe_state_t *state, const char *host, int timeout_ms) {
    if (state == NULL || state->sock < 0 || host == NULL) {
        return -1.0;
    }

    // Resolve host
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        return -1.0;
    }

    // Build ICMP Echo Request
    icmp_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.hdr.type = ICMP_ECHO;
    packet.hdr.code = 0;
    packet.hdr.un.echo.id = htons(state->identifier);
    packet.hdr.un.echo.sequence = htons(state->sequence++);

    // Fill payload with timestamp
    uint64_t send_time = now_us();
    memcpy(packet.data, &send_time, sizeof(send_time));

    // Calculate checksum
    packet.hdr.checksum = 0;
    packet.hdr.checksum = icmp_checksum(&packet, sizeof(packet));

    // Send packet
    ssize_t sent = sendto(state->sock, &packet, sizeof(packet), 0,
                          (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) {
        return -1.0;
    }

    // Wait for reply using poll
    struct pollfd pfd;
    pfd.fd = state->sock;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) {
        // Timeout or error
        return -1.0;
    }

    // Receive reply
    char recv_buf[1024];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t received = recvfrom(state->sock, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *)&from_addr, &from_len);
    if (received < 0) {
        return -1.0;
    }

    uint64_t recv_time = now_us();

    // Parse IP header to get to ICMP header
    struct iphdr *ip_hdr = (struct iphdr *)recv_buf;
    int ip_hdr_len = ip_hdr->ihl * 4;

    if (received < ip_hdr_len + (ssize_t)sizeof(struct icmphdr)) {
        return -1.0;
    }

    struct icmphdr *icmp_hdr = (struct icmphdr *)(recv_buf + ip_hdr_len);

    // Verify it's an Echo Reply for us
    if (icmp_hdr->type != ICMP_ECHOREPLY) {
        return -1.0;
    }

    if (ntohs(icmp_hdr->un.echo.id) != state->identifier) {
        // Not our reply, try again (simplified - just fail for now)
        return -1.0;
    }

    // Calculate RTT in milliseconds
    double rtt_ms = (double)(recv_time - send_time) / 1000.0;

    return rtt_ms;
}

bool icmp_probe_available(void) {
#ifdef HAS_ICMP_PROBE
    // Try to create a raw socket to check permissions
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock >= 0) {
        close(sock);
        return true;
    }
    return false;
#else
    return false;
#endif
}

const char *icmp_probe_unavailable_reason(void) {
#ifdef HAS_ICMP_PROBE
    // Check if it's a permission issue
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock >= 0) {
        close(sock);
        return NULL;  // It's actually available
    }

    if (errno == EPERM || errno == EACCES) {
        return "ICMP requires CAP_NET_RAW capability or root privileges. "
               "Run: sudo setcap cap_net_raw+ep ./build/netpulsed";
    }

    return "Failed to create raw socket for ICMP";
#else
    return "ICMP probing not compiled into this build";
#endif
}
