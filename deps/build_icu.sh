#!/usr/bin/env bash
set -e

build_dir="$(pwd)"

NAME="icu"
PKGURL="http://download.icu-project.org/files/icu4c/53.1/icu4c-53_1-src.tgz"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -vxz --strip-components=1 -C /tmp/${NAME}
cd /tmp/${NAME}

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"

./configure \
--prefix=${BUILD} \
--enable-draft \
--enable-static \
--with-data-packaging=archive \
--disable-shared \
--disable-tests \
--disable-extras \
--disable-layout \
--disable-icuio \
--disable-samples \
--disable-dyload

make
make install

# clear out shared libs
rm -f ${BUILD}/lib/{*.so,*.dylib}

cd $build_dir
