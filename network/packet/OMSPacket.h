#ifndef COMS_NETWORK_PACKET_OMS_H
#define COMS_NETWORK_PACKET_OMS_H

#include <stdio.h>

#include "../../stdlib/Types.h"
#include "../../compression/LZP.h"
#include "../../utils/EndianUtils.h"

#include "PacketHeader.h"

#if _WIN32
    #include <winsock2.h>
    #include <Ws2ipdef.h>
#elif __linux__
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

// WARNING: Since this requires admin priviledges, this can only be used for server-server communication
inline
uint16 packet_oms_create_raw(
    byte* __restrict packet,
    in6_addr* __restrict ipv6_src, in6_addr* __restrict ipv6_dst,
    uint16 flow,
    byte* __restrict data, uint16 data_length
) {
    // create ipv6 header
    HeaderIPv6Unpacked* ip6_header = (HeaderIPv6Unpacked *) packet;
    ip6_header->ip6_flow = SWAP_ENDIAN_BIG((6 << 28) | (0 << 20) | flow);
    ip6_header->ip6_plen = SWAP_ENDIAN_BIG(sizeof(UDPHeaderIPv6Unpacked) + data_length);
    ip6_header->ip6_nxt = 255;
    ip6_header->ip6_hops = 64;
    memcpy(&ip6_header->ip6_src, ipv6_src, sizeof(in6_addr));
    memcpy(&ip6_header->ip6_dst, ipv6_dst, sizeof(in6_addr));

    // create payload
    memcpy(packet + sizeof(HeaderIPv6Unpacked), data, data_length);

    return sizeof(HeaderIPv6Unpacked) + data_length;
}

#endif