/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BLOCK_H
#define COMS_MODELS_BLOCK_H

#include "../../stdlib/Types.h"
#include "../../stdlib/SIMD/SIMD_I32.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Block
//
// Blocks are landscape blocks that just get rendered as is
// Blocks are very simple -> fast to render
// WARNING: No other block can be placed at its position!
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Block coordinates are relative to the chunk coordinates
// Block coordinates can also be only grid-based
// Therefore the x,y,z coordinates can be byte datatypes
struct Block {
    // 0 - x
    // 1 - y
    // 2 - z
    // 3 - rot1
    // 4 - rot2
    // 5 - type
    int32 v[6];
};

struct BlockDB {
    int32 x;
    int32 y;
    int32 z;

    int32 rot1;
    int32 rot2;

    int32 type;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BlockObject
//
// BlockObjectss are any object that fits on the grid.
// BlocksObjects are very simple -> faster to render than other objects.
// WARNING: No other block can be placed at its position!
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Block coordinates are relative to the chunk coordinates
// Block coordinates can also be only grid-based
// Therefore the x,y,z coordinates can be byte datatypes
struct BlockObject {
    // 0 - x
    // 1 - y
    // 2 - z
    // 3 - type
    // 4 - rotx
    // 5 - roty
    Stdlib::SIMD::int32_8_simd v;
};

struct BlockObjectDB {
    byte x;
    byte y;
    byte z;

    uint32 type;

    // first 4 bits = rotation
    //      4 possible x rotations
    //      4 possible y rotations
    //      -> 16 possible combinations
    //      -> 2^4 -> 4 bits
    //
    // last 4 bits = scale
    //      -> first bit defines positive or negative scale factor
    //      -> last 3 bits define scale factor
    //          -> 2^3 = 8 scale factors
    //              -> A block can be maximum 9 times its original size or 1/9th of it
    //              -> note that 0 is the original scale -> 1+8
    byte rotation_scale;
};

#endif