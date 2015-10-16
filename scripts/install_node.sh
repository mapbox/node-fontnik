#!/usr/bin/env bash

set -e
set -o pipefail

rm -rf ~/.nvm
curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.28.0/install.sh | bash
source ~/.nvm/nvm.sh

nvm install ${NODE_EXE} ${NODE_VERSION}

${NODE_EXE} --version
npm --version
