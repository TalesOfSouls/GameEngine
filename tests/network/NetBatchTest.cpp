// winsock2.h must be included before windows.h (TestFramework.h)
#if _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "../TestFramework.h"
#include "../../utils/StringUtils.h"
#include "../../utils/BitUtils.h"
#include "../../network/packet/NetBatch.cpp"
#include "../../network/packet/packet_types.h"
#include "../../network/packet/mob/MobStatePacket.h"
#include "../../network/packet/mob/MobInfoPacket.h"
#include "../../network/packet/mob/player/PlayerInfoPacket.h"
#include "../../network/NetConnection.cpp"

/**
 * Bit packed game messages and batches
 */

static MobStateSnapshot net_batch_test_snapshot(uint32 id) {
    MobStateSnapshot snapshot = {};
    snapshot.mob_id = id;
    snapshot.mob_type = (uint8) (id % 200);
    snapshot.chunk = 1000 + id;
    snapshot.position.x = (f32) (id % 32);
    snapshot.position.y = 3.25f;
    snapshot.position.z = 31.5f;

    // 90 degrees around y
    snapshot.rotation.x = 0.0f;
    snapshot.rotation.y = 0.7071068f;
    snapshot.rotation.z = 0.0f;
    snapshot.rotation.w = 0.7071068f;

    snapshot.state_flag = 0xA5A5A5A5 ^ id;

    return snapshot;
}

static void test_net_batch_mob_state() {
    MobStateSnapshot snapshot = net_batch_test_snapshot(7);

    byte buffer[64];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    TEST_TRUE(net_packet_serialize(&writer, &snapshot));
    TEST_EQUALS(bit_writer_bits(&writer), 172);

    const uint32 size = bit_writer_flush(&writer);
    TEST_EQUALS(size, 22);
    TEST_LESSER_THAN(size, (uint32) sizeof(MobStateSnapshot));

    BitReader reader;
    bit_reader_init(&reader, buffer, size);

    MobStateSnapshot read = {};
    TEST_TRUE(net_packet_serialize(&reader, &read));

    const f32 precision = quantize_precision(0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS);
    TEST_EQUALS(read.mob_id, 7);
    TEST_EQUALS(read.mob_type, 7);
    TEST_EQUALS(read.chunk, 1007);
    TEST_EQUALS_WITH_DELTA(read.position.x, 7.0, precision);
    TEST_EQUALS_WITH_DELTA(read.position.y, 3.25, precision);
    TEST_EQUALS_WITH_DELTA(read.position.z, 31.5, precision);
    TEST_TRUE(mob_rotation_equal(&read.rotation, &snapshot.rotation) || fabsf(read.rotation.y - snapshot.rotation.y) < 0.01f);
    TEST_EQUALS(read.state_flag, 0xA5A5A5A5 ^ 7);

    // Truncated data fails, it never reads past the end
    bit_reader_init(&reader, buffer, size - 1);
    TEST_FALSE(net_packet_serialize(&reader, &read));
}

static void test_net_batch_mob_delta() {
    const MobStateSnapshot base = net_batch_test_snapshot(3);
    MobStateSnapshot current = base;

    MobStateDelta delta;
    TEST_FALSE(mob_state_delta_create(&base, &current, &delta));

    // Movement below the precision is no change
    current.position.x += quantize_precision(0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS) * 0.1f;
    TEST_FALSE(mob_state_delta_create(&base, &current, &delta));

    // Only the rotation changes -> id + mask + rotation
    current = base;
    current.rotation.y = 0.0f;
    current.rotation.w = 1.0f;
    TEST_TRUE(mob_state_delta_create(&base, &current, &delta));
    TEST_EQUALS(delta.changed, MOB_STATE_CHANGED_ROTATION);

    byte buffer[64];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_TRUE(net_packet_serialize(&writer, &delta));
    TEST_EQUALS(bit_writer_bits(&writer), 32 + MOB_STATE_CHANGED_BITS + 2 + 3 * MOB_ROTATION_BITS);

    BitReader reader;
    bit_reader_init(&reader, buffer, bit_writer_flush(&writer));

    MobStateDelta read = {};
    TEST_TRUE(net_packet_serialize(&reader, &read));
    TEST_EQUALS(read.changed, MOB_STATE_CHANGED_ROTATION);
    TEST_EQUALS_WITH_DELTA(read.rotation.w, 1.0, 0.01);

    // A chunk change always includes the position
    current = base;
    current.chunk = base.chunk + 1;
    TEST_TRUE(mob_state_delta_create(&base, &current, &delta));
    TEST_EQUALS(delta.changed, MOB_STATE_CHANGED_CHUNK | MOB_STATE_CHANGED_POSITION);
}

static void test_net_batch_mob_info() {
    MobInfo info = {};
    info.mob_id = 99;
    info.mob_type = 4;
    info.chunk = 77;
    info.level = 127;
    info.health = 100;
    info.resource = 0;
    info.xp = 255;
    info.scale = -7;
    info.weight = 7;
    info.body_type = 3;
    info.skin_color = 31;

    byte buffer[32];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_TRUE(net_packet_serialize(&writer, &info));
    TEST_EQUALS(bit_writer_bits(&writer), 116);

    BitReader reader;
    bit_reader_init(&reader, buffer, bit_writer_flush(&writer));

    MobInfo read = {};
    TEST_TRUE(net_packet_serialize(&reader, &read));
    TEST_MEMORY_EQUALS(&read, &info, sizeof(info));

    // Values that don't fit their bits are rejected instead of being truncated
    info.level = 128;
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_FALSE(net_packet_serialize(&writer, &info));

    info.level = 1;
    info.scale = 8;
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_FALSE(net_packet_serialize(&writer, &info));
}

static void test_net_batch_player_info() {
    static PlayerInfo info;
    memset(&info, 0, sizeof(info));

    info.mob_id = 1001;
    info.chunk = 5;
    str_copy(info.name, "Aldric");
    info.level = 60;
    info.race = 15;
    info.hair_style = 63;
    info.tattoo = 40;

    for (int32 i = 0; i < PLAYER_EQUIPMENT_SLOTS; ++i) {
        info.transmog[i].primary_color = (uint8) (i + 1);
        info.transmog[i].effect = (uint16) (500 - i);
        info.equipment[i] = i % 2 ? 0 : 100000 + i;
    }

    byte buffer[512];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_TRUE(net_packet_serialize(&writer, &info));

    const uint32 size = bit_writer_flush(&writer);
    TEST_LESSER_THAN(size, (uint32) sizeof(PlayerInfo) / 2);

    BitReader reader;
    bit_reader_init(&reader, buffer, size);

    static PlayerInfo read;
    memset(&read, 0xCD, sizeof(read));
    TEST_TRUE(net_packet_serialize(&reader, &read));

    TEST_EQUALS(str_compare(read.name, "Aldric"), 0);
    TEST_EQUALS(read.title[0], 0);
    TEST_EQUALS(read.level, 60);
    TEST_EQUALS(read.race, 15);
    TEST_EQUALS(read.hair_style, 63);
    TEST_EQUALS(read.tattoo, 40);

    for (int32 i = 0; i < PLAYER_EQUIPMENT_SLOTS; ++i) {
        TEST_EQUALS(read.transmog[i].primary_color, i + 1);
        TEST_EQUALS(read.transmog[i].effect, 500 - i);
        TEST_EQUALS(read.equipment[i], i % 2 ? 0 : 100000 + i);
    }
}

static void test_net_batch_roundtrip() {
    byte payload[NET_MAX_PAYLOAD_SIZE];

    NetBatchWriter batch;
    net_batch_begin(&batch, payload, sizeof(payload), 123456);

    // Fill until full
    uint32 added = 0;
    while (true) {
        MobStateSnapshot snapshot = net_batch_test_snapshot(added);
        if (!net_batch_add(&batch, PACKET_TYPE_MOB_STATE_SNAPSHOT, &snapshot)) {
            break;
        }

        ++added;
    }

    // 22 bytes per snapshot (+9 bits type/more) instead of 64 unpacked
    TEST_GREATER_THAN(added, (uint32) 55);
    TEST_EQUALS(batch.count, added);

    const int32 size = net_batch_end(&batch);
    TEST_GREATER_THAN(size, 0);
    TEST_LESSER_THAN(size, NET_MAX_PAYLOAD_SIZE + 1);

    NetBatchReader reader;
    TEST_TRUE(net_batch_read_begin(&reader, payload, (uint32) size));
    TEST_EQUALS(reader.tick, 123456);

    uint8 type;
    uint32 read_count = 0;
    bool all_equal = true;

    while (net_batch_next(&reader, &type)) {
        if (type != PACKET_TYPE_MOB_STATE_SNAPSHOT) {
            all_equal = false;
            net_batch_stop(&reader);
            break;
        }

        MobStateSnapshot snapshot;
        if (!net_batch_read(&reader, &snapshot)) {
            all_equal = false;
            break;
        }

        all_equal &= snapshot.mob_id == read_count && snapshot.chunk == 1000 + read_count;
        ++read_count;
    }

    TEST_TRUE(all_equal);
    TEST_EQUALS(read_count, added);
}

static void test_net_batch_mixed_and_invalid() {
    byte payload[256];

    NetBatchWriter batch;
    net_batch_begin(&batch, payload, sizeof(payload), 1);

    MobStateSnapshot snapshot = net_batch_test_snapshot(1);
    MobInfo info = {};
    info.mob_id = 2;
    info.level = 10;

    MobStateDelta delta = {};
    delta.mob_id = 3;
    delta.changed = MOB_STATE_CHANGED_STATE;
    delta.state_flag = 42;

    TEST_TRUE(net_batch_add(&batch, PACKET_TYPE_MOB_STATE_SNAPSHOT, &snapshot));
    TEST_TRUE(net_batch_add(&batch, PACKET_TYPE_MOB_INFO, &info));
    TEST_TRUE(net_batch_add(&batch, PACKET_TYPE_MOB_STATE_DELTA, &delta));

    // An invalid message is not added and doesn't corrupt the batch
    MobInfo invalid = {};
    invalid.level = 200;
    TEST_FALSE(net_batch_add(&batch, PACKET_TYPE_MOB_INFO, &invalid));
    TEST_EQUALS(batch.count, 3);

    const int32 size = net_batch_end(&batch);

    NetBatchReader reader;
    net_batch_read_begin(&reader, payload, (uint32) size);

    uint8 type;
    uint32 seen = 0;

    TEST_TRUE(net_batch_next(&reader, &type));
    TEST_EQUALS(type, PACKET_TYPE_MOB_STATE_SNAPSHOT);
    MobStateSnapshot s;
    TEST_TRUE(net_batch_read(&reader, &s));
    seen += s.mob_id;

    TEST_TRUE(net_batch_next(&reader, &type));
    TEST_EQUALS(type, PACKET_TYPE_MOB_INFO);
    MobInfo i;
    TEST_TRUE(net_batch_read(&reader, &i));
    seen += i.mob_id;
    TEST_EQUALS(i.level, 10);

    TEST_TRUE(net_batch_next(&reader, &type));
    TEST_EQUALS(type, PACKET_TYPE_MOB_STATE_DELTA);
    MobStateDelta d;
    TEST_TRUE(net_batch_read(&reader, &d));
    seen += d.mob_id;
    TEST_EQUALS(d.state_flag, 42);

    TEST_FALSE(net_batch_next(&reader, &type));
    TEST_EQUALS(seen, 6);

    // Truncated batch: the reader stops without reading out of bounds
    net_batch_read_begin(&reader, payload, (uint32) size / 2);
    uint32 parsed = 0;
    while (net_batch_next(&reader, &type)) {
        MobStateSnapshot any;
        if (type != PACKET_TYPE_MOB_STATE_SNAPSHOT || !net_batch_read(&reader, &any)) {
            net_batch_stop(&reader);
            break;
        }

        ++parsed;
    }

    TEST_LESSER_THAN(parsed, (uint32) 2);

    // Random garbage never crashes and always terminates
    uint32 x = 12345;
    for (int32 round = 0; round < 200; ++round) {
        byte garbage[64];
        for (int32 b = 0; b < (int32) sizeof(garbage); ++b) {
            x = x * 1664525 + 1013904223;
            garbage[b] = (byte) (x >> 24);
        }

        net_batch_read_begin(&reader, garbage, sizeof(garbage));

        int32 guard = 0;
        while (net_batch_next(&reader, &type) && guard++ < 100) {
            bool ok = false;
            switch (type) {
                case PACKET_TYPE_MOB_STATE_SNAPSHOT: { MobStateSnapshot m; ok = net_batch_read(&reader, &m); } break;
                case PACKET_TYPE_MOB_STATE_DELTA: { MobStateDelta m; ok = net_batch_read(&reader, &m); } break;
                case PACKET_TYPE_MOB_INFO: { MobInfo m; ok = net_batch_read(&reader, &m); } break;
                default: net_batch_stop(&reader);
            }

            if (!ok) {
                break;
            }
        }

        TEST_LESSER_THAN(guard, 100);
    }
}

static void test_net_batch_through_envelope() {
    // Client -> server through a real (in memory) connection on the unreliable channel
    static NetConnection client;
    static NetConnection server;
    static NetResendBuffer buffers[4];
    static int32 next[4];

    NetResendPool pool;
    net_resend_pool_init(&pool, buffers, next, 4);

    net_connection_init(&client, false, 0);
    net_connection_init(&server, true, 0);

    client.state = NET_STATE_CONNECTED;
    server.state = NET_STATE_CONNECTED;
    client.connection_id = server.connection_id = 0x00100001;

    for (int32 i = 0; i < NET_KEY_SIZE; ++i) {
        client.tx_key[i] = server.rx_key[i] = (byte) i;
        client.rx_key[i] = server.tx_key[i] = (byte) (i + 100);
    }

    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetBatchWriter batch;
    net_batch_begin(&batch, payload, sizeof(payload), 99);

    for (uint32 i = 0; i < 20; ++i) {
        MobStateSnapshot snapshot = net_batch_test_snapshot(i);
        TEST_TRUE(net_batch_add(&batch, PACKET_TYPE_MOB_STATE_SNAPSHOT, &snapshot));
    }

    const int32 batch_size = net_batch_end(&batch);

    byte packet[NET_MAX_PACKET_SIZE];
    const int32 packet_size = net_connection_write(&client, &pool, packet, NET_CHANNEL_UNRELIABLE, payload, (uint16) batch_size, 0);

    // 20 mobs in one datagram (60 fit): 15 byte header + batch + 16 byte tag
    TEST_EQUALS(packet_size, NET_HEADER_SIZE + batch_size + NET_TAG_SIZE);
    TEST_LESSER_THAN(packet_size, 500);

    byte received[NET_MAX_PAYLOAD_SIZE];
    uint8 channel;
    const int32 received_size = net_connection_read(&server, &pool, packet, packet_size, 0, received, &channel);
    TEST_EQUALS(received_size, batch_size);
    TEST_EQUALS(channel, NET_CHANNEL_UNRELIABLE);

    NetBatchReader reader;
    net_batch_read_begin(&reader, received, (uint32) received_size);

    uint8 type;
    uint32 count = 0;
    while (net_batch_next(&reader, &type)) {
        MobStateSnapshot snapshot;
        if (!net_batch_read(&reader, &snapshot)) {
            break;
        }

        ++count;
    }

    TEST_EQUALS(count, 20);
    TEST_EQUALS(reader.tick, 99);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main NetworkNetBatchTest
#endif

int main() {
    TEST_INIT(512);

    TEST_RUN(test_net_batch_mob_state);
    TEST_RUN(test_net_batch_mob_delta);
    TEST_RUN(test_net_batch_mob_info);
    TEST_RUN(test_net_batch_player_info);
    TEST_RUN(test_net_batch_roundtrip);
    TEST_RUN(test_net_batch_mixed_and_invalid);
    TEST_RUN(test_net_batch_through_envelope);

    TEST_FINALIZE();

    return 0;
}
