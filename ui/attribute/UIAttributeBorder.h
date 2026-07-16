#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"
#include "../../gpuapi/RenderUtils.h"
#include "../../camera/Camera.cpp"
#include "../UIAlignment.h"
#include "UIAttributeBorder.h"

enum UIBorderType {
    UI_BORDER_TL,
    UI_BORDER_T,
    UI_BORDER_TR,
    UI_BORDER_R,
    UI_BORDER_BR,
    UI_BORDER_B,
    UI_BORDER_BL,
    UI_BORDER_L,
};

struct UIAttributeBorder {
    uint32 color;
    UIAttributeImage image;
};

#endif