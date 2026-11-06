#include "../TestFramework.h"
#include "../../asset/AssetArchive.cpp"

/**
 * Asset archive (.asset) format test
 *
 * The archive is built by hand (like ArchiveBuilder does it) from in-memory payloads,
 * written to a file, loaded and then verified. In-memory payloads keep the test
 * independent of the working directory.
 */
static void test_asset_archive() {
    const char payload_a[] = "Asset one";         // 9 characters
    const char payload_b[] = "Asset two content"; // 17 characters

    const uint32 payload_a_size = (uint32) (sizeof(payload_a) - 1);
    const uint32 payload_b_size = (uint32) (sizeof(payload_b) - 1);

    const uint32 asset_count = 2;
    const uint32 dependency_count = 0;

    // Fixed size header: 3 * 4 bytes + asset elements (+ dependencies)
    const uint32 header_size = (uint32) (
        sizeof(int32) * 3
        + asset_count * sizeof(AssetArchiveElement)
        + dependency_count * sizeof(uint32)
    );

    alignas(8) byte archive_buffer[4096];
    memset(archive_buffer, 0, sizeof(archive_buffer));

    byte* header = archive_buffer;

    header = write_le(header, (int32) ASSET_ARCHIVE_VERSION);
    header = write_le(header, asset_count);
    header = write_le(header, dependency_count);

    // Asset a
    header = write_le(header, (uint32) ASSET_TYPE_GENERAL);
    header = write_le(header, (uint32) header_size);
    header = write_le(header, payload_a_size);
    header = write_le(header, payload_a_size);
    header = write_le(header, (uint32) 0); // dependency start
    header = write_le(header, (uint32) 0); // dependency count

    // Asset b
    header = write_le(header, (uint32) ASSET_TYPE_GENERAL);
    header = write_le(header, (uint32) (header_size + payload_a_size));
    header = write_le(header, payload_b_size);
    header = write_le(header, payload_b_size);
    header = write_le(header, (uint32) 0);
    header = write_le(header, (uint32) 0);

    // Payloads
    memcpy(archive_buffer + header_size, payload_a, payload_a_size);
    memcpy(archive_buffer + header_size + payload_a_size, payload_b, payload_b_size);

    const size_t archive_size = header_size + payload_a_size + payload_b_size;

    /////////////////////////////////////////////////
    // 1. Write the archive file
    /////////////////////////////////////////////////
    // Own file name (never touches a real archive) and it is written next to the executable
    // so the test doesn't litter the repository
    wchar_t archive_path[PATH_MAX_LENGTH];
    relative_to_absolute(L"./test_asset_archive.asset", archive_path);

    FileBody archive_file = {
        archive_size,
        archive_buffer
    };

    TEST_TRUE(file_write(archive_path, &archive_file));

    /////////////////////////////////////////////////
    // 2. Load the archive file
    /////////////////////////////////////////////////
    BufferMemory mem = {0};
    buffer_alloc(&mem, 64 * KILOBYTE, 64 * KILOBYTE);

    AssetArchive archive = {0};
    archive.data = (byte *) memory_get(&mem, 16 * KILOBYTE, alignof(size_t));
    archive.data_size = 16 * KILOBYTE;

    asset_archive_load(&archive, archive_path, &mem);

    /////////////////////////////////////////////////
    // 3. Verify the archive
    /////////////////////////////////////////////////
    TEST_EQUALS(archive.header.version, ASSET_ARCHIVE_VERSION);
    TEST_EQUALS(archive.header.asset_count, asset_count);
    TEST_EQUALS(archive.header.asset_dependency_count, dependency_count);

    const AssetArchiveElement* element = asset_archive_element_find(&archive, 0);
    TEST_EQUALS(element->type, ASSET_TYPE_GENERAL);
    TEST_EQUALS(element->start, header_size);
    TEST_EQUALS(element->length, payload_a_size);
    TEST_EQUALS(element->uncompressed, payload_a_size);
    TEST_EQUALS(element->dependency_start, 0);
    TEST_EQUALS(element->dependency_count, 0);

    element = asset_archive_element_find(&archive, 1);
    TEST_EQUALS(element->type, ASSET_TYPE_GENERAL);
    TEST_EQUALS(element->start, header_size + payload_a_size);
    TEST_EQUALS(element->length, payload_b_size);
    TEST_EQUALS(element->uncompressed, payload_b_size);
    TEST_EQUALS(element->dependency_count, 0);

    // The payloads are stored unmodified
    FileBody loaded_file = {0};
    loaded_file.content = (byte *) memory_get(&mem, archive_size + 1, alignof(size_t));
    file_read(archive_path, &loaded_file, 0, &mem);

    TEST_EQUALS(loaded_file.size, archive_size);
    TEST_MEMORY_EQUALS(
        loaded_file.content + asset_archive_element_find(&archive, 0)->start,
        payload_a,
        payload_a_size
    );
    TEST_MEMORY_EQUALS(
        loaded_file.content + asset_archive_element_find(&archive, 1)->start,
        payload_b,
        payload_b_size
    );

    // Asset id = archive id (0) | asset index
    TEST_EQUALS(
        ASSET_RAW_ID_FROM_ID(ASSET_ID_FROM_ARCHIVE_AND_ASSET(1, 0)),
        1
    );
    TEST_EQUALS(
        ARCHIVE_ID_FROM_ASSET_ID(ASSET_ID_FROM_ARCHIVE_AND_ASSET(0, 4)),
        4
    );

    // Cleanup
    asset_archive_close(&archive);
    buffer_free(&mem);
    file_delete(archive_path);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main AssetArchiveTest
#endif

int main() {
    TEST_INIT(5);

    TEST_RUN(test_asset_archive);

    TEST_FINALIZE();

    return 0;
}
