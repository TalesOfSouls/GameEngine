/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PATHFINDING_JPS_H
#define COMS_PATHFINDING_JPS_H

#include <stdio.h>
#include <stdlib.h>

#include "../../stdlib/Types.h"

#include "JpsGrid.h"
#include "../Path.h"

void jps_find_path(
    v3_int32 start, v3_int32 end,
    const JpsGrid* grid, Path* path,
    int32 heuristic, int32 movement
) {
    JpsNode* start_node = &grid->nodes[
        grid->dimension.x * grid->dimension.y * start.z
        + grid->dimension.x * start.y
        + start.x
    ];

    JpsNode* end_node = &grid->nodes[
        grid->dimension.x * grid->dimension.y * end.z
        + grid->dimension.x * end.y
        + end.x
    ];

    if (!start_node->is_walkable || !end_node->is_walkable) {
        return;
    }
}
#endif