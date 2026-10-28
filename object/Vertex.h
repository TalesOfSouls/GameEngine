/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_VERTEX_H
#define COMS_OBJECT_VERTEX_H

#include "../stdlib/Stdlib.h"

struct Vertex4DSamplerTextureColor {
    v4_f32 position;
    // <= 0 = solid color, 1 = font atlas, 2 = ui atlas
    int32 sampler;

    // Texture coordinates, only used if sampler > 0
    v2_f32 tex_coord;
    uint32 color;
};

struct VertexUI {
    v2_f32 position;
    v2_f32 uv;
    int32 sampler;

    // e.g. color and outline color
    uint32 idata[2];

    // e.g. outline width
    f32 fdata[2];
};

struct Vertex3D {
    v3_f32 position;
    v3_f32 normal;
    v2_f32 tex_coord;
    v4_f32 color;
};

struct Vertex3DNormal {
    v3_f32 position;
    v3_f32 normal;
};

struct Vertex3DSamplerTextureColor {
    v3_f32 position;
    // <= 0 = solid color, 1 = font atlas, 2 = ui atlas
    int32 sampler;

    // Texture coordinates, only used if sampler > 0
    v2_f32 tex_coord;

    uint32 color;
};

struct Vertex3DTextureColor {
    v3_f32 position;

    // Texture coordinates, only used if color == 0
    v2_f32 tex_coord;

    // 0 = use the texture instead
    uint32 color;
};

struct Vertex3DColorIndex {
    v3_f32 position;
    f32 color;
};

struct Vertex3DColor {
    v3_f32 position;
    v4_f32 color;
};

struct Vertex2D {
    v2_f32 position;
    v2_f32 tex_coord;
    v4_f32 color;
};

struct Vertex2DTexture {
    v2_f32 position;
    v2_f32 tex_coord;
};

struct Vertex2DSamplerTextureColor {
    v2_f32 position;
    // <= 0 = solid color, 1 = font atlas, 2 = ui atlas
    int32 sampler;

    // Texture coordinates, only used if sampler > 0
    v2_f32 tex_coord;
    uint32 color;
};

struct Vertex2DColor {
    v2_f32 position;
    v4_f32 color;
};

struct VertexRef {
    uint32 data_id;

    uint32 position_id;
    uint32 normal_id;

    uint32 tex_coord_id;
    uint32 sampler_id; // e.g. GL_TEXTURE0

    uint32 color_id;

    uint32 index_id;
};

// Data for the text shader
struct TextPipeline {
    uint32 program_id;

    uint32 matrix_id;
    uint32 matrix_addr;

    uint32 uv_addr;
    uint32 color_addr;
    uint32 sampler_addr;
};

enum VertexType {
    VERTEX_TYPE_POSITION = 1,
    VERTEX_TYPE_NORMAL = 2,
    VERTEX_TYPE_TEXTURE_COORD = 4,
    VERTEX_TYPE_COLOR = 8,
    VERTEX_TYPE_ALL = 15,
};

const CONSTEXPR int32 _GPUAPI_ATTRIBUTE_STRIDE[] = {
    sizeof(Vertex3D), // GPU_ATTRIBUTE_TYPE_VERTEX_3D
    sizeof(Vertex3DNormal), // GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL
    sizeof(Vertex3DColor), // GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR
    sizeof(Vertex3DTextureColor), // GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR
    sizeof(Vertex3DSamplerTextureColor), // GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR
    sizeof(Vertex2DTexture), // GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE
    sizeof(Vertex2DSamplerTextureColor), // GPU_ATTRIBUTE_TYPE_VERTEX_2D_SAMPLER_TEXTURE_COLOR
};

#endif