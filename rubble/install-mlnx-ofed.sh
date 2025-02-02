#!/bin/bash
# Usage: nohup sudo bash install-mlnx-ofed.sh &

set -x

wget https://content.mellanox.com/ofed/MLNX_OFED-5.6-2.0.9.0/MLNX_OFED_LINUX-5.6-2.0.9.0-ubuntu20.04-x86_64.tgz

tar zxvf MLNX_OFED_LINUX-5.6-2.0.9.0-ubuntu20.04-x86_64.tgz

yes | MLNX_OFED_LINUX-5.6-2.0.9.0-ubuntu20.04-x86_64/mlnxofedinstall --with-nvmf > log.txt 2>&1

/etc/init.d/openibd restart

# from https://forums.developer.nvidia.com/t/ofed-nvmet-and-nvmet-rdma-on-ubuntu-symbol-errors/206456
update-initramfs -u

reboot
