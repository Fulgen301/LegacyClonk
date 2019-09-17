#!/bin/bash

set -ex

readonly GCC=gcc-9
readonly GXX=g++-9

export readonly DEBIAN_FRONTEND=noninteractive

# Use sudo if it is installed
sudo=$(hash sudo 2> /dev/null && echo sudo || echo)

# Prepare package installation
$sudo sed -i 's/\bhttp:\/\/archive.ubuntu.com\//http:\/\/us-central1.gce.archive.ubuntu.com\//g' /etc/apt/sources.list
$sudo apt-get update
install="$sudo apt-get --no-install-recommends -y install"

# Install gcc and g++. Use PPA if normal installation fails.
if ! $install $GCC $GXX ; then
	$install software-properties-common
	$sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	$install $GCC $GXX
fi

# Install remaining packages
$install cmake freeglut3-dev git libc-dev libfreetype6-dev libgl1-mesa-dev libglew-dev libglu1-mesa-dev libjpeg-dev libpng-dev libsdl-mixer1.2-dev libsdl1.2-dev libssl-dev libxpm-dev make zlib1g-dev

# Run CMake and build project
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=$GCC -DCMAKE_CXX_COMPILER=$GXX -DCMAKE_INSTALL_PREFIX="/usr" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
tar -cvzf LegacyClonk.tar.gz clonk c4group
