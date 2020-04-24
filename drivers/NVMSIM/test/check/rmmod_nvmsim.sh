#!/bin/bash

dmesg | grep $1 > $1.txt
sudo rmmod $1
#sudo ls  /dev/$1/*
lsmod | grep $1
dmesg | tail -10

#dd if=/dev/zero of=/dev/sdd bs=4k count=300000 oflag=direct