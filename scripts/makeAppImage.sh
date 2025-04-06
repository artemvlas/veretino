#! /bin/bash

# based on the script from AppImage docs:
# https://docs.appimage.org/packaging-guide/from-source/native-binaries.html#packaging-from-source

set -x
set -e

# building in temporary directory to keep system clean
# use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" AppImageLauncher-build-XXXXXX)

# make sure to clean up build dir, even if errors occur
cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

# store repo root as variable; to use in current dir: REPO_ROOT="$PWD" OLD_CWD="$PWD"
REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
OLD_CWD=$(readlink -f .)

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with cmake
cmake "$REPO_ROOT" -DCMAKE_BUILD_TYPE=Release

# build project and install files into AppDir
make -j$(nproc)
make install DESTDIR=AppDir

# now, build AppImage using linuxdeploy and linuxdeploy-plugin-qt
# download linuxdeploy and its Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

# make them executable
chmod +x linuxdeploy*.AppImage

# add /usr/plugins/iconengines/libqsvgicon.so
export EXTRA_QT_PLUGINS="svg;"

# initialize AppDir, bundle shared libraries, add desktop file and icon, use Qt plugin to bundle additional resources, and build AppImage, all in one command
./linuxdeploy-x86_64.AppImage --appdir AppDir -i "$REPO_ROOT"/res/veretino.png -d "$REPO_ROOT"/res/veretino.desktop --plugin qt --output appimage

# move built AppImage back into original CWD
mv Veretino*.AppImage "$OLD_CWD"

echo "All done..."
