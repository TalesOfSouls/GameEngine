#pragma once
#ifndef COMS_UI_CORE_C
#define COMS_UI_CORE_C

#include "UICore.h"
#include "UILayout.h"
#include "../utils/BitUtils.h"

UICore* ui_parent_element_by_type(byte* base, UICore* element, UIElementType type) NO_EXCEPT
{
    if (!element) {
        return NULL;
    }

    element = (UICore*) (((uintptr_t) base) + element->parent_offset);

    while (element->type != type) {
        if (!element->parent_offset) {
            return NULL;
        }

        element = (UICore*) (((uintptr_t) base) + element->parent_offset);
    }

    return element;
}

UICore* ui_parent_element_by_type(byte* base, UICore* element, int32 type_flags) NO_EXCEPT
{
    if (!element) {
        return NULL;
    }

    element = (UICore*) (((uintptr_t) base) + element->parent_offset);

    while (true) {
        for (int i = 0; i < sizeof(type_flags) * 8; ++i) {
            if (!OMS_BIT_SET(type_flags, i)) {
                continue;
            }

            if (element->type == i) {
                return element;
            }
        }

        if (!element->parent_offset) {
            return NULL;
        }

        element = (UICore*) (((uintptr_t) base) + element->parent_offset);
    }
}

UICore* ui_custom_create(UILayout* layout, int16 update_func, int16 render_func) NO_EXCEPT
{
    UICore* element = (UICore*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UICore);
    MEMORY_ELEMENT_ZERO(element);

    element->type = UI_ELEMENT_TYPE_CUSTOM;
    element->parent_offset = (int32) MEMORY_OFFSET(layout->ui_root, layout->ui_element_buffer.memory);
    element->dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;
    element->dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM;
    element->update_func = update_func;
    element->render_func = render_func;

    return element;
}

inline
void ui_chroma_codes_update(
    int32 id,
    UICore* const __restrict core,
    UIChromaCodes* const __restrict chroma_codes
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_CHROMA_CODES);

    const uint16 width = chroma_codes->width;
    uint32* codes = chroma_codes->codes;

    const int x0 = (int) core->dimension.pos.x;
    const int y0 = (int) core->dimension.pos.y;
    const int w  = (int) core->dimension.dim.x;
    const int h  = (int) core->dimension.dim.y;

    for (int y = 0; y < h; ++y) {
        uint32* dst = codes + (y0 + y) * width + x0;
        for (int x = 0; x < w; ++x) {
            *dst++ = id;
        }
    }
}

#endif