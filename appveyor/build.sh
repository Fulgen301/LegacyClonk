#!/bin/bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends zlib1g-dev libgl1-mesa-dev libjpeg-dev libpng-dev libssl-dev libglu1-mesa-dev libglew-dev libsdl1.2-dev libsdl-mixer1.2-dev libfreetype6-dev freeglut3-dev libxpm-dev libgtk2.0-dev cmake make git gcc-10 -y
sudo update-alternatives --set gcc /usr/bin/gcc-10
sudo update-alternatives --set g++ /usr/bin/g++-10
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="/usr" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_DEVELOPER_MODE=On ..
make -j$(nproc)
test -e clonk && tar -cvzf LegacyClonk.tar.gz clonk c4group
