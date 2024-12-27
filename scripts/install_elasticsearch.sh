#!/bin/sh
sudo apt-get -y update &&
wget -qO - https://artifacts.elastic.co/GPG-KEY-elasticsearch | sudo apt-key add - &&
sudo apt-get -y install apt-transport-https &&
echo "deb https://artifacts.elastic.co/packages/7.x/apt stable main" | sudo tee /etc/apt/sources.list.d/elastic-7.x.list
sudo apt-get -y update && sudo apt-get -y install elasticsearch &&
sudo /bin/systemctl daemon-reload &&
sudo /bin/systemctl enable elasticsearch.service
