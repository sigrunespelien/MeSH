#!/bin/sh
sudo apt-get -y update &&
sudo apt-get -y install certbot &&
sudo certbot certonly --domain mesh.uia.no --standalone --agree-tos &&
sudo openssl dhparam -out /etc/ssl/dh4096.pem 4096
