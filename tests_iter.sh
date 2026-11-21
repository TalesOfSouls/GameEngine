#!/bin/bash

# No terminal (e.g. CI) -> nothing to clear
if [ -t 1 ]; then
    clear
fi

# Everything is relative to the repository root (= directory of this script, independent of its name)
# The tests also run from there (test data paths like "tests/object/model")
cd "$(dirname "$0")" || exit 1

EXE_NAME="tests"
DESTINATION_DIR="../build/tests"

# Create build directories if they don't exist
mkdir -p "../build"
mkdir -p "$DESTINATION_DIR"

# Clean up previous build files
rm -f "$DESTINATION_DIR"/*.pdb 2>/dev/null
rm -f "$DESTINATION_DIR"/*.idb 2>/dev/null

# Default build configuration
BUILD_TYPE="DEBUG"
BUILD_FLAGS="-g -O0 -Wall -Werror -Wextra -DDEBUG -ggdb -Werror"
DEBUG_DATA="-g"

# Parse command-line arguments
if [ "$1" = "-r" ]; then
    BUILD_TYPE="RELEASE"
    BUILD_FLAGS="-O3 -DNDEBUG"
    DEBUG_DATA=""
elif [ "$1" = "-d" ]; then
    BUILD_TYPE="DEBUG"
    BUILD_FLAGS="-g -O0 -Wall -Werror -Wextra -DDEBUG -ggdb -Werror"
    DEBUG_DATA="-g"
fi

# -m64 only exists for x86 (gcc on aarch64 rejects it)
ARCH_FLAGS=""
if [ "$(uname -m)" = "x86_64" ]; then
    ARCH_FLAGS="-m64"
fi

source ./check_cpu_features.sh

# Detect CPU features
CPU_FEATURES=$(check_cpu_features)

# Find all *Test.cpp files in the ./ directory and subdirectories
# Process substitution instead of a pipe: a piped loop runs in a subshell, its "exit 1" wouldn't end the script
while read -r test_file; do
    # Get the base name of the file without extension
    basename=$(basename "$test_file" .cpp)

    echo "Compiling $test_file..."

    # Compile each test file
    g++ $BUILD_FLAGS -std=c++23 $ARCH_FLAGS \
        -DUNICODE -D_UNICODE -D__linux__ \
        -Wno-unused-result \
        -fsanitize=address \
        -fsanitize=undefined \
        ${CPU_FEATURES} \
        -o "$DESTINATION_DIR/$basename" \
        "$test_file" \
        $DEBUG_DATA

    # Check if compilation was successful
    if [ $? -ne 0 ]; then
        echo "Compilation failed for $test_file"
        exit 1
    fi

    # Run the compiled test
    echo "Running $basename..."
    "$DESTINATION_DIR/$basename"

    # Check if test ran successfully
    if [ $? -ne 0 ]; then
        echo "Test $basename failed"
        exit 1
    fi
done < <(find ./ -name "*Test.cpp")
