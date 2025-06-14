#ifndef COMS_ENVIRONMENT_GLOBE_H
#define COMS_ENVIRONMENT_GLOBE_H

#include "../stdlib/Types.h"
#include <math.h>

/**
 * Calculates the time of the day
 *
 * @param f64 time Can be any reference value, as long as it increments every second
 * @param f32 day_length The length of a day in seconds
 *
 * @return f32 A day time ranging from 0.0 to 1.0
 */
f32 time_of_day(f64 time, f32 day_length) {
    time /= day_length;
    time -= (int64) time;

    return (f32) time;
}

/**
 * Calculates a daylight intensity based on the time of the day
 *
 * @param f64 time Can be any reference value, as long as it increments every second
 * @param f32 day_length The length of a day in seconds
 *
 * @return f32 Light intensity
 */
f32 daylight_get(f64 time, f32 day_length) {
    f32 timer = time_of_day(time, day_length);

    if (timer < 0.5f) {
        f32 t = (timer - 0.25f) * 100.0f;

        return 1.0f / (1.0f + powf(2.0f, -t));
    }

    f32 t = (timer - 0.85f) * 100.0f;

    return 1.0f - 1.0f / (1.0f + powf(2.0f, -t));
}

void sun_position_get(v3_f32* sun_position, f32 time, f32 radius, f32 day_duration) {
    f32 angle = (time / day_duration) * 2.0f * OMS_PI;

    sun_position->x = radius * cosf(angle);
    sun_position->y = radius * sinf(angle);
    sun_position->z = 0.0f;
}

#endif