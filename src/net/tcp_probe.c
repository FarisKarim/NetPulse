#include "net/tcp_probe.h"
#include "net/dns.h"
#include "platform/platform.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>

int tcp_probe_start(const char *host, uint16_t port) {
    struct addrinfo *addr = dns_resolve(host, port);
    if (addr == NULL) {
        return -1;
    }

    int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(addr);
        return -1;
    }

    // Set non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        freeaddrinfo(addr);
        return -1;
    }

    // Start non-blocking connect
    int ret = connect(fd, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);

    if (ret < 0 && errno != EINPROGRESS) {
        close(fd);
        return -1;
    }

    return fd;
}

probe_result_t tcp_probe_check(int fd) {
    if (fd < 0) {
        return PROBE_ERROR;
    }

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
        .revents = 0
    };

    int ret = poll(&pfd, 1, 0); // Non-blocking check

    if (ret < 0) {
        return PROBE_ERROR;
    }

    if (ret == 0) {
        return PROBE_PENDING;
    }

    // Check for errors
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        return PROBE_ERROR;
    }

    if (pfd.revents & POLLOUT) {
        // Check SO_ERROR to confirm connection success
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            return PROBE_ERROR;
        }
        return PROBE_SUCCESS;
    }

    return PROBE_PENDING;
}

void tcp_probe_cleanup(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

double tcp_probe_blocking(const char *host, uint16_t port, int timeout_ms) {
    uint64_t start = now_ms();

    int fd = tcp_probe_start(host, port);
    if (fd < 0) {
        return -1.0;
    }

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
        .revents = 0
    };

    int ret = poll(&pfd, 1, timeout_ms);

    if (ret <= 0) {
        // Timeout or error
        tcp_probe_cleanup(fd);
        return -1.0;
    }

    probe_result_t result = tcp_probe_check(fd);
    tcp_probe_cleanup(fd);

    if (result != PROBE_SUCCESS) {
        return -1.0;
    }

    uint64_t end = now_ms();
    return (double)(end - start);
}
