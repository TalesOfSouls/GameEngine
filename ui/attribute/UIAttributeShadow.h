#ifndef COMS_UI_ATTRIBUTE_SHADOW_H
#define COMS_UI_ATTRIBUTE_SHADOW_H

#include "../../stdlib/Stdlib.h"

struct UIAttributeShadow {
    f32 angle;
    uint32 color;
    byte fade;
    byte offset;
};

inline
void ui_attr_shadow_serialize(const UIAttributeShadow* __restrict shadow, byte** __restrict pos)
{
    f32 tempf32 = SWAP_ENDIAN_LITTLE(shadow->angle);
    memcpy(*pos, &tempf32, sizeof(tempf32));
    *pos += sizeof(shadow->angle);

    uint32 temp32 = SWAP_ENDIAN_LITTLE(shadow->color);
    memcpy(*pos, &temp32, sizeof(temp32));
    *pos += sizeof(shadow->color);

    **pos = shadow->fade;
    *pos += sizeof(shadow->fade);

    **pos = shadow->offset;
    *pos += sizeof(shadow->offset);
}

inline
void ui_attr_shadow_unserialize(UIAttributeShadow* __restrict shadow, const byte** __restrict pos)
{
    memcpy(&shadow->angle, *pos, sizeof(shadow->angle));
    SWAP_ENDIAN_LITTLE_SELF(shadow->angle);
    *pos += sizeof(shadow->angle);

    memcpy(&shadow->color, *pos, sizeof(shadow->color));
    SWAP_ENDIAN_LITTLE_SELF(shadow->color);
    *pos += sizeof(shadow->color);

    shadow->fade = **pos;
    *pos += sizeof(shadow->fade);

    shadow->offset = **pos;
    *pos += sizeof(shadow->offset);
}

#endif