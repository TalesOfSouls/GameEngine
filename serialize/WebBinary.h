/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SERIALICE_WEB_BINARY
#define COMS_SERIALICE_WEB_BINARY

#include <string.h>
#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../compiler/TypeName.h"

struct WebBinaryValue {
    const char* name;
    const char* type;
    const char* nested_schema = NULL;
};

CONSTEXPR
void web_binary_copy(char* dest, int32 src) {
    for (size_t i = 0; i < sizeof(int32); ++i) {
        dest[i] = static_cast<char>(src >> (8 * i));
    }
}

template<const WebBinaryValue* binary_struct, int32 count>
CONSTEXPR int32 web_binary_schema_size() {
    int32 size = 0;
    for (int32 i = 0; i < count; ++i) {
        size += strlen(binary_struct[i].name) + 1;
        size += strlen(binary_struct[i].type) + 1;

        // Add size for nested schema if present
        if (binary_struct[i].nested_schema) {
            size += strlen(binary_struct[i].nested_schema) + 1;
        } else {
            size += 1; // Empty string for no nested schema
        }
    }

    return size;
}

template<size_t N>
struct WebBinarySchema {
    char data[N];
    CONSTEXPR WebBinarySchema() : data{} {}
    CONSTEXPR const char* c_str() const { return data; }
};

template<const WebBinaryValue* binary_struct, int32 count>
CONSTEXPR auto web_binary_schema() {
    CONSTEXPR int32 size = web_binary_schema_size<binary_struct, count>();
    WebBinarySchema<size> schema;

    char* buffer = schema.data;
    for (int32 i = 0; i < count; ++i) {
        str_copy(buffer, binary_struct[i].name);
        buffer += strlen(binary_struct[i].name) + 1;

        str_copy(buffer, binary_struct[i].type);
        buffer += strlen(binary_struct[i].type) + 1;

        // Write nested schema if present
        if (binary_struct[i].nested_schema) {
            str_copy(buffer, binary_struct[i].nested_schema);
            buffer += strlen(binary_struct[i].nested_schema) + 1;
        } else {
            *buffer++ = '\0'; // Empty string
        }

        web_binary_copy(buffer, binary_struct[i].offset);
        buffer += sizeof(int32);

        web_binary_copy(buffer, binary_struct[i].length);
        buffer += sizeof(int32);
    }

    return schema;
}

#define WEB_BINARY_FIELD(StructType, Field) \
    { \
        #Field, \
        GetTypeName<decltype(StructType::Field)>() \
    }

#define WEB_BINARY_FIELD_WITH_SCHEMA(StructType, Field, Schema) \
    { \
        #Field, \
        GetTypeName<decltype(StructType::Field)>(), \
        Schema.c_str() \
    }

#endif