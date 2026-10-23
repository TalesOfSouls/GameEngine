/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SOUND_XAUDIO2_H
#define COMS_SOUND_XAUDIO2_H

#include <xaudio2.h>

// Amount of buffers we cycle through
// A buffer must not be modified while XAudio2 still has it queued
// -> we only fill a new buffer if less than XAUDIO2_BUFFER_COUNT buffers are queued
#define XAUDIO2_BUFFER_COUNT 3

struct XAudio2Setting {
    IXAudio2* xaudio2_handle;
    IXAudio2SourceVoice* source_voice;
    IXAudio2MasteringVoice* mastering_voice;

    // Size of every internal buffer in bytes (= latency)
    uint32 chunk_size;

    // Frames submitted to the source voice since it was created (@see xaudio2_buffer_delay)
    uint64 frames_submitted;

    // Did we successfully initialize COM on this thread (needs to be released again)
    bool com_initialized;

    XAUDIO2_BUFFER internal_buffer[XAUDIO2_BUFFER_COUNT];
};

#endif
