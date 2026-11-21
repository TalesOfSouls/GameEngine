/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ASSET_TYPE_H
#define COMS_ASSET_TYPE_H

#include "../stdlib/Stdlib.h"

enum AssetType : byte {
    ASSET_TYPE_GENERAL,
    ASSET_TYPE_OBJ,
    ASSET_TYPE_AUDIO,
    ASSET_TYPE_LANGUAGE,
    ASSET_TYPE_FONT,
    ASSET_TYPE_TEXTURE_ATLAS,
    ASSET_TYPE_THEME,
    ASSET_TYPE_IMAGE,

    // Model file: model + materials/animations/particles
    ASSET_TYPE_MODEL,

    ASSET_TYPE_SIZE
};

#endif