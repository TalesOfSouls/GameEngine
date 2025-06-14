/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_H
#define COMS_AUDIO_H

#include "../stdlib/Types.h"

// This represents the audio file
struct Audio {
    // bits per sample
    // usually 48000 or 44100
    uint16 sample_rate;

    // audio channels
    // usually 2
    byte channels;

    // usually 16 = 2
    byte bloc_size;

    // bytes per bloc
    // channel count * bit
    // usually 2 * 16 = 4
    // channels * bloc_size
    byte sample_size;

    // sample_rate * sample_size
    uint32 byte_per_sec;

    uint32 size;
    byte* data; // owner of data
};

#endif
