#ifndef COMS_MODULE_MANAGER_H
#define COMS_MODULE_MANAGER_H

#include "Module.h"
#include "../memory/RingMemory.h"
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
    file.content = ring_get_memory(ring, MEGABYTE * 1);
    file_read(path, &file);

    char *rest = NULL;
    char *line = strtok_r((char *) file.content, "\n", &rest);

    const int32 MAX_LENGTH = 128;
    char name[MAX_LENGTH];
    char value[MAX_LENGTH];

    while (line != NULL) {
        const char* space = strchr(line, ' ');
        if (space) {
            const size_t name_length = space - line;
            strncpy_s(name, MAX_LENGTH, line, name_length);
            name[name_length] = '\0';
            strncpy_s(value, MAX_LENGTH, space + 1, MAX_LENGTH - 1);
            value[MAX_LENGTH - 1] = '\0';

            if (strcmp(name, "name") == 0) {
                strncpy_s(module->name, MAX_LENGTH, value, sizeof(module->name) - 1);
                module->name[strlen(value)] = '\0';
            } else if (strcmp(name, "version") == 0) {
                module->version = (byte) atol(value);
            } else if (strcmp(name, "type") == 0) {
                module->type = (ModuleType) atol(value);
            }
        }

        line = strtok_r(rest, "\n", &rest);
    }
}

#endif