/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SOUND_WASAPI_C
#define COMS_SOUND_WASAPI_C

#include <windows.h>
#include "Wasapi.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <commdlg.h>
#include <stdio.h>
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

#include "../../../stdlib/Stdlib.h"
#include "../../../audio/AudioSetting.h"
#include "../../../log/Log.h"
#include "../../../audio/Audio.cpp"

#include "../libs/ole32_static.h"

//#pragma comment(lib, "avrt.lib")

// BEGIN: Dynamically load DirectSound
typedef HRESULT (WINAPI *IMMDeviceEnumerator_GetDefaultAudioEndpoint_t)(IMMDeviceEnumerator*, EDataFlow, ERole, IMMDevice**);
typedef HRESULT (WINAPI *IMMDevice_Activate_t)(IMMDevice*, REFIID, DWORD, PROPVARIANT*, void**);

typedef HRESULT (WINAPI *IAudioClient_GetMixFormat_t)(IAudioClient*, WAVEFORMATEX**);
typedef HRESULT (WINAPI *IAudioClient_Initialize_t)(IAudioClient*, AUDCLNT_SHAREMODE, DWORD, REFERENCE_TIME, REFERENCE_TIME, WAVEFORMATEX*, void*);
typedef HRESULT (WINAPI *IAudioClient_Start_t)(IAudioClient*);
typedef HRESULT (WINAPI *IAudioClient_Stop_t)(IAudioClient*);
typedef HRESULT (WINAPI *IAudioClient_GetService_t)(IAudioClient*, REFIID, void**);

static IMMDeviceEnumerator_GetDefaultAudioEndpoint_t IMMDeviceEnumerator_GetDefaultAudioEndpoint = NULL;
static IMMDevice_Activate_t IMMDevice_Activate = NULL;
static IAudioClient_GetMixFormat_t pIAudioClient_GetMixFormat = NULL;
static IAudioClient_Initialize_t pIAudioClient_Initialize = NULL;
static IAudioClient_Start_t pIAudioClient_Start = NULL;
static IAudioClient_Stop_t pIAudioClient_Stop = NULL;
static IAudioClient_GetService_t pIAudioClient_GetService = NULL;

static LibraryHandle _mmdevapi_lib;
static LibraryHandle _audioclient_lib;

static int _mmdevapi_lib_ref_count = 0;
static int _audioclient_lib_ref_count = 0;
// END: Dynamically load DirectSound

static inline
bool wasapi_settings_load(WasapiSetting* const __restrict api_setting) {
    IMMDeviceEnumerator* enumerator;
    IMMDevice* device;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void **) &enumerator
    );

    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: Wasapi CreateInstance failed");

        return false;
    }

    hr  = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: Wasapi DefaultAudioEndpoint failed");

        enumerator->Release();

        return false;
    }

    hr = IMMDevice_Activate(device, __uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &api_setting->wasapi_handle);
    if (FAILED(hr)) {
        LOG_1("[ERROR] Wasapi: Wasapi DeviceActivate failed");

        device->Release();
        enumerator->Release();

        return false;
    }

    device->Release();
    enumerator->Release();

    // Initializing the audio client
    WAVEFORMATEX *pwfx = NULL;
    api_setting->wasapi_handle->GetMixFormat(&pwfx);
    api_setting->wasapi_handle->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, pwfx, NULL);
    api_setting->wasapi_handle->GetService(__uuidof(IAudioRenderClient), (void **) &api_setting->render_client);

    return true;
}

bool wasapi_load(WasapiSetting* const __restrict api_setting) {
    LOG_1("[INFO] Load audio API WASAPI");

    if (_mmdevapi_lib && _audioclient_lib) {
        wasapi_settings_load(api_setting);
        ++_mmdevapi_lib;
        ++_audioclient_lib;
        return true;
    }

    bool success = library_dyn_load(&_mmdevapi_lib, L"mmdevapi.dll");
    if (!success) {
        return false;
    }

    IMMDeviceEnumerator_GetDefaultAudioEndpoint = (IMMDeviceEnumerator_GetDefaultAudioEndpoint_t) library_dyn_proc(_mmdevapi_lib, "IMMDeviceEnumerator_GetDefaultAudioEndpoint");
    IMMDevice_Activate = (IMMDevice_Activate_t) library_dyn_proc(_mmdevapi_lib, "IMMDevice_Activate");
    if (!IMMDeviceEnumerator_GetDefaultAudioEndpoint || !IMMDevice_Activate) {
        return false;
    }

    success = library_dyn_load(&_audioclient_lib, L"audioclient.dll");
    if (!success) {
        return false;
    }

    pIAudioClient_GetMixFormat = (IAudioClient_GetMixFormat_t) library_dyn_proc(_audioclient_lib, "IAudioClient_GetMixFormat");
    pIAudioClient_Initialize = (IAudioClient_Initialize_t) library_dyn_proc(_audioclient_lib, "IAudioClient_Initialize");
    pIAudioClient_Start = (IAudioClient_Start_t) library_dyn_proc(_audioclient_lib, "IAudioClient_Start");
    pIAudioClient_Stop = (IAudioClient_Stop_t) library_dyn_proc(_audioclient_lib, "IAudioClient_Stop");
    pIAudioClient_GetService = (IAudioClient_GetService_t) library_dyn_proc(_audioclient_lib, "IAudioClient_GetService");
    if (!pIAudioClient_GetMixFormat
        || !pIAudioClient_Initialize
        || !pIAudioClient_Start
        || !pIAudioClient_Stop
        || !pIAudioClient_GetService
    ) {
        return false;
    }

    ++_mmdevapi_lib_ref_count;
    ++_audioclient_lib_ref_count;

    return wasapi_settings_load(api_setting);
}

inline
void wasapi_play(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->wasapi_handle);
    /*if (!api_setting->wasapi_handle) {
        return;
    }*/

    api_setting->wasapi_handle->Start();
}

inline
void wasapi_stop(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->wasapi_handle);
    /*if (!api_setting->wasapi_handle) {
        return;
    }*/

    api_setting->wasapi_handle->Stop();
}

inline
void wasapi_free(WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    if (_mmdevapi_lib_ref_count > 1 && _audioclient_lib_ref_count > 1) {
        --_mmdevapi_lib_ref_count;
        --_audioclient_lib_ref_count;

        return;
    }

    if (!api_setting->render_client) {
        api_setting->render_client->Release();
    }

    if (!api_setting->wasapi_handle) {
        api_setting->wasapi_handle->Release();
    }

    library_dyn_unload(&_audioclient_lib);
    library_dyn_unload(&_mmdevapi_lib);

    --_mmdevapi_lib_ref_count;
    --_audioclient_lib_ref_count;

}

inline
uint32 wasapi_buffer_fillable(const WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_BUFFER_FILLABLE);
    if (!api_setting->wasapi_handle) {
        return 0;
    }

    uint32 buffer_frame_count;
    api_setting->wasapi_handle->GetBufferSize(&buffer_frame_count);

    uint32 frames_padding;
    api_setting->wasapi_handle->GetCurrentPadding(&frames_padding);

    return buffer_frame_count - frames_padding;
}

inline
void wasapi_play_buffer(AudioSetting* __restrict setting, WasapiSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_PLAY_BUFFER);
    if (!api_setting->wasapi_handle || setting->sample_buffer_size == 0) {
        return;
    }

    // @question Do we have to change it from sample_buffer_size to sample count?
    byte* buffer;
    api_setting->render_client->GetBuffer(setting->sample_buffer_size, (byte **) &buffer);

    memcpy(buffer, setting->buffer, setting->sample_buffer_size);
}

#endif