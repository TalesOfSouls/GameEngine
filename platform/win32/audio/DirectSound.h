/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SOUND_DIRECT_SOUND_H
#define COMS_SOUND_DIRECT_SOUND_H

#include <mmeapi.h>
#include <dsound.h>

struct DirectSoundSetting {
    LPDIRECTSOUND8 direct_sound_handle;
    LPDIRECTSOUNDBUFFER primary_buffer;
    LPDIRECTSOUNDBUFFER secondary_buffer;
};

#endif