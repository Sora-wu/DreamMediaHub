#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_config="${1:-Debug}"

cmake -S "$script_dir/third/DreamMediaHubNative" -B "$script_dir/build/native/linux" \
    -DCMAKE_INSTALL_PREFIX="$script_dir/third/build/linux" -DCMAKE_BUILD_TYPE="$build_config"
cmake --build "$script_dir/build/native/linux" --parallel
cmake --install "$script_dir/build/native/linux"
