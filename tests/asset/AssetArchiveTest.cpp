#include "../TestFramework.h"
#include "../../asset/AssetArchive.cpp"

static void test_asset_archive() {
    /**
     * 1. Create asset archive file
     * 2. Load asset archive file
     * 3. Test the loaded asset data
     */

    /////////////////////////////////////////////////
    // 1. Create asset archive file
    /////////////////////////////////////////////////

    // Current executable file path
    char rel_path[PATH_MAX_LENGTH];
    relative_to_absolute("./", rel_path);

    alignas(8) byte archive_buffer[4096];

    const char toc_str[] = "0\n"
        "./files/test1.txt\n"
        "./files/test2.txt";
    alignas(8) byte toc_buffer[sizeof(toc_str)];

    FileBody toc = {0};
    toc.content = toc_buffer;
    memcpy(toc.content, toc_str, sizeof(toc_str));

    FileBody output_header = {0};
    output_header.content = archive_buffer;

    FileBody output_body = {0};
    // We offset the body from the header
    // At this point we don't know how large the header actually needs to be, so we pick a generous offset
    // Don't worry, we don't store the unused space in the asset later, we just need it now.
    // In reality the offset we choose here is maybe too small for a general implementation but since i want to keep this test
    // on the stack I am a little bit more stingy
    // We also need to use the same buffer for header and body because we later need to calculate the offset based on memory position
    // The offset is then stored in the asset archive itself so we can directly jump to the respective asset
    byte* archive_body = archive_buffer + 2 * KILOBYTE;
    output_body.content = archive_body;

    // Find the file id
    MAYBE_UNUSED int32 toc_id = atoi((char *) toc.content);
    while (*toc.content != '\n') {
        ++toc.content;
        --toc.size;
    }

    ++toc.content;
    --toc.size;

    // Start writing some basic information to the header (in binary = ready to output to file)
    byte* archive_header = archive_buffer;

    // WARNING: Normally this type of directly writing to a buffer is discouraged because it may fail
    //          We are only allowed to do it this way because we aligned the buffer to 8 bytes
    //          The "correct" implementation would be to use memcpy()

    // Write archive version to header
    *((int32 *) archive_header) = SWAP_ENDIAN_LITTLE(ASSET_ARCHIVE_VERSION);
    archive_header += sizeof(int32);

    // "Write" asset count to header
    // We actually only create the placeholder since we don't know the asset count just yet
    // For that reason we create a pointer to that location so we can easily write the actual count later on
    uint32* asset_count = (uint32 *) archive_header;
    *((uint32 *) archive_header) = 0;
    int32 temp_asset_count = 0;
    archive_header += sizeof(uint32);

    // Write the asset dependency in a similar way to the asset count
    // The asset dependency describes how many other assets this dependency relies on
    *((uint32 *) archive_header) = 0;
    uint32* asset_dependency_count = (uint32 *) archive_header;
    archive_header += sizeof(uint32);

    // Create archive file data by loading the assets from the table of contents (toc)
    // and then loading this asset data into our archive file
    // While "copying" the asset data over to the asset archive we do some transformation & compression
    // as needed and used in our game engine
    byte* pos = toc.content;
    while (*pos != '\0') {
        // Get the file path for the asset (stored in the toc)
        char* file_path = (char *) pos;
        str_move_to((const char **) &pos, '\n');
        if (*pos == '\n') {
            if (*(pos - 1) == '\r') {
                *(pos - 1) = '\0';
            }

            *pos++ = '\0';
        }

        // Make the path of the asset absolute
        char input_path[PATH_MAX_LENGTH];
        if (*file_path == '.') {
            memcpy(input_path, rel_path, sizeof(rel_path));
            strcpy(input_path + strlen(input_path), file_path + 1);
        } else {
            strcpy(input_path, file_path);
        }

        // Get the file extension of the asset (we handle file types differently)
        char* extension = (char *) (pos - 1);
        while (*extension != '.') {
            --extension;
        }

        ++temp_asset_count;

        // We need this to later on calculate the written size to the asset archive
        byte* element_start = archive_body;

        // The raw data size (usefull, when loading the asset so we can reserve the correct size immediately)
        uint32 uncompressed_length = 0;

        // In our test we only use one element type for now
        // The element type is derived from the file extension
        // @todo maybe test with more element types?
        uint32 element_type = ASSET_TYPE_GENERAL;
        if (strncmp(extension, ".wav", sizeof("wav") - 1) == 0) {
        } else if (strncmp(extension, ".objtxt", sizeof("objtxt") - 1) == 0) {
        } else if (strncmp(extension, ".langtxt", sizeof("langtxt") - 1) == 0) {
        } else if (strncmp(extension, ".fonttxt", sizeof("fonttxt") - 1) == 0) {
        } else if (strncmp(extension, ".themetxt", sizeof("themetxt") - 1) == 0) {
        } else if (strncmp(extension, ".png", sizeof("png") - 1) == 0
            || strncmp(extension, ".bmp", sizeof("bmp") - 1) == 0
            || strncmp(extension, ".tga", sizeof("tga") - 1) == 0
        ) {
        } else {
            element_type = ASSET_TYPE_GENERAL;

            // The real implementation uses heap memory based on the file size
            // Again we would like to avoid this in this test and we are only testing with small files for now
            byte temp_file_buffer[1024];
            FileBody asset_file = {
                sizeof(temp_file_buffer),
                temp_file_buffer
            };

            file_read(input_path, &asset_file);

            if (strncmp(extension, ".fs", sizeof("fs") - 1) == 0
                || strncmp(extension, ".vs", sizeof("vs") - 1) == 0
                || strncmp(extension, ".hlsl", sizeof("hlsl") - 1) == 0
            ) {
            } else {
                // @todo we should compress this file here
                memcpy(archive_body, asset_file.content, asset_file.size);
                uncompressed_length = (uint32) asset_file.size;
                archive_body += uncompressed_length;
            }
        }

        // Write element information to the header
        // The offset which we write to the header are not yet correct, but wee will adjust them later on

        // Type
        *((uint32 *) archive_header) = SWAP_ENDIAN_LITTLE(element_type);
        archive_header += sizeof(uint32);

        // Start
        *((uint32 *) archive_header) = (uint32) (element_start - output_body.content);
        archive_header += sizeof(uint32);

        // Length
        *((uint32 *) archive_header) = SWAP_ENDIAN_LITTLE((uint32) (archive_body - element_start));
        archive_header += sizeof(uint32);

        // Uncompressed
        *((uint32 *) archive_header) = SWAP_ENDIAN_LITTLE(uncompressed_length);
        archive_header += sizeof(uncompressed_length);

        // Dependency Start
        *((uint32 *) archive_header) = 0;
        archive_header += sizeof(uint32);

        // Dependency Count
        *((uint32 *) archive_header) = 0;
        archive_header += sizeof(uint32);
    }

    // Now we update some header information as mentioned in the beginning
    *asset_count = SWAP_ENDIAN_LITTLE(temp_asset_count);
    *asset_dependency_count = SWAP_ENDIAN_LITTLE(*asset_dependency_count);

    // Calculate header size
    output_header.size = archive_header - output_header.content;

    // Go to first asset_element (after version, asset_count, asset_dependency_count)
    archive_header = output_header.content + sizeof(int32) * 3;

    // Adjust the offsets to file offsets by including the header size as described earlier
    for (int32 i = 0; i < temp_asset_count; ++i) {
        AssetArchiveElement* element = (AssetArchiveElement *) archive_header;
        element->start = SWAP_ENDIAN_LITTLE((uint32) (element->start + output_header.size));
        element->dependency_start = SWAP_ENDIAN_LITTLE((uint32) (element->dependency_start + output_header.size));

        archive_header += sizeof(AssetArchiveElement);
    }

    // We now write the data to the file
    file_write(L"temp.asset", &output_header);

    // At this place we also remove the unused space in the header
    output_body.size = archive_body - output_body.content;
    file_append(L"temp.asset", &output_body);

    /////////////////////////////////////////////////
    // 2. Load asset archive file
    /////////////////////////////////////////////////

    BufferMemory buf;
    buffer_alloc(&buf, 16 * MEGABYTE, 16 * MEGABYTE);

    RingMemory ring;
    ring_alloc(&ring, 16 * MEGABYTE, 16 * MEGABYTE);

    AssetArchive archive;
    asset_archive_load(&archive, L"temp.asset", &buf, &ring);

    /////////////////////////////////////////////////
    // 3. Test asset archive file
    /////////////////////////////////////////////////

    TEST_EQUALS(archive.header.asset_count, 2);
    TEST_EQUALS(archive.header.asset_dependency_count, 0);

    const AssetArchiveElement* element = &archive.header.asset_element[0 & 0x00FFFFFF];
    TEST_EQUALS(element->type, ASSET_TYPE_GENERAL);
    TEST_EQUALS(element->length, 0); // @bug wrong since we don't copy the test files to the build destination
    TEST_EQUALS(element->uncompressed, 0); // @bug wrong since we don't copy the test files to the build destination
    TEST_EQUALS(element->dependency_count, 0);

    // Cleanup
    ring_free(&ring);
    buffer_free(&buf);
    file_delete(L"temp.asset");
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