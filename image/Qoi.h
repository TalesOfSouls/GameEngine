/**
 * Jingga
 *
 * @copyright 2021, Dominic Szablewski - https://phoboslab.org
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_IMAGE_QOI_H
#define COMS_IMAGE_QOI_H

#include "../stdlib/Stdlib.h"
#include "Image.cpp"

#define QOI_OP_INDEX 0b00000000
#define QOI_OP_DIFF 0b01000000
#define QOI_OP_LUMA 0b10000000
#define QOI_OP_RUN 0b11000000

// These definitions are important and impact how large our run can be:
// Run has 6 free bits -> 2^6 = 64
// However, the first bit is used to indicate RGB or RGBA -> 64 - 2^1 = 62
#define QOI_OP_RGB 0b11111110
#define QOI_OP_RGBA 0b11111111

#define QOI_MASK_2 0b11000000

static const CONSTEXPR byte qoi_padding[8] = {0,0,0,0,0,0,0,1};

int32 qoi_encode(const Image* const image, byte* data) NO_EXCEPT
{
    const byte* const start = data;

    v4_byte px_prev = {0, 0, 0, 255};
	v4_byte px = px_prev;

	const int32 channels = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);

	const int32 px_len = image->width * image->height * channels;
	const int32 px_end = px_len - channels;

	v4_byte index[64];
	memset(index, 0, sizeof(index));

	ASSERT_STRICT(((uintptr_t) image->pixels) % 4 == 0);

    int32 run = 0;
	for (int32 px_pos = 0; px_pos < px_len; px_pos += channels) {
		if (channels == 4) { LIKELY
			px.val = SWAP_ENDIAN_LITTLE(*((uint32 *) (image->pixels + px_pos)));
		} else {
			px.r = image->pixels[px_pos + 0];
			px.g = image->pixels[px_pos + 1];
			px.b = image->pixels[px_pos + 2];
		}

		if (px.val == px_prev.val) {
			++run;
			if (run == 62 || px_pos == px_end) {
				*data++ = (byte) (QOI_OP_RUN | (run - 1));
				run = 0;
			}
		} else {
			if (run > 0) {
				*data++ = (byte) (QOI_OP_RUN | (run - 1));
				run = 0;
			}

			byte index_pos = (px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & (64 - 1);
			if (index[index_pos].val == px.val) {
				*data++ = QOI_OP_INDEX | index_pos;
			} else {
				index[index_pos] = px;

				if (px.a == px_prev.a) {
					char vr = px.r - px_prev.r;
					char vg = px.g - px_prev.g;
					char vb = px.b - px_prev.b;

					char vg_r = vr - vg;
					char vg_b = vb - vg;

					if (vr > -3 && vr < 2
						&& vg > -3 && vg < 2
						&& vb > -3 && vb < 2
					) {
						*data++ = QOI_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
					} else if (vg_r > -9 && vg_r < 8
						&& vg > -33 && vg < 32
						&& vg_b > -9 && vg_b < 8
					) {
						*data++ = QOI_OP_LUMA | (vg + 32);
						*data++ = (vg_r + 8) << 4 | (vg_b + 8);
					} else {
						*data++ = QOI_OP_RGB;
						*data++ = px.r;
						*data++ = px.g;
						*data++ = px.b;
					}
				} else {
					*data++ = QOI_OP_RGBA;
					*data++ = px.r;
					*data++ = px.g;
					*data++ = px.b;
					*data++ = px.a;
				}
			}
		}

		px_prev = px;
	}

	for (int i = 0; i < (int)sizeof(qoi_padding); ++i) {
		*data++ = qoi_padding[i];
	}

	return (int32) (data - start);
}

inline
int32 qoi_decode(const byte* data, Image* const image) NO_EXCEPT
{
	LOG_3("[INFO] QOI decode image");
    const int32 channels = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
	const uint32 px_len = image->width * image->height * channels;
    v4_byte px = {0, 0, 0, 255};
    int32 run = 0;

	v4_byte index[64];
	memset(index, 0, sizeof(index));

	const int32 chunks_len = px_len - (int) sizeof(qoi_padding);
	int32 p = 0;

	for (uint32 px_pos = 0; px_pos < px_len; px_pos += channels) {
		if (run > 0) {
			--run;
		} else if (p < chunks_len) {
			const byte b1 = data[p++];

			if (b1 == QOI_OP_RGB) {
				px.r = data[p++];
				px.g = data[p++];
				px.b = data[p++];
			} else if (b1 == QOI_OP_RGBA) {
				px.r = data[p++];
				px.g = data[p++];
				px.b = data[p++];
				px.a = data[p++];
			} else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX) {
				px = index[b1];
			} else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF) {
				px.r += ((b1 >> 4) & 0x03) - 2;
				px.g += ((b1 >> 2) & 0x03) - 2;
				px.b += (b1 & 0x03) - 2;
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA) {
				byte b2 = data[p++];
				byte vg = (b1 & 0x3f) - 32;
				px.r += vg - 8 + ((b2 >> 4) & 0x0f);
				px.g += vg;
				px.b += vg - 8 + (b2 & 0x0f);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
				run = (b1 & 0x3f);
			}

			index[(px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & (64 - 1)] = px;
		}

		image->pixels[px_pos + 0] = px.r;
		image->pixels[px_pos + 1] = px.g;
		image->pixels[px_pos + 2] = px.b;

		if (channels == 4) {
			image->pixels[px_pos + 3] = px.a;
		}
	}

    return 0;
}

#endif