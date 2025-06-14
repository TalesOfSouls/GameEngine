/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_C
#define COMS_MODELS_MOB_C

#include "Mob.h"
#include "MobState.h"

void mob_interpolate(Mob* mob, f32 time)
{
    MobState *s1 = &mob->state1;
    MobState *s2 = &mob->state2;

    float t1 = s2->t - s1->t;
    float t2 = time - s2->t;

    t1 = OMS_MIN(t1, 1.0f);
    t1 = OMS_MAX(t1, 0.1f);

    float p = OMS_MIN(t2 / t1, 1.0f);

    mob->state.location.position.x = s1->location.position.x + (s2->location.position.x - s1->location.position.x) * p;
    mob->state.location.position.y = s1->location.position.y + (s2->location.position.y - s1->location.position.y) * p;
    mob->state.location.position.z = s1->location.position.z + (s2->location.position.z - s1->location.position.z) * p;

    mob->state.location.orientation.x = s1->location.orientation.x + (s2->location.orientation.x - s1->location.orientation.x) * p;
    mob->state.location.orientation.y = s1->location.orientation.y + (s2->location.orientation.y - s1->location.orientation.y) * p;
    mob->state.location.orientation.z = s1->location.orientation.z + (s2->location.orientation.z - s1->location.orientation.z) * p;
    mob->state.location.orientation.w = s1->location.orientation.w + (s2->location.orientation.w - s1->location.orientation.w) * p;
}

#endif