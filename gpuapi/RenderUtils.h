/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_RENDER_UTILS_H
#define COMS_GPUAPI_RENDER_UTILS_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../font/Font.cpp"
#include "../font/FontSystem.h"
#include "../memory/RingMemory.cpp"
#include "../object/Vertex.h"
#include "../ui/UIAlignment.h"
#include "../architecture/Intrinsics.h"

FORCE_INLINE
int32 vertex_degenerate_create(
    Vertex3DSamplerTextureColor* const __restrict vertices, f32 zindex,
    f32 x, f32 y
) NO_EXCEPT
{
    // Degenerate triangles
    // They are alternating every loop BUT since we use references they look the same in code
    // WARNING: Before using we must make sure that the 0 index is defined
    //          The easiest way is to just define a "degenerate" starting point
    vertices[0] = {{vertices[-1].position.x, vertices[-1].position.y, zindex}, -1, {}};
    vertices[1] = {{x, y, zindex}, -1, {}};

    return 2;
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
int32 vertex_line_create(
    Vertex3DSamplerTextureColor* const vertices, f32 zindex,
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

    int32 idx = 0;

    vertices[idx++] = {{v0.x, v0.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v1.x, v1.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v2.x, v2.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    ASSERT_STRICT(fabsf(vertices[0].position.x - vertices[1].position.x) > OMS_EPSILON_F32 || fabsf(vertices[0].position.x - vertices[2].position.x) > OMS_EPSILON_F32);
    ASSERT_STRICT(fabsf(vertices[0].position.y - vertices[1].position.y) > OMS_EPSILON_F32 || fabsf(vertices[0].position.y - vertices[2].position.y) > OMS_EPSILON_F32);

    vertices[idx++] = {{v2.x, v2.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v1.x, v1.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v3.x, v3.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    ASSERT_STRICT(fabsf(vertices[0].position.x - vertices[1].position.x) > OMS_EPSILON_F32 || fabsf(vertices[0].position.x - vertices[2].position.x) > OMS_EPSILON_F32);
    ASSERT_STRICT(fabsf(vertices[0].position.y - vertices[1].position.y) > OMS_EPSILON_F32 || fabsf(vertices[0].position.y - vertices[2].position.y) > OMS_EPSILON_F32);

    return idx;
}

inline
int32 vertex_rect_create(
    Vertex3DSamplerTextureColor* const vertices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    uint32 rgba = 0, v2_f32 tex1 = {}, v2_f32 tex2 = {}
) NO_EXCEPT
{
    PROFILE(PROFILE_VERTEX_RECT_CREATE);
    if (alignment) {
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
    int32 idx = 0;

    vertices[idx++] = {{dimension.x, dimension.y, zindex}, sampler, tex1};
    vertices[idx++] = {{dimension.x, y_height, zindex}, sampler, {tex1.x, tex2.y}};
    vertices[idx++] = {{x_width, dimension.y, zindex}, sampler, {tex2.x, tex1.y}};

    vertices[idx++] = {{x_width, dimension.y, zindex}, sampler, {tex2.x, tex1.y}};
    vertices[idx++] = {{dimension.x, y_height, zindex}, sampler, {tex1.x, tex2.y}};
    vertices[idx++] = {{x_width, y_height, zindex}, sampler, tex2};

    return idx;
}

inline
int32 vertex_circle_create(
    Vertex3DSamplerTextureColor* const vertices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments, // defines the detail (should be a multiple of 4)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (alignment) {
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

    int32 idx = 0;

    // Generate a triangle fan: center + pairs of edge vertices

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

        // center
        vertices[idx++] = {{cx, cy, zindex}, sampler, tex_center};
        vertices[idx++] = {{x0, y0, zindex}, sampler, tex_edge};
        vertices[idx++] = {{x1, y1, zindex}, sampler, tex_edge};
    }

    return idx;
}

inline
int32 vertex_arc_create(
    Vertex3DSamplerTextureColor* const vertices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments,              // number of subdivisions (should be >= 3)
    f32 start_angle,             // radians — where the arc starts
    f32 arc_angle,               // radians — how wide the arc is (e.g. OMS_PI_F32 for semicircle)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT
{
    if (alignment) {
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

    int32 idx = 0;

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

        vertices[idx++] = {{cx, cy, zindex}, sampler, tex_center};
        vertices[idx++] = {{x0, y0, zindex}, sampler, tex_edge};
        vertices[idx++] = {{x1, y1, zindex}, sampler, tex_edge};
    }

    return idx;
}

static
v2_f32 text_calculate_dimensions(
    const Glyph** const glyphs, int32 length, f32 line_height, f32 scale
) NO_EXCEPT
{
    line_height = line_height * scale;
    f32 x = 0;
    f32 y = line_height;

    f32 offset_x = 0;

    for (int32 i = 0; i < length; ++i) {
        if (!glyphs[i] || glyphs[i]->codepoint == '\n') {
            x = max_branched(x, offset_x);
            y += line_height;

            offset_x = 0;

            continue;
        }

        offset_x += (glyphs[i]->metrics.width + glyphs[i]->metrics.offset_x + glyphs[i]->metrics.advance_x) * scale;
    }

    return { max_branched(x, offset_x), y };
}

static
f32 text_calculate_dimensions_width(
    const Glyph** const glyphs, int32 length, f32 scale
) NO_EXCEPT
{
    f32 x = 0;
    f32 offset_x = 0;

    for (int32 i = 0; i < length; ++i) {
        if (!glyphs[i] || glyphs[i]->codepoint == '\n') {
            x = max_branched(x, offset_x);
            offset_x = 0;

            continue;
        }

        offset_x += (glyphs[i]->metrics.width + glyphs[i]->metrics.offset_x + glyphs[i]->metrics.advance_x) * scale;
    }

    return max_branched(x, offset_x);
}

static
f32 text_calculate_dimensions_height(
    const Glyph** const glyphs, int32 length, f32 line_height, f32 scale
) NO_EXCEPT
{
    line_height = line_height * scale;
    //f32 x = 0;
    f32 y = line_height;

    //f32 offset_x = 0;

    for (int32 i = 0; i < length; ++i) {
        if (!glyphs[i] || glyphs[i]->codepoint == '\n') {
            //x = max_branched(x, offset_x);
            y += line_height;

            //offset_x = 0;

            //continue;
        }

        //offset_x += (glyphs[i]->metrics.width + glyphs[i]->metrics.offset_x + glyphs[i]->metrics.advance_x) * scale;
    }

    return y;
    //return { max_branched(x, offset_x), y };
}

v3_int32 vertex_text_create(
    Vertex3DSamplerTextureColor* const __restrict vertices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    FontSystem* const __restrict font, const char* const __restrict text,
    f32 size, MAYBE_UNUSED uint32 rgba, RingMemory* const __restrict ring
) NO_EXCEPT
{
    PROFILE(PROFILE_VERTEX_TEXT_CREATE);
    PSEUDO_USE(rgba);

    int32 length;
    if (!text || (length = utf8_strlen(text)) < 1) {
        return {};
    }

    const bool is_ascii = (int32) strlen(text) == length;
    const f32 scale = size / font->base.size;

    const Glyph** glyphs = (const Glyph**) ring_memory_get(ring, length * sizeof(Glyph*), alignof(uintptr_t));
    for (int32 i = 0; i < length; ++i) {
        const int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);
        if (character == '\n') {
            glyphs[i] = NULL;
            continue;
        }

        glyphs[i] = font_glyph_find(&font->base, character);
        if (!glyphs[i] && font->has_extended) {
            //glyphs[i] = hashmap_get(&font->font_map, character);

            if (!glyphs[i]) {
                const Glyph* extended_glyph = font_glyph_find(&font->extended, character);
                if (!extended_glyph) {
                    glyphs[i] = NULL;
                    continue;
                }

                // @bug Not correct:
                //      1. we need to add the value not the pointer.
                //      2. we need to find a free spot in the texture or replace old ones
                //      3. we need to update the uv values to the new position
                //      4. we need to create a custom _insert function
                //      5. we probably need a custom struct for the hash map to also contain the priority for replacing elements
                //hashmap_insert(&font->font_map, extended_glyph->codepoint, extended_glyph);

                // @bug this is wrong, we need the inserted pointer since the texture position is different
                glyphs[i] = extended_glyph;

                font->has_changes = true;
            } else {
                // @todo update glyph priority
            }
        }

        if (!glyphs[i]) {
            // @todo add unknown character glyph
            glyphs[i] = &font->base.glyphs[0];
        }
    }

    const f32 line_height = font->base.line_height;

    // If we do a different alignment we need to pre-calculate the width and height
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        if ((alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER))
            && (alignment & (UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER))
        ) {
            const v2_f32 dim = text_calculate_dimensions(glyphs, length, line_height, scale);
            dimension.width = dim.width;
            dimension.height = dim.height;
        } else if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER)) {
            dimension.width = text_calculate_dimensions_width(glyphs, length, scale);
        } else {
            dimension.height = text_calculate_dimensions_height(glyphs, length, line_height, scale);
        }

        adjust_aligned_position(&dimension, alignment);
    }

    const f32 line_height_scaled = line_height * scale;

    f32 rendered_width = 0;
    f32 rendered_height = line_height_scaled;

    int32 idx = 0;

    f32 offset_x = dimension.x;
    for (int32 i = 0; i < length; ++i) {
        const Glyph* const glyph = glyphs[i];
        if (!glyph || glyph->codepoint == '\n') {
            rendered_height += line_height_scaled;
            rendered_width = max_branched(rendered_width, offset_x - dimension.x);

            dimension.y -= line_height_scaled;
            offset_x = dimension.x;

            continue;
        }

        const GlyphMetrics* metrics = &glyph->metrics;
        const GlyphVertex* glyph_vertices = glyph->vertices;

        const f32 offset_y = dimension.y + metrics->offset_y * scale;
        offset_x += metrics->offset_x * scale;
        rendered_width += metrics->offset_x * scale;

        if (glyph->codepoint != ' ' && glyph->codepoint != '\t') {
            // @bug we cannot pass the rgba here since the rgba overwrites the texture coordinates
            //      we would have to add at least an additional 4 bytes to allow texture coordinates + recoloring
            vertices[idx++] = {{offset_x + glyph_vertices[0].pos.x * scale, offset_y + glyph_vertices[0].pos.y * scale, zindex}, sampler, glyph_vertices[0].uv};
            vertices[idx++] = {{offset_x + glyph_vertices[1].pos.x * scale, offset_y + glyph_vertices[1].pos.y * scale, zindex}, sampler, glyph_vertices[1].uv};
            vertices[idx++] = {{offset_x + glyph_vertices[2].pos.x * scale, offset_y + glyph_vertices[2].pos.y * scale, zindex}, sampler, glyph_vertices[2].uv};
            ASSERT_STRICT(fabsf(vertices[idx - 3].position.x - vertices[idx - 2].position.x) > OMS_EPSILON_F32 || fabsf(vertices[idx - 3].position.x - vertices[idx - 1].position.x) > OMS_EPSILON_F32);
            ASSERT_STRICT(fabsf(vertices[idx - 3].position.y - vertices[idx - 2].position.y) > OMS_EPSILON_F32 || fabsf(vertices[idx - 3].position.y - vertices[idx - 1].position.y) > OMS_EPSILON_F32);

            if (glyph->vertex_count > 3) {
                vertices[idx++] = {{offset_x + glyph_vertices[1].pos.x * scale, offset_y + glyph_vertices[1].pos.y * scale, zindex}, sampler, glyph_vertices[1].uv};
                vertices[idx++] = {{offset_x + glyph_vertices[3].pos.x * scale, offset_y + glyph_vertices[3].pos.y * scale, zindex}, sampler, glyph_vertices[3].uv};
                vertices[idx++] = {{offset_x + glyph_vertices[2].pos.x * scale, offset_y + glyph_vertices[2].pos.y * scale, zindex}, sampler, glyph_vertices[2].uv};
                ASSERT_STRICT(fabsf(vertices[idx - 3].position.x - vertices[idx - 2].position.x) > OMS_EPSILON_F32 || fabsf(vertices[idx - 3].position.x - vertices[idx - 1].position.x) > OMS_EPSILON_F32);
                ASSERT_STRICT(fabsf(vertices[idx - 3].position.y - vertices[idx - 2].position.y) > OMS_EPSILON_F32 || fabsf(vertices[idx - 3].position.y - vertices[idx - 1].position.y) > OMS_EPSILON_F32);
            }
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

#endif