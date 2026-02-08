/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_MIXER_C
#define COMS_AUDIO_MIXER_C

#include "../stdlib/Stdlib.h"
#include "Audio.h"
#include "AudioSetting.h"
#include "AudioWrapper.h"
#include "../utils/Utils.h"
#include "../memory/ChunkMemory.h"
#include "../math/matrix/Matrix.h"
#include "../thread/Atomic.h"
#include "AudioMixer.h"
#include "AudioWrapper.cpp"

bool audio_mixer_is_active(AudioMixer* const mixer) NO_EXCEPT
{
    AudioMixerState mixer_state = (AudioMixerState) atomic_get_relaxed(&mixer->state_new);
    if (mixer->state_old == AUDIO_MIXER_STATE_ACTIVE
        && mixer_state == AUDIO_MIXER_STATE_ACTIVE
    ) {
        return true;
    }

    if (mixer_state != mixer->state_old) {
        if (mixer->state_old == AUDIO_MIXER_STATE_UNINITIALIZED) {
            audio_load(
                mixer->platform_window,
                &mixer->settings,
                &mixer->api_setting
            );

            mixer->state_old = AUDIO_MIXER_STATE_INACTIVE;
        }

        if (mixer_state == AUDIO_MIXER_STATE_ACTIVE) {
            audio_play(&mixer->settings, &mixer->api_setting);
            mixer_state = AUDIO_MIXER_STATE_ACTIVE;
        }

        // I don't think we need atomics here
        mixer->state_old = mixer_state;
    }

    return mixer_state == AUDIO_MIXER_STATE_ACTIVE;
}

void audio_mixer_play(AudioMixer* const mixer, int32 id, Audio* const audio, const AudioInstance* const settings = NULL) NO_EXCEPT
{
    int32 index = chunk_reserve_one(&mixer->audio_instances);
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

void audio_mixer_play(AudioMixer* const mixer, const AudioInstance* const settings) NO_EXCEPT
{
    int32 index = chunk_reserve_one(&mixer->audio_instances);
    if (index < 0) {
        return;
    }

    AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, index);
    memcpy(instance, settings, sizeof(AudioInstance));
}

void audio_mixer_play_unique(AudioMixer* mixer, int32 id, Audio* audio, const AudioInstance* settings = NULL) NO_EXCEPT
{
    for (uint32 i = 0; i < mixer->audio_instances.capacity; ++i) {
        // @performance We are not really utilizing chunk memory.
        // Maybe a simple array would be better
        // Or we need to use more chunk functions / maybe even create a chunk_iterate() function?
        const AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == id) {
            return;
        }
    }

    audio_mixer_play(mixer, id, audio, settings);
}

void audio_mixer_play_unique(AudioMixer* mixer, const AudioInstance* settings) NO_EXCEPT
{
    for (uint32 i = 0; i < mixer->audio_instances.capacity; ++i) {
        // @performance We are not really utilizing chunk memory.
        // Maybe a simple array would be better
        // Or we need to use more chunk functions / maybe even create a chunk_iterate() function?
        const AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == settings->id) {
            return;
        }
    }

    audio_mixer_play(mixer, settings);
}

void audio_mixer_remove(AudioMixer* mixer, int32 id) NO_EXCEPT
{
    for (uint32 i = 0; i < mixer->audio_instances.capacity; ++i) {
        AudioInstance* instance = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (instance->id == id) {
            instance->id = 0;
            chunk_free_elements(&mixer->audio_instances, i);

            // No return, since we want to remove all instances
        }
    }
}

int32 apply_speed(int16* buffer, int32 buffer_size, f32 speed) NO_EXCEPT
{
    if (speed == 1.0f) {
        return 0;
    }

    // Has to be multiple of 2 to ensure stereo is implemented correctly
    int new_size = align_up((int) (buffer_size / speed), 2);

    // Speed up
    if (speed > 1.0f) {
        for (int i = 0; i < new_size; ++i) {
            // @bug What if 2 consecutive values fall onto the same int index for stereo. This would break it.
            // The problem is, even by doing this as stereo calculation we would still have the same issue just not on the current value but the next loop
            int src_index = (int) (i * speed);
            buffer[i] = buffer[src_index];
        }

        // A speed up reduces the sample_index -> we reduce the data in the buffer
        return new_size - buffer_size;
    }

    // Slow down
    for (int i = buffer_size - 1; i > 0; --i) {
        int src_index = (int) (i * speed);
        buffer[i] = buffer[src_index];
    }

    return 0;
}

// @performance Whenever we handle left and right the same we could half the buffer_size
// This allows us to re-use existing helper variables without re-calculating them for the next loop (e.g. delay below)
// Or, if the multiplier is an int we can even perform the multiplication on int32 through casting instead of 2 operations on int16
// We might have to adjust some of the values to ensure correct multiplication if possible (e.g. feedback, intensity, ...)
// @todo We probably want to handle left and right channel differently to add some depth
static inline
void apply_echo(int16* buffer, int32 buffer_size, f32 delay, f32 feedback, int32 sample_rate) NO_EXCEPT
{
    int delay_samples = (int) (delay * sample_rate);
    for (int i = delay_samples; i < buffer_size; ++i) {
        buffer[i] += (int16) (buffer[i - delay_samples] * feedback);
    }
}

// @todo We probably want to handle left and right channel differently to add some depth
static inline
void apply_reverb(int16* buffer, int32 buffer_size, f32 intensity) NO_EXCEPT
{
    intensity *= 0.5f;
    for (int i = 1; i < buffer_size; ++i) {
        buffer[i] += (int16) (buffer[i - 1] * intensity); // Simple reverb with decay
    }
}

static inline
void apply_cave(int16* buffer, int32 buffer_size, int32 sample_rate) NO_EXCEPT
{
    f32 echo_delay = 0.1f; // Echo delay in seconds
    f32 feedback = 0.3f;  // Echo feedback level
    apply_echo(buffer, buffer_size, echo_delay, feedback, sample_rate);
    apply_reverb(buffer, buffer_size, 0.4f); // Add mild reverb
}

static inline
void apply_underwater(int16* buffer, int32 buffer_size) NO_EXCEPT
{
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) sinf(buffer[i] * 0.5f); // Dampen + distortion
    }
}

static inline
void apply_flanger(int16* buffer, int32 buffer_size, f32 rate, f32 depth, int32 sample_rate) NO_EXCEPT
{
    f32 delay_samples = depth * sample_rate;
    f32 temp = OMS_TWO_PI_F32 * rate / sample_rate;

    for (int i = 0; i < buffer_size; ++i) {
        int delay = (int) (delay_samples * (0.5f + 0.5f * sinf(i * temp)));
        if (i >= delay) {
            buffer[i] += (int16) (buffer[i - delay] * 0.5f);
        }
    }
}

static inline
void apply_tremolo(int16* buffer, int32 buffer_size, f32 rate, f32 depth, int32 sample_rate) NO_EXCEPT
{
    f32 temp = OMS_TWO_PI_F32 * rate / sample_rate;
    f32 temp2 = (1.0f - depth) + depth;

    for (int i = 0; i < buffer_size; ++i) {
        f32 mod = temp2 * (0.5f + 0.5f * sinf(i * temp));
        buffer[i] = (int16) (buffer[i] * mod);
    }
}

static inline
void apply_distortion(int16* buffer, int32 buffer_size, f32 gain) NO_EXCEPT
{
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) tanh(buffer[i] * gain);
    }
}

static inline
void apply_chorus(int16* buffer, int32 buffer_size, f32 rate, f32 depth, int32 sample_rate) NO_EXCEPT
{
    f32 temp = OMS_TWO_PI_F32 * rate / sample_rate;

    int max_delay = (int) (depth * sample_rate);
    for (int i = 0; i < buffer_size; ++i) {
        int delay = (int) (max_delay * (0.5f + 0.5f * sinf(i * temp)));
        if (i >= delay) {
            buffer[i] += (int16) (buffer[i - delay] * 0.5f);
        }
    }
}

static inline
void apply_pitch_shift(int16* buffer, int32 buffer_size, f32 pitch_factor) NO_EXCEPT
{
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * pitch_factor);
    }
}

static inline
void apply_granular_delay(int16* buffer, int32 buffer_size, f32 delay, f32 granularity, int32 sample_rate) NO_EXCEPT
{
    int delay_samples = (int) (delay * sample_rate);
    int limit = (int) (granularity * sample_rate);

    for (int i = 0; i < buffer_size; ++i) {
        if (i % limit == 0 && i >= delay_samples) {
            buffer[i] += (int16) (buffer[i - delay_samples] * 0.6f);
        }
    }
}

static inline
void apply_frequency_modulation(int16* buffer, int32 buffer_size, f32 mod_freq, f32 mod_depth, int32 sample_rate) NO_EXCEPT
{
    f32 temp = OMS_TWO_PI_F32 * mod_freq / sample_rate;
    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * sinf(i * temp) * mod_depth);
    }
}

static inline
void apply_stereo_panning(int16* buffer, int32 buffer_size, f32 pan) NO_EXCEPT
{
    f32 left_gain = 1.0f - pan;
    f32 right_gain = pan;

    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (int16) (buffer[i] * left_gain);
        buffer[i + 1] = (int16) (buffer[i + 1] * right_gain);
    }
}

static inline
void apply_highpass(int16* buffer, int32 buffer_size, f32 cutoff, int32 sample_rate) NO_EXCEPT
{
    f32 rc = 1.0f / (OMS_TWO_PI_F32 * cutoff);
    f32 dt = 1.0f / sample_rate;
    f32 alpha = rc / (rc + dt);
    f32 previous = buffer[0];
    f32 previous_output = buffer[0];

    for (int i = 1; i < buffer_size; ++i) {
        f32 current = buffer[i];
        buffer[i] = (int16) (alpha * (previous_output + current - previous));
        previous = current;
        previous_output = buffer[i];
    }
}

static inline
void apply_lowpass(int16* buffer, int32 buffer_size, f32 cutoff, int32 sample_rate) NO_EXCEPT
{
    f32 rc = 1.0f / (OMS_TWO_PI_F32 * cutoff);
    f32 dt = 1.0f / sample_rate;
    f32 alpha = dt / (rc + dt);
    f32 previous = buffer[0];

    for (int i = 1; i < buffer_size; ++i) {
        buffer[i] = (int16) (previous + alpha * (buffer[i] - previous));
        previous = buffer[i];
    }
}

static inline
int32 mixer_effects_mono(AudioMixer* mixer, uint64 effect, int32 samples) NO_EXCEPT
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

static inline
int32 mixer_effects_stereo() NO_EXCEPT
{
    return 0;
}

void audio_mixer_mix(AudioMixer* mixer, uint32 size) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_MIXER_MIX);
    memset(mixer->settings.buffer, 0, size);

    mixer->settings.sample_buffer_size = 0;
    const uint32 limit_max = size / mixer->settings.sample_size;

    const bool has_location = !is_empty((byte *) &mixer->camera.audio_location, sizeof(mixer->camera.audio_location));

    const f32 volume_scale = mixer->settings.master_volume * mixer->settings.master_volume;

    for (uint32 i = 0; i < mixer->audio_instances.capacity; ++i) {
        AudioInstance* const sound = (AudioInstance *) chunk_get_element(&mixer->audio_instances, i);
        if (sound->id == 0) {
            continue;
        }

        uint32 limit = limit_max;

        // Compute the vector from the player to the sound's origin
        v3_f32 to_sound = {0};
        f32 total_attenuation = 1.0f;
        const bool has_origin = !is_empty((byte *) &sound->origin.audio_location, sizeof(sound->origin.audio_location));

        if (has_location && has_origin) {
            to_sound = vec3_sub(sound->origin.audio_location, mixer->camera.audio_location);

            const f32 distance = vec3_length(to_sound);
            if (distance) {
                f32 distance_attenuation = max_branched(0.0f, 1.0f - (distance / 50.0f));

                vec3_normalize(&to_sound);
                f32 alignment = vec3_dot(mixer->camera.audio_lookat, to_sound);
                f32 directional_attenuation = max_branched(0.0f, alignment);

                total_attenuation = distance_attenuation * directional_attenuation;
            }
        }

        const uint32 sound_sample_count = sound->audio_size / mixer->settings.sample_size;
        uint32 sound_sample_index = sound->sample_index;
        const int16* audio_data = (int16 *) sound->audio_data;

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