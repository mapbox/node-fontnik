#!/bin/bash

add-apt-repository -y ppa:boost-latest/ppa
add-apt-repository --yes ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install boost1.55 gcc-4.8 g++-4.8

export platform=$(uname -s | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")
export CXX=g++-4.8
export CC=gcc-4.8;

mkdir -p $BUILD
./deps/build_icu.sh 1>> build.log
./deps/build_protobuf.sh 1>> build.log
./deps/build_freetype.sh 1>> build.log
./deps/build_harfbuzz.sh 1>> build.log

export PATH="$BUILD/bin:$PATH"
echo "PKG_CONFIG_PATH is set to $PKG_CONFIG_PATH"
ls /tmp/fontserver-build/lib/pkgconfig

