#include "../TestFramework.h"
#include "../../image/Qoi.h"
#include "../../image/Image.cpp"
#include "../../system/FileUtils.cpp"

#define STB_IMAGE_IMPLEMENTATION 1
#include "../../image/stb_image.h"

static void test_qoi() {
    int w;
    int h;
    int n;

    char path[MAX_PATH];
    relative_to_absolute("./../../cOMS/tests/image/test_src.png", path);

    unsigned char* data = stbi_load(path, &w, &h, &n, 4);

    unsigned char rgba[] = {
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

    TEST_EQUALS(memcmp(data, rgba, sizeof(rgba)), 0);

    unsigned char rgba_encoded[128];
    Image image = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        data // .pixels =
    };
    qoi_encode(&image, rgba_encoded);

    unsigned char rgba_decoded[128];
    Image image2 = {
        3, // .width =
        3, // .height =
        9, // .pixel_count =
        4, // .image_settings =
        rgba_decoded // .pixels =
    };
    qoi_decode(rgba_encoded, &image2);

    TEST_TRUE(memcmp(image2.pixels, rgba, sizeof(rgba)) == 0);

    /*
    for (int i = 0; i < sizeof(rgba); i++) {
        if (i % 12 == 0) {
            printf("\n");
        }
        if (i % 4 == 0) {
            printf(" - ");
        }
        printf("%02X ", (unsigned int) image2.pixels[i]);
    }
    printf("\n");

    for (int i = 0; i < sizeof(rgba); i++) {
        if (i % 12 == 0) {
            printf("\n");
        }
        if (i % 4 == 0) {
            printf(" - ");
        }
        printf("%02X ", (unsigned int) rgba[i]);
    }
    printf("\n");
    */
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
