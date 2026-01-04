/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_DESCRIPTOR_SET_LAYOUT_BINDING_H
#define COMS_GPUAPI_SOFTWARE_DESCRIPTOR_SET_LAYOUT_BINDING_H

#include "../../stdlib/Stdlib.h"

struct SoftwareDescriptorSetLayoutBinding {
    // These pointers are never the owner of the data
    // The binding is 1 indexed, not 0 indexed
    // This makes it easier to check if a binding is set
    int32 binding;
    const char* name;

    int32 size;
    void* data;
};

template <typename T>
inline T* soft_layout_find(
    T* layouts, int32 length,
    const char* name
)
{
    for (int32 i = 0; i < length; ++i) {
        if (layouts[i].name && str_compare(layouts[i].name, name) == 0) {
            return &layouts[i];
        }
    }

    return NULL;
}

#endif