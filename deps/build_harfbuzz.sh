#!/bin/bash

set -e

build_dir="$(pwd)"

NAME="harfbuzz"
PKGURL="http://www.freedesktop.org/software/harfbuzz/release/harfbuzz-0.9.24.tar.bz2"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -vxj --strip-components=1 -C /tmp/${NAME}
cd /tmp/${NAME}

export PATH="${BUILD}/bin:/usr/local/bin$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC -I${BUILD}/include"
export CFLAGS="$CFLAGS -fPIC -I${BUILD}/include"
export LDFLAGS="$LDFLAGS -L${BUILD}/lib"

./configure \
--prefix=${BUILD} \
--enable-static \
--disable-shared \
--disable-dependency-tracking \
--with-icu \
--with-cairo=no \
--with-glib=no \
--with-gobject=no \
--with-graphite2=no \
--with-freetype \
--with-uniscribe=no \
--with-coretext=no

make
make install

# clear out shared libs
rm -f ${BUILD}/lib/{*.so,*.dylib}

cd $build_dir
