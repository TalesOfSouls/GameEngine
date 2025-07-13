/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SOUND_XAUDIO2_H
#define COMS_SOUND_XAUDIO2_H

#include <windows.h>
#include <objbase.h>
#include <xaudio2.h>

#include "../../../stdlib/Types.h"
#include "../../../audio/AudioSetting.h"
#include "../../../log/Log.h"
#include "../../../audio/Audio.cpp"

struct XAudio2Setting {
    IXAudio2* audio_handle;
    IXAudio2SourceVoice* source_voice;
    IXAudio2MasteringVoice* mastering_voice;

    XAUDIO2_BUFFER internal_buffer[2];
};

// BEGIN: Dynamically load XAudio2
typedef HRESULT WINAPI XAudio2Create_t(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);
HRESULT WINAPI XAudio2CreateStub(IXAudio2**, UINT32, XAUDIO2_PROCESSOR) {
    return 0;
}
// END: Dynamically load XAudio2

void audio_load(HWND hwnd, AudioSetting* setting, XAudio2Setting* api_setting) {
    LOG_1("Load audio API");

    CoInitialize(NULL);
    HMODULE lib = LoadLibraryExA((LPCSTR) "xaudio2_9.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!lib) {
        LOG_1("Xaudio2: Couldn't load xaudio2_9.dll");

        lib = LoadLibraryExA((LPCSTR) "xaudio2_8.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    }

    if (!lib) {
        LOG_1("Xaudio2: Couldn't load xaudio2_8.dll");

        return;
    }

    XAudio2Create_t* XAudio2Create = (XAudio2Create_t *) GetProcAddress(lib, "XAudio2Create");
    if (!XAudio2Create || !SUCCEEDED(XAudio2Create(&api_setting->audio_handle, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        LOG_1("Xaudio2: XAudio2Create failed");

        return;
    }

    HRESULT hr;
    if (!SUCCEEDED(hr = api_setting->audio_handle->CreateMasteringVoice(
        &api_setting->mastering_voice,
        XAUDIO2_DEFAULT_CHANNELS,
        setting->sample_rate,
        0,
        NULL))
    ) {
        LOG_1("Xaudio2: CreateMasteringVoice failed");

        return;
    }

    WAVEFORMATEX wf = {};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.wBitsPerSample = (uint16) ((setting->sample_size * 8) / wf.nChannels); // = sample_size per channel
    wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8; // = sample_szie
    wf.nSamplesPerSec = setting->sample_rate;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign; // = buffer_size
    wf.cbSize = 0;

    if (!SUCCEEDED(api_setting->audio_handle->CreateSourceVoice(&api_setting->source_voice, &wf))) {
        LOG_1("Xaudio2: CreateSourceVoice failed");

        return;
    }

    // @todo consider to remove mallocs/callocs
    setting->buffer_size = setting->sample_rate * setting->sample_size;
    setting->buffer = (int16 *) calloc(setting->sample_rate, setting->sample_size);

    api_setting->internal_buffer[0].Flags = 0;
    api_setting->internal_buffer[0].AudioBytes = setting->buffer_size;
    api_setting->internal_buffer[0].pAudioData = (byte *) malloc(setting->buffer_size * sizeof(byte));
    api_setting->internal_buffer[0].PlayBegin = 0;
    api_setting->internal_buffer[0].PlayLength = 0;
    api_setting->internal_buffer[0].LoopBegin = 0;
    api_setting->internal_buffer[0].LoopLength = 0;
    api_setting->internal_buffer[0].LoopCount = 0;
    api_setting->internal_buffer[0].pContext = NULL;

    api_setting->internal_buffer[1].Flags = 0;
    api_setting->internal_buffer[1].AudioBytes = setting->buffer_size;
    api_setting->internal_buffer[1].pAudioData = (byte *) malloc(setting->buffer_size * sizeof(byte));
    api_setting->internal_buffer[1].PlayBegin = 0;
    api_setting->internal_buffer[1].PlayLength = 0;
    api_setting->internal_buffer[1].LoopBegin = 0;
    api_setting->internal_buffer[1].LoopLength = 0;
    api_setting->internal_buffer[1].LoopCount = 0;
    api_setting->internal_buffer[1].pContext = NULL;
}

inline
void audio_play(AudioSetting* setting, XAudio2Setting* api_setting) {
    if (!api_setting->source_voice) {
        return;
    }

    api_setting->source_voice->Start(0, XAUDIO2_COMMIT_NOW);
}

inline
void audio_stop(AudioSetting* setting, XAudio2Setting* api_setting) {
    if (!api_setting->source_voice) {
        return;
    }

    api_setting->source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
}

inline
void audio_free(AudioSetting* setting, XAudio2Setting* api_setting)
{
    if (api_setting->source_voice) {
        api_setting->source_voice->DestroyVoice();
    }

    if (api_setting->mastering_voice) {
        api_setting->mastering_voice->DestroyVoice();
    }

    if (api_setting->audio_handle) {
        api_setting->audio_handle->Release();
    }

    if (api_setting->internal_buffer[0].pAudioData) {
        free((void *) api_setting->internal_buffer[0].pAudioData);
    }

    if (api_setting->internal_buffer[1].pAudioData) {
        free((void *) api_setting->internal_buffer[1].pAudioData);
    }
}

/**
 * Calculates the samples to generate for the buffer
 *
 * For XAudio2 we currently always fill the entire buffer size.
 * For other audio APIs we maybe have to do something else
 */
inline
uint32 audio_buffer_fillable(const AudioSetting* setting, const XAudio2Setting* api_setting)
{
    PROFILE(PROFILE_AUDIO_BUFFER_FILLABLE);
    if (!api_setting->source_voice) {
        return 0;
    }

    XAUDIO2_VOICE_STATE state;
    api_setting->source_voice->GetState(&state);
    if (state.BuffersQueued > 1) {
        return 0;
    }

    return setting->buffer_size;
}

inline
void audio_play_buffer(AudioSetting* setting, XAudio2Setting* api_setting) {
    PROFILE(PROFILE_AUDIO_PLAY_BUFFER);

    if (!api_setting->source_voice || setting->sample_buffer_size == 0) {
        return;
    }

    uint32 idx = setting->sample_output % 2;

    memcpy(
        (void *) api_setting->internal_buffer[idx].pAudioData,
        setting->buffer,
        setting->sample_buffer_size
    );

    if (!SUCCEEDED(api_setting->source_voice->SubmitSourceBuffer(&api_setting->internal_buffer[idx]))) {
        LOG_1("Xaudio2: SubmitSourceBuffer failed");

        return;
    }

    ++setting->sample_output;
    setting->sample_buffer_size = 0;
}

#endif
