#!/bin/sh
ln -sf /usr/local/share/Wt/resources ./MeSHWeb/ &&

# Last ned cpp-elasticsearch
cd ./MeSHImport/ &&
git clone https://github.com/frodegill/cpp-elasticsearch.git &&

cd ../MeSHWeb/ &&
ln -sf ../MeSHImport/cpp-elasticsearch &&
cd .. &&

sudo mkdir -p /opt/Helsebib/MeSHWeb/ &&
sudo ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/
