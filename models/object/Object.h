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

    // Defined in obj_type in the database
    uint16 type;

    // We only define this general state value here because of the available padding
    // Otherwise the space would be reserved due to padding but unused
    // In reality we often use special object specific states if needed
    // Sure we could reserve like 16 bytes of state information here but that would be a waste of memory in 90% of the cases
    // @question re-check if we can change the size of members here so we can remove it
    // int16 state;

    // ObjectFlag
    uint8 flag;

    // e.g. ObjectInteractionFlag
    uint8 flag_sub;
};

#endif