#include "../TestFramework.h"
#include "../../image/Qoi.h"
#include "../../image/Image.cpp"

#define STB_IMAGE_IMPLEMENTATION 1
#include "../../image/stb_image.h"

static void test_qoi() {
    int w;
    int h;
    unsigned char* data = stbi_load("test_src.png", &w, &h);

    unsigned char rgba[] = {
        0xFF, 0x00, 0x00, 0xFF,
        0x00, 0xFF, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0xFF,

        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,

        0xA8, 0x2C, 0xE2, 0xFF,
        0xA8, 0x2C, 0xE2, 0xFF,
        0xA8, 0x2C, 0xE2, 0xFF
    };

    TEST_TRUE(memcmp(data, rgba, sizeof(rgba)) == 0);

    unsigned char rgba_encoded[128];
    Image image = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        data // .pixels =
    };
    int32 qoi_encode_length = qoi_encode(&image, rgba_encoded);

    unsigned char rgba_decoded[128];
    Image image2 = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        rgba_decoded // .pixels =
    };
    int32 qoi_decode_length = qoi_decode(rgba_encoded, &image2);

    TEST_TRUE(memcmp(image2.pixels, rgba, sizeof(rgba)) == 0);
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
