#!/usr/bin/env bash
set -e

build_dir="$(pwd)"

NAME="boost"
PKGURL="http://softlayer-dal.dl.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.bz2"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -xj --strip-components=1 -C /tmp/${NAME}
cd /tmp/${NAME}

./bootstrap.sh

./b2 -d0 \
--prefix=${BUILD} \
--with-system --with-filesystem \
-sHAVE_ICU=0 \
link=static \
variant=release \
cxxflags="-fPIC" \
install
