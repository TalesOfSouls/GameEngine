#ifndef COMS_MODULE_WEB_H
#define COMS_MODULE_WEB_H

#include "../system/Library.cpp"
#include "../log/DebugContainer.h"
#include "../memory/RingMemory.h"
#include "../../models/ApplicationSettings.h"

struct ApplicationData;

typedef void dll_load_module(DebugContainer*);
typedef void dll_load_functions(void*);

struct WebModule {
    dll_load_module* load_module;
    dll_load_functions* load_functions;
};

#endif