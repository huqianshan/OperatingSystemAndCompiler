#!/bin/bash
sudo dmesg -c
make clean;
make nvmsim;
sudo insmod nvmsim.ko
lsmod | grep nvm
cat /proc/devices | grep nvm
dmesg | grep NVM > ./test/logs/nvmsim_begin_`date -d last-day +%Y-%m-%d-%H-%M-%S`.txt