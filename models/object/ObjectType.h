/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_Object_TYPE_H
#define COMS_MODELS_Object_TYPE_H

#include "../../config.h"
#include "../../../stdlib/Types.h"

struct ObjectType {
    byte id;
    byte destructable;
    bool climbable;
    bool collectable;
    bool drops;
    bool changable;
};

#endif