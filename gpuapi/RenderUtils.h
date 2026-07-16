/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
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
    array_vector_insert(vertices, {{vertices->elements[vertices->count - 1].position.x, vertices->elements[vertices->count - 1].position.y, zindex}, -1, {}});
    array_vector_insert(vertices, {{x, y, zindex}, -1, {}});
}

static inline
void adjust_aligned_position(
    f32* const __restrict x, f32* const __restrict y,
    f32 width, f32 height,
    byte alignment
) NO_EXCEPT
{
    if (alignment & UI_ALIGN_H_RIGHT) {
        *x -= width;
    } else if (alignment & UI_ALIGN_H_CENTER) {
        *x -= width / 2.0f;
    }

    if (alignment & UI_ALIGN_V_TOP) {
        *y -= height;
    } else if (alignment & UI_ALIGN_V_CENTER) {
        *y -= height / 2.0f;
    }
}

static FORCE_INLINE
void adjust_aligned_position(
    v4_f32* const vec,
    byte alignment
) NO_EXCEPT
{
    if (alignment & UI_ALIGN_H_RIGHT) {
        vec->x -= vec->width;
    } else if (alignment & UI_ALIGN_H_CENTER) {
        vec->x -= vec->width / 2.0f;
    }

    if (alignment & UI_ALIGN_V_TOP) {
        vec->y -= vec->height;
    } else if (alignment & UI_ALIGN_V_CENTER) {
        vec->y -= vec->height / 2.0f;
    }
}

inline
void vertex_line_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex,
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

    // Perpendicular vector (normalized)
    const f32 px = -dy;
    const f32 py = dx;

    // Scale by half-thickness
    const f32 hx = px * (thickness * 0.5f);
    const f32 hy = py * (thickness * 0.5f);

    // Four corners of the line quad
    const v2_f32 v0 = { start.x - hx, start.y - hy };
    const v2_f32 v1 = { start.x + hx, start.y + hy };
    const v2_f32 v2 = { end.x   - hx, end.y   - hy };
    const v2_f32 v3 = { end.x   + hx, end.y   + hy };

    array_vector_insert(indices, vertices->count);
    array_vector_insert(indices, vertices->count + 1);
    array_vector_insert(indices, vertices->count + 3);

    array_vector_insert(indices, vertices->count + 1);
    array_vector_insert(indices, vertices->count + 2);
    array_vector_insert(indices, vertices->count + 3);

    array_vector_insert(vertices, {{v1.x, v1.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}}); // tl
    array_vector_insert(vertices, {{v3.x, v3.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}}); // tr
    array_vector_insert(vertices, {{v2.x, v2.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}}); // br
    array_vector_insert(vertices, {{v0.x, v0.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}}); // bl
}

inline HOT_CODE
void vertex_rect_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    uint32 rgba = 0, v2_f32 tex1 = {}, v2_f32 tex2 = {}
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_RECT_CREATE);
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, alignment);
    }

    if (rgba) {
        tex1.x = -1.0f;
        tex1.y = BITCAST(rgba, f32);

        tex2.x = -1.0f;
        tex2.y = BITCAST(rgba, f32);
    }

    const f32 y_height = dimension.y + dimension.height;
    const f32 x_width = dimension.x + dimension.width;

    array_vector_insert(indices, vertices->count);
    array_vector_insert(indices, vertices->count + 1);
    array_vector_insert(indices, vertices->count + 3);

    array_vector_insert(indices, vertices->count + 1);
    array_vector_insert(indices, vertices->count + 2);
    array_vector_insert(indices, vertices->count + 3);

    array_vector_insert(vertices, {{dimension.x, y_height, zindex}, sampler, {tex1.x, tex2.y}}); // tl
    array_vector_insert(vertices, {{x_width, y_height, zindex}, sampler, tex2}); // tr
    array_vector_insert(vertices, {{x_width, dimension.y, zindex}, sampler, {tex2.x, tex1.y}}); // br
    array_vector_insert(vertices, {{dimension.x, dimension.y, zindex}, sampler, tex1}); // bl
}

inline
void vertex_circle_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments, // defines the detail (should be a multiple of 4)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, alignment);
    }

    if (rgba) {
        tex_center.x = -1.0f;
        tex_center.y = BITCAST(rgba, f32);

        tex_edge.x = -1.0f;
        tex_edge.y = BITCAST(rgba, f32);
    }

    // Circle center + radii
    const f32 cx = dimension.x + dimension.width * 0.5f;
    const f32 cy = dimension.y + dimension.height * 0.5f;
    const f32 rx = dimension.width * 0.5f;
    const f32 ry = dimension.height * 0.5f;

    // Generate a triangle fan: center + pairs of edge vertices

    const int32 center_index = vertices->count;
    array_vector_insert(vertices, {{cx, cy, zindex}, sampler, tex_center});

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

        array_vector_insert(indices, center_index);
        array_vector_insert(indices, vertices->count);
        array_vector_insert(indices, vertices->count + 1);

        // @performance Isn't one of the points on the circle arcs a duplicate same as the center
        //              We fixed it for the center but not for the previous circle arc point
        array_vector_insert(vertices, {{x0, y0, zindex}, sampler, tex_edge});
        array_vector_insert(vertices, {{x1, y1, zindex}, sampler, tex_edge});
    }
}

inline
void vertex_arc_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments,  // number of subdivisions (should be >= 3)
    f32 start_angle, // radians where the arc starts
    f32 arc_angle,   // radians how wide the arc is (e.g. OMS_PI_F32 for semicircle)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        adjust_aligned_position(&dimension, alignment);
    }

    if (rgba) {
        tex_center.x = -1.0f;
        tex_center.y = BITCAST(rgba, f32);

        tex_edge.x = -1.0f;
        tex_edge.y = BITCAST(rgba, f32);
    }

    // Circle center + radii
    const f32 cx = dimension.x + dimension.width * 0.5f;
    const f32 cy = dimension.y + dimension.height * 0.5f;
    const f32 rx = dimension.width * 0.5f;
    const f32 ry = dimension.height * 0.5f;

    const int32 center_index = vertices->count;
    array_vector_insert(vertices, {{cx, cy, zindex}, sampler, tex_center});

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

        array_vector_insert(indices, center_index);
        array_vector_insert(indices, vertices->count);
        array_vector_insert(indices, vertices->count + 1);

        // @performance Isn't one of the points on the circle arcs a duplicate same as the center
        //              We fixed it for the center but not for the previous circle arc point
        array_vector_insert(vertices, {{x0, y0, zindex}, sampler, tex_edge});
        array_vector_insert(vertices, {{x1, y1, zindex}, sampler, tex_edge});
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

    const Glyph* const base_glyphs = font->base.glyphs;
    const Glyph* const extended_glyphs = font->base.glyphs;

    for (int i = 0; i < length; ++i) {
        if (glyphs[i] == font->newline_id) {
            // @performance calculating a new x after a line break seems wrong
            x = max_branched(x, offset_x);
            y += line_height;

            offset_x = 0;

            continue;
        }

        const Glyph* const glyph = !(glyphs[i] & 0x8000)
            ? &base_glyphs[glyphs[i]]
            : &extended_glyphs[abs(glyphs[i])];

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

    const Glyph* const base_glyphs = font->base.glyphs;
    const Glyph* const extended_glyphs = font->base.glyphs;

    for (int i = 0; i < length; ++i) {
        if (glyphs[i] == font->newline_id) {
            // @performance calculating a new x after a new line feels wrong
            x = max_branched(x, offset_x);
            offset_x = 0;

            continue;
        }

        const Glyph* const glyph = !(glyphs[i] & 0x8000)
            ? &base_glyphs[glyphs[i]]
            : &extended_glyphs[abs(glyphs[i])];

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return max_branched(x, offset_x);
}

static
f32 text_calculate_dimensions_height(
    const FontSystem* const __restrict font, const int16* const glyphs, int32 length, f32 scale
) NO_EXCEPT
{
    const f32 line_height = font->base.line_height * scale;
    f32 y = line_height;

    // Now we just count how often that newline character exists
    for (int i = 0; i < length; ++i) {
        if (glyphs[i] == font->newline_id) {
            y += line_height;
        }
    }

    return y;
}

HOT_CODE
v3_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    FontSystem* const __restrict font, const int16* const __restrict glyphs, int32 length,
    f32 size, MAYBE_UNUSED uint32 rgba
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);
    PSEUDO_USE(rgba);

    const Font* const font_base = &font->base;
    const f32 scale = size / font_base->size;

    // @performance I don't like that we have to iterate the glyphs twice, do we really need this?
    // If we do a different alignment we need to pre-calculate the width and height
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        if ((alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER))
            && (alignment & (UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER))
        ) {
            const v2_f32 dim = text_calculate_dimensions(font, glyphs, length, scale);
            dimension.width = dim.width;
            dimension.height = dim.height;
        } else if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER)) {
            dimension.width = text_calculate_dimensions_width(font, glyphs, length, scale);
        } else {
            dimension.height = text_calculate_dimensions_height(font, glyphs, length, scale);
        }

        adjust_aligned_position(&dimension, alignment);
    }

    const f32 line_height_scaled = font_base->line_height * scale;

    f32 rendered_width = 0;
    f32 rendered_height = line_height_scaled;

    int32 idx = 0;
    f32 offset_x = dimension.x;

    const Glyph* const base_glyphs = font->base.glyphs;
    const Glyph* const extended_glyphs = font->base.glyphs;

    for (int32 i = 0; i < length; ++i) {
        if (glyphs[i] == font->newline_id) {
            rendered_height += line_height_scaled;
            rendered_width = max_branched(rendered_width, offset_x - dimension.x);

            dimension.y -= line_height_scaled;
            offset_x = dimension.x;

            continue;
        }

        const Glyph* const glyph = !(glyphs[i] & 0x8000)
            ? &base_glyphs[glyphs[i]]
            : &extended_glyphs[abs(glyphs[i])];

        const GlyphMetrics* const metrics = &glyph->metrics;
        const f32 offset_y = dimension.y + metrics->offset_y * scale;
        offset_x += metrics->offset_x * scale;
        rendered_width += metrics->offset_x * scale;

        if (glyph->codepoint != ' ' && glyph->codepoint != '\t') {
            // @bug we cannot pass the rgba here since the rgba overwrites the texture coordinates
            //      we would have to add at least an additional 4 bytes to allow texture coordinates + recoloring

            // @performance We might want to cache the vertex data per glyph
            //              Then we only need to multiply the x/y coordinates with the scale factor
            //              But are we even saving anything? We would increase the memory and therefore reduce cache locality
            const f32 x_end_scaled = offset_x + metrics->width * scale;
            const f32 y_end_scaled = offset_y + metrics->height * scale;

            array_vector_insert(indices, vertices->count);
            array_vector_insert(indices, vertices->count + 1);
            array_vector_insert(indices, vertices->count + 3);

            array_vector_insert(indices, vertices->count + 1);
            array_vector_insert(indices, vertices->count + 2);
            array_vector_insert(indices, vertices->count + 3);

            array_vector_insert(vertices, {{offset_x, y_end_scaled, zindex}, sampler, glyph->uv_start}); // tl
            array_vector_insert(vertices, {{x_end_scaled, y_end_scaled, zindex}, sampler, {glyph->uv_end.x, glyph->uv_start.y}}); // tr
            array_vector_insert(vertices, {{x_end_scaled, offset_y, zindex}, sampler, glyph->uv_end}); // br
            array_vector_insert(vertices, {{offset_x, offset_y, zindex}, sampler, {glyph->uv_start.x, glyph->uv_end.y}}); // bl
        }

        const f32 add_offset = (metrics->width + metrics->advance_x) * scale;
        offset_x += add_offset;
        rendered_width += add_offset;
    }

    // @question How and where to cut off text out of view (here or somewhere else)
    //      We could just prepare the entire text here but then decide what to render later?
    // @todo If width or height (usually just width) > 0 we use those values for automatic wrapping
    //      This way we can ensure no overflow easily
    // @todo implement line alignment, currently only total alignment is considered

    return {(int32) rendered_width, (int32) rendered_height, idx};
}

// @todo It is stupid that we effectively have the same function twice and the only difference is how we calculate int32 character
template <typename T>
HOT_CODE
v3_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex, int32 sampler,
    const v4_f32& dimension, byte alignment,
    FontSystem* const __restrict font, const wchar_t* const __restrict text,
    f32 size, MAYBE_UNUSED uint32 rgba, T* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);
    PSEUDO_USE(rgba);

    size_t length;
    if (!text || (length = wcslen(text)) < 1) {
        return {};
    }

    // Try to find all the necessary glyphs
    // We use offsets instead of pointer chasing
    // 0x7FFF = offset, 0x8000 = either base (= 0) or extended (= 1)
    int16* const glyphs = (int16*) memory_get(mem, length * sizeof(int16), alignof(uintptr_t));
    //alignas(alignof(size_t)) int16 glyphs[10000];
    for (int32 i = 0; i < length; ++i) {
        const int32 character = text[i];

        // @question Do I even want to handle this in a special way?
        //          This is no longer required if we force \n to be part of the glyph file
        if (character == '\n') {
            glyphs[i] = font->newline_id;
            continue;
        }

        glyphs[i] = font_glyph_index_find(&font->base, character);
        if (glyphs[i] < 0 && font->has_extended) {
            //glyphs[i] = hashmap_get(&font->font_map, character);

            //if (glyphs[i] < 0) {
                const int16 extended_glyph = font_glyph_index_find(&font->extended, character);
                if (extended_glyph < 0) {
                    //glyphs[i] = -1;
                    continue;
                }

                // @bug Not correct:
                //      1. we need to add the value not the pointer.
                //      2. we need to find a free spot in the texture or replace old ones
                //      3. we need to update the uv values to the new position
                //      4. we need to create a custom _insert function
                //      5. we probably need a custom struct for the hash map to also contain the priority for replacing elements
                //hashmap_insert(&font->font_map, extended_glyph->codepoint, extended_glyph);

                glyphs[i] = extended_glyph | 0x8000;

                font->has_changes = true;
            //} else {
                // @todo update glyph priority
            //}
        }

        if (glyphs[i] < 0) {
            // @todo add unknown character glyph
            glyphs[i] = 0;
        }
    }

    return vertex_text_create(
        vertices, indices, zindex, sampler,
        dimension, alignment,
        font, glyphs, (int32) length,
        size, rgba
    );
}

template <typename T>
HOT_CODE
v3_int32 vertex_text_create(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, f32 zindex, int32 sampler,
    const v4_f32& dimension, byte alignment,
    FontSystem* const __restrict font, const char* const __restrict text,
    f32 size, MAYBE_UNUSED uint32 rgba, T* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_VERTEX_TEXT_CREATE);
    PSEUDO_USE(rgba);

    int32 length;
    if (!text || (length = utf8_strlen(text)) < 1) {
        return {};
    }

    const bool is_ascii = (int32) strlen(text) == length;

    // Try to find all the necessary glyphs
    // We use offsets instead of pointer chasing
    // 0x7FFF = offset, 0x8000 = either base (= 0) or extended (= 1)
    int16* const glyphs = (int16*) memory_get(mem, length * sizeof(int16), alignof(uintptr_t));
    for (int32 i = 0; i < length; ++i) {
        const int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);

        // @question Do I even want to handle this in a special way?
        //          This is no longer required if we force \n to be part of the glyph file
        if (character == '\n') {
            glyphs[i] = font->newline_id;
            continue;
        }

        // @performance Do I really want to use an index array instead of a pointer array?
        //              Indices are smaller (4x) but require an additional if statement to decide between base and extended later on
        //              Pointers are larger but no additional if required
        glyphs[i] = font_glyph_index_find(&font->base, character);
        if (glyphs[i] < 0 && font->has_extended) {
            //glyphs[i] = hashmap_get(&font->font_map, character);

            //if (glyphs[i] < 0) {
                const int16 extended_glyph = font_glyph_index_find(&font->extended, character);
                if (extended_glyph < 0) {
                    //glyphs[i] = -1;
                    continue;
                }

                // @bug Not correct:
                //      1. we need to add the value not the pointer.
                //      2. we need to find a free spot in the texture or replace old ones
                //      3. we need to update the uv values to the new position
                //      4. we need to create a custom _insert function
                //      5. we probably need a custom struct for the hash map to also contain the priority for replacing elements
                //hashmap_insert(&font->font_map, extended_glyph->codepoint, extended_glyph);

                glyphs[i] = extended_glyph | 0x8000;

                font->has_changes = true;
            //} else {
                // @todo update glyph priority
            //}
        }

        if (glyphs[i] < 0) {
            // @todo add unknown character glyph, conflicts with \n
            glyphs[i] = 0;
        }
    }

    return vertex_text_create(
        vertices, indices, zindex, sampler,
        dimension, alignment,
        font, glyphs, length,
        size, rgba
    );
}

#endif