/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_TEXTUREATLAS_H
#define COMS_MODELS_TEXTUREATLAS_H

#include "Texture.h"

/**
 * TEXT FORMAT: .atlastxt
 *
 *      texture: ./ui_atlas.png        <- atlas image (asset path; a number = existing asset id)
 *      image_width: 1024              <- used to normalize the coordinates
 *      image_height: 1024
 *
 *      #hud_minimap                   <- element, the id is the order of appearance
 *      0 0                            <- polygon point in atlas pixels, one "x y" per line
 *      225 0
 *      225 230
 *      0 230
 *
 *      #hud_globes
 *      225 0
 *      ...
 *
 *      The header ends at the first line that is not texture/image_width/image_height.
 *      An element is the list of points between its "#name" and the next "#name".
 *      The name is only used at build/load time (generated <name>.h enum, UINameId
 *      tables), the binary format stores no names - only the element index.
 *
 * BINARY FORMAT: ASSET_TYPE_TEXTURE_ATLAS payload, written by atlas_to_data()
 *
 *      offset      size        type                    field
 *      0           4           int32                   element_count = E
 *      4           4           int32                   uv_count = U
 *      8           8 * E       TextureAtlasElement[E]  element table
 *      8 + 8*E     8 * U       v2_f32[U]              texture coordinates
 *      total       8 + 8*E + 8*U
 *
 * TextureAtlasElement (8 bytes)
 *
 *      0   4   int32   uv_count    amount of points of this element
 *      4   4   int32   uv_start    offset into the global uv array
 *
 * v2_f32 (8 bytes): f32 x, f32 y - normalized (0-1) atlas coordinates.
 *
 * There is no version field. The texture is NOT part of this payload: it is a
 * separate ASSET_TYPE_IMAGE asset that the archive builder adds as a dependency
 * (@see AssetArchive.h). atlas_from_data() aligns the runtime uv array to 64 bytes
 * for AVX-512, the file itself contains no padding.
 */
struct TextureAtlasElement {
    int32 uv_count;

    // Offset into "uv" in the TextureAtlas struct
    int32 uv_start;
};

struct TextureAtlas {
    // The data before the elements can be considered header data
    // @performance I hate that this is here. The only place that uses this is the archive_builder
    char texture_name[32];
    Texture* texture;

    int32 element_count;
    int32 uv_count;

    TextureAtlasElement* elements;
    v2_f32* uv;
};

#endif