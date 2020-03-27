#!/bin/bash

dmesg | grep NVMSIM > dmesg.txt
sudo rmmod nvmsim
sudo ls  /dev/nvm*
lsmod | grep nvm
dmesg | tail -1000 > ./test/logs/nvmsim_end_`date -d last-day +%Y-%m-%d-%H-%M-%S`.txt

#dd if=/dev/zero of=/dev/sdd bs=4k count=300000 oflag=direct