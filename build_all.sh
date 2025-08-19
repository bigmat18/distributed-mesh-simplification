#!/bin/bash

set -e

BUILD_TYPES=("Debug" "Release" "RelWithDebInfo")
ROOT_DIR="build"

mkdir -p "$ROOT_DIR"

for TYPE in "${BUILD_TYPES[@]}"; do
    DIR="$ROOT_DIR/$TYPE"
    echo "==> Configuring $TYPE in $DIR"
    cmake -B "$DIR" -DCMAKE_BUILD_TYPE=$TYPE .
done

cd build
ln -s Debug/compile_commands.json compile_commands.json
cd ..
