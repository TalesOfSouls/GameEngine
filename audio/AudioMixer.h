/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_MIXER_H
#define COMS_AUDIO_MIXER_H

#include "../stdlib/Stdlib.h"
#include "Audio.h"
#include "AudioSetting.h"
#include "AudioWrapper.h"
#include "../memory/ChunkMemory.h"

enum AudioEffect {
    AUDIO_EFFECT_NONE,
    AUDIO_EFFECT_ECHO = 1 << 0,
    AUDIO_EFFECT_REVERB = 1 << 1,
    AUDIO_EFFECT_UNDERWATER = 1 << 2,
    AUDIO_EFFECT_CAVE = 1 << 3,
    AUDIO_EFFECT_LOWPASS = 1 << 4,
    AUDIO_EFFECT_HIGHPASS = 1 << 5,
    AUDIO_EFFECT_FLANGER = 1 << 6,
    AUDIO_EFFECT_TREMOLO = 1 << 7,
    AUDIO_EFFECT_DISTORTION = 1 << 8,
    AUDIO_EFFECT_CHORUS = 1 << 9,
    AUDIO_EFFECT_PITCH_SHIFT = 1 << 10,
    AUDIO_EFFECT_GRANULAR_DELAY = 1 << 11,
    AUDIO_EFFECT_FM = 1 << 12,
    AUDIO_EFFECT_STEREO_PANNING = 1 << 13,
    AUDIO_EFFECT_EASE_IN = 1 << 14,
    AUDIO_EFFECT_EASE_OUT = 1 << 15,
    AUDIO_EFFECT_SPEED = 1 << 16,
    AUDIO_EFFECT_REPEAT = 1 << 17,
};

struct AudioInstance {
    int32 id;
    uint32 audio_size;

    alignas(8) AudioLocationSetting origin;

    byte* audio_data;

    uint64 effect;
    uint32 sample_index;
    sbyte channels;

    // @todo How to implement audio that is only supposed to be played after a certain other sound file is finished
    // e.g. queueing soundtracks/ambient noise
};

enum AudioMixerState {
    AUDIO_MIXER_STATE_UNINITIALIZED,
    AUDIO_MIXER_STATE_INACTIVE,
    AUDIO_MIXER_STATE_ACTIVE,
};

struct AudioMixer {
    ChunkMemory audio_instances;
    AudioMixerState state_old;
    atomic_32 int32 state_new;

    uint64 effect;

    AudioSetting settings;
    AudioLocationSetting camera;

    ApiAudioSetting api_setting;

    // Some platforms require the window data for audio
    void* platform_window;

    int16* buffer_temp;

    // @todo add mutex for locking and create threaded functions
    // do we need a condition or semaphore?
    // Wait, why do we even need threading? Isn't the threading handled by the file loading
};


#endif