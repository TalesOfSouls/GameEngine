#pragma once
#ifndef COMS_UI_CLIP_H
#define COMS_UI_CLIP_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "../object/Vertex.h"

/**
 * Clipping rectangle in ui coordinates (y-up)
 *
 * The ui is rendered as one big vertex buffer without scissor rects (no extra draw calls).
 * Instead the vertices of clipped elements (e.g. children of a scroll panel) are clipped on the cpu.
 * This only happens while (re-)caching the ui, the clipped vertices are just normal vertices for the gpu.
 */
struct UIClipRect {
    f32 left;
    f32 bottom;
    f32 right;
    f32 top;
};

// x, y = top left corner (same format as ui_dimension_rect)
FORCE_INLINE
UIClipRect ui_clip_from_rect(v4_f32 rect) NO_EXCEPT
{
    return {rect.x, rect.y - rect.height, rect.x + rect.width, rect.y};
}

FORCE_INLINE
UIClipRect ui_clip_intersect(const UIClipRect& a, const UIClipRect& b) NO_EXCEPT
{
    UIClipRect r = {
        max_branched(a.left, b.left),
        max_branched(a.bottom, b.bottom),
        min_branched(a.right, b.right),
        min_branched(a.top, b.top)
    };

    // Empty rects stay empty (no negative size)
    r.right = max_branched(r.right, r.left);
    r.top = max_branched(r.top, r.bottom);

    return r;
}

FORCE_INLINE
bool ui_clip_contains(const UIClipRect& clip, f32 x, f32 y) NO_EXCEPT
{
    return x >= clip.left && x < clip.right && y > clip.bottom && y <= clip.top;
}

/**
 * Is the vertex quad axis aligned in the layout of vertex_rect_create()/vertex_text_create()?
 *      v[0] = top left, v[1] = top right, v[2] = bottom right, v[3] = bottom left
 */
FORCE_INLINE
bool ui_clip_is_quad(const Vertex3DSamplerTextureColor* const v) NO_EXCEPT
{
    return v[0].position.y == v[1].position.y
        && v[1].position.x == v[2].position.x
        && v[2].position.y == v[3].position.y
        && v[3].position.x == v[0].position.x
        && v[0].position.x <= v[1].position.x
        && v[3].position.y <= v[0].position.y;
}

/**
 * Clips the vertices [start, vertices->count)
 *
 * Axis aligned quads (rects, glyphs, images) are clipped exactly incl. their texture coordinates.
 * Other geometry (e.g. circles) only has its vertices clamped which is correct for solid colors.
 * Quads completely outside of the clip rect become zero sized (invisible).
 *
 * @return false if all vertices are outside of the clip rect (the caller can remove them)
 */
static inline
bool ui_vertices_clip(
    ArrayVector<Vertex3DSamplerTextureColor>* const vertices,
    int32 start,
    const UIClipRect& clip
) NO_EXCEPT
{
    Vertex3DSamplerTextureColor* const v = vertices->elements;
    const int32 end = (int32) vertices->count;

    bool visible = false;

    int32 i = start;
    while (i < end) {
        if (i + 3 < end && ui_clip_is_quad(v + i)) {
            Vertex3DSamplerTextureColor* const q = v + i;

            const f32 x0 = q[0].position.x;
            const f32 x1 = q[1].position.x;
            const f32 y0 = q[3].position.y; // bottom
            const f32 y1 = q[0].position.y; // top

            // Completely inside -> nothing to do
            if (x0 >= clip.left && x1 <= clip.right && y0 >= clip.bottom && y1 <= clip.top) {
                visible = true;
                i += 4;

                continue;
            }

            const f32 cx0 = clamp_branched(x0, clip.left, clip.right);
            const f32 cx1 = clamp_branched(x1, clip.left, clip.right);
            const f32 cy0 = clamp_branched(y0, clip.bottom, clip.top);
            const f32 cy1 = clamp_branched(y1, clip.bottom, clip.top);

            visible |= cx1 > cx0 && cy1 > cy0;

            // Texture coordinates are linear along the quad
            // Solid colors use tex.x = -1 and tex.y = color -> must not be interpolated
            const bool textured = q[0].texture_color.x != -1.0f;
            if (textured) {
                const f32 w = x1 - x0;
                const f32 h = y1 - y0;

                // u along x (left -> right), v along y (top -> bottom)
                const f32 u_left = q[0].texture_color.x;
                const f32 u_right = q[1].texture_color.x;
                const f32 v_top = q[0].texture_color.y;
                const f32 v_bottom = q[3].texture_color.y;

                const f32 tx0 = w > 0.0f ? (cx0 - x0) / w : 0.0f;
                const f32 tx1 = w > 0.0f ? (cx1 - x0) / w : 1.0f;
                const f32 ty0 = h > 0.0f ? (y1 - cy1) / h : 0.0f; // from the top
                const f32 ty1 = h > 0.0f ? (y1 - cy0) / h : 1.0f;

                const f32 nu0 = u_left + (u_right - u_left) * tx0;
                const f32 nu1 = u_left + (u_right - u_left) * tx1;
                const f32 nv0 = v_top + (v_bottom - v_top) * ty0;
                const f32 nv1 = v_top + (v_bottom - v_top) * ty1;

                q[0].texture_color = {nu0, nv0};
                q[1].texture_color = {nu1, nv0};
                q[2].texture_color = {nu1, nv1};
                q[3].texture_color = {nu0, nv1};
            }

            q[0].position.x = cx0; q[0].position.y = cy1;
            q[1].position.x = cx1; q[1].position.y = cy1;
            q[2].position.x = cx1; q[2].position.y = cy0;
            q[3].position.x = cx0; q[3].position.y = cy0;

            i += 4;

            continue;
        }

        // Other geometry: clamp the vertex
        Vertex3DSamplerTextureColor* const p = v + i;
        if (p->position.x >= clip.left && p->position.x <= clip.right
            && p->position.y >= clip.bottom && p->position.y <= clip.top
        ) {
            visible = true;
        }

        p->position.x = clamp_branched(p->position.x, clip.left, clip.right);
        p->position.y = clamp_branched(p->position.y, clip.bottom, clip.top);

        ++i;
    }

    return visible;
}

/**
 * Clips everything that was created since vertex_start/index_start
 * Completely invisible geometry is removed again
 */
static inline
void ui_vertices_clip_or_remove(
    ArrayVector<Vertex3DSamplerTextureColor>* const vertices,
    ArrayVector<int32>* const indices,
    int32 vertex_start, int32 index_start,
    const UIClipRect& clip
) NO_EXCEPT
{
    if ((int32) vertices->count <= vertex_start) {
        return;
    }

    if (!ui_vertices_clip(vertices, vertex_start, clip)) {
        vertices->count = vertex_start;
        indices->count = index_start;
    }
}

#endif
