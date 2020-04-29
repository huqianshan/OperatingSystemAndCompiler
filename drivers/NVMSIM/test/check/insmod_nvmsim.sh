#!/bin/bash
sudo dmesg -c
make clean;
make;
sudo insmod $1.ko;
lsmod | grep $1;
#cat /proc/devices | grep $1;