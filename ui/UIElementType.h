#ifndef COMS_UI_ELEMENT_TYPE_H
#define COMS_UI_ELEMENT_TYPE_H

#include <stdio.h>
#include "../stdlib/Types.h"
#include "../compiler/CompilerUtils.h"
#include "UIButton.h"
#include "UISelect.h"
#include "UIInput.h"
#include "UIText.h"
#include "UILabel.h"
#include "UITextarea.h"
#include "UIImage.h"
#include "UILink.h"
#include "UIWindow.h"
#include "UITable.h"
#include "UIPanel.h"
#include "UITab.h"
#include "UICursor.h"
#include "UICustom.h"

constexpr
int32 ui_element_type_size(UIElementType e)
{
    switch (e) {
        case UI_ELEMENT_TYPE_BUTTON:
            return sizeof(UIButton);
        case UI_ELEMENT_TYPE_SELECT:
            return sizeof(UISelect);
        case UI_ELEMENT_TYPE_INPUT:
            return sizeof(UIInput);
        case UI_ELEMENT_TYPE_LABEL:
            return sizeof(UILabel);
        case UI_ELEMENT_TYPE_TEXT:
            return sizeof(UIText);
        case UI_ELEMENT_TYPE_TEXTAREA:
            return sizeof(UITextarea);
        case UI_ELEMENT_TYPE_IMAGE:
            return sizeof(UIImage);
        case UI_ELEMENT_TYPE_LINK:
            return sizeof(UILink);
        case UI_ELEMENT_TYPE_TABLE:
            return sizeof(UITable);
        case UI_ELEMENT_TYPE_VIEW_WINDOW:
            return sizeof(UIWindow);
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            return sizeof(UIPanel);
        case UI_ELEMENT_TYPE_VIEW_TAB:
            return sizeof(UITab);
        case UI_ELEMENT_TYPE_CURSOR:
            return sizeof(UICursor);
        case UI_ELEMENT_TYPE_CUSTOM:
            return sizeof(UICustom);
        case UI_ELEMENT_TYPE_MANUAL:
            return sizeof(UICustom);
        default: {
            UNREACHABLE();
        }
    }
}

constexpr
int32 ui_element_state_size(UIElementType e)
{
    switch (e) {
        case UI_ELEMENT_TYPE_BUTTON:
            return sizeof(UIButtonState);
        case UI_ELEMENT_TYPE_SELECT:
            return sizeof(UISelectState);
        case UI_ELEMENT_TYPE_INPUT:
            return sizeof(UIInputState);
        case UI_ELEMENT_TYPE_LABEL:
            return sizeof(UILabelState);
        case UI_ELEMENT_TYPE_TEXT:
            return sizeof(UITextState);
        case UI_ELEMENT_TYPE_TEXTAREA:
            return sizeof(UITextareaState);
        case UI_ELEMENT_TYPE_IMAGE:
            return sizeof(UIImageState);
        case UI_ELEMENT_TYPE_LINK:
            return sizeof(UILinkState);
        case UI_ELEMENT_TYPE_TABLE:
            return sizeof(UITableState);
        case UI_ELEMENT_TYPE_VIEW_WINDOW:
            return sizeof(UIWindowState);
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            return sizeof(UIPanelState);
        case UI_ELEMENT_TYPE_VIEW_TAB:
            return sizeof(UITabState);
        case UI_ELEMENT_TYPE_CURSOR:
            return sizeof(UICursorState);
        case UI_ELEMENT_TYPE_CUSTOM:
            return sizeof(UICustomState);
        case UI_ELEMENT_TYPE_MANUAL:
            return sizeof(UICustomState);
        default: {
            UNREACHABLE();
        }
    }
}

constexpr
int32 ui_element_type_to_id(const char* str)
{
    if (str_compare("button", str) == 0) {
        return UI_ELEMENT_TYPE_BUTTON;
    } else if (str_compare("select", str) == 0) {
        return UI_ELEMENT_TYPE_SELECT;
    } else if (str_compare("input", str) == 0) {
        return UI_ELEMENT_TYPE_INPUT;
    } else if (str_compare("textarea", str) == 0) {
        return UI_ELEMENT_TYPE_TEXTAREA;
    } else if (str_compare("image", str) == 0) {
        return UI_ELEMENT_TYPE_IMAGE;
    } else if (str_compare("label", str) == 0) {
        return UI_ELEMENT_TYPE_LABEL;
    } else if (str_compare("text", str) == 0) {
        return UI_ELEMENT_TYPE_TEXT;
    } else if (str_compare("link", str) == 0) {
        return UI_ELEMENT_TYPE_LINK;
    } else if (str_compare("table", str) == 0) {
        return UI_ELEMENT_TYPE_TABLE;
    } else if (str_compare("window", str) == 0) {
        return UI_ELEMENT_TYPE_VIEW_WINDOW;
    } else if (str_compare("panel", str) == 0) {
        return UI_ELEMENT_TYPE_VIEW_PANEL;
    } else if (str_compare("tab", str) == 0) {
        return UI_ELEMENT_TYPE_VIEW_TAB;
    } else if (str_compare("cursor", str) == 0) {
        return UI_ELEMENT_TYPE_CURSOR;
    } else if (str_compare("custom", str) == 0) {
        return UI_ELEMENT_TYPE_CUSTOM;
    } else if (str_compare("manual", str) == 0) {
        return UI_ELEMENT_TYPE_MANUAL;
    }

    ASSERT_SIMPLE(false);

    return -1;
}

#endif