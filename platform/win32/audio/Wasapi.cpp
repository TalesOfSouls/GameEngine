/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SOUND_WASAPI_C
#define COMS_SOUND_WASAPI_C

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "Wasapi.h"

#include "../../../stdlib/Stdlib.h"
#include "../../../audio/AudioSetting.h"
#include "../../../log/Log.h"
#include "../../../audio/Audio.cpp"

#include "../libs/ole32.h"

// NOTE: WASAPI is a pure COM API. The device enumerator, device and audio client functions are
//      interface (vtable) methods and NOT dll exports. That's why we don't load any wasapi dll.
//      The only dll function we need is CoCreateInstance (ole32), which is dynamically loaded in ole32.h
static int _wasapi_ref_count = 0;

// REFERENCE_TIME is in 100ns units
#define WASAPI_REFTIMES_PER_SEC 10000000

static inline
void wasapi_settings_free(WasapiSetting* const __restrict api_setting) NO_EXCEPT
{
    if (api_setting->wasapi_handle) {
        api_setting->wasapi_handle->Stop();
    }

    if (api_setting->render_client) {
        api_setting->render_client->Release();
        api_setting->render_client = NULL;
    }

    if (api_setting->wasapi_handle) {
        api_setting->wasapi_handle->Release();
        api_setting->wasapi_handle = NULL;
    }

    api_setting->buffer_frame_count = 0;

    if (api_setting->com_initialized) {
        OLE32_CoUninitialize();
        api_setting->com_initialized = false;
    }
}

static inline
bool wasapi_settings_load(
    const AudioSetting* const __restrict setting,
    WasapiSetting* const __restrict api_setting
) NO_EXCEPT
{
    // COM must be initialized on the thread that uses WASAPI (= the audio thread, not the main thread)
    // S_FALSE = already initialized on this thread (still needs a matching CoUninitialize)
    // RPC_E_CHANGED_MODE = already initialized with a different mode (usable, but no CoUninitialize)
    // WARNING: Requires a successful ole32_init() (see *_load())
    const HRESULT hr_com = OLE32_CoInitializeEx(NULL, COINIT_MULTITHREADED);
    api_setting->com_initialized = SUCCEEDED(hr_com);

    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;

    HRESULT hr = OLE32_CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void **) &enumerator
    );

    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: CreateInstance failed");
        wasapi_settings_free(api_setting);

        return false;
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    enumerator->Release();

    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: DefaultAudioEndpoint failed");
        wasapi_settings_free(api_setting);

        return false;
    }

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &api_setting->wasapi_handle);
    device->Release();

    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: DeviceActivate failed");
        api_setting->wasapi_handle = NULL;
        wasapi_settings_free(api_setting);

        return false;
    }

    // We provide the same format as our mixer output (16 bit stereo PCM)
    // The device mix format is usually 32 bit float (and maybe a different sample rate/channel count)
    // -> We let WASAPI convert it for us (AUTOCONVERTPCM + SRC_DEFAULT_QUALITY)
    WAVEFORMATEX wf = {
        WAVE_FORMAT_PCM, // .wFormatTag =
        2, // .nChannels =
        setting->sample_rate, // .nSamplesPerSec =
        (DWORD) (setting->sample_rate * 4), // .nAvgBytesPerSec =
        (WORD) 4, // .nBlockAlign =
        16, // .wBitsPerSample =
        0
    };

    // We poll the buffer from the audio thread -> no AUDCLNT_STREAMFLAGS_EVENTCALLBACK
    // (event callback requires SetEventHandle(), otherwise Start() fails)
    // The buffer must be able to hold at least 2x our latency so we can stay "latency" ahead of the device
    const REFERENCE_TIME buffer_duration = (REFERENCE_TIME) WASAPI_REFTIMES_PER_SEC * 2 * setting->latency / setting->sample_rate;

    hr = api_setting->wasapi_handle->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        buffer_duration,
        0,
        &wf,
        NULL
    );

    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: Initialize failed");
        wasapi_settings_free(api_setting);

        return false;
    }

    hr = api_setting->wasapi_handle->GetBufferSize(&api_setting->buffer_frame_count);
    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: GetBufferSize failed");
        wasapi_settings_free(api_setting);

        return false;
    }

    hr = api_setting->wasapi_handle->GetService(__uuidof(IAudioRenderClient), (void **) &api_setting->render_client);
    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: GetService failed");
        api_setting->render_client = NULL;
        wasapi_settings_free(api_setting);

        return false;
    }

    return true;
}

bool wasapi_load(
    const AudioSetting* const __restrict setting,
    WasapiSetting* const __restrict api_setting
) NO_EXCEPT
{
    LOG_1("[INFO] Load audio API WASAPI");

    // Ensures OLE32_CoCreateInstance/OLE32_CoInitializeEx are available
    if (!ole32_init()) {
        return false;
    }

    if (!wasapi_settings_load(setting, api_setting)) {
        ole32_free();

        return false;
    }

    ++_wasapi_ref_count;

    return true;
}

inline
void wasapi_play(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->wasapi_handle);
    api_setting->wasapi_handle->Start();
}

inline
void wasapi_stop(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->wasapi_handle);
    api_setting->wasapi_handle->Stop();
}

inline
void wasapi_free(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    // Every audio setting has its own audio client -> always release it
    wasapi_settings_free(api_setting);
    ole32_free();

    if (_wasapi_ref_count > 0) {
        --_wasapi_ref_count;
    }
}

/**
 * Calculates the bytes to generate for the buffer
 *
 * We try to stay "latency" samples ahead of the device
 */
inline
uint32 wasapi_buffer_fillable(const AudioSetting* __restrict setting, const WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_AUDIO_BUFFER_FILLABLE);
    if (!api_setting->wasapi_handle || !api_setting->render_client) {
        return 0;
    }

    uint32 frames_padding;
    if (FAILED(api_setting->wasapi_handle->GetCurrentPadding(&frames_padding))) {
        return 0;
    }

    // Padding = frames that are queued but not yet played
    const uint32 target_frames = OMS_MIN((uint32) setting->latency, api_setting->buffer_frame_count);
    if (frames_padding >= target_frames) {
        return 0;
    }

    uint32 frames = target_frames - frames_padding;

    // Our mixer buffer can only hold buffer_size bytes
    frames = OMS_MIN(frames, setting->buffer_size / setting->sample_size);

    // The mixer works in bytes, NOT in frames
    return frames * setting->sample_size;
}

inline
void wasapi_play_buffer(AudioSetting* __restrict setting, WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_AUDIO_PLAY_BUFFER);
    if (!api_setting->render_client || setting->sample_buffer_size == 0) {
        return;
    }

    const uint32 frames = setting->sample_buffer_size / setting->sample_size;
    setting->sample_buffer_size = 0;

    byte* buffer;
    if (FAILED(api_setting->render_client->GetBuffer(frames, &buffer))) {
        // E.g. AUDCLNT_E_BUFFER_TOO_LARGE if the device consumed less than expected
        return;
    }

    memcpy(buffer, setting->buffer, frames * setting->sample_size);

    // Without releasing the buffer nothing is ever played
    api_setting->render_client->ReleaseBuffer(frames, 0);
}

/**
 * Frames that are queued but not played yet + the latency of the audio stream (= delay of the audio that is mixed now)
 */
inline
uint32 wasapi_buffer_delay(const AudioSetting* __restrict setting, const WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    if (!api_setting->wasapi_handle) {
        return 0;
    }

    uint32 frames_padding;
    if (FAILED(api_setting->wasapi_handle->GetCurrentPadding(&frames_padding))) {
        return 0;
    }

    // Latency of the stream in 100 ns units (e.g. the device period)
    REFERENCE_TIME stream_latency;
    if (FAILED(api_setting->wasapi_handle->GetStreamLatency(&stream_latency))) {
        stream_latency = 0;
    }

    return frames_padding + (uint32) ((uint64) stream_latency * setting->sample_rate / 10000000ULL);
}

#endif
