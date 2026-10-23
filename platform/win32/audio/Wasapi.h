/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SOUND_WASAPI_H
#define COMS_SOUND_WASAPI_H

#include <mmdeviceapi.h>
#include <audioclient.h>

struct WasapiSetting {
    IAudioClient* wasapi_handle;
    IAudioRenderClient* render_client;

    // Size of the WASAPI buffer in frames (1 frame = 1 sample for all channels)
    uint32 buffer_frame_count;

    // Did we successfully initialize COM on this thread (needs to be released again)
    bool com_initialized;
};

#endif
