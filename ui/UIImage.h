#pragma once
#ifndef COMS_UI_IMAGE_H
#define COMS_UI_IMAGE_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeImage.h"
#include "UICore.h"

// 76 bytes
struct UIImage {
    UICore core;
    UIAttributeImage image;
};

#endif
