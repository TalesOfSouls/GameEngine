/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_WRAPPER_C
#define COMS_AUDIO_WRAPPER_C

#include "../stdlib/Stdlib.h"
#include "AudioSetting.h"
#include "AudioWrapper.h"

#if _WIN32
    #include "../platform/win32/audio/DirectSound.cpp"
    #include "../platform/win32/audio/XAudio2.cpp"
    #include "../platform/win32/audio/Wasapi.cpp"
#endif

inline
void audio_load(
    void* platform_window,
    AudioSetting* const __restrict setting,
    ApiAudioSetting* const __restrict api_setting
) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_INIT, NULL, PROFILE_FLAG_SHOULD_LOG);
    bool success = false;

    // Try different audio wrappers if one fails in the following loop
    // wasapi -> xaudio2 -> direct sound -> wasapi
    for (int i = 0; i < 3 && !success; ++i) {
        switch (setting->type) {
            case SOUND_API_TYPE_DIRECT_SOUND: {
                success = direct_sound_load(platform_window, setting, &api_setting->direct_sound_setting);
                if (!success) {
                    setting->type = SOUND_API_TYPE_WASAPI;
                    LOG_1("[WARNING] Failed loading DirectSound");
                }
            } break;
            case SOUND_API_TYPE_XAUDIO2: {
                success = xaudio2_load(setting, &api_setting->xaudio2_setting);
                if (!success) {
                    setting->type = SOUND_API_TYPE_DIRECT_SOUND;
                    LOG_1("[WARNING] Failed loading XAudio2");
                }
            } break;
            case SOUND_API_TYPE_WASAPI: {
                success = wasapi_load(&api_setting->wasapi_setting);
                if (!success) {
                    setting->type = SOUND_API_TYPE_XAUDIO2;
                    LOG_1("[WARNING] Failed loading WASAPI");
                }
            } break;
            default:
                UNREACHABLE();
        }
    }
}

inline
void audio_play(
    AudioSetting* const __restrict setting,
    ApiAudioSetting* const __restrict api_setting
) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_play(setting, &api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_play(&api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_play(&api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
uint32 audio_buffer_fillable(
    const AudioSetting* const __restrict setting,
    const ApiAudioSetting* const __restrict api_setting
) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            return direct_sound_buffer_fillable(setting, &api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            return xaudio2_buffer_fillable(setting, &api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            return wasapi_buffer_fillable(&api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
void audio_play_buffer(
    AudioSetting* const __restrict setting,
    ApiAudioSetting* const __restrict api_setting
) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_play_buffer(setting, &api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_play_buffer(setting, &api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_play_buffer(setting, &api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
void audio_free(
    AudioSetting* const __restrict setting,
    ApiAudioSetting* const __restrict api_setting
) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_free(setting, &api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_free(&api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_free(&api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

#endif