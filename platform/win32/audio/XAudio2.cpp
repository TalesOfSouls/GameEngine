/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SOUND_XAUDIO2_C
#define COMS_SOUND_XAUDIO2_C

#include <windows.h>
#include <objbase.h>
#include <xaudio2.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../audio/AudioSetting.h"
#include "../../../log/Log.h"
#include "../../../audio/Audio.cpp"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"
#include "XAudio2.h"

#include "../libs/ole32.h"

// BEGIN: Dynamically load XAudio2
typedef HRESULT (WINAPI *XAudio2Create_t)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

static XAudio2Create_t pXAudio2Create = NULL;

static LibraryHandle _xaudio2_lib;

static int _xaudio2_lib_ref_count = 0;
// END: Dynamically load XAudio2

bool xaudio2_settings_load(AudioSetting* const __restrict setting, XAudio2Setting* const __restrict api_setting)
{
    if (!pXAudio2Create || !SUCCEEDED(pXAudio2Create(&api_setting->xaudio2_handle, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        LOG_1("Xaudio2: XAudio2Create failed");

        return false;
    }

    HRESULT hr;
    if (!SUCCEEDED(hr = api_setting->xaudio2_handle->CreateMasteringVoice(
        &api_setting->mastering_voice,
        XAUDIO2_DEFAULT_CHANNELS,
        setting->sample_rate,
        0,
        NULL))
    ) {
        LOG_1("Xaudio2: CreateMasteringVoice failed");

        return false;
    }

    WAVEFORMATEX wf = {
        WAVE_FORMAT_PCM, // .wFormatTag =
        2, // .nChannels =
        setting->sample_rate, // .nSamplesPerSec =
        (DWORD) (setting->sample_rate * 4), // .nAvgBytesPerSec =
        (WORD) 4, // .nBlockAlign =
        16, // .wBitsPerSample =
        0
    };
    /*
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.wBitsPerSample = (uint16) ((setting->sample_size * 8) / wf.nChannels); // = sample_size per channel
    wf.nBlockAlign = setting->sample_size; //(wf.nChannels * wf.wBitsPerSample) / 8; // = sample_szie
    wf.nSamplesPerSec = setting->sample_rate;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign; // = buffer_size
    wf.cbSize = 0;
    */

    if (!SUCCEEDED(api_setting->xaudio2_handle->CreateSourceVoice(&api_setting->source_voice, &wf))) {
        LOG_1("Xaudio2: CreateSourceVoice failed");

        return false;
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

    return true;
}

bool xaudio2_load(AudioSetting* const __restrict setting, XAudio2Setting* const __restrict api_setting) {
    LOG_1("Load audio API XAudio2");

    if (_xaudio2_lib_ref_count) {
        xaudio2_settings_load(setting, api_setting);
        ++_xaudio2_lib_ref_count;

        return true;
    }

    bool success = library_dyn_load(&_xaudio2_lib, L"xaudio2_9.dll");
    if (!success) {
        success = library_dyn_load(&_xaudio2_lib, L"xaudio2_8.dll");
        if (!success) {
            return false;
        }
    }

    pXAudio2Create = (XAudio2Create_t) library_dyn_proc(_xaudio2_lib, "XAudio2Create");

    ++_xaudio2_lib_ref_count;

    return xaudio2_settings_load(setting, api_setting);
}

inline
void xaudio2_play(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->source_voice);
    /*if (!api_setting->source_voice) {
        return;
    }*/

    api_setting->source_voice->Start(0, XAUDIO2_COMMIT_NOW);
}

inline
void xaudio2_stop(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->source_voice);
    /*if (!api_setting->source_voice) {
        return;
    }*/

    api_setting->source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
}

inline
void xaudio2_free(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    if (api_setting->source_voice) {
        api_setting->source_voice->DestroyVoice();
    }

    if (api_setting->mastering_voice) {
        api_setting->mastering_voice->DestroyVoice();
    }

    if (api_setting->xaudio2_handle) {
        api_setting->xaudio2_handle->Release();
    }

    if (api_setting->internal_buffer[0].pAudioData) {
        free((void *) api_setting->internal_buffer[0].pAudioData);
    }

    if (api_setting->internal_buffer[1].pAudioData) {
        free((void *) api_setting->internal_buffer[1].pAudioData);
    }

    library_dyn_unload(&_xaudio2_lib);

    --_xaudio2_lib_ref_count;
}

/**
 * Calculates the samples to generate for the buffer
 *
 * For XAudio2 we currently always fill the entire buffer size.
 * For other audio APIs we maybe have to do something else
 */
inline
uint32 xaudio2_buffer_fillable(const AudioSetting* __restrict setting, const XAudio2Setting* __restrict api_setting) NO_EXCEPT
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
void xaudio2_play_buffer(AudioSetting* __restrict setting, XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_PLAY_BUFFER);

    if (!api_setting->source_voice || setting->sample_buffer_size == 0) {
        return;
    }

    const uint32 idx = setting->sample_output % 2;

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
