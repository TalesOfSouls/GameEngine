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

#include <xaudio2.h>

struct XAudio2Setting {
    IXAudio2* xaudio2_handle;
    IXAudio2SourceVoice* source_voice;
    IXAudio2MasteringVoice* mastering_voice;

    XAUDIO2_BUFFER internal_buffer[2];
};

#endif
