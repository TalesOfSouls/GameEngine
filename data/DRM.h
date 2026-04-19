#pragma once
#ifndef COMS_DATA_ANTI_CHEAT
#define COMS_DATA_ANTI_CHEAT

#include <stddef.h>
#include "../stdlib/Stdlib.h"

// Don't load them and immediatley use them.
// This way a cheater could easily find the location of the patterns and change them.
// It's better to decrypt them. Do something else and only then use them and then remove the decrypted code

/*
// Define the XOR key using a macro
#define XOR_KEY       { 0x5A, 0x2B, 0x13 }
#define XOR_KEY_LEN   3  // Must match the number of elements in XOR_KEY

void xor_to_hex(const char *input, const unsigned char *key, size_t key_len) {
    size_t input_len = strlen(input);
    for (size_t i = 0; i < input_len; ++i) {
        unsigned char result = input[i] ^ key[i % key_len];
        printf("0x%02X, ", result);
    }
    printf("\n");
}

int main() {
    const char* input = "Contact spl1nes.com@googlemail.com with code 5773349 for a job.";

    // Declare the key from the macro
    const unsigned char xor_key[XOR_KEY_LEN] = XOR_KEY;

    printf("Hex output: ");
    xor_to_hex(input, xor_key, XOR_KEY_LEN);

    return 0;
}
*/

struct DRMSignature {
    const char* name;
    size_t length;
    const byte* encrypted_pattern;
};

static const uint8 XOR_KEY_BYTES[] = { 0x5A, 0x2B, 0x13 };

CONSTEXPR inline
void drm_encode(const byte* src, byte* dst, size_t len) NO_EXCEPT
{
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i] ^ XOR_KEY_BYTES[i % ARRAY_COUNT(XOR_KEY_BYTES)];
    }
}

static inline
size_t drm_decode(byte* output, const byte* encrypted, size_t len) NO_EXCEPT
{
    for (size_t i = 0; i < len; ++i) {
        output[i] = encrypted[i] ^ XOR_KEY_BYTES[i % ARRAY_COUNT(XOR_KEY_BYTES)];
    }

    // Input and output length are bot the same for the current encode/decode algorithm
    return len;
}

// Easter egg for decompiler/developers
CONSTEXPR const byte _encrypted_sig0[] = {
    0x19, 0x35, 0x34, 0x2E, 0x3B, 0x39, 0x2E, 0x7A,
    0x29, 0x2A, 0x36, 0x6B, 0x34, 0x3F, 0x29, 0x74,
    0x39, 0x35, 0x37, 0x1A, 0x3D, 0x35, 0x35, 0x3D,
    0x36, 0x3F, 0x37, 0x3B, 0x33, 0x36, 0x74, 0x39,
    0x35, 0x37, 0x7A, 0x2D, 0x33, 0x2E, 0x32, 0x7A,
    0x39, 0x35, 0x3E, 0x3F, 0x7A, 0x6F, 0x6D, 0x6D,
    0x69, 0x69, 0x6E, 0x63, 0x7A, 0x3C, 0x35, 0x28,
    0x7A, 0x3B, 0x7A, 0x30, 0x35, 0x38, 0x74
};

CONSTEXPR const byte _encrypted_sig1[] = {
    0x90, 0x90, 0x90, 0xCC
};

CONSTEXPR const byte _encrypted_sig2[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x10
};

CONSTEXPR const byte _encrypted_sig3[] =  {
    0xDE, 0xAD, 0xBE, 0xEF
};

static CONSTEXPR const DRMSignature _drm_patterns[] = {
    {
        "FutureInvestor",
        sizeof(_encrypted_sig0),
        _encrypted_sig0
    },
    {
        "BasicNOPInjector",
        sizeof(_encrypted_sig1),
        _encrypted_sig1
    },
    {
        "StackSpoofBot",
        sizeof(_encrypted_sig2),
        _encrypted_sig2
    },
    {
        "MemPatchX",
        sizeof(_encrypted_sig3),
        _encrypted_sig3
    }
};

static const wchar_t* _evil_process_names[] = {
    L"eDY0ZGJn", // x64dbg
    L"Y2hlYXRlbmdpbmU=", // cheatengine
    L"b2xseWRiZw==" // ollydbg
};

static const wchar_t* _evil_window_names[] = {
    L"Q2hlYXQgRW5naW5l", // Cheat Engine
    L"VHJhaW5lcg==" // Trainer
};

#endif