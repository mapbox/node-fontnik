#!/bin/bash

set -e
set -o pipefail

# Inspect binary.
if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    ldd ./lib/fontnik.node
else
    otool -L ./lib/fontnik.node
fi

COMMIT_MESSAGE=$(git show -s --format=%B $TRAVIS_COMMIT | tr -d '\n')
PACKAGE_JSON_VERSION=$(node -e "console.log(require('./package.json').version)")

if [[ ${TRAVIS_TAG} == v${PACKAGE_JSON_VERSION} ]] || test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE" && [[ ${COVERAGE} == false ]]; then

    npm install aws-sdk

    ./node_modules/.bin/node-pre-gyp package testpackage
    ./node_modules/.bin/node-pre-gyp publish info

    rm -rf build
    rm -rf lib
    npm install --fallback-to-build=false
    npm test

    ./node_modules/.bin/node-pre-gyp info
fi
