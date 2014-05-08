#!/bin/bash

sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-4.8 g++-4.8

mkdir -p $BUILD
./deps/build_icu.sh 1>> build.log
./deps/build_boost.sh
cat build.log
# ./deps/build_protobuf.sh 1>> build.log
# ./deps/build_freetype.sh 1>> build.log
# ./deps/build_harfbuzz.sh 1>> build.log
