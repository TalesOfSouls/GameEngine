#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"
#include "UIAnimation.h"
#include "UICore.h"

struct UIWindowState {
};

struct UIWindow {
    UICore core;
    UIAnimation animation;
    byte padding;
    byte alignment;
    byte opacity;

    uintptr_t background;
    UIBackgroundStyle background_style;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;

    // @todo Remove, this is now in the UIOffset
    UIWindow* styles[UI_STYLE_TYPE_SIZE];
};

#endif