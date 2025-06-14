/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BASE_GEO_LOCATION_H
#define COMS_MODELS_BASE_GEO_LOCATION_H

#include "../../stdlib/Types.h"
#include "../../serialize/WebBinary.h"

struct GeoLocation {
    f64 lat;
    f64 lon;
};

constexpr WebBinaryValue GeoLocationSchemaStruct[] = {
    WEB_BINARY_FIELD(GeoLocation, lat),
    WEB_BINARY_FIELD(GeoLocation, lon)
};

constexpr auto GeoLocationSchema = web_binary_schema<
    GeoLocationSchemaStruct,
    ARRAY_COUNT(GeoLocationSchemaStruct)
>();

#endif