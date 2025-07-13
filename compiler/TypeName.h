/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_COMPILER_UTILS_H
#define COMS_COMPILER_COMPILER_UTILS_H

#include "../utils/TestUtils.h"
#include "../stdlib/Types.h"

#if _MSC_VER
    #include "msvc/TypeName.h"
#else
    #include "gcc/TypeName.h"
#endif

template<typename T>
struct NormalizedTypeName {
    static constexpr const char* Get() {
        return GetRawTypeName<T>();
    }
};

#define REGISTER_TYPENAME(Actual) \
    template<> \
    struct NormalizedTypeName<Actual> { \
        static constexpr const char* Get() { return #Actual; } \
    };

constexpr const char* RemoveQualifiers(const char* typeName) {
    const char* qualifiers[] = {"enum ", "struct ", "class ", "const ", "volatile ", "restrict "};

    for (const char* qual : qualifiers) {
        size_t len = str_length_constexpr(qual);
        if (str_compare(typeName, qual, len) == 0) {
            return typeName + len;
        }
    }
    return typeName;
}

// Not allowed since uint8 = unsigned char
//REGISTER_TYPENAME(byte)
REGISTER_TYPENAME(bool)

REGISTER_TYPENAME(uint8)
REGISTER_TYPENAME(uint16)
REGISTER_TYPENAME(uint32)
REGISTER_TYPENAME(uint64)

REGISTER_TYPENAME(int8)
REGISTER_TYPENAME(int16)
REGISTER_TYPENAME(int32)
REGISTER_TYPENAME(int64)

REGISTER_TYPENAME(f32)
REGISTER_TYPENAME(f64)

template<typename T>
constexpr const char* GetTypeName() {
    const char* raw_name = NormalizedTypeName<T>::Get();
    return RemoveQualifiers(raw_name);
}

#endif