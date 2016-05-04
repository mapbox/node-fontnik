#!/usr/bin/env bash

# This script is sourced, so do not set -e or -o pipefail here. Doing so would
# bleed into Travis' wrapper script, which messes with their workflow, e.g.
# preventing after_failure scripts to be triggered.

case `uname -s` in
    'Darwin') JOBS=$((`sysctl -n hw.ncpu` + 2)) ;;
    'Linux')  JOBS=$((`nproc` + 2)) ;;
    *)        JOBS=2 ;;
esac

export JOBS

git submodule update --init .mason

export PATH="`pwd`/.mason:${PATH}"
export MASON_DIR="`pwd`/.mason"

export BOOST_VERSION=1.58.0
export HARFBUZZ_VERSION=1.2.6
export FREETYPE_VERSION=2.6
export PROTOBUF_VERSION=2.6.1
export PROTOZERO_VERSION=1.3.0

mason install boost ${BOOST_VERSION}
mason install harfbuzz ${HARFBUZZ_VERSION}
mason install freetype ${FREETYPE_VERSION}
mason install protobuf ${PROTOBUF_VERSION}
mason install protozero ${PROTOZERO_VERSION}

# Add protoc to $PATH
export PATH="`mason prefix protobuf ${PROTOBUF_VERSION}`/bin:${PATH}"
