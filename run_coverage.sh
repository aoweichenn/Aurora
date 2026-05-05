#!/bin/bash
# Coverage script for Aurora MiniC frontend
# Usage: ./run_coverage.sh

set -e

echo "=== Configuring with coverage ==="
cmake -B build-coverage -S . -DCMAKE_BUILD_TYPE=Debug -DAURORA_BUILD_TESTS=ON -DAURORA_BUILD_BENCHMARKS=OFF -DAURORA_ENABLE_COVERAGE=ON

echo "=== Building and running coverage gate ==="
cmake --build build-coverage -j8 --target minic_coverage

echo "=== Coverage Summary ==="
cat build-coverage/coverage/minic/coverage-summary.txt

echo ""
echo "Coverage report: build-coverage/coverage/minic/coverage-summary.json"
