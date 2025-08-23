/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_OBJECT_TYPE_H
#define COMS_MODELS_OBJECT_TYPE_H

#include "../../../stdlib/Types.h"

enum ObjectFlag : uint8 {
    OBJECT_FLAG_CLIMBABLE = 1 << 0,
    OBJECT_FLAG_SWIMMABLE = 1 << 1,
    OBJECT_FLAG_NO_HITBOX = 1 << 2,
    OBJECT_FLAG_HITBOX = 1 << 3,
    OBJECT_FLAG_LIGHTSOURCE = 1 << 4,
    OBJECT_FLAG_INTERACTABLE = 1 << 5, // Object is dynamic (may disappear soon, e.g. dropped item)
    OBJECT_FLAG_DYNAMIC = 1 << 6, // Object is dynamic (may disappear soon, e.g. dropped item)
};

enum ObjectInteractionFlag : uint8 {
    OBJECT_INTERACTION_FLAG_DESTRUCTIBLE = 1 << 0,
    OBJECT_INTERACTION_FLAG_MOVABLE = 1 << 2,
    OBJECT_INTERACTION_FLAG_COLLECTABLE = 1 << 3,
    OBJECT_INTERACTION_FLAG_CHANGEABLE = 1 << 4,
};

#endif