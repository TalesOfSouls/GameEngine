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

    ThreadPool* pool;

    PlatformSoftwareRenderer platform;
};

FORCE_INLINE
void soft_clear(SoftwareRenderer* renderer) NO_EXCEPT
{
    const int32 dim = renderer->dimension.width * renderer->dimension.height;
    memset(renderer->pixels, 0, sizeof(uint32) * dim);
    memset(renderer->zbuffer, 0x7E, sizeof(f32) * dim);
}

inline
void soft_clear(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    const int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    const int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        uint32* pixel_row = renderer->pixels + j * renderer->dimension.width + x;
        f32* z_row = renderer->zbuffer + j * renderer->dimension.width + x;
        memset(pixel_row, 0, sizeof(uint32) * (x1 - x));
        memset(z_row, 0x7E, sizeof(f32) * (x1 - x));
    }
}

FORCE_INLINE
void soft_clear_color(SoftwareRenderer* renderer) NO_EXCEPT
{
    memset(renderer->pixels, 0, sizeof(uint32) * renderer->dimension.width * renderer->dimension.height);
}

inline
void soft_clear_color(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    const int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    const int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        uint32* pixel_row = renderer->pixels + j * renderer->dimension.width + x;
        memset(pixel_row, 0x7E, sizeof(uint32) * (x1 - x));
    }
}

FORCE_INLINE
void soft_clear_depth(SoftwareRenderer* renderer) NO_EXCEPT
{
    memset(renderer->zbuffer, 0x7E, sizeof(f32) * renderer->dimension.width * renderer->dimension.height);
}

inline
void soft_clear_depth(SoftwareRenderer* renderer, uint32 x, uint32 y, uint32 w, uint32 h) NO_EXCEPT
{
    const int32 x1 = x + w > renderer->dimension.width  ? renderer->dimension.width  : x + w;
    const int32 y1 = y + h > renderer->dimension.height ? renderer->dimension.height : y + h;

    for (int32 j = y; j < y1; ++j) {
        f32* z_row = renderer->zbuffer + j * renderer->dimension.width + x;
        memset(z_row, 0x7E, sizeof(f32) * (x1 - x));
    }
}

static inline
v4_byte soft_sample_texture_nearest(const Texture* texture, f32 u, f32 v) NO_EXCEPT
{
    const int32 tw = texture->image.width;
    const int32 th = texture->image.height;

    ASSERT_TRUE(tw * u > 0);
    ASSERT_TRUE(th * v > 0);

    const int32 x = oms_min((int32) (u * (f32) tw), tw - 1);
    const int32 y = oms_min((int32) (v * (f32) th), th - 1);
    const byte* ptr = &texture->image.pixels[(y * tw + x) * sizeof(uint32)];

    return {
        ptr[0],
        ptr[1],
        ptr[2],
        ptr[3]
    };
}

static inline v4_uint32 soft_sample_texture_nearest_sse(
    const byte* base_pixels,
    __m128 u4, __m128 v4,
    int32 tw, int32 th,
    uint32 final_mask
) {
    // @performance (f32) tw, tw and tw - 1 should all already be loaded in simd registers
    // we are loading them here every single time, which is slow
    // we could pass xi and yi instead of u4 and v4

    // Convert u,v -> texel coordinates
    const __m128 x_f = _mm_mul_ps(u4, _mm_set1_ps((f32) tw));
    const __m128 y_f = _mm_mul_ps(v4, _mm_set1_ps((f32) th));

    const __m128i xi = _mm_min_epi32(_mm_cvttps_epi32(x_f), _mm_set1_epi32(tw - 1));
    const __m128i yi = _mm_min_epi32(_mm_cvttps_epi32(y_f), _mm_set1_epi32(th - 1));

    // byte offsets = (y*tw + x) * 4
    // @performance Could we work with floats here instead and then use fmadd?
    //      It might be faster to do that and then convert to int
    const __m128i row_off = _mm_mullo_epi32(yi, _mm_set1_epi32(tw));
    const __m128i idx = _mm_add_epi32(row_off, xi);

    const __m128i byte_offset = _mm_slli_epi32(idx, 2);

    alignas(16) int32 offs[4];
    _mm_store_si128((__m128i *) offs, byte_offset);

    v4_uint32 px = {};
    for (int32 lane = 0; lane < 4; ++lane) {
        if (!(final_mask & (1 << lane))) {
            continue;
        }

        px.vec[lane] = *((uint32*)(base_pixels + offs[lane]));
    }

    return px;
}

static inline
v4_byte soft_sample_texture_bilinear(const Texture* texture, f32 u, f32 v) NO_EXCEPT
{
    const int32 tw = texture->image.width;
    const int32 th = texture->image.height;

    ASSERT_TRUE(tw * u > 0);
    ASSERT_TRUE(th * v > 0);

    // Map u,v in [0,1] to continuous texel space 0..(N-1)
    // Using (N-1) here makes the edges sample correctly between pixels
    // (so u==0 -> texel 0, u==1 -> texel N-1).
    const f32 fx = u * (tw - 1.0f);
    const f32 fy = v * (th - 1.0f);

    int32 x0 = (int32) FLOORF(fx);
    int32 y0 = (int32) FLOORF(fy);
    const int32 x1 = oms_min(x0 + 1, tw - 1);
    const int32 y1 = oms_min(y0 + 1, th - 1);

    const f32 sx = fx - (f32) x0;
    const f32 sy = fy - (f32) y0;

    x0 = oms_min(x0, tw - 1);
    y0 = oms_min(y0, th - 1);

    const byte* p00 = &texture->image.pixels[(y0 * tw + x0) * 4];
    const byte* p10 = &texture->image.pixels[(y0 * tw + x1) * 4];
    const byte* p01 = &texture->image.pixels[(y1 * tw + x0) * 4];
    const byte* p11 = &texture->image.pixels[(y1 * tw + x1) * 4];

    // interpolate each channel in f32s then convert to u8
    const f32 r0 = OMS_LERP((f32) p00[0], (f32) p10[0], sx);
    const f32 r1 = OMS_LERP((f32) p01[0], (f32) p11[0], sx);
    const f32 r  = OMS_LERP(r0, r1, sy);

    const f32 g0 = OMS_LERP((f32) p00[1], (f32) p10[1], sx);
    const f32 g1 = OMS_LERP((f32) p01[1], (f32) p11[1], sx);
    const f32 g  = OMS_LERP(g0, g1, sy);

    const f32 b0 = OMS_LERP((f32) p00[2], (f32) p10[2], sx);
    const f32 b1 = OMS_LERP((f32) p01[2], (f32) p11[2], sx);
    const f32 b  = OMS_LERP(b0, b1, sy);

    const f32 a0 = OMS_LERP((f32) p00[3], (f32) p10[3], sx);
    const f32 a1 = OMS_LERP((f32) p01[3], (f32) p11[3], sx);
    const f32 a  = OMS_LERP(a0, a1, sy);

    return {
        (byte) oms_clamp((int32) (r + 0.5f), 0, 255),
        (byte) oms_clamp((int32) (g + 0.5f), 0, 255),
        (byte) oms_clamp((int32) (b + 0.5f), 0, 255),
        (byte) oms_clamp((int32) (a + 0.5f), 0, 255)
    };
}

static inline
v2_f32 soft_ndc_to_screen(
    f32 ndc_x, f32 ndc_y,
    v2_uint16 dimension
) NO_EXCEPT
{
    return {
        // Map from NDC (-1..1) -> [0..width-1], [0..height-1]
        (f32) ((ndc_x * 0.5f + 0.5f) * (f32) dimension.width + 0.5f),
        // Flip Y for top-down DIB memory layout
        (f32) (((1.0f - (ndc_y * 0.5f + 0.5f)) * (f32) dimension.height) + 0.5f)
    };
}

static FORCE_INLINE
f32 soft_edge(v2_f32 a, v2_f32 b, v2_f32 c) NO_EXCEPT
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

static FORCE_INLINE
v2_f32 soft_edge_coeff(v2_f32 a, v2_f32 b) NO_EXCEPT {
    return {b.y - a.y, b.x - a.x};
}

static inline
void soft_rasterize(
    const SoftwareRenderer* const __restrict renderer,
    Vertex4DSamplerTextureColor v0,
    Vertex4DSamplerTextureColor v1,
    Vertex4DSamplerTextureColor v2,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    PSEUDO_USE(steps);

    // @question Do we want to maybe change this to v2_int32?
    //          The problem with that is that we need f32s for AA?
    // Convert NDC [-1,1] -> screen (Y flipped for top-down DIB)
    /*
    const v2_f32 pos_1 = soft_ndc_to_screen(v0.position.x, v0.position.y, renderer->dimension);
    const v2_f32 pos_2 = soft_ndc_to_screen(v1.position.x, v1.position.y, renderer->dimension);
    const v2_f32 pos_3 = soft_ndc_to_screen(v2.position.x, v2.position.y, renderer->dimension);
    */

    // @todo the inversion of the y-coordinate should be part of the orth/ui matrix multiplication
    const v2_f32 pos_1 = *((v2_f32 *) v0.position.vec);
    const v2_f32 pos_2 = *((v2_f32 *) v1.position.vec);
    const v2_f32 pos_3 = *((v2_f32 *) v2.position.vec);

    const f32 area = soft_edge(pos_1, pos_2, pos_3);
    if (area <= 0.0f) {
        // == 0 is degenerate triangle
        // < 0 is backface -> backface culling
        return;
    }

    const f32 inv_area = 1.0f / area;
    const v2_f32 e0 = soft_edge_coeff(pos_2, pos_3);
    const v2_f32 e1 = soft_edge_coeff(pos_3, pos_1);
    const v2_f32 e2 = soft_edge_coeff(pos_1, pos_2);

    #ifdef __SSE4_2__
        const __m128 invA_v = _mm_set1_ps(inv_area);

        const __m128 e0x_v = _mm_set1_ps(e0.x);
        const __m128 e0y_v = _mm_set1_ps(e0.y);
        const __m128 e0x_ref_v = _mm_set1_ps(e0.x * pos_2.x);
        const __m128 e0y_ref_v = _mm_set1_ps(e0.y * pos_2.y);

        const __m128 e1x_v = _mm_set1_ps(e1.x);
        const __m128 e1y_v = _mm_set1_ps(e1.y);
        const __m128 e1x_ref_v = _mm_set1_ps(e1.x * pos_3.x);
        const __m128 e1y_ref_v = _mm_set1_ps(e1.y * pos_3.y);

        const __m128 e2x_v = _mm_set1_ps(e2.x);
        const __m128 e2y_v = _mm_set1_ps(e2.y);
        const __m128 e2x_ref_v = _mm_set1_ps(e2.x * pos_1.x);
        const __m128 e2y_ref_v = _mm_set1_ps(e2.y * pos_1.y);

        // vertex z
        const __m128 z0_v = _mm_set1_ps(v0.position.z);
        const __m128 z1_v = _mm_set1_ps(v1.position.z);
        const __m128 z2_v = _mm_set1_ps(v2.position.z);
    #endif

    const int32 minx = oms_max(0, oms_min(oms_min((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    const int32 maxx = oms_min(renderer->dimension.width - 1, oms_max(oms_max((int32) pos_1.x, (int32) pos_2.x), (int32) pos_3.x));
    const int32 miny = oms_max(0, oms_min(oms_min((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));
    const int32 maxy = oms_min(renderer->dimension.height - 1, oms_max(oms_max((int32) pos_1.y, (int32) pos_2.y), (int32) pos_3.y));

    uint32* pixels = renderer->pixels;
    f32* zbuf = renderer->zbuffer;

    // Check if the triangle uses texture or solid color
    const bool textured = (v0.texture_color.x >= 0.0f || v1.texture_color.x >= 0.0f || v2.texture_color.x >= 0.0f);

    v4_byte color1 = {};
    v4_byte color2 = {};
    v4_byte color3 = {};

    #ifdef __SSE4_2__
        __m128 u0_v = {0}, u1_v = {0}, u2_v = {0}, v0_v = {0}, v1_v = {0}, v2_v = {0};
        __m128 c1r_v = {0}, c1g_v = {0}, c1b_v = {0}, c1a_v = {0};
        __m128 c2r_v = {0}, c2g_v = {0}, c2b_v = {0}, c2a_v = {0};
        __m128 c3r_v = {0}, c3g_v = {0}, c3b_v = {0}, c3a_v = {0};
        __m128 zero_v = _mm_setzero_ps();
    #endif

    if(!textured) {
        color1.val = BITCAST(v0.texture_color.y, uint32);
        color2.val = BITCAST(v1.texture_color.y, uint32);
        color3.val = BITCAST(v2.texture_color.y, uint32);

        #ifdef __SSE4_2__
            c1r_v = _mm_set1_ps((f32) color1.r);
            c1g_v = _mm_set1_ps((f32) color1.g);
            c1b_v = _mm_set1_ps((f32) color1.b);
            c1a_v = _mm_set1_ps((f32) color1.a);

            c2r_v = _mm_set1_ps((f32) color2.r);
            c2g_v = _mm_set1_ps((f32) color2.g);
            c2b_v = _mm_set1_ps((f32) color2.b);
            c2a_v = _mm_set1_ps((f32) color2.a);

            c3r_v = _mm_set1_ps((f32) color3.r);
            c3g_v = _mm_set1_ps((f32) color3.g);
            c3b_v = _mm_set1_ps((f32) color3.b);
            c3a_v = _mm_set1_ps((f32) color3.a);
        #endif
    }
    #ifdef __SSE4_2__
    else {
        // Pre-broadcast texture u/v per-vertex (for textured path)
        u0_v = _mm_set1_ps(v0.texture_color.x);
        u1_v = _mm_set1_ps(v1.texture_color.x);
        u2_v = _mm_set1_ps(v2.texture_color.x);

        v0_v = _mm_set1_ps(v0.texture_color.y);
        v1_v = _mm_set1_ps(v1.texture_color.y);
        v2_v = _mm_set1_ps(v2.texture_color.y);
     }
    #endif

    for (int32 y = miny; y <= maxy; ++y) {
        int32 x = minx;
        const int32 y_width = y * renderer->dimension.width;
        v2_f32 pos_0 = {(f32) x, (f32) y};

        #ifdef __SSE4_2__
            if (steps >= 4) {
                const __m128 fy_v = _mm_set1_ps((f32) y);

                for (; x <= maxx - 3; x += 4) {
                    const __m128 cx_v = _mm_set_ps((f32) (x + 0), (f32) (x + 1), (f32) (x + 2), (f32) (x + 3));

                    __m128 w0 = _mm_sub_ps(_mm_mul_ps(e0x_v, cx_v), _mm_mul_ps(e0y_v, _mm_sub_ps(fy_v, _mm_set1_ps(pos_2.y))));
                    w0 = _mm_sub_ps(w0, e0x_ref_v);
                    w0 = _mm_mul_ps(w0, invA_v);

                    __m128 w1 = _mm_sub_ps(_mm_mul_ps(e1x_v, cx_v), _mm_mul_ps(e1y_v, _mm_sub_ps(fy_v, _mm_set1_ps(pos_3.y))));
                    w1 = _mm_sub_ps(w1, e1x_ref_v);
                    w1 = _mm_mul_ps(w1, invA_v);

                    __m128 w2 = _mm_sub_ps(_mm_mul_ps(e2x_v, cx_v), _mm_mul_ps(e2y_v, _mm_sub_ps(fy_v, _mm_set1_ps(pos_1.y))));
                    w2 = _mm_sub_ps(w2, e2x_ref_v);
                    w2 = _mm_mul_ps(w2, invA_v);

                    const __m128 m0 = _mm_cmpge_ps(w0, zero_v);
                    const __m128 m1 = _mm_cmpge_ps(w1, zero_v);
                    const __m128 m2 = _mm_cmpge_ps(w2, zero_v);
                    const __m128 mask = _mm_and_ps(_mm_and_ps(m0, m1), m2);

                    const int32 cov_mask = _mm_movemask_ps(mask);
                    if (!cov_mask) {
                        continue;
                    }

                    // depth interpolated: z = w0*z0 + w1*z1 + w2*z2
                    const __m128 z = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, z0_v), _mm_mul_ps(w1, z1_v)), _mm_mul_ps(w2, z2_v));

                    const int32 idx = y_width + x;
                    // @performance It would be nice if we could do aligned load but the width could be uneven
                    //      Maybe we force a padded zbuffer width to avoid that?
                    const __m128 zbuf_v = _mm_loadu_ps(&zbuf[idx]);

                    // depth test: z < zbuf
                    const __m128 depth_mask_v = _mm_cmplt_ps(z, zbuf_v);

                    // final mask = cov & depth_mask
                    const __m128 cov = _mm_and_ps(_mm_and_ps(m0, m1), m2);
                    const __m128 final_mask_v = _mm_and_ps(cov, depth_mask_v);
                    const int32 final_mask = _mm_movemask_ps(final_mask_v);
                    if (!final_mask) {
                        continue;
                    }

                    // update zbuf where final_mask true
                    // pick z where mask set
                    const __m128 new_zbuf_v = _mm_blendv_ps(zbuf_v, z, final_mask_v);
                    _mm_storeu_ps(&zbuf[idx], new_zbuf_v);

                    if (textured) {
                        // textured: compute u,v per lane then scalar-sample and write
                        // compute u = w0 * u0 + w1 * u1 + w2 * u2  (texture_color.x)
                        const __m128 uu = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, u0_v), _mm_mul_ps(w1, u1_v)), _mm_mul_ps(w2, u2_v));
                        const __m128 vv = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, v0_v), _mm_mul_ps(w1, v1_v)), _mm_mul_ps(w2, v2_v));

                        v4_uint32 rgba = soft_sample_texture_nearest_sse(
                            renderer->textures[v0.sampler]->image.pixels,
                            uu, vv,
                            renderer->textures[v0.sampler]->image.width, renderer->textures[v0.sampler]->image.height,
                            final_mask
                        );

                        for (int32 lane = 0; lane < 4; ++lane) {
                            const uint32 coverage_mask = (final_mask & (1 << lane)) ? 0xFFFFFFFF : 0x0;

                            // Extract source channels from rgba.vec[lane]
                            const uint32 src = rgba.vec[lane];
                            const uint32 src_r = (src >> 24) & 0xFF;
                            const uint32 src_g = (src >> 16) & 0xFF;
                            const uint32 src_b = (src >> 8) & 0xFF;
                            const uint32 src_a = src & 0xFF;

                            const uint32 dst = pixels[idx + lane];
                            const uint32 dst_r = (dst >> 24) & 0xFF;
                            const uint32 dst_g = (dst >> 16) & 0xFF;
                            const uint32 dst_b = (dst >> 8) & 0xFF;
                            const uint32 dst_a = dst & 0xFF;

                            // True alpha blending
                            const uint32 out_r = (src_r * src_a + dst_r * (255 - src_a)) / 255;
                            const uint32 out_g = (src_g * src_a + dst_g * (255 - src_a)) / 255;
                            const uint32 out_b = (src_b * src_a + dst_b * (255 - src_a)) / 255;
                            const uint32 out_a = (src_a * src_a + dst_a * (255 - src_a)) / 255;

                            const uint32 new_pixel = (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;

                            pixels[idx + lane] = (pixels[idx + lane] & ~coverage_mask) | (new_pixel & coverage_mask);
                        }
                    } else {
                        // Interpolate color components
                        const __m128 rf = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, c1r_v), _mm_mul_ps(w1, c2r_v)), _mm_mul_ps(w2, c3r_v));
                        const __m128 gf = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, c1g_v), _mm_mul_ps(w1, c2g_v)), _mm_mul_ps(w2, c3g_v));
                        const __m128 bf = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, c1b_v), _mm_mul_ps(w1, c2b_v)), _mm_mul_ps(w2, c3b_v));
                        const __m128 af = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, c1a_v), _mm_mul_ps(w1, c2a_v)), _mm_mul_ps(w2, c3a_v));

                        alignas(16) f32 rf_arr[4], gf_arr[4], bf_arr[4], af_arr[4];
                        _mm_store_ps(rf_arr, rf);
                        _mm_store_ps(gf_arr, gf);
                        _mm_store_ps(bf_arr, bf);
                        _mm_store_ps(af_arr, af);

                        for (int32 lane = 0; lane < 4; ++lane) {
                            const uint32 coverage_mask = (final_mask & (1 << lane)) ? 0xFFFFFFFF : 0x0;

                            const uint32 r = (uint32) (rf_arr[lane] + 0.5f) & 0xFF;
                            const uint32 g = (uint32) (gf_arr[lane] + 0.5f) & 0xFF;
                            const uint32 b = (uint32) (bf_arr[lane] + 0.5f) & 0xFF;
                            const uint32 a = (uint32) (af_arr[lane] + 0.5f) & 0xFF;

                            const uint32 dst = pixels[idx + lane];

                            // Extract destination channels
                            const uint32 dst_r = (dst >> 24) & 0xFF;
                            const uint32 dst_g = (dst >> 16) & 0xFF;
                            const uint32 dst_b = (dst >> 8)  & 0xFF;
                            const uint32 dst_a = dst & 0xFF;

                            // True alpha blending per channel
                            const uint32 out_r = (r * a + dst_r * (255 - a)) / 255;
                            const uint32 out_g = (g * a + dst_g * (255 - a)) / 255;
                            const uint32 out_b = (b * a + dst_b * (255 - a)) / 255;
                            const uint32 out_a = (a * a + dst_a * (255 - a)) / 255;

                            const uint32 new_pixel = (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;

                            pixels[idx + lane] = (pixels[idx + lane] & ~coverage_mask) | (new_pixel & coverage_mask);
                        }
                    }
                }
            }
        #endif

        for (; x <= maxx; ++x) {
            /*
            pos_0.x = (f32) x;
            f32 w0 = soft_edge(pos_2, pos_3, pos_0) * inv_area;
            f32 w1 = soft_edge(pos_3, pos_1, pos_0) * inv_area;
            f32 w2 = soft_edge(pos_1, pos_2, pos_0) * inv_area;
            */

            // soft_edge but uses cached coefficients for faster calculation
            const f32 w0 = (e0.x * (x - pos_2.x) - e0.y * (y - pos_2.y)) * inv_area;
            const f32 w1 = (e1.x * (x - pos_3.x) - e1.y * (y - pos_3.y)) * inv_area;
            const f32 w2 = (e2.x * (x - pos_1.x) - e2.y * (y - pos_1.y)) * inv_area;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f || (w0 + w1 + w2) <= 0.0f) {
                continue;
            }

            const f32 depth = w0 * v0.position.z
                + w1 * v1.position.z
                + w2 * v2.position.z;

            const int32 idx = y_width + x;

            if (depth >= zbuf[idx]) {
                continue;
            }

            zbuf[idx] = depth;

            if (textured) {
                // Interpolate UVs
                const f32 u = w0 * v0.texture_color.x
                    + w1 * v1.texture_color.x
                    + w2 * v2.texture_color.x;

                const f32 v = w0 * v0.texture_color.y
                    + w1 * v1.texture_color.y
                    + w2 * v2.texture_color.y;

                // @bug how to handle different textures for v0, v1, v2
                const v4_byte color = soft_sample_texture_nearest(renderer->textures[v0.sampler], u, v);

                const uint32 dst = pixels[idx];

                // Extract destination channels
                const uint32 dst_r = (dst >> 24) & 0xFF;
                const uint32 dst_g = (dst >> 16) & 0xFF;
                const uint32 dst_b = (dst >> 8)  & 0xFF;
                const uint32 dst_a = dst & 0xFF;

                // Branchless alpha blending
                const uint32 out_r = (color.r * color.a + dst_r * (255 - color.a)) / 255;
                const uint32 out_g = (color.g * color.a + dst_g * (255 - color.a)) / 255;
                const uint32 out_b = (color.b * color.a + dst_b * (255 - color.a)) / 255;
                const uint32 out_a = (color.a * color.a + dst_a * (255 - color.a)) / 255;

                pixels[idx] = (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;
            } else {
                // Interpolate color components
                const f32 rf = (w0 * color1.r + w1 * color2.r + w2 * color3.r);
                const f32 gf = (w0 * color1.g + w1 * color2.g + w2 * color3.g);
                const f32 bf = (w0 * color1.b + w1 * color2.b + w2 * color3.b);
                const f32 af = (w0 * color1.a + w1 * color2.a + w2 * color3.a);

                const uint32 r = ((uint32)(rf + 0.5f)) & 0xFF;
                const uint32 g = ((uint32)(gf + 0.5f)) & 0xFF;
                const uint32 b = ((uint32)(bf + 0.5f)) & 0xFF;
                const uint32 a = ((uint32)(af + 0.5f)) & 0xFF;

                const uint32 dst = pixels[idx];

                // Extract destination channels
                const uint32 dst_r = (dst >> 24) & 0xFF;
                const uint32 dst_g = (dst >> 16) & 0xFF;
                const uint32 dst_b = (dst >> 8)  & 0xFF;
                const uint32 dst_a = dst & 0xFF;

                // Blend channels with source alpha
                const uint32 out_r = (r * a + dst_r * (255 - a)) / 255;
                const uint32 out_g = (g * a + dst_g * (255 - a)) / 255;
                const uint32 out_b = (b * a + dst_b * (255 - a)) / 255;
                const uint32 out_a = (a * a + dst_a * (255 - a)) / 255;

                pixels[idx] = (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;
            }
        }
    }
}

// This version is not a true msaa
// We would need a sample buffer for full msaa implementation
static
void soft_rasterize_msaa(
    const SoftwareRenderer* const __restrict renderer,
    Vertex4DSamplerTextureColor v0,
    Vertex4DSamplerTextureColor v1,
    Vertex4DSamplerTextureColor v2,
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
                            v4_byte tex_color = soft_sample_texture_nearest(renderer->textures[v0.sampler], u, v);
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
    const SoftwareRenderer* const __restrict renderer,
    int32 data_index,
    int32 instance_index,
    void* __restrict data,
    int32 data_count,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
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
                soft_rasterize_msaa(renderer, v0, v1, v2, steps);
            } break;
            default:
                soft_rasterize(renderer, v0, v1, v2, steps);
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

void soft_shader_ui(
    const SoftwareRenderer* const __restrict renderer,
    int32 data_index,
    int32 instance_index,
    void* __restrict data,
    int32 data_count,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
    /*
    const SoftwareDescriptorSetLayoutBinding* camera_layout = soft_layout_find(
        renderer->descriptor_set_layout,
        ARRAY_COUNT(renderer->descriptor_set_layout),
        "camera"
    );

    ASSERT_TRUE(camera_layout);

    const ShaderCamera* camera = (ShaderCamera *) camera_layout->data;
    const v16_f32 orth = mat4_load(camera->orth, steps);
    */

    //alignas(16) Vertex4DSamplerTextureColor v0;
    //alignas(16) Vertex4DSamplerTextureColor v1;
    //alignas(16) Vertex4DSamplerTextureColor v2;

    const Vertex3DSamplerTextureColor* vertices = (const Vertex3DSamplerTextureColor *) data;

    // alignas(16) v4_f32 t0;
    // alignas(16) v4_f32 t1;
    // alignas(16) v4_f32 t2;

    // @performance Put into the worker threads but of course not per 3 vertices but in chunks
    for (int32 i = 0; i < data_count; i += 3) {
        //t0 = {vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, 0.0f};
        //t1 = {vertices[i + 1].position.x, vertices[i + 1].position.y, vertices[i + 1].position.z, 0.0f};
        //t2 = {vertices[i + 2].position.x, vertices[i + 2].position.y, vertices[i + 2].position.z, 0.0f};

        alignas(16) const Vertex4DSamplerTextureColor v0 = {
            {vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, 0.0f},
            2,
            vertices[i].texture_color
        };
        alignas(16) const Vertex4DSamplerTextureColor v1 = {
            {vertices[i + 1].position.x, vertices[i + 1].position.y, vertices[i + 1].position.z, 0.0f},
            2,
            vertices[i + 1].texture_color
        };
        alignas(16) const Vertex4DSamplerTextureColor v2 = {
            {vertices[i + 2].position.x, vertices[i + 2].position.y, vertices[i + 2].position.z, 0.0f},
            2,
            vertices[i + 2].texture_color
        };

        // mat4vec4_mult(&orth, t0.vec, v0.position.vec, steps);
        // mat4vec4_mult(&orth, t1.vec, v1.position.vec, steps);
        // mat4vec4_mult(&orth, t2.vec, v2.position.vec, steps);

        switch(renderer->aa_type) {
            case ANTI_ALIASING_TYPE_MSAA: {
                soft_rasterize_msaa(renderer, v0, v1, v2, steps);
            } break;
            default:
                soft_rasterize(renderer, v0, v1, v2, steps);
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

struct ThrdSoftwareShaderArg {
    const SoftwareRenderer* renderer;
    void* data;
    int32 data_count;
    const uint32* data_indices;
    int32 data_index_count;
    int32 steps;
    SoftShaderFunc func;
};

static inline
void thrd_soft_shader(void* arg)
{
    PoolWorker* job = (PoolWorker *) arg;
    ThrdSoftwareShaderArg* shader = (ThrdSoftwareShaderArg *) job->arg;
    shader->func(
        shader->renderer,
        0, 0,
        shader->data, shader->data_count,
        shader->data_indices, shader->data_index_count,
        NULL, 0,
        shader->steps
    );
}

// @todo Allow to define the rasterizer (soft_rasterize, soft_rasterize_msaa, soft_rasterize_ssa)
// WARNING: data_size is not the individual vertex size, but vertex size * vertex count that creates one triangle
inline
void soft_render(
    const SoftwareRenderer* const __restrict renderer,
    void* __restrict data = NULL,
    int32 data_count = 0,
    int32 data_size = 0, // @question Consider to split into size and stride
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
    if (renderer->pool) {
        const int32 MAX_CHUNKS = 4;
        ThrdSoftwareShaderArg args[MAX_CHUNKS];
        PoolWorker* jobs[MAX_CHUNKS];

        const int32 data_chunks = data_count < MAX_CHUNKS ? data_count : MAX_CHUNKS;
        const int32 index_chunks = data_index_count < MAX_CHUNKS ? data_index_count : MAX_CHUNKS;

        for (int32 i = 0; i < renderer->active_shader->shader_count; ++i) {
            for (int32 j = 0; j < MAX_CHUNKS; ++j) {
                int32 data_start = 0;
                int32 data_chunk_size = 0;
                if (data_count > 0 && j < data_chunks) {
                    data_start = j * data_count / (3 * data_chunks); // @todo replace 3 with stride
                    int32 data_end = oms_min((j + 1) * data_count / (3 * data_chunks), data_count / 3); // @todo replace 3 with stride
                    data_chunk_size = (data_end - data_start) * 3; // @todo replace 3 with stride
                }

                int32 index_start = 0;
                int32 index_chunk_size = 0;
                if (data_index_count > 0 && j < index_chunks) {
                    index_start = j * data_index_count / index_chunks;
                    int32 index_end = (j + 1) * data_index_count / index_chunks;
                    index_chunk_size = index_end - index_start;
                }

                // @bug if indices exist we must alway send the entire data array
                args[j] = {
                    renderer,
                    data ? (void*)((uint8 *)data + data_start * data_size) : NULL, // @todo replace 3 with stride
                    data_chunk_size,
                    data_indices ? data_indices + index_start : NULL,
                    index_chunk_size,
                    steps,
                    renderer->active_shader->shader_functions[i]
                };

                PoolWorker job = {
                    0,
                    POOL_WORKER_STATE_WAITING,
                    thrd_soft_shader,
                    NULL,
                    &args[j]
                };

                jobs[j] = thread_pool_add_work(renderer->pool, &job);
            }

            thread_pool_join(jobs, MAX_CHUNKS);
        }
    } else {
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
}

inline
void soft_render_instanced(
    const SoftwareRenderer* const __restrict renderer,
    void* __restrict data = NULL,
    int32 data_count = 0,
    void* __restrict instance_data = NULL,
    int32 instance_data_count = 0,
    const uint32* __restrict data_indices = NULL,
    int32 data_index_count = 0,
    int32 steps = 8
) NO_EXCEPT
{
    for (int32 i = 0; i < renderer->active_shader->shader_count; ++i) {
        for (int32 j = 0; j < data_count; ++j) {
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