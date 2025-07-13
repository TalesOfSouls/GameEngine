#ifndef COMS_UI_BUTTON_H
#define COMS_UI_BUTTON_H

#include "../stdlib/Types.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeDimension.h"
#include "UIStyleType.h"
#include "UILayout.h"
#include "UIElement.h"
#include "../math/Evaluator.h"

struct UIButtonState {
};

struct UIButton {
    UIAttributeDimension dimension;
};

void ui_button_state_serialize(const UIButtonState* __restrict state, byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_button_state_unserialize(UIButtonState* __restrict state, const byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_button_state_populate(const UIAttributeGroup* __restrict group, UIButtonState* __restrict state)
{
    (void *) group;
    (void *) state;
}

void ui_button_element_serialize(const UIButton* __restrict details, byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_button_element_unserialize(UIButton* __restrict details, const byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_button_element_populate(
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

int32 ui_button_element_update(UILayout* layout, UIElement* element)
{
    (void*) layout;
    (void*) element;
    return 0;
}

#endif