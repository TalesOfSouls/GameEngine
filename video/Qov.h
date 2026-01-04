#ifndef COMS_IMAGE_QOI_H
#define COMS_IMAGE_QOI_H

#include "../stdlib/Stdlib.h"
#include <string.h>

#define QOI_OP_LUMA555_H 0b00000000
#define QOI_OP_LUMA222_H 0b01000000
#define QOI_OP_LUMA777_H 0b00100000
#define QOI_OP_RUN_H 0b01100000

#define QOI_OP_LUMA555_V 0b10000000
#define QOI_OP_LUMA222_V 0b11000000
#define QOI_OP_LUMA777_V 0b10100000
#define QOI_OP_RUN_V 0b11100000

// These definitions are important and impact how large our run can be:
// Run has 5 free bits -> 2^5 = 32
// However, the second bit is used to indicate RGB or RGBA -> 64 - 2^1-2^2 = 58
#define QOI_OP_RGB_H  0b11111100
#define QOI_OP_RGB_V  0b11111101

#define QOI_OP_RGBA_H 0b11111110
#define QOI_OP_RGBA_V 0b11111111

#define QOI_MASK_1 0b10000000
#define QOI_MASK_2 0b11000000
#define QOI_MASK_3 0b11100000

#endif