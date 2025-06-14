#ifndef COMS_NETWORK_PACKET_UDP_H
#define COMS_NETWORK_PACKET_UDP_H

#include <stdio.h>

#include "../../stdlib/Types.h"
#include "../../compression/LZP.h"
#include "../../utils/EndianUtils.h"

#include "PacketHeader.h"

#if _WIN32
    #include <winsock2.h>
#elif __linux__
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

// The message loop is as follows:
//      Game Data                   -> pack data
//      Game Message (packed)       -> compress data
//      Game Message (compressed)   -> combine (ipv6 header + udp header + body)
//      Packet
//      ... Send ...
//      ... Receive ...
//      Packet                     -> split (ipv6 header + udp header + body)
//      Game Message (compressed)   -> decompress data
//      Game Message (packed)       -> unpack data
//      Game Data

// Game messages
//      Type 1: Snapshot = absolute state
//      Type 2: Delta = relative data to previous data
//              Can be usually much smaller data size, since deltas are usually small

struct UDPPacketIPv6 {
    byte* data;
};

struct UDPMessageIPv6 {
    HeaderIPv6Unpacked header_ipv6;
    UDPHeaderIPv6Unpacked header_udp;

    byte* data;
};

uint16 packet_udp_calculate_checksum(uint16* buf, int32 count) {
    uint32 sum = 0;

    // First we create the checksum from the ipv6 header (not all of the data is required)
    // Alternatively we would have had to create a pseudo_header and do a bunch of memcpy which we didn't want to do
    HeaderIPv6Unpacked* ipv6_header = (HeaderIPv6Unpacked *) buf;
    const byte* temp_data = ((byte *) (&ipv6_header->ip6_src));
    for (uint32 i = 0; i < sizeof(ipv6_header->ip6_src) / 2; ++i) {
        uint16 word = (uint16) temp_data[i] << 8 | (uint16) temp_data[i + 1];
        sum += word;
    }

    temp_data = ((byte *) (&ipv6_header->ip6_dst));
    for (uint32 i = 0; i < sizeof(ipv6_header->ip6_dst) / 2; ++i) {
        uint16 word = (uint16) temp_data[i] << 8 | (uint16) temp_data[i + 1];
        sum += word;
    }

    temp_data = ((byte *) (&ipv6_header->ip6_plen));
    for (uint32 i = 0; i < sizeof(ipv6_header->ip6_plen) / 2; ++i) {
        uint16 word = (uint16) temp_data[i] << 8 | (uint16) temp_data[i + 1];
        sum += word;
    }

    // The next header has some 0s prefixed, this is required since the checksum works on uint16
    byte next_header[] = {0, 0, 0, ipv6_header->ip6_nxt};
    for (uint32 i = 0; i < sizeof(ipv6_header->ip6_nxt) / 2; ++i) {
        uint16 word = (uint16) next_header[i] << 8 | (uint16) next_header[i + 1];
        sum += word;
    }

    // Now create checksum for udp header and payload/body
    while (count > 0) {
        sum += *buf++;
        --count;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16) ~sum;
}

// WARNING: ports need to be already in big endian
inline
uint16 packet_udp_create_raw(
    byte* __restrict packet,
    in6_addr* __restrict ipv6_src, uint16 port_src,
    in6_addr* __restrict ipv6_dst, uint16 port_dst,
    uint16 flow,
    const byte* __restrict data, uint16 data_length
) {
    // create ipv6 header
    HeaderIPv6Unpacked* ip6_header = (HeaderIPv6Unpacked *) packet;
    ip6_header->ip6_flow = SWAP_ENDIAN_BIG((6 << 28) | (0 << 20) | flow);
    ip6_header->ip6_plen = SWAP_ENDIAN_BIG((uint16) (sizeof(UDPHeaderIPv6Unpacked) + data_length));
    ip6_header->ip6_nxt = IPPROTO_UDP;
    ip6_header->ip6_hops = 64;
    memcpy(&ip6_header->ip6_src, ipv6_src, sizeof(in6_addr));
    memcpy(&ip6_header->ip6_dst, ipv6_dst, sizeof(in6_addr));

    // create udp header
    UDPHeaderIPv6Unpacked* udp_header = (UDPHeaderIPv6Unpacked *) (packet + sizeof(HeaderIPv6Unpacked));

    udp_header->source = port_src;
    udp_header->dest = port_dst;
    udp_header->len = ip6_header->ip6_plen;
    udp_header->check = 0;

    // @performance consider to do the compression right here instead of the memcpy
    // create payload
    memcpy(packet + sizeof(HeaderIPv6Unpacked) + sizeof(UDPHeaderIPv6Unpacked), data, data_length);

    udp_header->check = SWAP_ENDIAN_BIG(packet_udp_calculate_checksum(
        (uint16 *) (packet),
        (sizeof(UDPHeaderIPv6Unpacked) + data_length) / 2
    ));

    // Raw sockets must use the entire packet size
    return sizeof(HeaderIPv6Unpacked) + sizeof(UDPHeaderIPv6Unpacked) + data_length;
}

// WARNING: ports need to be already in big endian
inline
uint16 packet_udp_create(
    byte* __restrict packet,
    const byte* __restrict data, uint16 data_length
) {
    // @performance consider to do the compression right here instead of the memcpy
    // create payload
    memcpy(packet, data, data_length);

    return data_length;
}

inline
void packet_flowinfo_set(sockaddr_in6* dest, uint16 flow)
{
    dest->sin6_flowinfo = SWAP_ENDIAN_BIG((6 << 28) | (0 << 20) | flow);
}

#endif