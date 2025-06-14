/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_OBJECT_H
#define COMS_MODELS_OBJECT_H

#include "../../../stdlib/Types.h"

// Object coordinates are relative to the chunk coordinates
struct Object {
    float x;
    float y;
    float z;

    uint32 type; // defined in object_list
};

#endif