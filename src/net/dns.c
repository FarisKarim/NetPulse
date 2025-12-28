#define _POSIX_C_SOURCE 200809L

#include "net/dns.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

struct addrinfo *dns_resolve(const char *host, uint16_t port) {
    if (host == NULL) {
        return NULL;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4 for simplicity
    hints.ai_socktype = SOCK_STREAM;  // TCP

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    struct addrinfo *result = NULL;
    int err = getaddrinfo(host, port_str, &hints, &result);

    if (err != 0) {
        return NULL;
    }

    return result;
}
