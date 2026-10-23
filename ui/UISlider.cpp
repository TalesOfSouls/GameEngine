#pragma once
#ifndef COMS_UI_SLIDER_C
#define COMS_UI_SLIDER_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UICore.cpp"
#include "UISlider.h"

#define UI_SLIDER_KNOB_WIDTH 10.0f
#define UI_SLIDER_TRACK_HEIGHT 4.0f

/**
 * @param parent NULL = root
 */
inline
UISlider* ui_slider_create(UILayout* const layout, f32 value = 0.0f, UICore* parent = NULL) NO_EXCEPT
{
    UISlider* const slider = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_SLIDER, UISlider, parent);
    slider->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    slider->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    slider->core.dimension.dim_raw = {150.0f, 16.0f};

    slider->track_color = UI_DEFAULT_CONTROL_BORDER;
    slider->fill_color = UI_DEFAULT_CONTROL_ACCENT;
    slider->knob_color = 0xDDDDDDFF;
    slider->knob_color_hover = 0xFFFFFFFF;
    slider->value = value;

    return slider;
}

/**
 * @return true if the value changed
 */
inline
bool ui_slider_set(UILayout* const layout, UISlider* const slider, f32 value) NO_EXCEPT
{
    value = clamp_branched(value, 0.0f, 1.0f);

    if (slider->step > 0.0f) {
        value = ((f32) (int32) (value / slider->step + 0.5f)) * slider->step;
        value = clamp_branched(value, 0.0f, 1.0f);
    }

    if (value == slider->value) {
        return false;
    }

    slider->value = value;
    slider->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Sets the value from a pointer x position (ui coordinates)
 *
 * @return true if the value changed
 */
inline
bool ui_slider_set_from_position(UILayout* const layout, UISlider* const slider, f32 x) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &slider->core.dimension;
    const f32 range = dim->dim.x - UI_SLIDER_KNOB_WIDTH;
    if (range <= 0.0f) {
        return false;
    }

    return ui_slider_set(layout, slider, (x - dim->pos.x - UI_SLIDER_KNOB_WIDTH * 0.5f) / range);
}

void ui_slider_cache(
    UISlider* const slider,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_SLIDER);

    ui_dimension_calculate(layout, &slider->core);
    const UIAttributeDimension* const dim = &slider->core.dimension;

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const f32 track_y = dim->pos.y - (dim->dim.y - UI_SLIDER_TRACK_HEIGHT) * 0.5f;
    const f32 knob_x = dim->pos.x + (dim->dim.x - UI_SLIDER_KNOB_WIDTH) * slider->value;

    ui_rect_vertices({dim->pos.x, track_y, dim->dim.x, UI_SLIDER_TRACK_HEIGHT}, slider->track_color, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    ui_rect_vertices(
        {dim->pos.x, track_y, knob_x - dim->pos.x + UI_SLIDER_KNOB_WIDTH * 0.5f, UI_SLIDER_TRACK_HEIGHT},
        slider->fill_color, vertices, indices, *zindex
    );
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    const uint32 knob_color = (slider->core.state & (UI_STATE_HOVER | UI_STATE_ACTIVE))
        ? slider->knob_color_hover
        : slider->knob_color;

    ui_rect_vertices({knob_x, dim->pos.y, UI_SLIDER_KNOB_WIDTH, dim->dim.y}, knob_color, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
