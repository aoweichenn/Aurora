#!/bin/bash
BUILD_DIR="/home/aoweichen/demos/STU/CPP/COMPLIER/Aurora/build"
SRC_DIR="/home/aoweichen/demos/STU/CPP/COMPLIER/Aurora"

for f in $(find "${SRC_DIR}/lib" "${SRC_DIR}/tools" "${SRC_DIR}/tests" -name '*.cpp' -not -path '*/build/*'); do
    echo "=== $f ==="
    clang-tidy -p "${BUILD_DIR}" --quiet "$f" 2>/dev/null
done
