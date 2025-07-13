/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ANIMATION_ENTITY_H
#define COMS_ANIMATION_ENTITY_H

#include "../stdlib/Types.h"
#include "../animation/AnimationEaseType.h"
#include "../animation/Animation.h"
#include "../utils/BitUtils.h"
#include "EntityComponentSystem.h"
#include "Entity.h"

enum AnimationSetting {
    ANIMATION_SETTING_PAUSE = 1 << 0,
    ANIMATION_SETTING_REPEAT = 1 << 1,
};

struct AnimationEntityComponent {
    Entity* entity;
    AnimationEaseType type;
    uint32 start_time;
    uint32 last_time;
    f32 interval;
    f32 progress;
    byte state_last;
    byte state;

    // Contains repeat, pause etc
    byte setting;

    // @question Do we want another flag that indicates if the entity got handled by the main loop?
    // this way we could do the animation process in a thread and only overwrite the state_last whenever the flag is true
    // However, we would have to implement locking or atomics which might be really bad depending on how we use this data
};

static inline
void update_animation_entity(AnimationEntityComponent* anim, uint32 time, uint32 delay)
{
    anim->state_last = anim->state;

    switch (anim->type) {
        case ANIMATION_EASE_DISCRETE: {
                anim->progress = anim_discrete((f32) (time - anim->start_time + delay) / (f32) anim->interval);
                anim->state = (byte) ((f32) anim->state - anim->progress + FLOAT_CAST_EPS);
            } break;
        default: {}
    }
}

void update_animation_entities(ChunkMemory* anim_ec, uint32 time, uint32 delay)
{
    uint32 chunk_id = 0;
    chunk_iterate_start(anim_ec, chunk_id) {
        AnimationEntityComponent* anim = (AnimationEntityComponent *) chunk_get_element(anim_ec, chunk_id);
        if (anim->setting & ANIMATION_SETTING_PAUSE) {
            continue;
        }

        update_animation_entity(anim, time, delay);
    } chunk_iterate_end;
}

#endif