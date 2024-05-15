#! /bin/bash

# Install 'make' to system:
# sudo pacman -S make

set -x
set -e

TEMP_BASE=/tmp

REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" veretino-build-XXXXXX)

cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

pushd "$BUILD_DIR"

# make project
qmake6 "$REPO_ROOT"

# build the application on all CPU cores
make -j$(nproc)

# install to system
sudo make install

echo "All done..."
