#ifndef COMS_UI_CURSOR_H
#define COMS_UI_CURSOR_H

#include "../stdlib/Types.h"
#include "attribute/UIAttributeDimension.h"
#include "UILayout.h"
#include "UIElement.h"

struct UICursorState {
};

struct UICursor {
    UIAttributeDimension dimension;
    byte opacity; // 1 byte alpha channel
};

int32 ui_cursor_element_update(UILayout* layout, UIElement* element)
{
    //UICursor* input = (UICursor *) (layout->data + element->style_types[element->style_new]);
    //UICursorState* state = (UICursorState *) (layout->data + element->state);

    (void *) layout;
    (void *) element;

    return 0;
}

#endif