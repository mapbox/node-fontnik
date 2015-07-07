#!/usr/bin/env bash

set -e
set -o pipefail

git clone https://github.com/creationix/nvm.git ../.nvm
source ../.nvm/nvm.sh

nvm install ${NODE_EXE} ${NODE_VERSION}

${NODE_EXE} --version
npm --version
