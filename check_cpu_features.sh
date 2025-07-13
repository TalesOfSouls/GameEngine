#!/bin/bash

check_cpu_features() {
    local FLAGS=""
    local MACROS=""

    # Check for SSE4.2
    if lscpu | grep -qi sse4_2; then
        FLAGS+=" -msse4.2"
        MACROS+=" -D__SSE4_2__"
    fi

    # Check for AVX
    if lscpu | grep -qi avx; then
        FLAGS+=" -mavx"
        MACROS+=" -D__AVX__"
    fi

    # Check for AVX2
    if lscpu | grep -qi avx2; then
        FLAGS+=" -mavx2"
        MACROS+=" -D__AVX2__"
    fi

    # Check for AVX512
    if lscpu | grep -qi avx512; then
        FLAGS+=" -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mavx512ifma -mavx512vbmi"
        MACROS+=" -D__AVX512F__"
    fi

    # Check for FMA
    if lscpu | grep -qi fma; then
        FLAGS+=" -mfma"
        MACROS+=" -D__FMA__"
    fi

    # Check for POPCNT
    if lscpu | grep -qi popcnt; then
        FLAGS+=" -mpopcnt"
        MACROS+=" -D__POPCNT__"
    fi

    # Check for endianness
    if [ "$(lscpu | grep -i 'byte order' | awk '{print $3}')" = "Little" ]; then
        MACROS+=" -D__LITTLE_ENDIAN__"
    elif [ "$(lscpu | grep -i 'byte order' | awk '{print $3}')" = "Big" ]; then
        MACROS+=" -D__BIG_ENDIAN__"
    fi

    # Return the results (two different methods shown)

    # Method 1: Set global variables
    CPU_FLAGS="$FLAGS"
    CPU_MACROS="$MACROS"

    # Method 2: Output to stdout (alternative approach)
    echo "$FLAGS $MACROS"
}