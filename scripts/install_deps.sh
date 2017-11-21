#!/bin/bash

set -eu
set -o pipefail

function install() {
  mason install $1 $2
  mason link $1 $2
}

# setup mason
./scripts/setup.sh --config local.env
source local.env

install boost 1.63.0
#link boost 1.63.0 # is linking needed?
install freetype 2.7.1
#link freetype 2.7.1
install protobuf 3.2.0
#link protobuf 3.2.0
install sdf-glyph-foundry 0.1.1
#link sdf-glyph-foundry 0.1.1