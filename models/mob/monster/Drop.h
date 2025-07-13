/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_DROP_H
#define COMS_MODELS_MOB_DROP_H

#include "../../../stdlib/Types.h"

struct Drop {
    uint64 item;
    uint32 quantity;
    // @todo Add item specifications (e.g. affixes)
};

#endif