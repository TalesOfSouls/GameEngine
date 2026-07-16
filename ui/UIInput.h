#pragma once
#ifndef COMS_UI_INPUT_H
#define COMS_UI_INPUT_H

#include "../stdlib/Stdlib.h"
#include "../camera/Camera.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeBorder.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeDimension.h"
#include "UIAnimation.h"
#include "UIStyleType.h"
#include "UICursor.h"
#include "UIWindow.h"
#include "UIPanel.h"
#include "UILayout.h"
#include "../gpuapi/RenderUtils.h"
#include "../object/Vertex.h"

enum UIInputType : byte {
    UI_INPUT_TYPE_TEXT,
    UI_INPUT_TYPE_NUMERIC,
    UI_INPUT_TYPE_DATE,
    UI_INPUT_TYPE_DATE_TIME,
    UI_INPUT_TYPE_TIME,
};

struct UIInputState {
    uint16 cursor_pos_x;
    uint16 cursor_pos_y;
    UIInputType type;
    int32 min_value;
    int32 max_value;
    uint16 max_input_length;
};

struct UIInput {
    UICore core;
    uint16 cursor_pos_x;
    uint16 cursor_pos_y;
    UIInputType type;
    int32 min_value;
    int32 max_value;
    uint16 max_input_length;
    CharType char_type;
    char* content;

    uint32 background_color;
    UIAttributeBorder border;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;
};

#endif