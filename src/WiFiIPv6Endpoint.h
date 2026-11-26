#ifndef WIFI_IPV6_ENDPOINT_H
#define WIFI_IPV6_ENDPOINT_H

#include <stdint.h>

struct WiFiIPv6Endpoint {
    uint8_t  addr[16];
    uint16_t port;       // Host byte order
    uint32_t scope_id;   // Local lwIP interface index
};

#endif