/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NETWORK_PACKET_CACHE_H
#define COMS_NETWORK_PACKET_CACHE_H

#include "../../memory/RingMemory.h"
#include "../../memory/BufferMemory.h"

#if _WIN32
    #include <winsock2.h>
    #include <Ws2ipdef.h>
    #include <windows.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

struct PacketCache {
    sockaddr_in6 addr;

    int last_received;
    int last_sent;

    RingMemory packets_received;
    RingMemory packets_sent;

    int packets_received_ack[64];
    int packets_sent_ack[64];
};

struct PacketCacheHashEntry {
    in6_addr* key;
    PacketCache* value;
    PacketCacheHashEntry* next;
};

struct PacketCacheHashTable {
    uint32 size;
    PacketCacheHashEntry** entries;

    // BufferMemory for PacketCacheHashEntry NOT PacketCache
    BufferMemory buf;
};

uint32 hash_ipv6_address(uint32 hashmap_size, sockaddr_in6 *addr) {
    uint32 hash_val = addr->sin6_addr;

    hash_val = ((hash_val >> 16) ^ hash_val) * 0x45d9f3b;
    hash_val = ((hash_val >> 16) ^ hash_val) * 0x45d9f3b;
    hash_val = (hash_val >> 16) ^ hash_val;

    return hash_val % hashmap_size;
}

PacketCache* hash_packet_cache_get(PacketCacheHashTable* ht, sockaddr_in6* addr) {
    uint32 index = hash_ipv6_address(ht->size, addr);

    PacketCacheHashEntry* entry;
    if (!(entry = ht->entries[index])) {
        return NULL;
    }

    bool no_match;
    while ((no_match = (memcmp(&addr->sin6_addr, entry->key, sizeof(in6_addr)) != 0))
        && entry->next != NULL
    ) {
        entry = entry->next;
    }

    if (no_match) {
        return NULL;
    }

    return entry->value;
}

PacketCacheHashEntry* hash_packet_cache_insert(PacketCacheHashTable* ht, sockaddr_in6* addr, PacketCache* value) {
    uint32 index = hash_ipv6_address(ht->size, addr);

    PacketCacheHashEntry* element = (PacketCacheHashEntry *) buffer_find_free(&ht->buf);
    element->key = &value->addr.sin6_addr;
    element->value = value;
    element->next = NULL;

    PacketCacheHashEntry* entry = ht->entries[index];
    if (entry == NULL) {
        ht->entries[index] = element;

        return element;
    }

    while (entry->next != NULL) {
        entry = entry->next;
    }

    entry->next = element;

    return element;
}

void hash_packet_cache_remove(PacketCacheHashTable* ht, sockaddr_in6* addr) {
    uint32 index = hash_ipv6_address(ht->size, addr);

    PacketCacheHashEntry* entry;
    if (!(entry = ht->entries[index])) {
        return;
    }

    PacketCacheHashEntry* prev = NULL;

    bool no_match = false;
    while (entry && (no_match = (memcmp(&addr->sin6_addr, entry->key, sizeof(in6_addr)) != 0))) {
        prev = entry;
        entry = entry->next;
    }

    if (no_match) {
        return;
    }

    if (!prev) {
        ht->entries[index] = NULL;

        return;
    }

    prev->next = NULL;
}

#endif