#!/bin/bash
set -e
set -x

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y g++ nlohmann-json3-dev

mkdir -p build-release

g++ -std=c++20 -O2 -Wall -Wextra \
    -I. \
    -Isrc \
    -Isrc/data_structures \
    -Isrc/microarchitecture \
    -Isrc/program_loader \
    $(find src -name '*.cpp' | sort) \
    -o build-release/hw1