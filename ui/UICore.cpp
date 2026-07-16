#pragma once
#ifndef COMS_UI_CORE_C
#define COMS_UI_CORE_C

#include "UICore.h"
#include "UILayout.h"

UICore* ui_parent_element_by_type(UICore* base, UIElementType type) NO_EXCEPT
{
    if (!base) {
        return NULL;
    }

    base = (UICore*) (((uintptr_t) base) - base->parent_offset);

    while (base->type != type) {
        if (!base->parent_offset) {
            return NULL;
        }

        base = (UICore*) (((uintptr_t) base) - base->parent_offset);
    }

    return base;
}

UICore* ui_parent_element_by_type(UICore* base, int32 type_flags) NO_EXCEPT
{
    if (!base) {
        return NULL;
    }

    base = (UICore*) (((uintptr_t) base) - base->parent_offset);

    while (true) {
        for (int i = 0; i < sizeof(type_flags) * 8; ++i) {
            if (!OMS_BIT_POS_IS_SET(type_flags, i)) {
                continue;
            }

            if (base->type == i) {
                return base;
            }
        }

        if (!base->parent_offset) {
            return NULL;
        }

        base = (UICore*) (((uintptr_t) base) - base->parent_offset);
    }
}

#endif