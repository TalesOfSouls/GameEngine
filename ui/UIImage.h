#pragma once
#ifndef COMS_UI_IMAGE_H
#define COMS_UI_IMAGE_H

#include "../stdlib/Stdlib.h"

struct UIImage {
    UICore core;
    int32 asset_id;
    int32 sampler;
};

#endif