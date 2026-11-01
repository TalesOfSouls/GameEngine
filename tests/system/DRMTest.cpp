#include "../TestFramework.h"
#include "../../memory/RingMemory.cpp"
#include "../../system/DRM.h"
#include "../../system/FileUtils.cpp"
#include "../../hash/Sha1.h"
#include "../../platform/win32/Process.h"

// Data driven DRM tests (@see data/DRM.h)
static void test_drm_encode_decode() {
    const byte plain[] = "MainTest";
    byte encrypted[sizeof(plain)];
    byte decoded[sizeof(plain)];

    drm_encode(plain, encrypted, sizeof(plain));

    // The encoded signature must not contain the plain text
    TEST_TRUE(memcmp(encrypted, plain, sizeof(plain)) != 0);

    const size_t length = drm_decode(decoded, encrypted, sizeof(plain));
    TEST_EQUALS(length, sizeof(plain));
    TEST_MEMORY_EQUALS(decoded, plain, sizeof(plain));
}

static void test_drm_decode_all() {
    drm_decode_all();

    TEST_EQUALS(DRM_PATTERN_COUNT, ARRAY_COUNT(_drm_patterns));
    TEST_TRUE(DRM_DECODED_BYTES > 0);
    TEST_TRUE(_drm_decoded_max_len > 0);

    size_t total = 0;
    for (size_t i = 0; i < DRM_PATTERN_COUNT; ++i) {
        TEST_NOT_EQUALS(_drm_decoded_sigs[i].data, NULL);
        TEST_EQUALS(_drm_decoded_sigs[i].length, _drm_patterns[i].length);

        // The decoded signature differs from the encoded one
        TEST_TRUE(
            memcmp(
                _drm_decoded_sigs[i].data,
                _drm_patterns[i].encrypted_pattern,
                _drm_patterns[i].length
            ) != 0
        );

        total += _drm_decoded_sigs[i].length;
    }

    TEST_EQUALS(total, DRM_DECODED_BYTES);
}

static void test_sha1() {
    // Known hashes (FIPS 180-1 test vectors)
    byte hash[20];

    sha1_hash((const byte *) "", 0, hash);
    const byte empty_hash[] = {
        0xDA, 0x39, 0xA3, 0xEE, 0x5E, 0x6B, 0x4B, 0x0D, 0x32, 0x55,
        0xBF, 0xEF, 0x95, 0x60, 0x18, 0x90, 0xAF, 0xD8, 0x07, 0x09
    };
    TEST_MEMORY_EQUALS(hash, empty_hash, sizeof(hash));

    sha1_hash((const byte *) "abc", 3, hash);
    const byte abc_hash[] = {
        0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
        0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D
    };
    TEST_MEMORY_EQUALS(hash, abc_hash, sizeof(hash));
}

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

    byte decoded_bytes[sizeof(self_signature)];
    DRMDecodedSignature decoded_patterns[1];
    drm_decode(decoded_bytes, drm_patterns[0].encrypted_pattern, drm_patterns[0].length);
    decoded_patterns[0].data = decoded_bytes;
    decoded_patterns[0].length = drm_patterns[0].length;

    const int64 self_process_id = process_id_get();
    DRMProcessInfo process_info = {0};
    process_info.pid = (uint32) self_process_id;

    RingMemory ring;
    ring_alloc(&ring, 64 * MEGABYTE, 64 * MEGABYTE);
    byte* helper_mem = memory_get(&ring, 64 * MEGABYTE, sizeof(size_t));

    drm_process_scan(
        decoded_patterns, ARRAY_COUNT(decoded_patterns), sizeof(self_signature),
        &process_info, 1,
        helper_mem, 64 * MEGABYTE
    );

    // The signature is part of our own .rdata section
    TEST_TRUE(process_info.scanned);
    TEST_TRUE(process_info.code_detected);

    ring_free(&ring);
}

static void test_drm_is_being_debugged() {
    TEST_FALSE(drm_is_being_debugged());
    TEST_FALSE(drm_has_hardware_breakpoints());
}

static void test_drm_check_process_name() {
    // Our own process name is part of the process list
    // (the name is taken from the executable, so the test also works for renamed builds)
    wchar_t executable_path[PATH_MAX_LENGTH];
    TEST_TRUE(self_file_path(executable_path) > 0);

    wchar_t executable_name[PATH_MAX_LENGTH];
    filename(executable_path, executable_name);
    TEST_TRUE(executable_name[0] != L'\0');

    const wchar_t* self_process[] = {
        executable_name,
    };
    TEST_TRUE(drm_check_process_name((const utf16 **) self_process, ARRAY_COUNT(self_process)));

    // Names that don't exist
    const wchar_t* fake_processes[] = {
        L"__drm_test_process_that_does_not_exist__",
    };
    TEST_FALSE(drm_check_process_name((const utf16 **) fake_processes, ARRAY_COUNT(fake_processes)));
}

static void test_drm_check_window_title() {
    // No window has this title
    const wchar_t* fake_windows[] = {
        L"__drm_test_window_that_does_not_exist__",
    };
    TEST_FALSE(drm_check_window_title(fake_windows, ARRAY_COUNT(fake_windows)));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main DRMTest
#endif

int main() {
    TEST_INIT(12);

    TEST_RUN(test_drm_encode_decode);
    TEST_RUN(test_drm_decode_all);
    TEST_RUN(test_sha1);
    TEST_RUN(test_drm_process_scan);
    TEST_RUN(test_drm_is_being_debugged);
    TEST_RUN(test_drm_check_process_name);
    TEST_RUN(test_drm_check_window_title);

    TEST_FINALIZE();

    return 0;
}
