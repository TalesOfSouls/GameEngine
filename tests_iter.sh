#!/bin/bash

clear

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

source ./check_cpu_features.sh

# Detect CPU features
CPU_FEATURES=$(check_cpu_features)

# Find all *Test.cpp files in the ./ directory and subdirectories
find ./ -name "*Test.cpp" | while read -r test_file; do
    # Get the base name of the file without extension
    basename=$(basename "$test_file" .cpp)

    echo "Compiling $test_file..."

    # Compile each test file
    g++ $BUILD_FLAGS -std=c++23 -m64 \
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
done
