#ifndef COMS_SCENE_INFO_H
#define COMS_SCENE_INFO_H

#include "../stdlib/Types.h"
#include "../ui/UILayout.h"
#include "../ui/UITheme.h"

struct SceneInfo {
    // @todo Find a better way to handle state data, the problem is we need the maximum scene size
    // I'm pretty sure for the main scene we probably need a couple MB
    // We also cannot pull the SceneGeneral out since it has different sizes based on gpuapi, unless we force it to a fixed size
    byte data[8192];
    UILayout ui_layout;

    // This is scene specific theme data
    // We also have global theme data, which is not defined in here
    UIThemeStyle ui_theme;
};

#endif