#!/usr/bin/env bash

set -e
set -o pipefail

node --version
npm --version
which node

if [[ ${COVERAGE} == true ]]; then
  npm install --build-from-source --debug --clang;
else
  npm install --build-from-source --clang;
fi;
