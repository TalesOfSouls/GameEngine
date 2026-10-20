#pragma once
#ifndef COMS_UI_SCROLLBAR_H
#define COMS_UI_SCROLLBAR_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "../object/Vertex.h"
#include "../gpuapi/RenderUtils.h"
#include "../gpuapi/GpuApiType.h"
#include "../camera/Camera.cpp"
#include "UIStyleDefaults.h"
#include "UIAlignment.h"

/**
 * Generic scrollbar geometry used by all scrollable elements (panel, textarea, option list, table)
 *
 * The elements don't store any scrollbar data, the scrollbar is fully defined by:
 *      track   = rect of the scrollbar (x, y = top left, width, height)
 *      view    = visible size along the axis
 *      content = total content size along the axis
 *      scroll  = current scroll offset (0 = start, max = content - view)
 * @see ui_element_scroll_info()
 */

enum UIScrollAxis : byte {
    UI_SCROLL_AXIS_X,
    UI_SCROLL_AXIS_Y,
};

struct UIScrollInfo {
    v4_f32 track;
    f32 view;
    f32 content;
    f32 scroll;
};

FORCE_INLINE
f32 ui_scroll_max(f32 content, f32 view) NO_EXCEPT
{
    return content > view ? content - view : 0.0f;
}

FORCE_INLINE
f32 ui_scroll_clamp(f32 scroll, f32 content, f32 view) NO_EXCEPT
{
    return clamp_branched(scroll, 0.0f, ui_scroll_max(content, view));
}

FORCE_INLINE
f32 ui_scrollbar_track_length(const UIScrollInfo* const info, UIScrollAxis axis) NO_EXCEPT
{
    return axis == UI_SCROLL_AXIS_Y ? info->track.height : info->track.width;
}

FORCE_INLINE
f32 ui_scrollbar_thumb_length(const UIScrollInfo* const info, UIScrollAxis axis) NO_EXCEPT
{
    const f32 track_length = ui_scrollbar_track_length(info, axis);
    if (info->content <= 0.0f) {
        return track_length;
    }

    const f32 length = max_branched(track_length * info->view / info->content, UI_SCROLLBAR_MIN_THUMB);

    return min_branched(length, track_length);
}

// Rect of the thumb (x, y = top left)
inline
v4_f32 ui_scrollbar_thumb(const UIScrollInfo* const info, UIScrollAxis axis) NO_EXCEPT
{
    const f32 track_length = ui_scrollbar_track_length(info, axis);
    const f32 thumb_length = ui_scrollbar_thumb_length(info, axis);

    const f32 max = ui_scroll_max(info->content, info->view);
    const f32 t = max > 0.0f ? clamp_branched(info->scroll / max, 0.0f, 1.0f) : 0.0f;
    const f32 pos = (track_length - thumb_length) * t;

    return axis == UI_SCROLL_AXIS_Y
        ? v4_f32{info->track.x, info->track.y - pos, info->track.width, thumb_length}
        : v4_f32{info->track.x + pos, info->track.y, thumb_length, info->track.height};
}

/**
 * How much the scroll offset changes if the thumb is moved by pointer_delta along the track
 * pointer_delta is positive towards the end of the track (right/down)
 */
inline
f32 ui_scrollbar_pointer_to_scroll(const UIScrollInfo* const info, UIScrollAxis axis, f32 pointer_delta) NO_EXCEPT
{
    const f32 free_length = ui_scrollbar_track_length(info, axis) - ui_scrollbar_thumb_length(info, axis);
    if (free_length <= 0.0f) {
        return 0.0f;
    }

    return pointer_delta * ui_scroll_max(info->content, info->view) / free_length;
}

/**
 * Scroll offset that centers the thumb at the pointer position (e.g. click on the track)
 */
inline
f32 ui_scrollbar_scroll_at(const UIScrollInfo* const info, UIScrollAxis axis, f32 x, f32 y) NO_EXCEPT
{
    const f32 thumb_length = ui_scrollbar_thumb_length(info, axis);

    // Distance from the start of the track (towards the end)
    const f32 pos = axis == UI_SCROLL_AXIS_Y
        ? info->track.y - y
        : x - info->track.x;

    return ui_scroll_clamp(
        ui_scrollbar_pointer_to_scroll(info, axis, pos - thumb_length * 0.5f),
        info->content, info->view
    );
}

FORCE_INLINE
bool ui_scrollbar_contains(const UIScrollInfo* const info, f32 x, f32 y) NO_EXCEPT
{
    return x >= info->track.x && x < info->track.x + info->track.width
        && y <= info->track.y && y > info->track.y - info->track.height;
}

/**
 * Track + thumb
 * WARNING: Overlapping geometry needs its own zindex (the depth test rejects equal depth values)
 */
inline
void ui_scrollbar_vertices(
    const UIScrollInfo* const info, UIScrollAxis axis, bool active,
    GpuApiType gpu_api_type,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices,
    ArrayVector<int32>* const __restrict indices,
    f32* const zindex
) NO_EXCEPT
{
    vertex_rect_create(
        vertices, indices, *zindex, 0,
        info->track, UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        UI_DEFAULT_SCROLLBAR_TRACK
    );

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    vertex_rect_create(
        vertices, indices, *zindex, 0,
        ui_scrollbar_thumb(info, axis), UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        active ? UI_DEFAULT_SCROLLBAR_THUMB_ACTIVE : UI_DEFAULT_SCROLLBAR_THUMB
    );

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
