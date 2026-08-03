#pragma once
#ifndef COMS_UI_CURSOR_H
#define COMS_UI_CURSOR_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeImage.h"
#include "UICore.h"

struct UICursor {
    UICore core;
    UIAttributeImage image;
};

#endif