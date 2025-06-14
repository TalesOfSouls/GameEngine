/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_OBJ_H
#define COMS_MODELS_OBJ_H

#include "../stdlib/Types.h"

struct ObjFile {
    // Amount of references to this object
    int32 references;

    bool facing; // front or back

    void* buffer;
    void* facingness;
    void* normals;
    void* uvs;
    void* parallaxs;

    void* animations;
    void* textures; // could be texture maps
    void* mipmaps;
    void* samplers;
    void* materials;
};

#endif