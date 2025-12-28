#ifndef NETPULSE_DNS_H
#define NETPULSE_DNS_H

#include <netdb.h>
#include <stdint.h>

/*
 * DNS resolution wrapper using getaddrinfo()
 */

// Resolve hostname to sockaddr. Returns addrinfo that must be freed with freeaddrinfo().
// Returns NULL on error.
struct addrinfo *dns_resolve(const char *host, uint16_t port);

#endif // NETPULSE_DNS_H
