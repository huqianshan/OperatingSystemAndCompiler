#!/bin/bash

#dmesg | grep NVMSIM > dmesg.txt
sudo rmmod $1;
lsmod | grep $1;

ls /dev/ | grep nvm*
#dmesg | tail -1000 > ./test/logs/nvmsim_end_`date -d last-day +%Y-%m-%d-%H-%M-%S`.txt

#dd if=/dev/zero of=/dev/sdd bs=4k count=300000 oflag=direct