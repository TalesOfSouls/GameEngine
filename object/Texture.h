/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_TEXTURE_H
#define COMS_MODELS_TEXTURE_H

#define TEXTURE_DATA_TYPE_2D 0
#define TEXTURE_DATA_TYPE_1D 1
#define TEXTURE_DATA_TYPE_3D 2
#define TEXTURE_DATA_TYPE_1D_ARRAY 3
#define TEXTURE_DATA_TYPE_2D_ARRAY 4
#define TEXTURE_DATA_TYPE_2D_MULTISAMPLED 5
#define TEXTURE_DATA_TYPE_2D_MULTISAMPLED_ARRAY 6

#define TEXTURE_WRAP_TYPE_NONE 0
#define TEXTURE_WRAP_TYPE_CLAMP_TO_EDGE 1
#define TEXTURE_WRAP_TYPE_CLAMP_TO_BORDER 2
#define TEXTURE_WRAP_TYPE_REPEAT 3
#define TEXTURE_WRAP_TYPE_MIRRORED_REPEAT 4
#define TEXTURE_WRAP_TYPE_MIRRORED_CLAMP_TO_EDGE 5

#define TEXTURE_MINIFICATION_NONE 0
#define TEXTURE_MINIFICATION_NEAREST 1
#define TEXTURE_MINIFICATION_LINEAR 2
#define TEXTURE_MINIFICATION_NEAREST_MIPMAP_NEAREST 3
#define TEXTURE_MINIFICATION_LINEAR_MIPMAP_NEAREST 4
#define TEXTURE_MINIFICATION_NEAREST_MIPMAP_LINEAR 5
#define TEXTURE_MINIFICATION_LINEAR_MIPMAP_LINEAR 6

#include "../stdlib/Types.h"
#include "../image/Image.h"

struct Texture {
    uint32 id;
    byte sample_id;

    // @question Should the texture hold the texture unit?
    //  If yes remember to update prepare_texture()

    byte texture_data_type;
    byte texture_minification;
    byte data[16];

    Image image;
};

#endif