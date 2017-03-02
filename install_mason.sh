#!/usr/bin/env bash

set -eu
set -o pipefail

if [ ! -d ./mason ]; then
    mkdir ./mason
    curl -sSfL https://github.com/mapbox/mason/archive/v0.7.0.tar.gz | tar --gunzip --extract --strip-components=1 --exclude="*md" --exclude="test*" --directory=./mason
    ./mason/mason install boost 1.63.0
    ./mason/mason link boost 1.63.0
    ./mason/mason install freetype 2.7.1
    ./mason/mason link freetype 2.7.1
    ./mason/mason install protobuf 3.2.0
    ./mason/mason link protobuf 3.2.0
fi