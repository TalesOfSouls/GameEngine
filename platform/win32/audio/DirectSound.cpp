/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SOUND_DIRECT_SOUND_C
#define COMS_SOUND_DIRECT_SOUND_C

#include <windows.h>
#include <mmeapi.h>
#include <dsound.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../audio/AudioSetting.h"
#include "../../../log/Log.h"
#include "../../../audio/Audio.cpp"
#include "../../../compiler/CompilerUtils.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"
#include "../Window.h"
#include "DirectSound.h"

// BEGIN: Dynamically load DirectSound
typedef HRESULT (WINAPI *DirectSoundCreate8_t)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);

static DirectSoundCreate8_t pDirectSoundCreate8 = NULL;

static LibraryHandle _dsound_lib;

static int _dsound_lib_ref_count = 0;
// END: Dynamically load DirectSound

static inline
bool direct_sound_settings_load(
    void* const __restrict platform_window,
    const AudioSetting* const __restrict setting,
    DirectSoundSetting* const __restrict api_setting
)
{
    if (!pDirectSoundCreate8 || !SUCCEEDED(pDirectSoundCreate8(0, &api_setting->direct_sound_handle, 0))) {
        LOG_1("[ERROR] DirectSound: DirectSoundCreate8 failed");

        return false;
    }

    WindowPlatform* window = (WindowPlatform *) platform_window;
    if(!SUCCEEDED(api_setting->direct_sound_handle->SetCooperativeLevel(
        window->hwnd,
        DSSCL_PRIORITY
    ))) {
        LOG_1("[ERROR] DirectSound: SetCooperativeLevel failed.");

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
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8; //(WORD) compiler_div_pow2(wf.nChannels * wf.wBitsPerSample, 8);
    wf.nSamplesPerSec = setting->sample_rate;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.cbSize = 0;
    */

    // Create primary buffer
    DSBUFFERDESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(DSBUFFERDESC));

    buffer_desc.dwSize = sizeof(DSBUFFERDESC);
    buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

    if(!SUCCEEDED(api_setting->direct_sound_handle->CreateSoundBuffer(
        &buffer_desc,
        &api_setting->primary_buffer,
        0
    ))) {
        LOG_1("[ERROR] DirectSound: CreateSoundBuffer1 failed.");

        return false;
    }

    if (!SUCCEEDED(api_setting->primary_buffer->SetFormat(&wf))) {
        LOG_1("[ERROR] DirectSound: SetFormat failed.");

        return false;
    }

    // Create secondary buffer
    DSBUFFERDESC buffer_desc2;
    ZeroMemory(&buffer_desc2, sizeof(DSBUFFERDESC));

    buffer_desc2.dwSize = sizeof(DSBUFFERDESC);
    // @todo check alternative flags
    buffer_desc2.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLFX | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    buffer_desc2.dwBufferBytes = setting->buffer_size;
    buffer_desc2.lpwfxFormat = &wf;

    if(!SUCCEEDED(api_setting->direct_sound_handle->CreateSoundBuffer(
        &buffer_desc2,
        &api_setting->secondary_buffer,
        0
    ))) {
        LOG_1("[ERROR] DirectSound: CreateSoundBuffer2 failed.");

        return false;
    }

    return true;
}

bool direct_sound_load(
    void* const __restrict platform_window,
    const AudioSetting* const __restrict setting,
    DirectSoundSetting* const __restrict api_setting
) {
    LOG_1("[INFO] Load audio API DirectSound");

    if (_dsound_lib_ref_count) {
        direct_sound_settings_load(platform_window, setting, api_setting);
        ++_dsound_lib_ref_count;

        return true;
    }

    bool success = library_dyn_load(&_dsound_lib, L"dsound.dll");
    if (!success) {
        return false;
    }

    pDirectSoundCreate8 = (DirectSoundCreate8_t) library_dyn_proc(_dsound_lib, "DirectSoundCreate8");
    ++_dsound_lib_ref_count;

    return direct_sound_settings_load(platform_window, setting, api_setting);
}

inline
void direct_sound_play(AudioSetting*, DirectSoundSetting* const __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->secondary_buffer);
    /*if (!api_setting->secondary_buffer) {
        return;
    }*/

    api_setting->secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
}

inline
void direct_sound_stop(AudioSetting*, DirectSoundSetting* const __restrict api_setting) NO_EXCEPT
{
    ASSERT_TRUE(api_setting->secondary_buffer);
    /*if (!api_setting->secondary_buffer) {
        return;
    }*/

    api_setting->secondary_buffer->Stop();
}

inline
void direct_sound_free(AudioSetting*, DirectSoundSetting* __restrict api_setting) NO_EXCEPT
{
    if (_dsound_lib_ref_count > 1) {
        --_dsound_lib_ref_count;
        return;
    }

    if (api_setting->direct_sound_handle) {
        api_setting->direct_sound_handle->Release();
    }

    if (api_setting->primary_buffer) {
        api_setting->primary_buffer->Release();
    }

    if (api_setting->secondary_buffer) {
        api_setting->secondary_buffer->Release();
    }

    library_dyn_unload(&_dsound_lib);

    _dsound_lib_ref_count = 0;
}

/**
 * Calculates the samples in bytes to generate for the buffer
 */
inline
uint32 direct_sound_buffer_fillable(const AudioSetting* __restrict setting, const DirectSoundSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_BUFFER_FILLABLE);

    DWORD player_cursor;
    DWORD write_cursor;
    if (!SUCCEEDED(api_setting->secondary_buffer->GetCurrentPosition(&player_cursor, &write_cursor))) {
        LOG_1("[ERROR] DirectSound: GetCurrentPosition failed.");

        return 0;
    }

    const DWORD bytes_to_lock = (setting->sample_index * setting->sample_size) % setting->buffer_size;
    DWORD bytes_to_write = 0;

    const DWORD target_cursor = (player_cursor + (setting->latency * setting->sample_size)) % setting->buffer_size;

    // @bug Why does this case even exist?
    if (bytes_to_lock == player_cursor) {
        // @bug What if just started?
        bytes_to_write = 0;
    } else if (bytes_to_lock > target_cursor) {
        bytes_to_write = setting->buffer_size - bytes_to_lock;
        bytes_to_write += target_cursor;
    } else {
        bytes_to_write = target_cursor - bytes_to_lock;
    }

    return bytes_to_write;
}

inline
void direct_sound_play_buffer(AudioSetting* __restrict setting, DirectSoundSetting* __restrict api_setting) NO_EXCEPT
{
    PROFILE(PROFILE_AUDIO_PLAY_BUFFER);
    if (setting->sample_buffer_size == 0) {
        return;
    }

    void* region1;
    DWORD region1_size;

    void* region2;
    DWORD region2_size;

    const DWORD bytes_to_lock = (setting->sample_index * setting->sample_size) % setting->buffer_size;

    api_setting->secondary_buffer->Lock(
        bytes_to_lock, setting->sample_buffer_size,
        &region1, &region1_size,
        &region2, &region2_size,
        0
    );

    memcpy(
        (void *) region1,
        (void *) setting->buffer,
        region1_size
    );

    if (region2_size > 0) {
        memcpy(
            (void *) region2,
            (void *) (setting->buffer + region1_size),
            region2_size
        );
    }

    api_setting->secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    setting->sample_index += (uint16) (setting->sample_buffer_size / setting->sample_size);
    setting->sample_buffer_size = 0;
}

#endif