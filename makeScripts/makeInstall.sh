#! /bin/bash

# Install 'make' to system:
# sudo pacman -S make

REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
cd $REPO_ROOT

# create out-of-source build dir and run qmake to prepare the Makefile
mkdir build
cd build
qmake ..

# build the application on all CPU cores
make -j$(nproc)
sudo make install

echo "All done..."
