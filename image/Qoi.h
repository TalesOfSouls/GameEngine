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

#include "../stdlib/Types.h"
#include <string.h>
#include "Image.cpp"

#define QOI_OP_LUMA555 0b00000000
#define QOI_OP_LUMA222 0b10000000
#define QOI_OP_LUMA777 0b01000000

#define QOI_OP_RUN 0b11000000

// These definitions are important and impact how large our run can be:
// Run has 6 free bits -> 2^6 = 64
// However, the first bit is used to indicate RGB or RGBA -> 64 - 2^1 = 62
#define QOI_OP_RGB  0b11111110
#define QOI_OP_RGBA 0b11111111

#define QOI_MASK_1 0b10000000
#define QOI_MASK_2 0b11000000
#define QOI_MASK_3 0b11100000

// @performance I feel like there is some more optimization possible by handling fully transparent pixels in a special way
// @todo We need to implement monochrome handling, which is very important for game assets that often use monochrome assets for all kinds of things (e.g. translucency)

static const byte optable[128] = {
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
};

int32 qoi_encode(const Image* image, byte* data) noexcept
{
    byte* start = data;
    data += image_header_to_data(image, data);

    v4_byte index[64];
	memset(index, 0, sizeof(index));

    v4_byte px_prev = {0, 0, 0, 255};
	v4_byte px = px_prev;

	const int32 channels = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);

	int32 px_len = image->width * image->height * channels;
	int32 px_end = px_len - channels;

    int32 run = 0;
	if (channels == 4) {
		for (int32 px_pos = 0; px_pos < px_len; px_pos += 4) {
			px.val = SWAP_ENDIAN_LITTLE(*((uint32 *) (image->pixels + px_pos)));

			while(px.val == px_prev.val) {
				++run;
				if(px_pos == px_end) {
					*data++ = (byte) (QOI_OP_RUN | (run - 1));
					px_pos = px_len;

                    break;
				} else if (run == 62) {
					*data++ = (byte) (QOI_OP_RUN | (run - 1));
					run = 0;
				}

				px_pos += 4;
                px.val = SWAP_ENDIAN_LITTLE(*((uint32 *) (image->pixels + px_pos)));
			}

			if (run) {
				*data++ = (byte) (QOI_OP_RUN | (run - 1));
				run = 0;
			}

			if(px.a != px_prev.a){
				*data++ = QOI_OP_RGBA;
				*data++ = px.a;
			}

			signed char vr = px.r - px_prev.r;
            signed char vg = px.g - px_prev.g;
            signed char vb = px.b - px_prev.b;

            signed char vg_r = vr - vg;
            signed char vg_b = vb - vg;

            byte ar = vg_r < 0 ? -vg_r - 1 : vg_r;
            byte ag = vg < 0 ? -vg - 1 : vg;
            byte ab = vg_b < 0 ? -vg_b - 1 : vg_b;
            byte argb = ar | ag | ab;

            switch(optable[argb]) {
                case 0:
                    *data++ = QOI_OP_LUMA222 | ((vg_r + 2) << 4) | ((vg_b + 2) << 2) | (vg + 2);
                    break;
                case 1:
                    *data++ = QOI_OP_LUMA555 | ((vg_b + 16) << 2) | ((vg_r + 16) >> 3);
                    *data++ = (((vg_r + 16) & 7) << 5) | (vg + 16);
                    break;
                case 2:
                    *data++ = QOI_OP_LUMA777 | ((vg_b + 64) >> 2);
                    *data++ = (((vg_b + 64) & 3) << 6) | ((vg_r + 64) >> 1);
                    *data++ = (((vg_r + 64) & 1) << 7) | (vg + 64);
                    break;
                case 3:
                    *data++ = QOI_OP_RGB;
                    *data++ = px.r;
                    *data++ = px.g;
                    *data++ = px.b;
                    break;
            }

			px_prev = px;
		}
	} else {
		for (int32 px_pos = 0; px_pos < px_len; px_pos += 3) {
			px.r = image->pixels[px_pos];
			px.g = image->pixels[px_pos + 1];
			px.b = image->pixels[px_pos + 2];

			while(px.val == px_prev.val) {
				++run;
				if(px_pos == px_end) {
					*data++ = (byte) (QOI_OP_RUN | (run - 1));
					px_pos = px_len;

                    break;
				} else if (run == 62) {
					*data++ = (byte) (QOI_OP_RUN | (run - 1));
					run = 0;
				}

				px_pos += 3;
				px.r = image->pixels[px_pos];
				px.g = image->pixels[px_pos + 1];
				px.b = image->pixels[px_pos + 2];
			}

			if (run) {
				*data++ = (byte) (QOI_OP_RUN | (run - 1));
				run = 0;
			}

            signed char vr = px.r - px_prev.r;
            signed char vg = px.g - px_prev.g;
            signed char vb = px.b - px_prev.b;

            signed char vg_r = vr - vg;
            signed char vg_b = vb - vg;

            byte ar = vg_r < 0 ? -vg_r - 1 : vg_r;
            byte ag = vg < 0 ? -vg - 1 : vg;
            byte ab = vg_b < 0 ? -vg_b - 1 : vg_b;
            byte argb = ar | ag | ab;

            switch(optable[argb]) {
                case 0:
                    *data++ = QOI_OP_LUMA222 | ((vg_r + 2) << 4) | ((vg_b + 2) << 2) | (vg + 2);
                    break;
                case 1:
                    *data++ = QOI_OP_LUMA555 | ((vg_b + 16) << 2) | ((vg_r + 16) >> 3);
                    *data++ = (((vg_r + 16) & 7) << 5) | (vg + 16);
                    break;
                case 2:
                    *data++ = QOI_OP_LUMA777 | ((vg_b + 64) >> 2);
                    *data++ = (((vg_b + 64) & 3) << 6) | ((vg_r + 64) >> 1);
                    *data++ = (((vg_r + 64) & 1) << 7) | (vg + 64);
                    break;
                case 3:
                    *data++ = QOI_OP_RGB;
                    *data++ = px.r;
                    *data++ = px.g;
                    *data++ = px.b;
                    break;
            }

			px_prev = px;
		}
	}

	return (int32) (data - start);
}

static
int32 qoi_decode_4(const byte* data, Image* image) noexcept
{
    uint32 px_len = image->width * image->height * 4;
    v4_byte px = {0, 0, 0, 255};
    v4_byte index[64] = {0};
    int32 run = 0;

    for (uint32 px_pos = 0; px_pos < px_len; px_pos += 4) {
		if (run > 0) {
			--run;
		} else {
            OP_RGBA_GOTO:
			byte b1 = *data++;

			if (b1 == QOI_OP_RGB) {
				px.r = *data++;
				px.g = *data++;
				px.b = *data++;
            } else if (b1 == QOI_OP_RGBA) {
				px.a = *data++;
				goto OP_RGBA_GOTO;
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA222) {
				byte vg = (b1 & 3) - 2;
				px.r += vg - 2 + ((b1 >> 4) & 3);
				px.g += vg;
				px.b += vg - 2 + ((b1 >> 2) & 3);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA555) {
				byte b2 = *data++;
				byte vg = (b2 & 31) - 16;
				px.r += vg - 16 + (((b1 & 3) << 3) | (b2 >> 5));
				px.g += vg;
				px.b += vg - 16 + ((b1 >> 2) & 31);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA777) {
				byte b2 = *data++;
				byte b3 = *data++;
				byte vg = (b3 & 0x7f) - 64;
				px.r += vg - 64 + ((b2 & 0x3f) << 1) + (b3 >> 7);
				px.g += vg;
				px.b += vg - 64 + ((b1 & 0x1f) << 2) + (b2 >> 6);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
				run = (b1 & 0x3f);
			}
		}

		*((uint32 *) &image->pixels[px_pos]) = SWAP_ENDIAN_LITTLE(px.val);
	}

    return px_len;
}

static
int32 qoi_decode_3(const byte* data, Image* image) noexcept
{
	uint32 px_len = image->width * image->height * 3;
    v3_byte px = {0, 0, 0};
    int32 run = 0;

	for (uint32 px_pos = 0; px_pos < px_len; px_pos += 3) {
		if (run > 0) {
			--run;
		} else {
			byte b1 = *data++;

			if (b1 == QOI_OP_RGB) {
				px.r = *data++;
				px.g = *data++;
				px.b = *data++;
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA222) {
				byte vg = (b1 & 3) - 2;
				px.r += vg - 2 + ((b1 >> 4) & 3);
				px.g += vg;
				px.b += vg - 2 + ((b1 >> 2) & 3);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA555) {
				byte b2 = *data++;
				byte vg = (b2 & 31) - 16;
				px.r += vg - 16 + (((b1 & 3) << 3) | (b2 >> 5));
				px.g += vg;
				px.b += vg - 16 + ((b1 >> 2) & 31);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA777) {
				byte b2 = *data++;
				byte b3 = *data++;
				byte vg = (b3 & 0x7f) - 64;
				px.r += vg - 64 + ((b2 & 0x3f) << 1) + (b3 >> 7);
				px.g += vg;
				px.b += vg - 64 + ((b1 & 0x1f) << 2) + (b2 >> 6);
			} else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
				run = (b1 & 0x3f);
			}
		}

		image->pixels[px_pos] = px.r;
		image->pixels[px_pos + 1] = px.g;
		image->pixels[px_pos + 2] = px.b;
	}

    return px_len;
}

int32 qoi_decode(const byte* data, Image* image) noexcept
{
	LOG_3("QOI decode image");
    int32 header_length = image_header_from_data(data, image);

    const int32 channels = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);

    int32 len = 0;
    if (channels == 4) {
        len = qoi_decode_4(data + header_length, image);
    } else if (channels == 3) {
        len = qoi_decode_3(data + header_length, image);
    }

    return header_length + len;
}

#endif