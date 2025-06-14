/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_WAV_H
#define COMS_AUDIO_WAV_H

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "../utils/EndianUtils.h"
#include "Audio.h"

// See: https://en.wikipedia.org/wiki/WAV
// IMPORTANT: Remember that we are not using packing for the headers
//      Because of that the struct size is different from the actual header size in the file
//      This means we have to manually asign the data to the headers

// Packed header size
#define WAV_HEADER_SIZE 44
struct WavHeader {
    // RIFF header
    byte file_type_bloc_id[4];
    uint32 size;
    byte file_format_id[4];

    // Data format header
    byte format_bloc_id[4];
    uint32 bloc_size;
    uint16 audio_format;
    uint16 nbr_channels;
    uint32 frequency;
    uint32 byte_per_sec;
    uint16 byte_per_bloc;
    uint16 bits_per_sample;

    // Sample data header
    byte data_bloc_id[4];
    uint32 data_size;
};

struct Wav {
    WavHeader header;

    byte* sample_data; // WARNING: This is not the owner of the data.

    uint32 size;
    byte* data; // Data owner
};

void generate_default_wav_references(const byte* data, uint32 size, Wav* wav)
{
    wav->size = size;
    ASSERT_SIMPLE(size >= WAV_HEADER_SIZE);

    // Check if we can copy memory directly
    // The struct layout and header size should match on x86, but we still check it
    if constexpr (sizeof(WavHeader) == WAV_HEADER_SIZE) {
        memcpy(&wav->header, data, WAV_HEADER_SIZE);

        // swap endian if we are on big endian system
        #if !_WIN32 && !__LITTLE_ENDIAN__
            wav->header.size = SWAP_ENDIAN_LITTLE(wav->header.size);
            wav->header.bloc_size = SWAP_ENDIAN_LITTLE(wav->header.bloc_size);
            wav->header.audio_format = SWAP_ENDIAN_LITTLE(wav->header.audio_format);
            wav->header.nbr_channels = SWAP_ENDIAN_LITTLE(wav->header.nbr_channels);
            wav->header.frequency = SWAP_ENDIAN_LITTLE(wav->header.frequency);
            wav->header.byte_per_sec = SWAP_ENDIAN_LITTLE(wav->header.byte_per_sec);
            wav->header.byte_per_bloc = SWAP_ENDIAN_LITTLE(wav->header.byte_per_bloc);
            wav->header.bits_per_sample = SWAP_ENDIAN_LITTLE(wav->header.bits_per_sample);
            wav->header.data_size = SWAP_ENDIAN_LITTLE(wav->header.data_size);
        #endif
    } else {
        // RIFF header
        wav->header.file_type_bloc_id[0] = *(wav->data + 0);
        wav->header.file_type_bloc_id[1] = *(wav->data + 1);
        wav->header.file_type_bloc_id[2] = *(wav->data + 2);
        wav->header.file_type_bloc_id[3] = *(wav->data + 3);
        // should be (0x52, 0x49, 0x46, 0x46)

        wav->header.size = *(wav->data + 4);
        SWAP_ENDIAN_LITTLE(&wav->header.size);
        // should be file size - 8 bytes

        wav->header.file_format_id[0] = *(wav->data + 8);
        wav->header.file_format_id[1] = *(wav->data + 9);
        wav->header.file_format_id[2] = *(wav->data + 10);
        wav->header.file_format_id[3] = *(wav->data + 11);
        // should be (0x57, 0x41, 0x56, 0x45)

        // Data format header
        wav->header.format_bloc_id[0] = *(wav->data + 12);
        wav->header.format_bloc_id[1] = *(wav->data + 13);
        wav->header.format_bloc_id[2] = *(wav->data + 14);
        wav->header.format_bloc_id[3] = *(wav->data + 15);
        // should be (0x66, 0x6D, 0x74, 0x20)

        wav->header.bloc_size = SWAP_ENDIAN_LITTLE(*((uint32 *) (wav->data + 16)));
        // should be 16

        wav->header.audio_format = SWAP_ENDIAN_LITTLE(*((uint16 *) (wav->data + 20)));
        wav->header.nbr_channels = SWAP_ENDIAN_LITTLE(*((uint16 *) (wav->data + 22)));
        wav->header.frequency = SWAP_ENDIAN_LITTLE(*((uint32 *) (wav->data + 24)));

        wav->header.byte_per_sec = SWAP_ENDIAN_LITTLE(*((uint32 *) (wav->data + 28)));
        // should be frequency * byte_per_bloc

        wav->header.byte_per_bloc = SWAP_ENDIAN_LITTLE(*((uint16 *) (wav->data + 32)));
        // should be nbr channels * bits_per_sample / 8

        wav->header.bits_per_sample = SWAP_ENDIAN_LITTLE(*((uint16 *) (wav->data + 34)));

        // Sample data header
        memcpy(wav->header.data_bloc_id, wav->data + 36, 4);

        wav->header.data_size = SWAP_ENDIAN_LITTLE(*((uint32 *) *(wav->data + WAV_HEADER_SIZE - sizeof(wav->header.data_bloc_id))));
    }

    wav->sample_data = wav->data + WAV_HEADER_SIZE;
    memcpy(wav->sample_data, data + WAV_HEADER_SIZE, wav->header.data_size);
}

void wav_from_data(const byte* data, uint32 size, Audio* audio, RingMemory* ring)
{
    // @performance We are generating the struct and then filling the data.
    //      There is some assignment/copy overhead
    Wav src = {};
    src.data = ring_get_memory(ring, size, 4);
    generate_default_wav_references(data, size, &src);

    if (!src.size) {
        return;
    }

    audio->sample_rate = (uint16) src.header.frequency;
    audio->sample_size = (byte) ((src.header.bits_per_sample / 8) * src.header.nbr_channels);
    audio->channels = (byte) src.header.nbr_channels;
    audio->byte_per_sec = (uint32) src.header.byte_per_sec;
    audio->bloc_size = (byte) src.header.bloc_size;
    audio->size = src.header.data_size;

    memcpy((void *) audio->data, src.sample_data, audio->size);
}

#endif