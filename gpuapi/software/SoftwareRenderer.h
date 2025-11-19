/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_RENDERER_H
#define COMS_GPUAPI_SOFTWARE_RENDERER_H

#include "../../stdlib/Types.h"
#include "../../object/Vertex.h"
#include "../../object/Texture.h"
#include "../../memory/ChunkMemory.h"
#include "SoftwareDescriptorSetLayoutBinding.h"
#include "Shader.h"
#include "../anti_aliasing/AntiAliasingType.h"
#include "../anti_aliasing/MSAA.h"
#include <windows.h>

#if _WIN32
    #include "win32/PlatformSoftwareRenderer.h"
#endif

/**
 * Usage:
 *      soft_clear()
 *      soft_render()
 *      ...
 *      soft_render()
 *      soft_buffer_swap()
 */

struct SoftwareRenderer {
    v2_uint16 dimension;
    v2_uint16 max_dimension;

    // @todo Consider to make these two arrays arrays of arrays so we can support frames in flight
    uint32* pixels;
    f32* zbuffer;

    Shader* active_shader;

    AntiAliasingType aa_type;
    int8 aa_details;

    v4_byte background_color;

    // We support up to 32 texture bindings (same as opengl)
    const Texture* textures[32];

    // Persistent descriptor set
    SoftwareDescriptorSetLayoutBinding descriptor_set_layout[12];

    // This simulates partly the VRAM but as temp memory
    ChunkMemory buf;

    PlatformSoftwareRenderer platform;
};

FORCE_INLINE
void soft_clear(SoftwareRenderer* renderer) NO_EXCEPT
{
    const int32 dim = renderer->dimension.width * renderer->dimension.width;
    memset(renderer->pixels, 0, sizeof(uint32) * dim);
    memset(renderer->zbuffer, 0, sizeof(f32) * dim);
}

inline
void soft_clear(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        uint32* pixel_row = renderer->pixels + j * renderer->dimension.width + x;
        f32* z_row = renderer->zbuffer + j * renderer->dimension.width + x;
        memset(pixel_row, 0, sizeof(uint32) * (x1 - x));
        memset(z_row, 0, sizeof(f32) * (x1 - x));
    }
}

FORCE_INLINE
void soft_clear_color(SoftwareRenderer* renderer) NO_EXCEPT
{
    memset(renderer->pixels, 0, sizeof(uint32) * renderer->dimension.width * renderer->dimension.width);
}

inline
void soft_clear_color(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        uint32* pixel_row = renderer->pixels + j * renderer->dimension.width + x;
        memset(pixel_row, 0, sizeof(uint32) * (x1 - x));
    }
}

FORCE_INLINE
void soft_clear_depth(SoftwareRenderer* renderer) NO_EXCEPT
{
    memset(renderer->zbuffer, 0, sizeof(f32) * renderer->dimension.width * renderer->dimension.width);
}

inline
void soft_clear_depth(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        f32* z_row = renderer->zbuffer + j * renderer->dimension.width + x;
        memset(z_row, 0, sizeof(f32) * (x1 - x));
    }
}

static inline
v4_byte soft_sample_texture(Texture* texture, f32 u, f32 v) NO_EXCEPT {
    const int32 tw = texture->image.width;
    const int32 th = texture->image.height;

    const int32 x = OMS_CLAMP((int32)(u * (tw-1) + 0.5f), 0, tw);
    const int32 y = OMS_CLAMP((int32)(v * (th-1) + 0.5f), 0, th - 1);

    v4_byte color;
    color.val = SWAP_ENDIAN_BIG_32(texture->image.pixels[(y * tw + x) * 4]);

    return color;
}

static inline
v2_f32 soft_ndc_to_screen(
    f32 ndc_x, f32 ndc_y,
    v2_uint16 dimension
) NO_EXCEPT {
    return {
        // Map from NDC (-1..1) -> [0..width-1], [0..height-1]
        (f32) ((ndc_x * 0.5f + 0.5f) * (f32) dimension.width + 0.5f),
        // Flip Y for top-down DIB memory layout
        (f32) (((1.0f - (ndc_y * 0.5f + 0.5f)) * (f32) dimension.height) + 0.5f)
    };
}

static FORCE_INLINE
f32 soft_edge(v2_f32 a, v2_f32 b, v2_f32 c) NO_EXCEPT {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static FORCE_INLINE
v3_f32 soft_edge_coeff(v2_f32 a, v2_f32 b) NO_EXCEPT {
    return {a.y - b.y, b.x - a.x, a.x * b.y - a.y * b.x};
}

static
void soft_rasterize(
    const SoftwareRenderer* __restrict renderer,
    Vertex4DSamplerTextureColor v0,
    Vertex4DSamplerTextureColor v1,
    Vertex4DSamplerTextureColor v2,
    Texture* texture = NULL,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    PSEUDO_USE(steps);

    // @question Do we want to maybe change this to v2_int32?
    //          The problem with that is that we need f32s for AA?
    // Convert NDC [-1,1] -> screen (Y flipped for top-down DIB)
    v2_f32 pos_1 = soft_ndc_to_screen(v0.position.x, v0.position.y, renderer->dimension);
    v2_f32 pos_2 = soft_ndc_to_screen(v1.position.x, v1.position.y, renderer->dimension);
    v2_f32 pos_3 = soft_ndc_to_screen(v2.position.x, v2.position.y, renderer->dimension);

    f32 area = soft_edge(pos_1, pos_2, pos_3);
    if (area <= 0.0f) {
        // == 0 is degenerate triangle
        // < 0 is backface -> backface culling
        return;
    }

    f32 inv_area = 1.0f / area;

    v3_f32 e0 = soft_edge_coeff(pos_2, pos_3);
    v3_f32 e1 = soft_edge_coeff(pos_3, pos_1);
    v3_f32 e2 = soft_edge_coeff(pos_1, pos_2);

    int32 minx = OMS_MAX(0, OMS_MIN(OMS_MIN((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    int32 maxx = OMS_MIN(renderer->dimension.width - 1, OMS_MAX(OMS_MAX((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    int32 miny = OMS_MAX(0, OMS_MIN(OMS_MIN((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));
    int32 maxy = OMS_MIN(renderer->dimension.height - 1, OMS_MAX(OMS_MAX((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));

    uint32* pixels = renderer->pixels;
    f32* zbuf = renderer->zbuffer;

    // Check if the triangle uses texture or solid color
    bool textured = (v0.texture_color.x >= 0.0f || v1.texture_color.x >= 0.0f || v2.texture_color.x >= 0.0f);

    v4_byte color1 = {};
    v4_byte color2 = {};
    v4_byte color3 = {};
    if(!textured){
        color1.val = BITCAST(v0.texture_color.y, uint32);
        color2.val = BITCAST(v1.texture_color.y, uint32);
        color3.val = BITCAST(v2.texture_color.y, uint32);
    }

    for (int32 y = miny; y <= maxy - 7; y++) {
        int32 x = minx;
        int32 y_width = y * renderer->dimension.width;

        for (; x <= maxx; ++x) {
            v2_f32 pos_0 = {(f32) x, (f32) y};

            f32 w0 = soft_edge(pos_2, pos_3, pos_0) * inv_area;
            f32 w1 = soft_edge(pos_3, pos_1, pos_0) * inv_area;
            f32 w2 = soft_edge(pos_1, pos_2, pos_0) * inv_area;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                f32 depth = w0 * v0.position.z
                    + w1 * v1.position.z
                    + w2 * v2.position.z;

                int32 idx = y_width + x;

                if (depth < zbuf[idx]) {
                    zbuf[idx] = depth;

                    if (textured) {
                        // Interpolate UVs
                        f32 u = w0 * v0.texture_color.x
                            + w1 * v1.texture_color.x
                            + w2 * v2.texture_color.x;

                        f32 v = w0 * v0.texture_color.y
                            + w1 * v1.texture_color.y
                            + w2 * v2.texture_color.y;

                        pixels[idx] = soft_sample_texture(texture, u, v).val;
                    } else {
                        // Interpolate color components
                        f32 rf = (w0 * color1.r + w1 * color2.r + w2 * color3.r);
                        f32 gf = (w0 * color1.g + w1 * color2.g + w2 * color3.g);
                        f32 bf = (w0 * color1.b + w1 * color2.b + w2 * color3.b);
                        f32 af = (w0 * color1.a + w1 * color2.a + w2 * color3.a);

                        pixels[idx] = ((uint32) rf << 24)
                           | ((uint32) gf << 16)
                           | ((uint32) bf << 8)
                           | ((uint32) af);
                    }
                }
            }
        }
    }
}

// This version is not a true msaa
// We would need a sample buffer for full msaa implementation
static
void soft_rasterize_msaa(
    const SoftwareRenderer* __restrict renderer,
    Vertex4DSamplerTextureColor v0,
    Vertex4DSamplerTextureColor v1,
    Vertex4DSamplerTextureColor v2,
    Texture* texture = NULL,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    PSEUDO_USE(steps);

    // Convert NDC [-1,1] -> screen (Y flipped for top-down DIB)
    v2_f32 pos_1 = soft_ndc_to_screen(v0.position.x, v0.position.y, renderer->dimension);
    v2_f32 pos_2 = soft_ndc_to_screen(v1.position.x, v1.position.y, renderer->dimension);
    v2_f32 pos_3 = soft_ndc_to_screen(v2.position.x, v2.position.y, renderer->dimension);

    f32 area = soft_edge(pos_1, pos_2, pos_3);
    if (area <= 0.0f) {
        // == 0 is degenerate triangle
        // < 0 is backface -> backface culling
        return;
    }

    f32 inv_area = 1.0f / area;

    // msaa samples
    f32 inv_samples = 1.0f / renderer->aa_details;

    v3_f32 e0 = soft_edge_coeff(pos_2, pos_3);
    v3_f32 e1 = soft_edge_coeff(pos_3, pos_1);
    v3_f32 e2 = soft_edge_coeff(pos_1, pos_2);

    int32 minx = OMS_MAX(0, OMS_MIN(OMS_MIN((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    int32 maxx = OMS_MIN(renderer->dimension.width - 1, OMS_MAX(OMS_MAX((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    int32 miny = OMS_MAX(0, OMS_MIN(OMS_MIN((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));
    int32 maxy = OMS_MIN(renderer->dimension.height - 1, OMS_MAX(OMS_MAX((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));

    uint32* pixels = renderer->pixels;
    f32* zbuf = renderer->zbuffer;

    // Check if the triangle uses texture or solid color
    bool textured = (v0.texture_color.x >= 0.0f || v1.texture_color.x >= 0.0f || v2.texture_color.x >= 0.0f);

    v4_byte color1 = {};
    v4_byte color2 = {};
    v4_byte color3 = {};
    if(!textured){
        color1.val = BITCAST(v0.texture_color.y, uint32);
        color2.val = BITCAST(v1.texture_color.y, uint32);
        color3.val = BITCAST(v2.texture_color.y, uint32);
    }

    const v2_f32* offsets = get_msaa_offsets(renderer->aa_details);
    const v3_f32 background_color = {
        (f32) renderer->background_color.r / 255.0f,
        (f32) renderer->background_color.g / 255.0f,
        (f32) renderer->background_color.b / 255.0f,
    };

    for (int32 y = miny; y <= maxy - 7; y++) {
        int32 x = minx;
        int32 y_width = y * renderer->dimension.width;

        for (; x <= maxx; ++x) {
            v2_f32 pos_0 = {(f32) x, (f32) y};

            v3_f32 accum_color = {0,0,0};
            f32 accum_coverage = 0.0f;

            int32 idx = y_width + x;

            for (int32 s = 0; s < renderer->aa_details; ++s) {
                f32 sx = x + offsets[s].x;
                f32 sy = y + offsets[s].y;
                v2_f32 spos = {sx, sy};

                f32 w0 = soft_edge(pos_2, pos_3, spos) * inv_area;
                f32 w1 = soft_edge(pos_3, pos_1, spos) * inv_area;
                f32 w2 = soft_edge(pos_1, pos_2, spos) * inv_area;

                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    f32 depth = w0 * v0.position.z + w1 * v1.position.z + w2 * v2.position.z;

                    if (depth < zbuf[idx]) {
                        accum_coverage += 1.0f;
                        if (textured) {
                            f32 u = w0 * v0.texture_color.x + w1 * v1.texture_color.x + w2 * v2.texture_color.x;
                            f32 v = w0 * v0.texture_color.y + w1 * v1.texture_color.y + w2 * v2.texture_color.y;
                            v4_byte tex_color = soft_sample_texture(texture, u, v);
                            accum_color.x += tex_color.x;
                            accum_color.y += tex_color.y;
                            accum_color.z += tex_color.z;
                        } else {
                            accum_color.x += (w0 * color1.r + w1 * color2.r + w2 * color3.r);
                            accum_color.y += (w0 * color1.g + w1 * color2.g + w2 * color3.g);
                            accum_color.z += (w0 * color1.b + w1 * color2.b + w2 * color3.b);
                        }
                    }
                }
            }

            if (accum_coverage > 0.0f) {
                v3_f32 final_color = vec3_mul(accum_color, inv_samples);
                f32 coverage = accum_coverage / renderer->aa_details;

                v4_byte rgba = {
                    oms_min((byte) ceil(OMS_LERP(background_color.r, final_color.r, coverage) * 255.0f), (byte) 255),
                    oms_min((byte) ceil(OMS_LERP(background_color.g, final_color.g, coverage) * 255.0f), (byte) 255),
                    oms_min((byte) ceil(OMS_LERP(background_color.b, final_color.b, coverage) * 255.0f), (byte) 255),
                    0xFF,
                };

                pixels[idx] = rgba.val;
            }
        }
    }
}

void soft_shader_default3d(
    const SoftwareRenderer* __restrict renderer,
    int32 data_index,
    int32 instance_index,
    void* __restrict data,
    int32 data_count,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    int32 steps = 8
) NO_EXCEPT {
    const SoftwareDescriptorSetLayoutBinding* camera_layout = soft_layout_find(
        renderer->descriptor_set_layout,
        ARRAY_COUNT(renderer->descriptor_set_layout),
        "camera"
    );

    ASSERT_TRUE(camera_layout);

    const ShaderCamera* camera = (ShaderCamera *) camera_layout->data;
    const v16_f32 orth = mat4_load(camera->orth);

    alignas(16) Vertex4DSamplerTextureColor v0;
    alignas(16) Vertex4DSamplerTextureColor v1;
    alignas(16) Vertex4DSamplerTextureColor v2;

    Vertex3DSamplerTextureColor* vertices = (Vertex3DSamplerTextureColor *) data;

    for (int32 i = 0; i < data_count; ++i) {
        alignas(16) v4_f32 t0 = {vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, 1.0};
        alignas(16) v4_f32 t1 = {vertices[i + 1].position.x, vertices[i + 1].position.y, vertices[i + 1].position.z, 1.0};
        alignas(16) v4_f32 t2 = {vertices[i + 2].position.x, vertices[i + 2].position.y, vertices[i + 2].position.z, 1.0};

        if (steps >= 4) {
            mat4vec4_mult_sse(&orth, &t0, &v0.position, steps);
            mat4vec4_mult_sse(&orth, &t1, &v1.position, steps);
            mat4vec4_mult_sse(&orth, &t2, &v2.position, steps);
        } else {
            mat4vec4_mult_scalar(&orth, &t0, &v0.position);
            mat4vec4_mult_scalar(&orth, &t1, &v1.position);
            mat4vec4_mult_scalar(&orth, &t2, &v2.position);
        }
        /*
        mat4vec4_mult(&orth, t0.vec, v0.position.vec, steps);
        mat4vec4_mult(&orth, t1.vec, v1.position.vec, steps);
        mat4vec4_mult(&orth, t2.vec, v2.position.vec, steps);
        */

        switch(renderer->aa_type) {
            case ANTI_ALIASING_TYPE_MSAA: {
                soft_rasterize_msaa(renderer, v0, v1, v2, NULL, steps);
            } break;
            default:
                soft_rasterize(renderer, v0, v1, v2, NULL, steps);
        }
    }

    // @todo implement index support

    (void *) &data_index_count;
    (void *) data_indices;
    (void *) instance_data;
    (void) instance_data_count;
    (void) instance_index;
    (void) data_index;
}

void soft_shader_default2d(
    const SoftwareRenderer* __restrict renderer,
    int32 data_index,
    int32 instance_index,
    void* __restrict data,
    int32 data_count,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    int32 steps = 8
) NO_EXCEPT {
    const SoftwareDescriptorSetLayoutBinding* camera_layout = soft_layout_find(
        renderer->descriptor_set_layout,
        ARRAY_COUNT(renderer->descriptor_set_layout),
        "camera"
    );

    ASSERT_TRUE(camera_layout);

    const ShaderCamera* camera = (ShaderCamera *) camera_layout->data;
    const v16_f32 orth = mat4_load(camera->orth);

    alignas(16) Vertex4DSamplerTextureColor v0;
    alignas(16) Vertex4DSamplerTextureColor v1;
    alignas(16) Vertex4DSamplerTextureColor v2;

    Vertex3DSamplerTextureColor* vertices = (Vertex3DSamplerTextureColor *) data;

    for (int32 i = 0; i < data_count; i += 3) {
        alignas(16) v4_f32 t0 = {vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, 1.0};
        alignas(16) v4_f32 t1 = {vertices[i + 1].position.x, vertices[i + 1].position.y, vertices[i + 1].position.z, 1.0};
        alignas(16) v4_f32 t2 = {vertices[i + 2].position.x, vertices[i + 2].position.y, vertices[i + 2].position.z, 1.0};

        mat4vec4_mult(camera->orth, t0.vec, v0.position.vec, steps);
        mat4vec4_mult(camera->orth, t1.vec, v1.position.vec, steps);
        mat4vec4_mult(camera->orth, t2.vec, v2.position.vec, steps);

        switch(renderer->aa_type) {
            case ANTI_ALIASING_TYPE_MSAA: {
                soft_rasterize_msaa(renderer, v0, v1, v2, NULL, steps);
            } break;
            default:
                soft_rasterize(renderer, v0, v1, v2, NULL, steps);
        }
    }

    // @todo implement index support

    (void *) &data_index_count;
    (void *) data_indices;
    (void *) instance_data;
    (void) instance_data_count;
    (void) instance_index;
    (void) data_index;
}

#if _WIN32
    #include "win32/SoftwareRenderer.cpp"
#endif

// @todo Allow to define the rasterizer (soft_rasterize, soft_rasterize_msaa, soft_rasterize_ssa)
inline
void soft_render(
    const SoftwareRenderer* __restrict renderer,
    void* __restrict data = NULL,
    int32 data_count = 0,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
    for (int32 i = 0; i < renderer->active_shader->shader_count; ++i) {
        renderer->active_shader->shader_functions[i](
            renderer,
            i, 0,
            data, data_count,
            data_indices, data_index_count,
            NULL, 0,
            steps
        );
    }
}

inline
void soft_render_instanced(
    const SoftwareRenderer* __restrict renderer,
    void* __restrict data = NULL,
    int32 data_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
    for (int32 j = 0; j < data_count; ++j) {
        for (int32 i = 0; i < renderer->active_shader->shader_count; ++i) {
            renderer->active_shader->shader_functions[i](
                renderer,
                i, j,
                data, data_count,
                data_indices, data_index_count,
                instance_data, instance_data_count,
                steps
            );
        }
    }
}


#endif