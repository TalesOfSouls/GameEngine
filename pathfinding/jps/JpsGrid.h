/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PATHFINDING_JPS_GRID_H
#define COMS_PATHFINDING_JPS_GRID_H

#include <stdio.h>
#include <stdlib.h>

#include "../../stdlib/Types.h"

#include "JpsNode.h"

struct JpsGrid {
    // 1. left to right
    // 2. then front to back
    // 3. then height
    JpsNode* nodes;

    v3_int32 dimension;
};

#endif