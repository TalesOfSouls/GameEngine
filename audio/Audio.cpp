/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUDIO_C
#define COMS_AUDIO_C

#include "../utils/StringUtils.h"
#include "../memory/RingMemory.h"
#include "../system/FileUtils.cpp"

#include "Audio.h"
#include "AudioSetting.h"
#include "Wav.h"

void audio_from_file(Audio* __restrict audio, const char* __restrict path, RingMemory* __restrict ring)
{
    FileBody file = {};
    file_read(path, &file, ring);

    ASSERT_SIMPLE(file.size);

    if (str_ends_with(path, ".wav")) {
        wav_from_data(file.content, (uint32) file.size, audio, ring);
    }
}

int32 audio_data_size(const Audio* audio)
{
    return (int32) (audio->size
        + sizeof(audio->sample_rate)
        + sizeof(audio->channels)
        // + sizeof(audio->bloc_size) bit fiddling
        + sizeof(audio->size)
    );
}

inline
uint32 audio_header_from_data(const byte* __restrict data, Audio* __restrict audio)
{
    const byte* start = data;

    audio->sample_rate = SWAP_ENDIAN_LITTLE(*((uint16 *) data));
    data += sizeof(audio->sample_rate);

    audio->channels = (*data >> 4) & 0x0F;
    audio->bloc_size = *data & 0x0F;
    data += sizeof(byte);

    audio->sample_size = audio->channels * audio->bloc_size;

    audio->byte_per_sec = audio->sample_rate * audio->sample_size;

    audio->size = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(audio->size);

    return (int32) (data - start);
}

inline
uint32 audio_header_to_data(const Audio* __restrict audio, byte* __restrict data)
{
    byte* start = data;

    *((uint16 *) data) = SWAP_ENDIAN_LITTLE(audio->sample_rate);
    data += sizeof(audio->sample_rate);

    // Reducing data footprint through bit fiddling
    *data = ((audio->channels & 0x0F) << 4) | (audio->bloc_size & 0x0F);
    data += sizeof(byte);

    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(audio->size);
    data += sizeof(audio->size);

    return (int32) (data - start);
}

uint32 audio_from_data(const byte* __restrict data, Audio* __restrict audio)
{
    LOG_3("Load audio");
    data += audio_header_from_data(data, audio);

    memcpy(audio->data, data, audio->size);
    data += audio->size;

    LOG_3("Loaded audio");

    return audio_data_size(audio);
}

uint32 audio_to_data(const Audio* __restrict audio, byte* __restrict data)
{
    data += audio_header_to_data(audio, data);

    memcpy(data, audio->data, audio->size);
    data += audio->size;

    return audio_data_size(audio);
}

#endif
