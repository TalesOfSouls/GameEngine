#include "../TestFramework.h"
#include "../../image/Qoi.h"
#include "../../image/Image.cpp"
#include "../../system/FileUtils.cpp"

#define STB_IMAGE_IMPLEMENTATION 1
#include "../../../EngineDependencies/image/stb_image.h"

#ifndef QOI_TEST_IMAGE
    // Relative to the repository root (the test scripts run the tests from there)
    #define QOI_TEST_IMAGE "tests/image/test_src.png"
#endif

static void test_qoi() {
    TEST_TRUE(file_exists(QOI_TEST_IMAGE));

    int w = 0;
    int h = 0;
    int n = 0;

    unsigned char* data = stbi_load(QOI_TEST_IMAGE, &w, &h, &n, 4);
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
