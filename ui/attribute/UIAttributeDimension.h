#ifndef COMS_UI_ATTRIBUTE_DIMENSION_H
#define COMS_UI_ATTRIBUTE_DIMENSION_H

#include "../../stdlib/Types.h"

enum UIDimensionFlag : byte {
    // Are the values relative (based on container) or absolute
    UI_DIMENSION_POS_RELATIVE = 1 << 0,
    UI_DIMENSION_DIM_RELATIVE = 1 << 1,

    // Are the values dynamically calculated?
    UI_DIMENSION_POS_DYN = 1 << 2,
    UI_DIMENSION_DIM_DYN = 1 << 3,

    // Are the values modifiable by the user (live)
    UI_DIMENSION_POS_MODIFIABLE = 1 << 4,
    UI_DIMENSION_DIM_MODIFIABLE = 1 << 5,
};

struct UIAttributeDimension {
    // @see UIDimensionFlag
    byte flag;

    // @see UIAlign
    byte alignment;

    v4_f32 dimension;

    /*
    // We commented this out since we will try to work around it for now by simply reloading the UI,
    // whenever a in-game window gets resized, same as resizing the actual game window
    union {
        struct {
            f32 x, y;
            f32 width, height;
        };

        // We can never have position and dimension both be dynamic
        // This isn't really a technical limitation, it's more a "what happens in reality" kind of reason
        // This allows us to save 40 bytes
        struct {
            char x_str[24];
            char y_str[24];
            f32 width, height;
        };

        struct {
            f32 x, y;
            char width_str[24];
            char height_str[24];
        };
    };
    */
};

inline
void ui_attr_dimension_serialize(const UIAttributeDimension* __restrict dim, byte** __restrict pos)
{
    **pos = dim->flag;
    *pos += sizeof(dim->flag);

    **pos = dim->alignment;
    *pos += sizeof(dim->alignment);

    for (int32 i = 0; i < 4; ++i) {
        *((f32 *) *pos) = SWAP_ENDIAN_LITTLE(dim->dimension.vec[i]);
        *pos += sizeof(dim->dimension.vec[i]);
    }
}

inline
void ui_attr_dimension_unserialize(UIAttributeDimension* __restrict dim, const byte** __restrict pos)
{
    dim->flag = **pos;
    *pos += sizeof(dim->flag);

    dim->alignment = **pos;
    *pos += sizeof(dim->alignment);

    for (int32 i = 0; i < 4; ++i) {
        dim->dimension.vec[i] = SWAP_ENDIAN_LITTLE(*((f32 *) *pos));
        *pos += sizeof(dim->dimension.vec[i]);
    }
}

#endif