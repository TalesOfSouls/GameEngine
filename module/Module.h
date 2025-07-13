#ifndef COMS_MODULE_H
#define COMS_MODULE_H

#include "../stdlib/Types.h"
#include "../../cOMS/system/Library.h"

enum ModuleType {
    MODULE_TYPE_HUD,
    MODULE_TYPE_UI,
    MODULE_TYPE_WINDOW, // Additional window
    MODULE_TYPE_API, // Extracts data and sends it somewhere (website, file, etc.)
};

struct Module {
    char name[128];
    int32 version;
    ModuleType type;
    bool is_active;

    Library lib;
};

#endif