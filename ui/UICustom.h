#ifndef COMS_UI_CUSTOM_H
#define COMS_UI_CUSTOM_H

#include "../stdlib/Types.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeDimension.h"
#include "UIStyleType.h"
#include "UILayout.h"
#include "UIElement.h"
#include "../math/Evaluator.h"

struct UICustomState {
};

struct UICustom {

};

void ui_custom_state_serialize(const UICustomState* __restrict state, byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_custom_state_unserialize(UICustomState* __restrict state, const byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_custom_state_populate(const UIAttributeGroup* __restrict group, UICustomState* __restrict state)
{
    (void *) group;
    (void *) state;
}

void ui_custom_element_serialize(const UICustom* __restrict details, byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_custom_element_unserialize(UICustom* __restrict details, const byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_custom_element_populate(
    const UIAttributeGroup* __restrict  group,
    UIElement* __restrict element,
    UIStyleType style_type,
    EvaluatorVariable* __restrict variables
) {
    (void *) group;
    (void *) element;
    (void) style_type;
    (void *) variables;
}

int32 ui_custom_element_update(UILayout* layout, UIElement* element)
{
    (void*) layout;
    (void*) element;
    return 0;
}

#endif