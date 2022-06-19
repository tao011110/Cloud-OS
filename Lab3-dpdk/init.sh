#!/bin/bash
mkdir -p /dev/hugepages 
mountpoint -q /dev/hugepages || mount -t hugetlbfs nodev /dev/hugepages
echo 64 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

sudo ifconfig ens33 down
sudo modprobe uio 
cd ../dpdk/kernel/linux/igb_uio 
make 
sudo insmod igb_uio.ko 
cd ../../.. 
sudo usertools/dpdk-devbind.py --bind=igb_uio ens33 
sudo usertools/dpdk-devbind.py -s 