/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_ANTI_ALIASING_MSAA_H
#define COMS_GPUAPI_ANTI_ALIASING_MSAA_H

static const v2_f32 MSAA_OFFSETS_1[1] = { {0.5f, 0.5f} };
static const v2_f32 MSAA_OFFSETS_2[2] = { {0.25f, 0.5f}, {0.75f, 0.5f} };
static const v2_f32 MSAA_OFFSETS_4[4] = {
    {0.375f, 0.125f},
    {0.875f, 0.375f},
    {0.125f, 0.625f},
    {0.625f, 0.875f}
};
static const v2_f32 MSAA_OFFSETS_8[8] = {
    {0.5625f, 0.4375f}, {0.0625f, 0.6875f}, {0.3125f, 0.1875f}, {0.8125f, 0.9375f},
    {0.4375f, 0.8125f}, {0.6875f, 0.3125f}, {0.1875f, 0.0625f}, {0.9375f, 0.5625f}
};

static inline const v2_f32* get_msaa_offsets(int32 steps) {
    switch (steps) {
        case 2:
            return MSAA_OFFSETS_2;
        case 4:
            return MSAA_OFFSETS_4;
        case 8:
            return MSAA_OFFSETS_8;
        default:
            return MSAA_OFFSETS_1;
    }
}

#endif