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
};

#endif