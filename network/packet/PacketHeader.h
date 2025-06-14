#ifndef COMS_NETWORK_PACKET_HEADER_H
#define COMS_NETWORK_PACKET_HEADER_H

#include <stdio.h>

#include "../../stdlib/Types.h"

#if _WIN32
    #include <winsock2.h>
    #include <Ws2ipdef.h>
    #include <ws2tcpip.h>
#elif __linux__
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#define HEADER_IPV6_SIZE 40
// Size 40 bytes
struct HeaderIPv6 {
    byte data[HEADER_IPV6_SIZE];
};

// Size 40 bytes
struct PACKED_STRUCT HeaderIPv6Unpacked {
    uint32 ip6_flow; // also contains version and traffic class

    uint16 ip6_plen;
    byte ip6_nxt;
    byte ip6_hops;

    in6_addr ip6_src;
    in6_addr ip6_dst;
};
UNPACKED_STRUCT

#define HEADER_UDP_SIZE 8
// Size 8 bytes
struct UDPHeaderIPv6 {
    byte data[HEADER_UDP_SIZE];
};

// Size 8 bytes
struct PACKED_STRUCT UDPHeaderIPv6Unpacked {
    uint16 source;
    uint16 dest;
    uint16 len;
    uint16 check;
};
UNPACKED_STRUCT

struct PACKED_STRUCT UDPPseudoHeaderIPv6 {
    in6_addr src;
    in6_addr dst;
    uint32 length;
    byte zero[3];
    byte next_header;
};
UNPACKED_STRUCT

inline
void packet_create_destination_addr(sockaddr_in6* dest_addr, const char* ipv6, uint16 port)
{
    memset(dest_addr, 0, sizeof(sockaddr_in6));
    dest_addr->sin6_family = AF_INET6;
    dest_addr->sin6_port = SWAP_ENDIAN_BIG(port);
    inet_pton(AF_INET6, ipv6, &dest_addr->sin6_addr);
}

inline
void packet_create_destination_addr(sockaddr_in6* dest_addr, in6_addr* ipv6, uint16 port)
{
    dest_addr->sin6_family = AF_INET6;
    dest_addr->sin6_port = SWAP_ENDIAN_BIG(port);
    memcpy(&dest_addr->sin6_addr, ipv6, sizeof(in6_addr));
}

#endif