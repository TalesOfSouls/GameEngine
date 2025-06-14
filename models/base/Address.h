/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BASE_ADDRESS_H
#define COMS_MODELS_BASE_ADDRESS_H

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../serialize/WebBinary.h"
#include "GeoLocation.h"

#ifndef MAX_LOCATION_POSTAL_LENGTH
    #define MAX_LOCATION_POSTAL_LENGTH 32
#endif

#ifndef MAX_LOCATION_CITY_LENGTH
    #define MAX_LOCATION_CITY_LENGTH 32
#endif

#ifndef MAX_LOCATION_ADDRESS_LENGTH
    #define MAX_LOCATION_ADDRESS_LENGTH 32
#endif

#ifndef MAX_LOCATION_STATE_LENGTH
    #define MAX_LOCATION_STATE_LENGTH 32
#endif

struct Location {
    uint64 id;

    char postal[MAX_LOCATION_POSTAL_LENGTH];
    char city[MAX_LOCATION_CITY_LENGTH];
    char address[MAX_LOCATION_ADDRESS_LENGTH];
    char state[MAX_LOCATION_ADDRESS_LENGTH];

    GeoLocation geo;

    //ISO3166 country;
    //AddressType type;
};

// @question do we really want this to be part of the binary or should this be stored in a file/template?
constexpr WebBinaryValue LocationSchemaStruct[] = {
    WEB_BINARY_FIELD(Location, id),
    WEB_BINARY_FIELD(Location, postal),
    WEB_BINARY_FIELD(Location, city),
    WEB_BINARY_FIELD(Location, address),
    WEB_BINARY_FIELD(Location, state),
    WEB_BINARY_FIELD_WITH_SCHEMA(Location, geo, GeoLocationSchema)
    //WEB_BINARY_FIELD(Location, country),
    //WEB_BINARY_FIELD(Location, type)
};

constexpr auto LocationSchema = web_binary_schema<
    LocationSchemaStruct,
    ARRAY_COUNT(LocationSchemaStruct)
>();

#endif