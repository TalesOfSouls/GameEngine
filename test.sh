#!/bin/bash

start_time=$(date +%s.%N)

# No terminal (e.g. CI) -> nothing to clear
if [ -t 1 ]; then
    clear
fi

# Everything is relative to the repository root (= directory of this script, independent of its name)
# The tests also run from there (test data paths like "tests/object/model")
cd "$(dirname "$0")" || exit 1

EXE_NAME="tests"
DESTINATION_DIR="../build/tests"

mkdir -p "../build"
mkdir -p "$DESTINATION_DIR"

# Clean up previous build files
rm -f "$DESTINATION_DIR"/*.pdb 2>/dev/null
rm -f "$DESTINATION_DIR"/*.idb 2>/dev/null

# Parse command-line arguments
BUILD_TYPE="DEBUG"
BUILD_FLAGS="-g -O0 -Wall -Werror -Wextra -DDEBUG -DDEBUG -ggdb -Werror"
DEBUG_DATA="-g"

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

# Detect CPU features, TESTS_CPU_FEATURES overrides the detection (e.g. the CI uses fixed flags)
CPU_FEATURES=${TESTS_CPU_FEATURES:-$(check_cpu_features)}

# Additional compiler arguments, e.g. the CI: TESTS_EXTRA_ARGS="-DTEST_QOI=0"
g++ $BUILD_FLAGS -std=c++23 -m64 \
    -DUNICODE -D_UNICODE -D__linux__ \
    -Wno-unused-result \
    -fsanitize=address \
    -fsanitize=undefined \
    ${CPU_FEATURES} \
    ${TESTS_EXTRA_ARGS} \
    -o "$DESTINATION_DIR/MainTest" \
    MainTest.cpp \
    $DEBUG_DATA

# Check if the compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed for MainTest.cpp"
    exit 1
fi

# Run the compiled executable, its exit code is the result of this script (1 = failed tests)
"$DESTINATION_DIR/MainTest"
test_result=$?

# Calculate runtime
end_time=$(date +%s.%N)
elapsed_time=$(echo "$end_time - $start_time" | bc)

hours=$(printf "%.0f" $(echo "$elapsed_time / 3600" | bc))
remaining=$(echo "$elapsed_time % 3600" | bc)
minutes=$(printf "%.0f" $(echo "$remaining / 60" | bc))
seconds=$(echo "$remaining % 60" | bc)

# Format the output
printf "Test (incl. build) time %02d:%02d:%05.2f (%.2fs total)\n" \
    $hours $minutes $seconds $elapsed_time

exit $test_result
