#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_config="${1:-Debug}"
platform=linux
if [[ "$(uname -s)" == Darwin ]]; then
    platform=macos
fi

cmake -S "$script_dir/third/src/DreamMediaHubNative" -B "$script_dir/build/native/$platform" \
    -DCMAKE_INSTALL_PREFIX="$script_dir/third/build/$platform" -DCMAKE_BUILD_TYPE="$build_config"
cmake --build "$script_dir/build/native/$platform" --parallel
cmake --install "$script_dir/build/native/$platform"
