/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SHADER_TYPE_H
#define COMS_GPUAPI_SHADER_TYPE_H

enum ShaderType {
    SHADER_TYPE_NONE,
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_GEOMETRY,
    SHADER_TYPE_TESSELATION,
    SHADER_TYPE_PIXEL,
    SHADER_TYPE_MESH,
    SHADER_TYPE_RAYTRACING,
    SHADER_TYPE_TENSOR,
    SHADER_TYPE_SIZE
};

#endif