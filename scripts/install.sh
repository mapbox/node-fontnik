#!/usr/bin/env bash

set -e
set -o pipefail

source ./scripts/install_mason.sh

source ~/.nvm/nvm.sh
nvm use ${NODE_VERSION}

if [[ ${COVERAGE} == true ]]; then
  npm install --build-from-source --debug --clang;
else
  npm install --build-from-source --clang;
fi;
