#include "../TestFramework.h"
#include "../../image/Qoi.h"
#include "../../image/Image.cpp"
#include "../../system/FileUtils.cpp"

#define STB_IMAGE_IMPLEMENTATION 1
#include "../../../EngineDependencies/image/stb_image.h"

// The test can be executed from the cOMS directory, the build directory or any other
// working directory -> resolve the image relative to this source file first
static bool qoi_test_image_path(char* path) {
    char source[PATH_MAX_LENGTH];
    str_copy(source, __FILE__);

    char* last_separator = NULL;
    for (char* p = source; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            last_separator = p;
        }
    }

    if (last_separator) {
        str_copy(last_separator + 1, "test_src.png");

        // Absolute path of the source file (MSVC usually uses the path of the compiler invocation)
        if (source[0] && source[1] == ':' && file_exists(source)) {
            str_copy(path, source);
            return true;
        }

        // Relative to the directory the compiler was invoked from
        if (file_exists(source)) {
            str_copy(path, source);
            return true;
        }
    }

    // Relative to the executable (build/cOMS and build/tests are 2 directories below the repository)
    const char* candidates[] = {
        "./../../cOMS/tests/image/test_src.png",
        "./../../../cOMS/tests/image/test_src.png",
        "./tests/image/test_src.png",
    };

    for (int32 i = 0; i < ARRAY_COUNT(candidates); ++i) {
        relative_to_absolute(candidates[i], path);
        if (file_exists(path)) {
            return true;
        }
    }

    return false;
}

static void test_qoi() {
    char path[PATH_MAX_LENGTH];
    TEST_TRUE(qoi_test_image_path(path));

    int w = 0;
    int h = 0;
    int n = 0;

    unsigned char* data = stbi_load(path, &w, &h, &n, 4);
    TEST_NOT_EQUALS(data, NULL);

    if (!data) {
        return;
    }

    TEST_EQUALS(w, 3);
    TEST_EQUALS(h, 3);
    TEST_EQUALS(n, 4);

    const unsigned char rgba[] = {
        0xFF, 0x00, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0xFF,

        0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,

        0xA8, 0x2C, 0xE2, 0xFF,
        0xA8, 0x2C, 0xE2, 0xFF,
        0xA8, 0x2C, 0xE2, 0xFF
    };

    TEST_MEMORY_EQUALS(data, rgba, sizeof(rgba));

    unsigned char rgba_encoded[128];
    Image image = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        data // .pixels =
    };
    const int32 encoded_size = qoi_encode(&image, rgba_encoded);

    TEST_TRUE(encoded_size > 0);
    TEST_TRUE(encoded_size <= (int32) sizeof(rgba_encoded));

    unsigned char rgba_decoded[128];
    Image image2 = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        rgba_decoded // .pixels =
    };
    qoi_decode(rgba_encoded, &image2);

    TEST_MEMORY_EQUALS(image2.pixels, rgba, sizeof(rgba));

    stbi_image_free(data);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main QoiTest
#endif

int main() {
    TEST_INIT(10);

    TEST_RUN(test_qoi);

    TEST_FINALIZE();

    return 0;
}
