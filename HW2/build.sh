#!/bin/bash
set -e
set -x

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y g++ nlohmann-json3-dev

mkdir -p build-release

mapfile -d '' SOURCES < <(
    find src -name '*.cpp' \
        -print0 | sort -z
)

g++ -std=c++20 -O2 -Wall -Wextra \
    -I. \
    -Isrc \
    -Isrc/common \
    -Isrc/frontend \
    -Isrc/middleend \
    -Isrc/backend \
    -Isrc/backend/output \
    -Isrc/backend/loop \
    -Isrc/backend/looppip \
    "${SOURCES[@]}" \
    -o build-release/hw2