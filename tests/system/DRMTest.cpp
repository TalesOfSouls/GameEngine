#include "../TestFramework.h"
#include "../../memory/RingMemory.cpp"
#include "../../system/DRM.h"
#include "../../platform/win32/Process.h"
#include "../../hash/Sha1.h"

DECLARE_SECTION(".selfhash")
SECTION_ALLOC(".selfhash")
extern const unsigned char SECTION_START(selfhash)[];

SECTION_ALLOC(".selfhash")
extern const unsigned char self_hash[] = {
    0x32, 0xE2, 0xC0, 0xD5, 0x6C, 0x7C, 0xD0, 0xBB,
    0xCF, 0xC3, 0xB2, 0x0A, 0xE5, 0xDE, 0x51, 0xFB,
    0x53, 0x7A, 0x96, 0x75,
};

SECTION_ALLOC(".selfhash")
extern const unsigned char SECTION_END(selfhash)[];

static void test_drm_process_scan() {
    const byte self_signature[] = "MainTest";

    // @todo Currently the signature needs to be encoded.
    //      However it might make sense to pass decoded signatures to drm_process_scan
    byte self_signature_encoded[sizeof(self_signature)];

    drm_encode(self_signature, self_signature_encoded, sizeof(self_signature));

    const DRMSignature drm_patterns[] = {
        {
            "SelfPattern",
            sizeof(self_signature_encoded),
            self_signature_encoded
        }
    };

    int64 self_process_id = process_id_get();
    DRMProcessInfo process_info = {0};
    process_info.pid = (uint32) self_process_id;
    process_info.is_active = true;

    RingMemory ring;
    ring_alloc(&ring, 256 * MEGABYTE, 256 * MEGABYTE);
    byte* helper_mem = ring_get_memory(&ring, 128);

    drm_process_scan(
        drm_patterns, ARRAY_COUNT(drm_patterns),
        &process_info, 1,
        helper_mem, 128 * MEGABYTE
    );

    TEST_TRUE(process_info.code_detected);

    ring_free(&ring);
}

static void test_drm_is_being_debugged() {
    TEST_FALSE(drm_is_being_debugged());
}

static void test_drm_verify_code_integrity() {
    RingMemory ring;
    ring_alloc(&ring, 256 * MEGABYTE, 256 * MEGABYTE);

    wchar_t exe_path[PATH_MAX_LENGTH];
    self_file_path(exe_path);

    FileBody exe_file = {0};
    file_read(exe_path, &exe_file, &ring);

    byte hash[20];

    sha1_hash(exe_file.content + 0, 20, hash, 8);
    TEST_TRUE(memcmp(hash, self_hash, 20) == 0);

    sha1_hash(exe_file.content + 0, 40, hash, 8);
    TEST_FALSE(memcmp(hash, self_hash, 20) == 0);

    ring_free(&ring);
}

static void test_drm_check_window_title() {
    const wchar_t* malicious_window[] = {
        L"MainTest",
        L"DRMTest",
    };

    TEST_TRUE(drm_check_process_name(malicious_window, ARRAY_COUNT(malicious_window)));
}

static void test_drm_check_process_name() {
    const wchar_t* malicious_processes[] = {
        L"MainTest",
        L"DRMTest",
    };

    TEST_TRUE(drm_check_process_name(malicious_processes, ARRAY_COUNT(malicious_processes)));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main DRMTest
#endif

int main() {
    TEST_INIT(5);

    TEST_RUN(test_drm_process_scan);
    TEST_RUN(test_drm_is_being_debugged);
    TEST_RUN(test_drm_verify_code_integrity);
    TEST_RUN(test_drm_check_window_title);
    TEST_RUN(test_drm_check_process_name);

    TEST_FINALIZE();

    return 0;
}
