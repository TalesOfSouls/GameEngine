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
) noexcept {
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
) noexcept
{
    if (alignment & UI_ALIGN_H_RIGHT) {
        *x -= width;
    } else if (alignment & UI_ALIGN_H_CENTER) {
        *x -= width / 2;
    }

    if (alignment & UI_ALIGN_V_TOP) {
        *y -= height;
    } else if (alignment & UI_ALIGN_V_CENTER) {
        *y -= height / 2;
    }
}

static inline
void adjust_aligned_position(
    v4_f32* vec,
    byte alignment
) noexcept
{
    if (alignment & UI_ALIGN_H_RIGHT) {
        vec->x -= vec->width;
    } else if (alignment & UI_ALIGN_H_CENTER) {
        vec->x -= vec->width / 2;
    }

    if (alignment & UI_ALIGN_V_TOP) {
        vec->y -= vec->height;
    } else if (alignment & UI_ALIGN_V_CENTER) {
        vec->y -= vec->height / 2;
    }
}

inline
int32 vertex_line_create(
    Vertex3DSamplerTextureColor* __restrict vertices, f32 zindex,
    v2_f32 start, v2_f32 end, f32 thickness, byte alignment,
    uint32 rgba = 0
) noexcept {
    if (alignment & UI_ALIGN_H_RIGHT) {
        start.x -= thickness;
        end.x -= thickness;
    } else if (alignment & UI_ALIGN_H_CENTER) {
        start.x -= thickness / 2;
        end.x -= thickness / 2;
    }

    if (alignment & UI_ALIGN_V_TOP) {
        start.y -= thickness;
        end.y -= thickness;
    } else if (alignment & UI_ALIGN_V_CENTER) {
        start.y -= thickness / 2;
        end.y -= thickness / 2;
    }

    f32 n1 = -(end.y - start.y);
    f32 n2 = end.x - start.x;
    f32 n_ = intrin_rsqrt_f32(n2 * n2 + n1 * n1);
    f32 norm1 = n1 * n_;
    f32 norm2 = n2 * n_;

    int32 idx = 0;

    vertices[idx++] = {{start.x, start.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{start.x + thickness * norm1, start.y + thickness * norm2, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{end.x, end.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};

    vertices[idx++] = {{end.x, end.y, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{end.x + thickness * norm1, end.y + thickness * norm2, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};
    vertices[idx++] = {{start.x + thickness * norm1, start.y + thickness * norm2, zindex}, -1, {-1.0f, BITCAST(rgba, f32)}};

    return idx;
}

// @performance Do we really want to create the UI as one continuous mesh?
// Individual meshes without degenerates might be faster
inline
int32 vertex_rect_create(
    Vertex3DSamplerTextureColor* __restrict vertices, f32 zindex, int32 sampler,
    v4_f32 dimension, byte alignment,
    uint32 rgba = 0, v2_f32 tex1 = {}, v2_f32 tex2 = {}
) noexcept {
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

static inline
f32 text_calculate_dimensions_height(
    const Font* __restrict font, const char* __restrict text, f32 scale, int32 length
) noexcept {
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

static inline
f32 text_calculate_dimensions_width(
    const Font* __restrict font, const char* __restrict text, bool is_ascii, f32 scale, int32 length
) noexcept {
    f32 x = 0;
    f32 offset_x = 0;

    // @todo remember to restrict to width/height if value > 0 -> force width to remain below certain value

    for (int32 i = 0; i < length; ++i) {
        int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);

        if (character == '\n') {
            x = OMS_MAX(x, offset_x);
            offset_x = 0;

            continue;
        }

        Glyph* glyph = font_glyph_find(font, character);
        if (!glyph) {
            continue;
        }

        offset_x += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return OMS_MAX(x, offset_x);
}

static inline
void text_calculate_dimensions(
    f32* __restrict width, f32* __restrict height,
    const Font* __restrict font, const char* __restrict text, bool is_ascii, f32 scale, int32 length
) noexcept {
    f32 line_height = font->line_height * scale;
    f32 x = 0;
    f32 y = line_height;

    f32 offset_x = 0;

    // @todo remember to restrict to width/height if value > 0 -> force width to remain below certain value

    for (int32 i = 0; i < length; ++i) {
        int32 character = is_ascii ? text[i] : utf8_get_char_at(text, i);

        if (character == '\n') {
            x = OMS_MAX(x, offset_x);
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

    *width = OMS_MAX(x, offset_x);
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
) noexcept {
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
            rendered_width = OMS_MAX(rendered_width, offset_x - dimension.x);

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