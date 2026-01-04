/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_WRAPPER_H
#define COMS_AUDIO_WRAPPER_H

#include "../stdlib/Stdlib.h"
#include "AudioSetting.h"

#if _WIN32
    #include "../platform/win32/audio/DirectSound.h"
    #include "../platform/win32/audio/XAudio2.h"
    #include "../platform/win32/audio/Wasapi.h"
#endif

union ApiAudioSetting {
    #if _WIN32
        DirectSoundSetting direct_sound_setting;
        XAudio2Setting xaudio2_setting;
        WasapiSetting wasapi_setting;
    #endif
};

inline
void audio_load(HWND hwnd, AudioSetting* __restrict setting, ApiAudioSetting* __restrict api_setting) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_load(hwnd, setting, api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_load(hwnd, setting, api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_load(hwnd, setting, api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
void audio_play(AudioSetting* __restrict setting, ApiAudioSetting* __restrict api_setting) NO_EXCEPT
{
    switch (settings->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_play(setting, api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_play(setting, api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_play(setting, api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
uint32 audio_buffer_fillable(const AudioSetting* __restrict setting, const ApiAudioSetting* __restrict api_setting) NO_EXCEPT
{
    switch (settings->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_buffer_fillable(setting, api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_buffer_fillable(setting, api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_buffer_fillable(setting, api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
uint32 audio_play_buffer(const AudioSetting* __restrict setting, const ApiAudioSetting* __restrict api_setting) NO_EXCEPT
{
    switch (settings->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_play_buffer(setting, api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_play_buffer(setting, api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_play_buffer(setting, api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
void audio_free(HWND hwnd, AudioSetting* __restrict setting, ApiAudioSetting* __restrict api_setting) NO_EXCEPT
{
    switch (setting->type) {
        case SOUND_API_TYPE_DIRECT_SOUND: {
            direct_sound_free(hwnd, setting, api_setting->direct_sound_setting);
        } break;
        case SOUND_API_TYPE_XAUDIO2: {
            xaudio2_free(hwnd, setting, api_setting->xaudio2_setting);
        } break;
        case SOUND_API_TYPE_WASAPI: {
            wasapi_free(hwnd, setting, api_setting->wasapi_setting);
        } break;
        default:
            UNREACHABLE();
    }
}

#endif