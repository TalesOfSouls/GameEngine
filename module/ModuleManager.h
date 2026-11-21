#pragma once
#ifndef COMS_MODULE_MANAGER_H
#define COMS_MODULE_MANAGER_H

#include "Module.h"
#include "../memory/RingMemory.cpp"
#include "../system/FileUtils.cpp"

#if _WIN32
    #include "../platform/win32/UtilsWin32.h"
#elif __linux__
#endif

struct ModuleManager {
    int module_count;
    Module* modules;
};

void module_file_parse(const char* path, Module* module, RingMemory* const ring)
{
    FileBody file = {0};
    file.content = memory_get(ring, MEGABYTE * 1);
    file_read(path, &file);

    char* line = file.content;

    const int32 MAX_LENGTH = 128;
    char name[MAX_LENGTH];
    char value[MAX_LENGTH];

    while (line != NULL) {
        const char* space = str_find(line, ' ');
        if (space) {
            const size_t name_length = space - line;
            str_copy(name, line, OMS_MIN((int32) name_length, MAX_LENGTH - 1));
            str_copy(value, space + 1, MAX_LENGTH - 1);

            if (str_compare(name, "name") == 0) {
                str_copy(module->name, value, (int32) sizeof(module->name) - 1);
            } else if (str_compare(name, "version") == 0) {
                module->version = (byte) str_to_int(value);
            } else if (str_compare(name, "type") == 0) {
                module->type = (ModuleType) str_to_int(value);
            }
        }

        line = str_move_past(line, '\n');
    }
}

#endif