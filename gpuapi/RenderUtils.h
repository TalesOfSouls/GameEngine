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

#include <stdio.h>
#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"
#include "../font/Font.h"
#include "../object/Vertex.h"
#include "../ui/UIAlignment.h"
#include "../architecture/Intrinsics.h"

FORCE_INLINE
int32 vertex_degenerate_create(
    Vertex3DSamplerTextureColor* __restrict vertices, f32 zindex,
    f32 x, f32 y
) NO_EXCEPT {
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
    f32* __restrict x, f32* __restrict y,
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
    v4_f32* vec,
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
    Vertex3DSamplerTextureColor* vertices, f32 zindex,
    v2_f32 start, v2_f32 end, f32 thickness,
    uint32 rgba = 0
) NO_EXCEPT {
    f32 dx = end.x - start.x;
    f32 dy = end.y - start.y;

    // Normalize direction
    f32 len = intrin_rsqrt_f32(dx * dx + dy * dy);
    dx *= len;
    dy *= len;

    // Perpendicular vector (normalized)
    f32 px = -dy;
    f32 py = dx;

    // Scale by half-thickness
    f32 hx = px * (thickness * 0.5f);
    f32 hy = py * (thickness * 0.5f);

    // Four corners of the line quad
    v2_f32 v0 = { start.x - hx, start.y - hy };
    v2_f32 v1 = { start.x + hx, start.y + hy };
    v2_f32 v2 = { end.x   - hx, end.y   - hy };
    v2_f32 v3 = { end.x   + hx, end.y   + hy };

    int32 idx = 0;

    vertices[idx++] = {{v0.x, v0.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v1.x, v1.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v2.x, v2.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};

    vertices[idx++] = {{v2.x, v2.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v1.x, v1.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{v3.x, v3.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};

    return idx;
}

// @question Do we really want this to be inline? we are calling this function very often -> a lot of inlined code size
inline
int32 vertex_rect_create(
    Vertex3DSamplerTextureColor* __restrict vertices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    uint32 rgba = 0, v2_f32 tex1 = {}, v2_f32 tex2 = {}
) NO_EXCEPT {
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

    f32 y_height = dimension.y + dimension.height;
    f32 x_width = dimension.x + dimension.width;
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
    Vertex3DSamplerTextureColor* __restrict vertices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments, // defines the detail (should be a multiple of 4)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT {
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
    f32 cx = dimension.x + dimension.width * 0.5f;
    f32 cy = dimension.y + dimension.height * 0.5f;
    f32 rx = dimension.width * 0.5f;
    f32 ry = dimension.height * 0.5f;

    int32 idx = 0;

    // Generate a triangle fan: center + pairs of edge vertices
    f32 s;
    f32 c;
    for (int32 i = 0; i < segments; ++i) {
        f32 angle0 = (OMS_TWO_PI * i) / segments;
        f32 angle1 = (OMS_TWO_PI * (i + 1)) / segments;

        SINCOSF(angle0, s, c);
        f32 x0 = cx + c * rx;
        f32 y0 = cy + s * ry;

        SINCOSF(angle1, s, c);
        f32 x1 = cx + c * rx;
        f32 y1 = cy + s * ry;

        // center
        vertices[idx++] = {{cx, cy, zindex}, sampler, tex_center};
        vertices[idx++] = {{x0, y0, zindex}, sampler, tex_edge};
        vertices[idx++] = {{x1, y1, zindex}, sampler, tex_edge};
    }

    return idx;
}

inline
int32 vertex_arc_create(
    Vertex3DSamplerTextureColor* __restrict vertices,
    f32 zindex, int32 sampler,
    v4_f32 dimension,
    byte alignment,
    int32 segments,              // number of subdivisions (should be >= 3)
    f32 start_angle,             // radians — where the arc starts
    f32 arc_angle,               // radians — how wide the arc is (e.g. OMS_PI for semicircle)
    uint32 rgba = 0, v2_f32 tex_center = {}, v2_f32 tex_edge = {}
) NO_EXCEPT {
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
    f32 cx = dimension.x + dimension.width * 0.5f;
    f32 cy = dimension.y + dimension.height * 0.5f;
    f32 rx = dimension.width * 0.5f;
    f32 ry = dimension.height * 0.5f;

    int32 idx = 0;

    // Generate a triangle fan over the arc
    f32 s, c;
    for (int32 i = 0; i < segments; ++i) {
        f32 angle0 = start_angle + (arc_angle * (f32) i / segments);
        f32 angle1 = start_angle + (arc_angle * (f32) (i + 1) / segments);

        SINCOSF(angle0, s, c);
        f32 x0 = cx + c * rx;
        f32 y0 = cy + s * ry;

        SINCOSF(angle1, s, c);
        f32 x1 = cx + c * rx;
        f32 y1 = cy + s * ry;

        vertices[idx++] = {{cx, cy, zindex}, sampler, tex_center};
        vertices[idx++] = {{x0, y0, zindex}, sampler, tex_edge};
        vertices[idx++] = {{x1, y1, zindex}, sampler, tex_edge};
    }

    return idx;
}

static
f32 text_calculate_dimensions_height(
    const Font* __restrict font, const char* __restrict text, f32 scale, int32 length
) NO_EXCEPT {
    f32 line_height = font->line_height * scale;
    f32 y = line_height;

    // @todo remember to restrict to width/height if value > 0 -> force width to remain below certain value

    for (int32 i = 0; i < length; ++i) {
        if (text[i] == '\n') {
            y += line_height;
        }
    }

    return y;
}

static
f32 text_calculate_dimensions_width(
    const Font* __restrict font, const char* __restrict text, bool is_ascii, f32 scale, int32 length
) NO_EXCEPT {
    f32 x = 0;
    f32 offset_x = 0;

    // @todo remember to restrict to width/height if value > 0 -> force width to remain below certain value

    for (int32 i = 0; i < length; ++i) {
        int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);

        if (character == '\n') {
            x = OMS_MAX_BRANCHED(x, offset_x);
            offset_x = 0;

            continue;
        }

        Glyph* glyph = font_glyph_find(font, character);
        if (!glyph) {
            continue;
        }

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return OMS_MAX_BRANCHED(x, offset_x);
}

static
void text_calculate_dimensions(
    f32* __restrict width, f32* __restrict height,
    const Font* __restrict font, const char* __restrict text, bool is_ascii, f32 scale, int32 length
) NO_EXCEPT {
    f32 line_height = font->line_height * scale;
    f32 x = 0;
    f32 y = line_height;

    f32 offset_x = 0;

    // @todo remember to restrict to width/height if value > 0 -> force width to remain below certain value

    for (int32 i = 0; i < length; ++i) {
        int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);

        if (character == '\n') {
            x = OMS_MAX_BRANCHED(x, offset_x);
            y += line_height;

            offset_x = 0;

            continue;
        }

        Glyph* glyph = font_glyph_find(font, character);
        if (!glyph) {
            continue;
        }

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    *width = OMS_MAX_BRANCHED(x, offset_x);
    *height = y;
}

// @todo implement shadow (offset + angle + diffuse) or should this be a shader only thing? if so this would be a problem for us since we are handling text in the same shader as simple shapes
// we might want to implement distance field font atlas
// @todo We should be able to cut off text at an arbitrary position, not just at a line_height incremental
// we could probably get the MIN of the glyph height and the remaining window height
v3_int32 vertex_text_create(
    Vertex3DSamplerTextureColor* __restrict vertices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    const Font* __restrict font, const char* __restrict text,
    f32 size, uint32 rgba = 0
) NO_EXCEPT {
    PROFILE(PROFILE_VERTEX_TEXT_CREATE);
    int32 length = utf8_str_length(text);
    if (length < 1) {
        return {};
    }

    bool is_ascii = (int32) str_length(text) == length;
    f32 scale = size / font->size;

    (void) rgba; // @todo we don't have a way to change colors of text for now due to our reduce Vertex size
    // To fix this we would have to add an additional 4 bytes for every vertex which we maybe don't want to

    // If we do a different alignment we need to pre-calculate the width and height
    if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER | UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER)) {
        if ((alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER))
            && (alignment & (UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER))
        ) {
            text_calculate_dimensions(&dimension.width, &dimension.height, font, text, is_ascii, scale, length);
        } else if (alignment & (UI_ALIGN_H_RIGHT | UI_ALIGN_H_CENTER)) {
            dimension.width = text_calculate_dimensions_width(font, text, is_ascii, scale, length);
        } else {
            dimension.height = text_calculate_dimensions_height(font, text, scale, length);
        }

        adjust_aligned_position(&dimension, alignment);
    }

    f32 line_height_scaled = font->line_height * scale;

    f32 rendered_width = 0;
    f32 rendered_height = line_height_scaled;

    int32 idx = 0;

    f32 offset_x = dimension.x;
    for (int32 i = 0; i < length; ++i) {
        int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);
        if (character == '\n') {
            rendered_height += line_height_scaled;
            rendered_width = OMS_MAX_BRANCHED(rendered_width, offset_x - dimension.x);

            dimension.y -= line_height_scaled;
            offset_x = dimension.x;

            continue;
        }

        Glyph* glyph = font_glyph_find(font, character);
        if (!glyph) {
            continue;
        }

        f32 offset_y = dimension.y + glyph->metrics.offset_y * scale;
        offset_x += glyph->metrics.offset_x * scale;

        if (character != ' ' && character != '\t') {
            // @todo We should probably inline the code here, we might be able to even optimize it then
            idx += vertex_rect_create(
                vertices + idx, zindex, sampler,
                {offset_x, offset_y, glyph->metrics.width * scale, glyph->metrics.height * scale}, 0,
                0, glyph->coords.start, glyph->coords.end
            );
        }

        offset_x += (glyph->metrics.width + glyph->metrics.advance_x) * scale;
    }

    // @question How and where to cut off text out of view (here or somewhere else)
    //      We could just prepare the entire text here but then decide what to render later?
    // @todo If width or height (usually just width) > 0 we use those values for automatic wrapping
    //      This way we can ensure no overflow easily
    // @todo implement line alignment, currently only total alignment is considered

    return {(int32) rendered_width, (int32) rendered_height, idx};
}

#endif