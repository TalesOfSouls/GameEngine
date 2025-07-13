/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_SETTING_H
#define COMS_AUDIO_SETTING_H

#include "../stdlib/Types.h"
#include "../math/matrix/MatrixFloat32.h"

enum SoundApiType : byte {
    SOUND_API_TYPE_DIRECT_SOUND,
    SOUND_API_TYPE_XAUDIO2,
    SOUND_API_TYPE_WASAPI,
};

struct AudioSetting {
    f32 master_volume;

    // bits per sample
    // usually 48000 or 44100
    uint16 sample_rate;

    // This sample index is used to calculate the position in a ring buffer
    uint16 sample_index;

    // bytes per bloc
    // channel count * bit
    // usually 2 * 16 = 4
    byte sample_size;

    // how often has the audio_play been called (required for xaudio to switch between 2 buffers)
    byte sample_output;

    // max buffer content/size
    uint32 buffer_size;

    // Actual samples inside the buffer
    // The buffer could be larger than the data to output
    uint32 sample_buffer_size;
    int16* buffer;

    SoundApiType type;
    byte latency;
};

struct AudioLocationSetting {
    v3_f32 audio_location;
    v3_f32 audio_lookat;
    v3_f32 audio_moveto;
    f32 audio_velocity;
};

inline
void update_audio_location_settings(AudioLocationSetting* settings)
{
    vec3_normalize(&settings->audio_lookat);
}

#endif
