/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
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
#include "../../../memory/PointerMemory.h"
#include "XAudio2.h"

#include "../libs/ole32.h"

// BEGIN: Dynamically load XAudio2
typedef HRESULT (WINAPI *XAudio2Create_t)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);

static XAudio2Create_t pXAudio2Create = NULL;

static LibraryHandle _xaudio2_lib;

static int _xaudio2_lib_ref_count = 0;
// END: Dynamically load XAudio2

static inline
void xaudio2_settings_free(XAudio2Setting* const __restrict api_setting) NO_EXCEPT
{
    if (api_setting->source_voice) {
        api_setting->source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
        api_setting->source_voice->FlushSourceBuffers();
        api_setting->source_voice->DestroyVoice();
        api_setting->source_voice = NULL;
    }

    if (api_setting->mastering_voice) {
        api_setting->mastering_voice->DestroyVoice();
        api_setting->mastering_voice = NULL;
    }

    if (api_setting->xaudio2_handle) {
        api_setting->xaudio2_handle->Release();
        api_setting->xaudio2_handle = NULL;
    }

    // WARNING: The buffer memory (pAudioData) is NOT owned by us (setting->buffer_internal)
    //          -> we must not free it
    for (int32 i = 0; i < XAUDIO2_BUFFER_COUNT; ++i) {
        api_setting->internal_buffer[i].pAudioData = NULL;
    }

    if (api_setting->com_initialized) {
        OLE32_CoUninitialize();
        api_setting->com_initialized = false;
    }
}

static
bool xaudio2_settings_load(AudioSetting* const __restrict setting, XAudio2Setting* const __restrict api_setting) NO_EXCEPT
{
    // XAudio2 uses COM internally -> COM must be initialized on the thread that uses XAudio2
    // S_FALSE = already initialized on this thread (still needs a matching CoUninitialize)
    // RPC_E_CHANGED_MODE = already initialized with a different mode (usable, but no CoUninitialize)
    // WARNING: Requires a successful ole32_init() (see *_load())
    const HRESULT hr_com = OLE32_CoInitializeEx(NULL, COINIT_MULTITHREADED);
    api_setting->com_initialized = SUCCEEDED(hr_com);

    if (!pXAudio2Create || !SUCCEEDED(pXAudio2Create(&api_setting->xaudio2_handle, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        LOG_1("[ERROR] Xaudio2: XAudio2Create failed");
        xaudio2_settings_free(api_setting);

        return false;
    }

    if (!SUCCEEDED(api_setting->xaudio2_handle->CreateMasteringVoice(
        &api_setting->mastering_voice,
        XAUDIO2_DEFAULT_CHANNELS,
        setting->sample_rate,
        0,
        NULL))
    ) {
        LOG_1("[ERROR] Xaudio2: CreateMasteringVoice failed");
        xaudio2_settings_free(api_setting);

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

    if (!SUCCEEDED(api_setting->xaudio2_handle->CreateSourceVoice(&api_setting->source_voice, &wf))) {
        LOG_1("[ERROR] Xaudio2: CreateSourceVoice failed");
        xaudio2_settings_free(api_setting);

        return false;
    }

    // We don't submit 1 second buffers (that would result in 1-2 seconds of latency)
    // Instead every buffer holds "latency" samples and we keep up to XAUDIO2_BUFFER_COUNT buffers queued
    api_setting->chunk_size = setting->latency * setting->sample_size;
    ASSERT_TRUE(api_setting->chunk_size > 0);
    ASSERT_TRUE(api_setting->chunk_size <= setting->buffer_size);

    // WARNING: buffer_internal must be unique per mixer/audio setting and at least
    //          XAUDIO2_BUFFER_COUNT * (chunk_size + ASSUMED_CACHE_LINE_SIZE) large
    ASSERT_TRUE(setting->buffer_internal);
    ASSERT_TRUE(XAUDIO2_BUFFER_COUNT * (api_setting->chunk_size + ASSUMED_CACHE_LINE_SIZE) <= setting->buffer_size);

    byte* mem = setting->buffer_internal;
    for (int32 i = 0; i < XAUDIO2_BUFFER_COUNT; ++i) {
        XAUDIO2_BUFFER* const buffer = &api_setting->internal_buffer[i];
        memset(buffer, 0, sizeof(XAUDIO2_BUFFER));

        buffer->AudioBytes = api_setting->chunk_size;
        buffer->pAudioData = (byte *) memory_get(&mem, api_setting->chunk_size, ASSUMED_CACHE_LINE_SIZE);
    }

    setting->sample_output = 0;
    api_setting->frames_submitted = 0;

    return true;
}

bool xaudio2_load(AudioSetting* const __restrict setting, XAudio2Setting* const __restrict api_setting) NO_EXCEPT
{
    LOG_1("[INFO] Load audio API XAudio2");

    if (!_xaudio2_lib_ref_count) {
        bool success = library_dyn_load(&_xaudio2_lib, L"xaudio2_9.dll");
        if (!success) {
            success = library_dyn_load(&_xaudio2_lib, L"xaudio2_8.dll");
            if (!success) {
                return false;
            }
        }

        pXAudio2Create = (XAudio2Create_t) library_dyn_proc(_xaudio2_lib, "XAudio2Create");
        if (!pXAudio2Create) {
            library_dyn_unload(&_xaudio2_lib);

            return false;
        }
    }

    // Ensures OLE32_CoInitializeEx is available
    if (!ole32_init()) {
        if (!_xaudio2_lib_ref_count) {
            library_dyn_unload(&_xaudio2_lib);
            pXAudio2Create = NULL;
        }

        return false;
    }

    if (!xaudio2_settings_load(setting, api_setting)) {
        ole32_free();

        if (!_xaudio2_lib_ref_count) {
            library_dyn_unload(&_xaudio2_lib);
            pXAudio2Create = NULL;
        }

        return false;
    }

    ++_xaudio2_lib_ref_count;

    return true;
}

inline
void xaudio2_play(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->source_voice);
    api_setting->source_voice->Start(0, XAUDIO2_COMMIT_NOW);
}

inline
void xaudio2_stop(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->source_voice);
    api_setting->source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
}

inline
void xaudio2_free(XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    // Every audio setting has its own XAudio2 instance -> always release it
    xaudio2_settings_free(api_setting);
    ole32_free();

    // Only unload the library once the last user is gone
    if (_xaudio2_lib_ref_count > 1) {
        --_xaudio2_lib_ref_count;

        return;
    }

    library_dyn_unload(&_xaudio2_lib);
    pXAudio2Create = NULL;

    _xaudio2_lib_ref_count = 0;
}

/**
 * Calculates the bytes to generate for the next buffer
 *
 * We always fill exactly one chunk (= latency) as long as there is a free buffer
 */
inline
uint32 xaudio2_buffer_fillable(const AudioSetting*, const XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_AUDIO_BUFFER_FILLABLE);
    if (!api_setting->source_voice) {
        return 0;
    }

    XAUDIO2_VOICE_STATE state;
    api_setting->source_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

    // BuffersQueued includes the buffer that is currently played
    if (state.BuffersQueued >= XAUDIO2_BUFFER_COUNT) {
        return 0;
    }

    return api_setting->chunk_size;
}

inline
void xaudio2_play_buffer(AudioSetting* __restrict setting, XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_AUDIO_PLAY_BUFFER);

    if (!api_setting->source_voice || setting->sample_buffer_size == 0) {
        return;
    }

    // Since we only fill if less than XAUDIO2_BUFFER_COUNT buffers are queued,
    // the buffer that was submitted XAUDIO2_BUFFER_COUNT submits ago is guaranteed to be finished
    const uint32 idx = setting->sample_output % XAUDIO2_BUFFER_COUNT;
    XAUDIO2_BUFFER* const buffer = &api_setting->internal_buffer[idx];

    const uint32 size = OMS_MIN(setting->sample_buffer_size, api_setting->chunk_size);

    memcpy((void *) buffer->pAudioData, setting->buffer, size);
    buffer->AudioBytes = size;

    setting->sample_buffer_size = 0;

    if (!SUCCEEDED(api_setting->source_voice->SubmitSourceBuffer(buffer))) {
        LOG_1("[ERROR] Xaudio2: SubmitSourceBuffer failed");

        return;
    }

    api_setting->frames_submitted += size / setting->sample_size;

    // Wrap manually, sample_output is only a byte and 256 is no multiple of XAUDIO2_BUFFER_COUNT
    setting->sample_output = (byte) ((setting->sample_output + 1) % XAUDIO2_BUFFER_COUNT);
}

/**
 * Frames that are submitted but not played yet (= delay of the audio that is mixed now)
 */
inline
uint32 xaudio2_buffer_delay(const XAudio2Setting* __restrict api_setting) NO_EXCEPT
{
    if (!api_setting->source_voice) {
        return 0;
    }

    // Without XAUDIO2_VOICE_NOSAMPLESPLAYED -> SamplesPlayed = frames played since the voice was created
    XAUDIO2_VOICE_STATE state;
    api_setting->source_voice->GetState(&state, 0);

    return api_setting->frames_submitted > state.SamplesPlayed
        ? (uint32) (api_setting->frames_submitted - state.SamplesPlayed)
        : 0;
}

#endif
