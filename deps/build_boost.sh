#!/usr/bin/env bash
set -e

build_dir="$(pwd)"

NAME="boost"
PKGURL="http://softlayer-dal.dl.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.bz2"
PKGBASE=$(basename $PKGURL)

mkdir -p /tmp/${NAME}
wget ${PKGURL} -O - | tar -vxj --strip-components=1 -C /tmp/${NAME}
cd /tmp/${NAME}

export PATH="/usr/local/bin:$PATH"
export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="$CFLAGS -fPIC"
export ICU_CORE_CPP_FLAGS="-DU_CHARSET_IS_UTF8=1"
export BOOST_LDFLAGS="${BOOST_LDFLAGS} -L${BUILD}/lib -licuuc -licui18n -licudata"
export BOOST_CXXFLAGS="${BOOST_CXXFLAGS} ${ICU_CORE_CPP_FLAGS}"
export ICU_DETAILS="-sHAVE_ICU=1 -sICU_PATH=${BUILD}"

./bootstrap.sh

./b2 \
--prefix=${BUILD} \
--ignore-site-config \
${ICU_DETAILS} \
system filesystem \
link=static,shared \
variant=release \
linkflags="${BOOST_LDFLAGS}" \
cxxflags="${BOOST_CXXFLAGS}" \
stage install

./b2 tools/bcp \
--ignore-site-config

STAGING_DIR=bcp_staging

mkdir -p ${STAGING_DIR}
rm -rf ${STAGING_DIR}/*
for var in system filesystem
do
  ./dist/bin/bcp "${var}" ${STAGING_DIR} 1>/dev/null
done
du -h -d 0 ${STAGING_DIR}/boost/
mkdir -p ${BUILD}/include
cp -r ${STAGING_DIR}/boost ${BUILD}/include/
