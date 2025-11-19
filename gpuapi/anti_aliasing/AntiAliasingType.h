/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_ANTI_ALIASING_H
#define COMS_GPUAPI_ANTI_ALIASING_H

enum AntiAliasingType : byte {
    ANTI_ALIASING_TYPE_NONE,
    ANTI_ALIASING_TYPE_MSAA,
    ANTI_ALIASING_TYPE_SSAA,
    ANTI_ALIASING_TYPE_SMAA,
    ANTI_ALIASING_TYPE_FXAA,
    ANTI_ALIASING_TYPE_TAA,
};

#endif