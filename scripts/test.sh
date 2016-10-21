#!/usr/bin/env bash

set -e
set -o pipefail

source ~/.nvm/nvm.sh
nvm use ${NODE_VERSION}

npm test

if [[ ${COVERAGE} == true ]]; then
    ${PYTHONUSERBASE}/bin/cpp-coveralls --exclude node_modules --exclude mason_packages --exclude tests --build-root build --gcov-options '\-lp' --exclude doc --exclude build/Debug/obj/gen
fi
