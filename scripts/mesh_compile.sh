#!/bin/sh

git pull &&

# Oppdater cpp-elasticsearch
cd ./MeSHImport/cpp-elasticsearch/ &&
git pull &&

# Kompiler MeSHImport
cd .. &&
make clean &&
make -j2 &&

# Kompiler MeSHWeb
cd ../MeSHWeb/ &&
make clean &&
make -j2 &&
sudo make install &&

cd ..
