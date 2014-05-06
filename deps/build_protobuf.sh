#!/bin/bash

set -e

build_dir="$(pwd)"

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"

wget https://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz -O /tmp/protobuf-2.5.0.tar.gz
tar xf /tmp/protobuf-2.5.0.tar.gz -C /tmp
cd /tmp/protobuf-2.5.0
./configure --enable-static --disable-shared --disable-dependency-tracking --prefix=${BUILD}
make
make install
cd $build_dir
