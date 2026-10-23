#pragma once
#ifndef COMS_UI_ELEMENT_TYPE_C
#define COMS_UI_ELEMENT_TYPE_C

#include "../stdlib/Stdlib.h"
#include "UIElementType.h"
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
#include "UICheckbox.h"
#include "UISlider.h"
#include "UIColorPicker.h"

CONSTEXPR
int32 ui_element_type_size(UIElementType e) NO_EXCEPT
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
        case UI_ELEMENT_TYPE_CHECKBOX:
            return sizeof(UICheckbox);
        case UI_ELEMENT_TYPE_RADIO:
            return sizeof(UIRadio);
        case UI_ELEMENT_TYPE_SLIDER:
            return sizeof(UISlider);
        case UI_ELEMENT_TYPE_OPTION_LIST:
            return sizeof(UIOptionList);
        case UI_ELEMENT_TYPE_COLOR_PICKER:
            return sizeof(UIColorPicker);
        case UI_ELEMENT_TYPE_COLOR_PANEL:
            return sizeof(UIColorPanel);
        default: {
            UNREACHABLE();
        }
    }
}

// WARNING: The order matters since we only compare the prefix (e.g. textarea before text)
int32 ui_element_type_to_id(const char* str) NO_EXCEPT
{
    if (strncmp("button", str, sizeof("button") - 1) == 0) {
        return UI_ELEMENT_TYPE_BUTTON;
    } else if (strncmp("optionlist", str, sizeof("optionlist") - 1) == 0) {
        return UI_ELEMENT_TYPE_OPTION_LIST;
    } else if (strncmp("colorpicker", str, sizeof("colorpicker") - 1) == 0) {
        return UI_ELEMENT_TYPE_COLOR_PICKER;
    } else if (strncmp("colorpanel", str, sizeof("colorpanel") - 1) == 0) {
        return UI_ELEMENT_TYPE_COLOR_PANEL;
    } else if (strncmp("select", str, sizeof("select") - 1) == 0) {
        return UI_ELEMENT_TYPE_SELECT;
    } else if (strncmp("input", str, sizeof("input") - 1) == 0) {
        return UI_ELEMENT_TYPE_INPUT;
    } else if (strncmp("textarea", str, sizeof("textarea") - 1) == 0) {
        return UI_ELEMENT_TYPE_TEXTAREA;
    } else if (strncmp("image", str, sizeof("image") - 1) == 0) {
        return UI_ELEMENT_TYPE_IMAGE;
    } else if (strncmp("label", str, sizeof("label") - 1) == 0) {
        return UI_ELEMENT_TYPE_LABEL;
    } else if (strncmp("text", str, sizeof("text") - 1) == 0) {
        return UI_ELEMENT_TYPE_TEXT;
    } else if (strncmp("link", str, sizeof("link") - 1) == 0) {
        return UI_ELEMENT_TYPE_LINK;
    } else if (strncmp("table", str, sizeof("table") - 1) == 0) {
        return UI_ELEMENT_TYPE_TABLE;
    } else if (strncmp("window", str, sizeof("window") - 1) == 0) {
        return UI_ELEMENT_TYPE_VIEW_WINDOW;
    } else if (strncmp("panel", str, sizeof("panel") - 1) == 0) {
        return UI_ELEMENT_TYPE_VIEW_PANEL;
    } else if (strncmp("tab", str, sizeof("tab") - 1) == 0) {
        return UI_ELEMENT_TYPE_VIEW_TAB;
    } else if (strncmp("cursor", str, sizeof("cursor") - 1) == 0) {
        return UI_ELEMENT_TYPE_CURSOR;
    } else if (strncmp("custom", str, sizeof("custom") - 1) == 0) {
        return UI_ELEMENT_TYPE_CUSTOM;
    } else if (strncmp("manual", str, sizeof("manual") - 1) == 0) {
        return UI_ELEMENT_TYPE_MANUAL;
    } else if (strncmp("checkbox", str, sizeof("checkbox") - 1) == 0) {
        return UI_ELEMENT_TYPE_CHECKBOX;
    } else if (strncmp("radio", str, sizeof("radio") - 1) == 0) {
        return UI_ELEMENT_TYPE_RADIO;
    } else if (strncmp("slider", str, sizeof("slider") - 1) == 0) {
        return UI_ELEMENT_TYPE_SLIDER;
    }

    ASSERT_THROW();

    return -1;
}

#endif
