#ifndef COMS_MODULE_H
#define COMS_MODULE_H

#include "../stdlib/Stdlib.h"
#include "../../cOMS/system/Library.h"

enum ModuleType {
    MODULE_TYPE_HUD,
    MODULE_TYPE_UI,
    MODULE_TYPE_WINDOW, // Additional window
    MODULE_TYPE_API, // Extracts data and sends it somewhere (website, file, etc.)
};

struct Module {
    wchar_t name[64];
    int32 version;
    ModuleType type;
    bool is_active;

    Library lib;
    void* table;
};

#endif