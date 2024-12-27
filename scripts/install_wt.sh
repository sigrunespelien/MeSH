#!/bin/sh
sudo apt-get -y update &&
sudo apt-get -y install g++ libssl-dev libxml2-dev libboost-all-dev cmake make &&
mkdir -p ~/projects/ &&
cd ~/projects/ &&
wget https://github.com/emweb/wt/archive/4.8.2.tar.gz &&
gunzip 4.8.2.tar.gz &&
tar xf 4.8.2.tar &&
rm 4.8.2.tar &&
rm -f wt &&
ln -s wt-4.8.2 wt &&
cd wt &&
mkdir build &&
cd build &&
cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF &&
make -j2 &&
sudo make install &&
sudo ldconfig
