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

#include "../../../stdlib/Stdlib.h"

// Object coordinates are relative to the chunk coordinates
struct Object {
    float x;
    float y;
    float z;

    // Defined in obj_type in the database
    uint16 type;

    // ObjectFlag
    uint8 flag;

    // e.g. ObjectInteractionFlag
    uint8 flag_sub;
};

#endif