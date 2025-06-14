/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PATHFINDING_JPSP_H
#define COMS_PATHFINDING_JPSP_H

#include <stdio.h>
#include <stdlib.h>

#include "../stdlib/Types.h"

struct JPSNode {
    int32 x, y;
    f32 cost;
};

struct JPSPath {
    JPSNode node;
    JPSPath* next;
};

struct JPSMinHeap {
    int32 size;
    JPSNode* nodes;
};

struct JPSPGrid {
    int32 size;
    int32* points;
    bool* visited;
};

inline
void jpsp_swap(JPSNode* a, JPSNode* b) {
    JPSNode temp = *a;
    *a = *b;
    *b = temp;
}

void jpsp_push(JPSMinHeap* heap, JPSNode node) {
    int32 i = heap->size++;
    heap->nodes[i] = node;

    while (i != 0 && heap->nodes[(i - 1) / 2].cost > heap->nodes[i].cost) {
        jpsp_swap(&heap->nodes[i], &heap->nodes[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void jpsp_pop(JPSMinHeap* heap, JPSNode* root) {
    heap->nodes[0] = heap->nodes[--heap->size];
    int32 i = 0;

    while (2 * i + 1 < heap->size) {
        int32 child = 2 * i + 1;
        if (child + 1 < heap->size && heap->nodes[child].cost > heap->nodes[child + 1].cost) {
            child++;
        }

        if (heap->nodes[i].cost <= heap->nodes[child].cost) {
            break;
        }

        jpsp_swap(&heap->nodes[i], &heap->nodes[child]);
        i = child;
    }
}

bool jpsp_is_valid(int32 x, int32 y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE && grid[x][y] == 0;
}

JPSNode jpsp_jump(int32 x, int32 y, int32 px, int32 py, JPSNode* goal) {
    int32 dx = x - px;
    int32 dy = y - py;
    if (!jpsp_is_valid(x, y)) {
        return (JPSNode) { -1, -1, INF };
    }

    if (x == goal->x && y == goal->y) {
        return (JPSNode) { x, y, 0 };
    }

    // Check forced neighbors
    if ((dx != 0 && jpsp_is_valid(x + dx, y) && !jpsp_is_valid(x + dx, y - dy))
        || (dy != 0 && jpsp_is_valid(x, y + dy) && !jpsp_is_valid(x - dx, y + dy))
    ) {
        return (JPSNode) { x, y, 0 };
    }

    // Recursive jump in the direction (dx, dy)
    if (dx != 0 && dy != 0) {
        JPSNode j1 = jpsp_jump(x + dx, y, x, y, goal);
        JPSNode j2 = jpsp_jump(x, y + dy, x, y, goal);

        if (j1.x != -1 || j2.x != -1) {
            return (JPSNode) { x, y, 0 };
        }
    }

    return jpsp_jump(x + dx, y + dy, x, y, goal);
}

void jpsp_preprocess() {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (grid[x][y] == 0) {
                visited[x][y] = true;
            }
        }
    }
}

bool jpsp_plus_search(int startX, int startY, int goalX, int goalY) {
    preprocess();
    MinHeap openSet = { .size = 0 };
    push(&openSet, (Node) { startX, startY, 0 });

    while (openSet.size) {
        Node current = pop(&openSet);

        if (current.x == goalX && current.y == goalY) {
            printf("Path found to goal (%d, %d)\n", goalX, goalY);
            return true;
        }

        for (int i = 0; i < 8; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            if (isValid(nx, ny)) {
                Node jp = jump(nx, ny, current.x, current.y, (Node) { goalX, goalY, 0 });
                if (jp.x != -1 && !visited[jp.x][jp.y]) {
                    visited[jp.x][jp.y] = true;
                    double new_cost = current.cost + heuristic(current.x, current.y, jp.x, jp.y);
                    push(&openSet, (Node) { jp.x, jp.y, new_cost });
                }
            }
        }
    }
    return false;
}

#endif