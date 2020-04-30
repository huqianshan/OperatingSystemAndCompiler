#!/bin/bash


sudo dmesg -c
make clean;
make;
sudo insmod $1.ko
lsmod | grep $1
cat /proc/devices | grep $1
#dmesg | grep NVM > ./test/logs/nvmsim_begin_`date -d last-day +%Y-%m-%d-%H-%M-%S`.txt