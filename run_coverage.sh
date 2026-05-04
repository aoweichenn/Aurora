#!/bin/bash
# Coverage script for Aurora compiler backend
# Usage: ./run_coverage.sh

set -e

echo "=== Configuring with coverage ==="
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DAURORA_BUILD_TESTS=ON -DAURORA_ENABLE_COVERAGE=ON

echo "=== Building ==="
cmake --build build -j8

echo "=== Running tests ==="
ctest --test-dir build

echo "=== Generating coverage report ==="
mkdir -p build/coverage
gcovr -r . --object-directory build \
    --gcov-executable "llvm-cov gcov" \
    --exclude 'tools/*' \
    --exclude 'tests/*' \
    --exclude 'build/_deps/*' \
    --html --html-details \
    -o build/coverage/index.html

echo "=== Coverage Summary ==="
gcovr -r . --object-directory build \
    --gcov-executable "llvm-cov gcov" \
    --exclude 'tools/*' \
    --exclude 'tests/*' \
    --exclude 'build/_deps/*' \
    --print-summary

echo ""
echo "Coverage report: build/coverage/index.html"
