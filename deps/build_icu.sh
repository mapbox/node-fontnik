#!/usr/bin/env bash
set -e -u
set -o pipefail

build_dir="$(pwd)"

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"

wget http://download.icu-project.org/files/icu4c/53.1/icu4c-53_1-src.tgz -O /tmp/icu4c-53_1-src.tgz
tar xf /tmp/icu4c-53_1-src.tgz -C /tmp
cd /tmp/icu/source
./configure --prefix=${BUILD} \
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
