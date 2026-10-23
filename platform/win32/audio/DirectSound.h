/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SOUND_DIRECT_SOUND_H
#define COMS_SOUND_DIRECT_SOUND_H

#include <mmeapi.h>
#include <dsound.h>

#ifdef NOCTLMGR
    #error "NOCTLMGR Must not be defined"
#endif

#ifdef NOGDI
    #error "NOGDI Must not be defined"
#endif

struct DirectSoundSetting {
    LPDIRECTSOUND8 direct_sound_handle;
    LPDIRECTSOUNDBUFFER primary_buffer;
    LPDIRECTSOUNDBUFFER secondary_buffer;
};

#endif