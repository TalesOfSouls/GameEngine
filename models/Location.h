/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_LOCATION_H
#define COMS_MODELS_LOCATION_H

#include "../stdlib/Types.h"

struct Location {
    v3_f32 position;
    v4_f32 orientation;
};

#endif