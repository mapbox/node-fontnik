#!/bin/bash

set -e

build_dir="$(pwd)"

NAME="protobuf"
PKGURL="https://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -vxz --strip-components=1 -C /tmp/${NAME}
cd /tmp/{$NAME}

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"

./configure \
--prefix=${BUILD} \
--enable-static \
--disable-shared \
--disable-dependency-tracking

make
make install

# clear out shared libs
rm -f ${BUILD}/lib/{*.so,*.dylib}

cd $build_dir
