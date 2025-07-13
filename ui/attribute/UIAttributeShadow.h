#ifndef COMS_UI_ATTRIBUTE_SHADOW_H
#define COMS_UI_ATTRIBUTE_SHADOW_H

#include "../../stdlib/Types.h"

struct UIAttributeShadow {
    f32 angle;
    uint32 color;
    byte fade;
    byte offset;
};

inline
void ui_attr_shadow_serialize(const UIAttributeShadow* __restrict shadow, byte** __restrict pos)
{
    *((f32 *) *pos) = SWAP_ENDIAN_LITTLE(shadow->angle);
    *pos += sizeof(shadow->angle);

    *((uint32 *) *pos) = SWAP_ENDIAN_LITTLE(shadow->color);
    *pos += sizeof(shadow->color);

    **pos = shadow->fade;
    *pos += sizeof(shadow->fade);

    **pos = shadow->offset;
    *pos += sizeof(shadow->offset);
}

inline
void ui_attr_shadow_unserialize(UIAttributeShadow* __restrict shadow, const byte** __restrict pos)
{
    shadow->angle = SWAP_ENDIAN_LITTLE(*((f32 *) *pos));
    *pos += sizeof(shadow->angle);

    shadow->color = SWAP_ENDIAN_LITTLE(*((uint32 *) *pos));
    *pos += sizeof(shadow->color);

    shadow->fade = **pos;
    *pos += sizeof(shadow->fade);

    shadow->offset = **pos;
    *pos += sizeof(shadow->offset);
}

#endif