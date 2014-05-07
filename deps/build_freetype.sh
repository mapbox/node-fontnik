#!/usr/bin/env bash
set -e

build_dir="$(pwd)"

NAME="freetype"
PKGURL="http://download.savannah.gnu.org/releases/freetype/freetype-2.5.3.tar.bz2"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -vxj --strip-components=1 -C /tmp/${NAME}
cd /tmp/${NAME}

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="$CFLAGS -fPIC"

# NOTE: --with-zlib=yes means external, non-bundled zip will be used
./configure \
--prefix=${BUILD} \
--enable-static \
--disable-shared \
--with-zlib=yes \
--with-bzip2=no \
--with-harfbuzz=no \
--with-png=no \
--with-quickdraw-toolbox=no \
--with-quickdraw-carbon=no \
--with-ats=no \
--with-fsref=no \
--with-fsspec=no \

make
make install

# clear out shared libs
rm -f ${BUILD}/lib/{*.so,*.dylib}

cd $build_dir

