#!/usr/bin/env bash

set -e
set -o pipefail

source ~/.nvm/nvm.sh
nvm use ${NODE_VERSION}

npm test

if [[ ${COVERAGE} == true ]]; then
    cpp-coveralls --exclude node_modules --exclude tests --build-root build --gcov-options '\-lp' --exclude doc --exclude build/Debug/obj/gen
fi
