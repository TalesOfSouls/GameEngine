/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_RENDER_UTILS_H
#define COMS_GPUAPI_RENDER_UTILS_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../font/Font.cpp"
#include "../font/FontSystem.h"
#include "../memory/PointerMemory.h"
#include "../stdlib/ArrayVector.h"
#include "../object/Vertex.h"
#include "../ui/UIAlignment.h"

FORCE_INLINE
void vertex_degenerate_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const vertices, f32 zindex,
    f32 x, f32 y
) NO_EXCEPT
{
    // Degenerate triangles
    // They are alternating every loop BUT since we use references they look the same in code
    // WARNING: Before using we must make sure that the 0 index is defined
    //          The easiest way is to just define a "degenerate" starting point
    array_vector_reserve_one(vertices) = {{vertices->elements[vertices->count - 1].position.x, vertices->elements[vertices->count - 1].position.y, zindex}, -1, {}, 0};
    array_vector_reserve_one(vertices) = {{x, y, zindex}, -1, {}, 0};
}

static inline
void adjust_aligned_position(
    f32* const __restrict x, f32* const __restrict y,
    f32 width, f32 height,
    byte align_anchor
) NO_EXCEPT
{
    if (align_anchor & UI_ALIGN_H_RIGHT) {
        *x -= width;
    } else if (align_anchor & UI_ALIGN_H_CENTER) {
        *x -= width * 0.5f;
    }

    if (align_anchor & UI_ALIGN_V_TOP) {
        *y -= height;
    } else if (align_anchor & UI_ALIGN_V_CENTER) {
        *y -= height * 0.5f;
    }
}

static FORCE_INLINE
void adjust_aligned_position(
    v4_f32* const vec,
    byte align_anchor
) NO_EXCEPT
{
    if (align_anchor & UI_ALIGN_H_RIGHT) {
        vec->x -= vec->width;
    } else if (align_anchor & UI_ALIGN_H_CENTER) {
        vec->x -= vec->width * 0.5f;
    }

    if (align_anchor & UI_ALIGN_V_TOP) {
        vec->y -= vec->height;
    } else if (align_anchor & UI_ALIGN_V_CENTER) {
        vec->y -= vec->height * 0.5f;
    }
}

inline
void vertex_line_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices,
    ArrayVector<int32>* const __restrict indices, f32 zindex,
    v2_f32 start, v2_f32 end, f32 thickness,
    uint32 rgba = 0
) NO_EXCEPT
{
    f32 dx = end.x - start.x;
    f32 dy = end.y - start.y;

    // Normalize direction
    const f32 len = intrin_rsqrt_f32(dx * dx + dy * dy);
    dx *= len;
    dy *= len;

    thickness *= 0.5f;

    // Perpendicular vector (normalized) and then scaled by half-thickness
    const f32 hx = -dy * thickness;
    const f32 hy = dx * thickness;

    // Four corners of the line quad
    const v2_f32 v0 = { start.x - hx, start.y - hy };
    const v2_f32 v1 = { start.x + hx, start.y + hy };
    const v2_f32 v2 = { end.x   - hx, end.y   - hy };
    const v2_f32 v3 = { end.x   + hx, end.y   + hy };

    int32* const __restrict i = array_vector_reserve(indices, 6);
    const int32 vertex_count = vertices->count;
    i[0] = vertex_count;
    i[1] = vertex_count + 1;
    i[2] = vertex_count + 3;

    i[3] = vertex_count + 1;
    i[4] = vertex_count + 2;
    i[5] = vertex_count + 3;

    // Solid color (sampler <= 0)
    Vertex3DSamplerTextureColor* const __restrict v = array_vector_reserve(vertices, 4);
    v[0] = {{v1.x, v1.y, zindex}, -1, {}, rgba}; // tl
    v[1] = {{v3.x, v3.y, zindex}, -1, {}, rgba}; // tr
    v[2] = {{v2.x, v2.y, zindex}, -1, {}, rgba}; // br
    v[3] = {{v0.x, v0.y, zindex}, -1, {}, rgba}; // bl
}

inline HOT_CODE
void vertex_rect_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices,
    ArrayVector<int32>* const __restrict indices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte align_anchor,
    uint32 rgba = 0, v2_f32 tex1 = {}, v2_f32 tex2 = {}
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_RECT_CREATE);
    if (align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, align_anchor);
    }

    // A color always means a solid color rect (sampler <= 0), otherwise the texture of the sampler is used
    if (rgba) {
        sampler = 0;
        tex1 = {};
        tex2 = {};
    }

    const f32 y_height = dimension.y + dimension.height;
    const f32 x_width = dimension.x + dimension.width;

    int32* const __restrict i = array_vector_reserve(indices, 6);
    const int32 vertex_count = vertices->count;
    i[0] = vertex_count;
    i[1] = vertex_count + 1;
    i[2] = vertex_count + 3;

    i[3] = vertex_count + 1;
    i[4] = vertex_count + 2;
    i[5] = vertex_count + 3;

    Vertex3DSamplerTextureColor* const __restrict v = array_vector_reserve(vertices, 4);
    v[0] = {{dimension.x, y_height, zindex}, sampler, {tex1.x, tex2.y}, rgba}; // tl
    v[1] = {{x_width, y_height, zindex}, sampler, tex2, rgba}; // tr
    v[2] = {{x_width, dimension.y, zindex}, sampler, {tex2.x, tex1.y}, rgba}; // br
    v[3] = {{dimension.x, dimension.y, zindex}, sampler, tex1, rgba}; // bl
}

inline
void vertex_circle_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte align_anchor,
    int32 segments, // defines the detail (should be a multiple of 4)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, align_anchor);
    }

    // A color always means a solid color (sampler <= 0), otherwise the texture of the sampler is used
    if (rgba) {
        sampler = 0;
        tex_center = {};
        tex_edge = {};
    }

    // Circle center + radii
    const f32 cx = dimension.x + dimension.width * 0.5f;
    const f32 cy = dimension.y + dimension.height * 0.5f;
    const f32 rx = dimension.width * 0.5f;
    const f32 ry = dimension.height * 0.5f;

    // Generate a triangle fan: center + pairs of edge vertices

    const int32 center_index = vertices->count;
    array_vector_reserve_one(vertices) = {{cx, cy, zindex}, sampler, tex_center, rgba};

    // @performance For sure this is vectorizable (SIMD)
    for (int i = 0; i < segments; ++i) {
        const f32 angle0 = (OMS_TWO_PI_F32 * i) / segments;
        const f32 angle1 = (OMS_TWO_PI_F32 * (i + 1)) / segments;

        f32 s, c;

        SINCOSF(angle0, s, c);
        const f32 x0 = cx + c * rx;
        const f32 y0 = cy + s * ry;

        SINCOSF(angle1, s, c);
        const f32 x1 = cx + c * rx;
        const f32 y1 = cy + s * ry;

        // WARNING: Clockwise order (center -> next -> current) like all other primitives (e.g. vertex_rect_create)
        //          The renderer uses glFrontFace(GL_CW) + back face culling -> counter clockwise triangles are invisible
        array_vector_reserve_one(indices) = center_index;
        array_vector_reserve_one(indices) = vertices->count + 1;
        array_vector_reserve_one(indices) = vertices->count;

        // @performance Isn't one of the points on the circle arcs a duplicate same as the center
        //              We fixed it for the center but not for the previous circle arc point
        array_vector_reserve_one(vertices) = {{x0, y0, zindex}, sampler, tex_edge, rgba};
        array_vector_reserve_one(vertices) = {{x1, y1, zindex}, sampler, tex_edge, rgba};
    }
}

inline
void vertex_arc_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte align_anchor,
    int32 segments,  // number of subdivisions (should be >= 3)
    f32 start_angle, // radians where the arc starts
    f32 arc_angle,   // radians how wide the arc is (e.g. OMS_PI_F32 for semicircle)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, align_anchor);
    }

    // A color always means a solid color (sampler <= 0), otherwise the texture of the sampler is used
    if (rgba) {
        sampler = 0;
        tex_center = {};
        tex_edge = {};
    }

    // Circle center + radii
    const f32 cx = dimension.x + dimension.width * 0.5f;
    const f32 cy = dimension.y + dimension.height * 0.5f;
    const f32 rx = dimension.width * 0.5f;
    const f32 ry = dimension.height * 0.5f;

    const int32 center_index = vertices->count;
    array_vector_reserve_one(vertices) = {{cx, cy, zindex}, sampler, tex_center, rgba};

    // Generate a triangle fan over the arc
    for (int32 i = 0; i < segments; ++i) {
        const f32 angle0 = start_angle + (arc_angle * (f32) i / segments);
        const f32 angle1 = start_angle + (arc_angle * (f32) (i + 1) / segments);

        f32 s, c;

        SINCOSF(angle0, s, c);
        const f32 x0 = cx + c * rx;
        const f32 y0 = cy + s * ry;

        SINCOSF(angle1, s, c);
        const f32 x1 = cx + c * rx;
        const f32 y1 = cy + s * ry;

        // WARNING: Clockwise order (center -> next -> current) like all other primitives (e.g. vertex_rect_create)
        //          The renderer uses glFrontFace(GL_CW) + back face culling -> counter clockwise triangles are invisible
        array_vector_reserve_one(indices) = center_index;
        array_vector_reserve_one(indices) = vertices->count + 1;
        array_vector_reserve_one(indices) = vertices->count;

        // @performance Isn't one of the points on the circle arcs a duplicate same as the center
        //              We fixed it for the center but not for the previous circle arc point
        array_vector_reserve_one(vertices) = {{x0, y0, zindex}, sampler, tex_edge, rgba};
        array_vector_reserve_one(vertices) = {{x1, y1, zindex}, sampler, tex_edge, rgba};
    }
}

static
v2_f32 text_calculate_dimensions(
    const FontSystem* const __restrict font, const int16* const glyphs, int32 length, f32 scale
) NO_EXCEPT
{
    const f32 line_height = font->base.line_height * scale;
    f32 x = 0;
    f32 y = line_height;
    f32 offset_x = 0;

    const Glyph* const __restrict tables[] = { font->base.glyphs, font->extended.glyphs };

    for (int i = 0; i < length; ++i) {
        const uint16 g = glyphs[i];
        if (g == font->newline_id) {
            // @performance calculating a new x after a line break seems wrong
            x = max_branched(x, offset_x);
            y += line_height;

            offset_x = 0;

            continue;
        }

        const Glyph* const __restrict glyph = &tables[g >> 15][g & 0x7FFF];

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return { max_branched(x, offset_x), y };
}

static
f32 text_calculate_dimensions_width(
    const FontSystem* const __restrict font, const int16* const glyphs, int32 length, f32 scale
) NO_EXCEPT
{
    f32 x = 0;
    f32 offset_x = 0;

    const Glyph* const __restrict tables[] = { font->base.glyphs, font->extended.glyphs };

    for (int i = 0; i < length; ++i) {
        const uint16 g = glyphs[i];
        if (g == font->newline_id) {
            // @performance calculating a new x after a new line feels wrong
            x = max_branched(x, offset_x);
            offset_x = 0;

            continue;
        }

        const Glyph* const __restrict glyph = &tables[g >> 15][g & 0x7FFF];

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return max_branched(x, offset_x);
}

static
f32 text_calculate_dimensions_height(
    const FontSystem* const __restrict font, const int16* const glyphs, int32 length, f32 line_height_scaled
) NO_EXCEPT
{
    f32 y = line_height_scaled;

    // Now we just count how often that newline character exists
    for (int i = 0; i < length; ++i) {
        if (glyphs[i] == font->newline_id) {
            y += line_height_scaled;
        }
    }

    return y;
}

HOT_CODE
v2_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex,
    v4_f32 dimension, byte align_anchor,
    FontSystem* const __restrict font, const int16* const __restrict glyphs, int32 length,
    f32 size, uint32 rgba
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);

    // Text color, 0 = default (white)
    const uint32 text_color = rgba ? rgba : 0xFFFFFFFF;

    const Font* const __restrict font_base = &font->base;
    const f32 scale = size / font_base->size;
    const f32 line_height_scaled = font_base->line_height * scale;

    // The code below (if it is executed) results in an additional iteration over the glyphs array.
    // We tried to pull this out into the previous function call where we are generating the glyphs array.
    // We thought that this would improve the performance, because this way we could avoid one iteration over glyphs.
    // However, as it turns out this would only improve the performance in debug builds
    // BUT make it slower in release builds.
    if (align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        if ((align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER))
            && (align_anchor & (UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER))
        ) {
            const v2_f32 dim = text_calculate_dimensions(font, glyphs, length, scale);
            dimension.width = dim.width;
            dimension.height = dim.height;
        } else if (align_anchor & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER)) {
            dimension.width = text_calculate_dimensions_width(font, glyphs, length, scale);
        } else {
            dimension.height = text_calculate_dimensions_height(font, glyphs, length, line_height_scaled);
        }

        adjust_aligned_position(&dimension, align_anchor);
    }

    f32 rendered_width = 0;
    f32 rendered_height = line_height_scaled;

    f32 offset_x = dimension.x;
    const Glyph* const __restrict tables[] = { font->base.glyphs, font->extended.glyphs };

    for (int32 i = 0; i < length; ++i) {
        const uint16 g = glyphs[i];
        if (g == font->newline_id) {
            rendered_height += line_height_scaled;
            rendered_width = max_branched(rendered_width, offset_x - dimension.x);

            dimension.y -= line_height_scaled;
            offset_x = dimension.x;

            continue;
        }

        const Glyph* const __restrict glyph = &tables[g >> 15][g & 0x7FFF];

        const GlyphMetrics* const __restrict metrics = &glyph->metrics;
        const f32 offset_y = dimension.y + metrics->offset_y * scale;
        offset_x += metrics->offset_x * scale;
        rendered_width += metrics->offset_x * scale;

        if (glyph->codepoint != ' ' && glyph->codepoint != '\t') {
            const f32 x_end_scaled = offset_x + metrics->width * scale;
            const f32 y_end_scaled = offset_y + metrics->height * scale;

            int32* const idx = array_vector_reserve(indices, 6);
            const int32 vertex_count = vertices->count;
            idx[0] = vertex_count;
            idx[1] = vertex_count + 1;
            idx[2] = vertex_count + 3;

            idx[3] = vertex_count + 1;
            idx[4] = vertex_count + 2;
            idx[5] = vertex_count + 3;

            Vertex3DSamplerTextureColor* const __restrict v = array_vector_reserve(vertices, 4);
            v[0] = {{offset_x, y_end_scaled, zindex}, 1, glyph->uv_start, text_color}; // tl
            v[1] = {{x_end_scaled, y_end_scaled, zindex}, 1, {glyph->uv_end.x, glyph->uv_start.y}, text_color}; // tr
            v[2] = {{x_end_scaled, offset_y, zindex}, 1, glyph->uv_end, text_color}; // br
            v[3] = {{offset_x, offset_y, zindex}, 1, {glyph->uv_start.x, glyph->uv_end.y}, text_color}; // bl
        }

        const f32 add_offset = (metrics->width + metrics->advance_x) * scale;
        offset_x += add_offset;
        rendered_width += add_offset;
    }

    // @question How and where to cut off text out of view (here or somewhere else)
    //      We could just prepare the entire text here but then decide what to render later?
    // @todo If width or height (usually just width) > 0 we use those values for automatic wrapping
    //      This way we can ensure no overflow easily
    // @todo implement line align_anchor, currently only total align_anchor is considered

    return {(int32) rendered_width, (int32) rendered_height};
}

/**
 * Converts an utf8 string to glyph indices of the font
 * 0x7FFF = offset, 0x8000 = either base (= 0) or extended (= 1)
 *
 * The glyphs array is temporary memory (memory_get_temp) of mem
 */
template <typename T>
int16* text_to_glyphs(
    FontSystem* const __restrict font, const char* const __restrict text,
    T* const __restrict mem, int32* const __restrict glyph_count
) NO_EXCEPT
{
    // The byte length is an upper bound of the codepoint count (cheaper than utf8_strlen())
    const int32 byte_count = (int32) strlen(text);

    // Try to find all the necessary glyphs
    // We use offsets instead of pointer chasing
    int16* const __restrict glyphs = (int16*) memory_get_temp(mem, (byte_count + 1) * sizeof(int16), alignof(uintptr_t));
    int32 count = 0;

    int32 i = 0;
    while (text[i] != '\0') {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(text + i, &codepoint);

        if (byte_len == 0) {
            ++i;
            continue;
        }

        i += byte_len;

        // The performance difference of using pointers directly or smaller indices (current implementation)
        // is very small. Some tests show that the index version is approx. 0-5% faster.
        // If we ever need more than 16 bit of font data we have to re-check
        int16 glyph = font_glyph_index_find(&font->base, codepoint);
        if (glyph < 0 && font->has_extended) {
            const int16 extended_glyph = font_glyph_index_find(&font->extended, codepoint);
            if (extended_glyph < 0) {
                continue;
            }

            // @bug Not correct:
            //      1. we need to add the value not the pointer.
            //      2. we need to find a free spot in the texture or replace old ones
            //      3. we need to update the uv values to the new position
            //      4. we need to create a custom _insert function
            //      5. we probably need a custom struct for the hash map to also contain the priority for replacing elements
            glyph = extended_glyph | 0x8000;

            font->has_changes = true;
        }

        if (glyph < 0) {
            // The 0 index character MUST be the special character for nothing found
            glyph = 0;
        }

        glyphs[count++] = glyph;
    }

    *glyph_count = count;

    return glyphs;
}

template <typename T>
HOT_CODE
v2_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex,
    const v4_f32& dimension, byte align_anchor,
    FontSystem* const __restrict font, const char* const __restrict text,
    f32 size, uint32 rgba, T* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);

    int32 glyph_count;
    const int16* const glyphs = text_to_glyphs(font, text, mem, &glyph_count);

    return vertex_text_create(
        vertices, indices, zindex,
        dimension, align_anchor,
        font, glyphs, glyph_count,
        size, rgba
    );
}

template <typename T>
HOT_CODE
v2_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex,
    const v4_f32& dimension, byte align_anchor,
    FontSystem* const __restrict font, const Utf8String& text,
    f32 size, uint32 rgba, T* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);

    // Try to find all the necessary glyphs
    // We use offsets instead of pointer chasing
    // 0x7FFF = offset, 0x8000 = either base (= 0) or extended (= 1)
    int16* const __restrict glyphs = (int16*) memory_get_temp(mem, text.length * sizeof(int16), alignof(uintptr_t));
    int32 glyph_count = 0;

    int32 i = 0;
    int32 c = 0;

    while (c < text.length) {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(text.str + i, &codepoint);

        if (byte_len == 0) {
            ++i;
            ++c;
            continue;
        }

        i += byte_len;
        ++c;

        int16 glyph = font_glyph_index_find(&font->base, codepoint);
        if (glyph < 0 && font->has_extended) {
            const int16 extended_glyph = font_glyph_index_find(&font->extended, codepoint);
            if (extended_glyph < 0) {
                continue;
            }

            glyph = extended_glyph | 0x8000;
            font->has_changes = true;
        }

        if (glyph < 0) {
            glyph = 0;
        }

        glyphs[glyph_count++] = glyph;
    }

    return vertex_text_create(
        vertices, indices, zindex,
        dimension, align_anchor,
        font, glyphs, glyph_count,
        size, rgba
    );
}

/*
Example use case for the FontTextCache below:

static FontTextCache origin_cached;
CONSTEXPR const Utf8String utf8_origin = utf8_string_literal("Origin");
if (!vertex_text_is_cached(origin_cached, utf8_origin)) {
    origin_cached.cached_dim = vertex_text_create(
        &origin_cached.cached_vertices.view, &origin_cached.cached_indices.view, zindex,
        {1.0f, top, 0.0f, 0.0f}, UI_ALIGN_DEFAULT,
        font, utf8_origin, font_size, COLOR_WHITE_RGBA,
        glyph_buffer
    );
    memcpy(&origin_cached.cached_str, &utf8_origin, sizeof(utf8_origin));
}
text_dim = vertex_text_cached_append(vertices, indices, &origin_cached, zindex);
*/
#define MAX_STACK_GLYPH_CACHE 32
struct FontTextCache {
    Utf8String cached_str;
    // We could significantly reduce the size (currently almost 4KB)
    // "Simply" create a global arena for the two vectors which then the FontTextCache
    // or the algorithm below reference.
    // Why does this safe memory?
    // Well we no longer require a fixed size per FontTextCache and can make use of the fact,
    // that usually the string length is shorter
    FixedArrayVector<Vertex3DSamplerTextureColor, 4 * MAX_STACK_GLYPH_CACHE> cached_vertices;
    FixedArrayVector<int32, 6 * MAX_STACK_GLYPH_CACHE> cached_indices;
    v2_int32 cached_dim;
};

v2_int32 vertex_text_cached_append(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices,
    ArrayVector<int32>* const __restrict indices,
    const FontTextCache* const __restrict cache,
    float zindex,
    int32 steps = 8
) NO_EXCEPT
{
    const int32 vertex_count = cache->cached_vertices.view.count;
    const int32 index_count = cache->cached_indices.view.count;
    const int32 base_vertex = vertices->count;
    const int32 base_index = indices->count;

    // We may have to update the zindex
    if (vertex_count > 0
        && cache->cached_vertices.view.elements[0].position.z != zindex
    ) {
        Vertex3DSamplerTextureColor* const cached_vertices = cache->cached_vertices.view.elements;
        for (int32 v = 0; v < vertex_count; ++v) {
            cached_vertices[v].position.z = zindex;
        }
    }

    memcpy(
        vertices->elements + base_vertex,
        cache->cached_vertices.view.elements,
        vertex_count * sizeof(Vertex3DSamplerTextureColor)
    );
    memcpy(
        indices->elements + base_index,
        cache->cached_indices.view.elements,
        index_count * sizeof(int32)
    );

    int32* const dst = indices->elements + base_index;
    int32 i = 0;

    #if defined(__ARM_FEATURE_SVE)
        if (steps >= 4) {
            const svbool_t all = svptrue_b32();
            const svint32_t offset_sve = svdup_n_s32(base_vertex);
            for (; i < index_count; i += steps) {
                svbool_t pg = svwhilelt_b32(i, index_count);
                svint32_t v = svld1_s32(pg, dst + i);
                v = svadd_s32_x(pg, v, offset_sve);
                svst1_s32(pg, dst + i, v);
            }
        }
    #elif defined(__ARM_NEON)
        if (steps >= 4) {
            const int32x4_t offset4_neon = vdupq_n_s32(base_vertex);
            for (; i + 4 <= index_count; i += 4) {
                int32x4_t v = vld1q_s32(dst + i);
                v = vaddq_s32(v, offset4_neon);
                vst1q_s32(dst + i, v);
            }
        }
    #else
        #if defined(__AVX512F__)
            if (steps >= 16) {
                const __m512i offset16 = _mm512_set1_epi32(base_vertex);
                for (; i + 16 <= index_count; i += 16) {
                    __m512i v = _mm512_loadu_si512((const void*)(dst + i));
                    v = _mm512_add_epi32(v, offset16);
                    _mm512_storeu_si512((void*)(dst + i), v);
                }

                // Yes, we skip 8
                steps = 4;
            }
        #endif

        #ifdef __AVX2__
            if (steps >= 8) {
                const __m256i offset8 = _mm256_set1_epi32(base_vertex);
                for (; i + 8 <= index_count; i += 8) {
                    __m256i v = _mm256_loadu_si256((const __m256i*)(dst + i));
                    v = _mm256_add_epi32(v, offset8);
                    _mm256_storeu_si256((__m256i*)(dst + i), v);
                }

                steps = 1;
            }
        #endif

        #ifdef __SSE4_2__
            if (steps >= 4) {
                const __m128i offset4 = _mm_set1_epi32(base_vertex);
                for (; i + 4 <= index_count; i += 4) {
                    __m128i v = _mm_loadu_si128((const __m128i*)(dst + i));
                    v = _mm_add_epi32(v, offset4);
                    _mm_storeu_si128((__m128i*)(dst + i), v);
                }
            }
        #endif
    #endif

    for (; i < index_count; ++i) {
        dst[i] += base_vertex;
    }

    vertices->count += vertex_count;
    indices->count  += index_count;

    return cache->cached_dim;
}

FORCE_INLINE
bool vertex_text_is_cached(FontTextCache& __restrict cache, const Utf8String& __restrict new_str) NO_EXCEPT
{
    if (cache.cached_str.length == new_str.length
        && memcmp(cache.cached_str.str, new_str.str, new_str.length) == 0
    ) {
        return true;
    }

    cache.cached_vertices.view.count = 0;
    cache.cached_indices.view.count = 0;

    return false;
}

#endif