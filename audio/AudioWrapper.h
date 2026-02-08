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

#endif