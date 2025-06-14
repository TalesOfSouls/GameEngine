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

#include "../stdlib/Types.h"
#include "Audio.h"
#include "AudioSetting.h"
#include "../utils/Utils.h"
#include "../memory/ChunkMemory.h"
#include "../math/matrix/MatrixFloat32.h"
#include "../thread/Atomic.h"

#if DIRECT_SOUND
    #include "../platform/win32/audio/DirectSound.h"
#elif XAUDIO2
    #include "../platform/win32/audio/XAudio2.h"
#elif WASAPI
    #include "../platform/win32/audio/Wasapi.h"
#endif

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
    AudioLocationSetting origin;

    uint32 audio_size;
    byte* audio_data;

    uint64 effect;
    uint32 sample_index;
    byte channels;

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
    alignas(8) atomic_64 int32 state_new;

    uint64 effect;

    AudioSetting settings;
    AudioLocationSetting camera;

    #if DIRECT_SOUND
        DirectSoundSetting api_setting;
    #elif XAUDIO2
        XAudio2Setting api_setting;
    #elif WASAPI
        WasapiSetting api_setting;
    #endif

    // @todo Replace HWND with our own typedef for linux
    HWND window;

    int16* buffer_temp;

    // @todo add mutex for locking and create threaded functions
    // do we need a condition or semaphore?
    // Wait, why do we even need threading? Isn't the threading handled by the file loading
};

bool audio_mixer_is_active(AudioMixer* mixer) {
    if (mixer->state_old == AUDIO_MIXER_STATE_ACTIVE
        && atomic_get_relaxed(&mixer->state_new) == AUDIO_MIXER_STATE_ACTIVE
    ) {
        return true;
    }

    AudioMixerState mixer_state;
    if ((mixer_state = (AudioMixerState) atomic_get_relaxed(&mixer->state_new)) != mixer->state_old) {
        if (mixer->state_old == AUDIO_MIXER_STATE_UNINITIALIZED) {
            audio_load(
                mixer->window,
                &mixer->settings,
                &mixer->api_setting
            );

            mixer->state_old = AUDIO_MIXER_STATE_INACTIVE;
        }

        if (mixer_state == AUDIO_MIXER_STATE_ACTIVE) {
            audio_play(&mixer->settings, &mixer->api_setting);
            mixer_state = AUDIO_MIXER_STATE_ACTIVE;
        }
    }

    return (mixer->state_old = mixer_state) == AUDIO_MIXER_STATE_ACTIVE;
}

void audio_mixer_play(AudioMixer* mixer, int32 id, Audio* audio, AudioInstance* settings = NULL)
{
    int32 index = chunk_reserve(&mixer->audio_instances, 1);
    if (index < 0) {
        return;
    }

    AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, index);
    instance->id = id;
    instance->audio_size = audio->size;
    instance->audio_data = audio->data;
    instance->channels = audio->channels;

    if (settings) {
        memcpy(&instance->origin, &settings->origin, sizeof(AudioLocationSetting));
        instance->effect = settings->effect;
    }
}

void audio_mixer_play(AudioMixer* mixer, AudioInstance* settings)
{
    int32 index = chunk_reserve(&mixer->audio_instances, 1);
    if (index < 0) {
        return;
    }

    AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, index);
    memcpy(instance, settings, sizeof(AudioInstance));
}

void audio_mixer_play_unique(AudioMixer* mixer, int32 id, Audio* audio, AudioInstance* settings = NULL)
{
    for (uint32 i = 0; i < mixer->audio_instances.count; ++i) {
        // @performance We are not really utilizing chunk memory.
        // Maybe a simple array would be better
        // Or we need to use more chunk functions / maybe even create a chunk_iterate() function?
        AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == id) {
            return;
        }
    }

    audio_mixer_play(mixer, id, audio, settings);
}

void audio_mixer_play_unique(AudioMixer* mixer, AudioInstance* settings)
{
    for (uint32 i = 0; i < mixer->audio_instances.count; ++i) {
        // @performance We are not really utilizing chunk memory.
        // Maybe a simple array would be better
        // Or we need to use more chunk functions / maybe even create a chunk_iterate() function?
        AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == settings->id) {
            return;
        }
    }

    audio_mixer_play(mixer, settings);
}

void audio_mixer_remove(AudioMixer* mixer, int32 id)
{
    for (uint32 i = 0; i < mixer->audio_instances.count; ++i) {
        AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == id) {
            instance->id = 0;
            chunk_free_elements(&mixer->audio_instances, i);

            // No return, since we want to remove all instances
        }
    }
}

int32 apply_speed(int16* buffer, uint32 buffer_size, f32 speed) {
    if (speed == 1.0f) {
        return 0;
    }

    // Has to be multiple of 2 to ensure stereo is implemented correctly
    uint32 new_size = ROUND_TO_NEAREST((uint32) (buffer_size / speed), 2);

    // Speed up
    if (speed > 1.0f) {
        for (uint32 i = 0; i < new_size; ++i) {
            // @bug What if 2 consecutive values fall onto the same int index for stereo. This would break it.
            // The problem is, even by doing this as stereo calculation we would still have the same issue just not on the current value but the next loop
            uint32 src_index = (uint32) (i * speed);
            buffer[i] = buffer[src_index];
        }

        // A speed up reduces the sample_index -> we reduce the data in the buffer
        return new_size - buffer_size;
    }

    // Slow down
    for (int32 i = buffer_size - 1; i > 0; --i) {
        uint32 src_index = (uint32) (i * speed);
        buffer[i] = buffer[src_index];
    }

    return 0;
}

// @performance Whenever we handle left and right the same we could half the buffer_size
// This allows us to re-use existing helper variables without re-calculating them for the next loop (e.g. delay below)
// Or, if the multiplier is an int we can even perform the multiplication on int32 through casting instead of 2 operations on int16
// We might have to adjust some of the values to ensure correct multiplication if possible (e.g. feedback, intensity, ...)
// @todo We probably want to handle left and right channel differently to add some depth
void apply_echo(int16* buffer, uint32 buffer_size, f32 delay, f32 feedback, int32 sample_rate) {
    int32 delay_samples = (int32) (delay * sample_rate);
    for (uint32 i = delay_samples; i < buffer_size; ++i) {
        buffer[i] += (int16) (buffer[i - delay_samples] * feedback);
    }
}

// @todo We probably want to handle left and right channel differently to add some depth
void apply_reverb(int16* buffer, uint32 buffer_size, f32 intensity) {
    intensity *= 0.5f;
    for (uint32 i = 1; i < buffer_size; ++i) {
        buffer[i] += (int16) (buffer[i - 1] * intensity); // Simple reverb with decay
    }
}

void apply_cave(int16* buffer, uint32 buffer_size, int32 sample_rate) {
    f32 echo_delay = 0.1f; // Echo delay in seconds
    f32 feedback = 0.3f;  // Echo feedback level
    apply_echo(buffer, buffer_size, echo_delay, feedback, sample_rate);
    apply_reverb(buffer, buffer_size, 0.4f); // Add mild reverb
}

void apply_underwater(int16* buffer, uint32 buffer_size) {
    for (uint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) sinf(buffer[i] * 0.5f); // Dampen + distortion
    }
}

void apply_flanger(int16* buffer, uint32 buffer_size, f32 rate, f32 depth, int32 sample_rate) {
    f32 delay_samples = depth * sample_rate;
    f32 temp = OMS_TWO_PI * rate / sample_rate;

    for (uint32 i = 0; i < buffer_size; ++i) {
        uint32 delay = (uint32) (delay_samples * (0.5f + 0.5f * sinf(i * temp)));
        if (i >= delay) {
            buffer[i] += (int16) (buffer[i - delay] * 0.5f);
        }
    }
}

void apply_tremolo(int16* buffer, uint32 buffer_size, f32 rate, f32 depth, int32 sample_rate) {
    f32 temp = OMS_TWO_PI * rate / sample_rate;
    f32 temp2 = (1.0f - depth) + depth;

    for (uint32 i = 0; i < buffer_size; ++i) {
        f32 mod = temp2 * (0.5f + 0.5f * sinf(i * temp));
        buffer[i] = (int16) (buffer[i] * mod);
    }
}

void apply_distortion(int16* buffer, uint32 buffer_size, f32 gain) {
    for (uint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) tanh(buffer[i] * gain);
    }
}

void apply_chorus(int16* buffer, uint32 buffer_size, f32 rate, f32 depth, int32 sample_rate) {
    f32 temp = OMS_TWO_PI * rate / sample_rate;

    uint32 max_delay = (uint32) (depth * sample_rate);
    for (uint32 i = 0; i < buffer_size; ++i) {
        uint32 delay = (uint32) (max_delay * (0.5f + 0.5f * sinf(i * temp)));
        if (i >= delay) {
            buffer[i] += (int16) (buffer[i - delay] * 0.5f);
        }
    }
}

void apply_pitch_shift(int16* buffer, uint32 buffer_size, f32 pitch_factor) {
    for (uint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * pitch_factor);
    }
}

void apply_granular_delay(int16* buffer, uint32 buffer_size, f32 delay, f32 granularity, int32 sample_rate) {
    uint32 delay_samples = (uint32) (delay * sample_rate);
    uint32 limit = (uint32) (granularity * sample_rate);

    for (uint32 i = 0; i < buffer_size; ++i) {
        if (i % limit == 0 && i >= delay_samples) {
            buffer[i] += (int16) (buffer[i - delay_samples] * 0.6f);
        }
    }
}

void apply_frequency_modulation(int16* buffer, uint32 buffer_size, f32 mod_freq, f32 mod_depth, int32 sample_rate) {
    f32 temp = OMS_TWO_PI * mod_freq / sample_rate;
    for (uint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * sinf(i * temp) * mod_depth);
    }
}

void apply_stereo_panning(int16* buffer, uint32 buffer_size, f32 pan) {
    f32 left_gain = 1.0f - pan;
    f32 right_gain = pan;

    for (uint32 i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * left_gain);
        buffer[i + 1] = (int16) (buffer[i + 1] * right_gain);
    }
}

void apply_highpass(int16* buffer, uint32 buffer_size, f32 cutoff, int32 sample_rate) {
    f32 rc = 1.0f / (OMS_TWO_PI * cutoff);
    f32 dt = 1.0f / sample_rate;
    f32 alpha = rc / (rc + dt);
    f32 previous = buffer[0];
    f32 previous_output = buffer[0];

    for (uint32 i = 1; i < buffer_size; ++i) {
        f32 current = buffer[i];
        buffer[i] = (int16) (alpha * (previous_output + current - previous));
        previous = current;
        previous_output = buffer[i];
    }
}

void apply_lowpass(int16* buffer, uint32 buffer_size, f32 cutoff, int32 sample_rate) {
    f32 rc = 1.0f / (OMS_TWO_PI * cutoff);
    f32 dt = 1.0f / sample_rate;
    f32 alpha = dt / (rc + dt);
    f32 previous = buffer[0];

    for (uint32 i = 1; i < buffer_size; ++i) {
        buffer[i] = (int16) (previous + alpha * (buffer[i] - previous));
        previous = buffer[i];
    }
}

int32 mixer_effects_mono(AudioMixer* mixer, uint64 effect, int32 samples)
{
    int32 sound_sample_index = 0;

    if (effect & AUDIO_EFFECT_ECHO) {
        apply_echo(mixer->buffer_temp, samples * 2, 0.2f, 0.4f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_REVERB) {
        apply_reverb(mixer->buffer_temp, samples * 2, 0.3f);
    }

    if (effect & AUDIO_EFFECT_UNDERWATER) {
        apply_underwater(mixer->buffer_temp, samples * 2);
    }

    if (effect & AUDIO_EFFECT_CAVE) {
        apply_cave(mixer->buffer_temp, samples * 2, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_LOWPASS) {
        apply_lowpass(mixer->buffer_temp, samples * 2, 500.0f, mixer->settings.sample_rate); // Cutoff frequency 500
    }

    if (effect & AUDIO_EFFECT_HIGHPASS) {
        apply_highpass(mixer->buffer_temp, samples * 2, 2000.0f, mixer->settings.sample_rate); // Cutoff frequency 2 kHz
    }

    if (effect & AUDIO_EFFECT_FLANGER) {
        apply_flanger(mixer->buffer_temp, samples * 2, 0.25f, 0.005f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_TREMOLO) {
        apply_tremolo(mixer->buffer_temp, samples * 2, 5.0f, 0.8f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_DISTORTION) {
        apply_distortion(mixer->buffer_temp, samples * 2, 10.0f);
    }

    if (effect & AUDIO_EFFECT_CHORUS) {
        apply_chorus(mixer->buffer_temp, samples * 2, 0.25f, 0.005f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_PITCH_SHIFT) {
        apply_pitch_shift(mixer->buffer_temp, samples * 2, 1.2f); // Slight pitch increase
    }

    if (effect & AUDIO_EFFECT_GRANULAR_DELAY) {
        apply_granular_delay(mixer->buffer_temp, samples * 2, 0.1f, 0.2f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_FM) {
        apply_frequency_modulation(mixer->buffer_temp, samples * 2, 2.0f, 0.5f, mixer->settings.sample_rate);
    }

    if (effect & AUDIO_EFFECT_STEREO_PANNING) {
        apply_stereo_panning(mixer->buffer_temp, samples * 2, 0.5f);
    }

    /*
    if (effect & AUDIO_EFFECT_EASE_IN) {
        apply_ease_in(mixer->buffer_temp, samples * 2, 0.5f);
    }

    if (effect & AUDIO_EFFECT_EASE_IN) {
        apply_ease_out(mixer->buffer_temp, samples * 2, 0.5f);
    }
    */

    if (effect & AUDIO_EFFECT_SPEED) {
        sound_sample_index += apply_speed(mixer->buffer_temp, samples * 2, 1.0f);
    }

    return sound_sample_index;
}

int32 mixer_effects_stereo()
{
    return 0;
}

void audio_mixer_mix(AudioMixer* mixer, uint32 size) {
    PROFILE(PROFILE_AUDIO_MIXER_MIX);
    memset(mixer->settings.buffer, 0, size);

    mixer->settings.sample_buffer_size = 0;
    uint32 limit_max = size / mixer->settings.sample_size;

    bool has_location = !is_empty((byte *) &mixer->camera.audio_location, sizeof(mixer->camera.audio_location));

    f32 volume_scale = mixer->settings.master_volume * mixer->settings.master_volume;

    for (uint32 i = 0; i < mixer->audio_instances.count; ++i) {
        AudioInstance* sound = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (sound->id == 0) {
            continue;
        }

        uint32 limit = limit_max;

        // Compute the vector from the player to the sound's origin
        v3_f32 to_sound = {};
        f32 total_attenuation = 1.0f;
        bool has_origin = !is_empty((byte *) &sound->origin.audio_location, sizeof(sound->origin.audio_location));

        if (has_location && has_origin) {
            vec3_sub(&to_sound, &sound->origin.audio_location, &mixer->camera.audio_location);

            f32 distance = vec3_length(&to_sound);
            if (distance) {
                f32 distance_attenuation = OMS_MAX(0.0f, 1.0f - (distance / 50.0f));

                vec3_normalize(&to_sound);
                f32 alignment = vec3_dot(&mixer->camera.audio_lookat, &to_sound);
                f32 directional_attenuation = OMS_MAX(0.0f, alignment);

                total_attenuation = distance_attenuation * directional_attenuation;
            }
        }

        uint32 sound_sample_count = sound->audio_size / mixer->settings.sample_size;
        uint32 sound_sample_index = sound->sample_index;
        int16* audio_data = (int16 *) sound->audio_data;

        // Temporary buffer for effects processing
        // @performance If there are situations where only one file exists in the mixer that should be played we could directly write to
        // the output buffer improving the performance. Some of those mixers are: music, cinematic, ui
        // Careful, NOT voice since we will probably manually layer them according to their position?
        if (sound->channels == 1) {
            // We make it stereo
            for (uint32 j = 0; j < limit; ++j) {
                if (sound_sample_index >= sound_sample_count) {
                    if (!(sound->effect & AUDIO_EFFECT_REPEAT)) {
                        limit = j;
                        break;
                    }

                    sound_sample_index = 0;
                }

                // We could make the temp buffer stereo here but we later on have to touch the array anyways.
                // This way we can easily perform mixer effects on a mono output.
                mixer->buffer_temp[j] = (int16) (audio_data[sound_sample_index] * volume_scale * total_attenuation);

                ++sound_sample_index;

                // @performance Some adjustments could be made right here the question is if this is faster.
                // Probably depends on how likely the adjustment is to happen. Orientation effects are probably very likely.
            }

            // Apply effects based on sound's effect type
            if (sound->effect && sound->effect != AUDIO_EFFECT_REPEAT) {
                int32 sample_adjustment = mixer_effects_mono(mixer, sound->effect, sound_sample_index);
                sound_sample_index += sample_adjustment;
                limit += sample_adjustment;
            }
        } else {
            for (uint32 j = 0; j < limit; ++j) {
                if (sound_sample_index >= sound_sample_count) {
                    if (!(sound->effect & AUDIO_EFFECT_REPEAT)) {
                        limit = j;
                        break;
                    }

                    sound_sample_index = 0;
                }

                mixer->buffer_temp[j * 2] = (int16) (audio_data[sound_sample_index * 2] * volume_scale * total_attenuation);
                mixer->buffer_temp[j * 2 + 1] = (int16) (audio_data[sound_sample_index * 2 + 1] * volume_scale * total_attenuation);

                ++sound_sample_index;

                // @performance Some adjustments could be made right here the question is if this is faster.
                // Probably depends on how likely the adjustment is to happen. Orientation effects are probably very likely.
            }

            // Apply effects based on sound's effect type
            if (sound->effect && sound->effect != AUDIO_EFFECT_REPEAT) {
                int32 sample_adjustment = mixer_effects_stereo() / 2;;
                sound_sample_index += sample_adjustment;
                limit += sample_adjustment;
            }
        }

        // @bug if we use speed up effect, this value could be negative. Fix.
        sound->sample_index = sound_sample_index;

        // Add the processed sound to the output buffer
        if (sound->channels == 1) {
            // We turn it stereo here
            for (uint32 j = 0; j < limit; ++j) {
                mixer->settings.buffer[j * 2] += mixer->buffer_temp[j];
                mixer->settings.buffer[j * 2 + 1] += mixer->buffer_temp[j];
            }
        } else {
            for (uint32 j = 0; j < limit * 2; ++j) {
                mixer->settings.buffer[j] += mixer->buffer_temp[j];
            }
        }

        mixer->settings.sample_buffer_size = OMS_MAX(
            mixer->settings.sample_buffer_size,
            limit * mixer->settings.sample_size
        );
    }

    if (mixer->effect) {
        mixer_effects_stereo();
    }
}

#endif