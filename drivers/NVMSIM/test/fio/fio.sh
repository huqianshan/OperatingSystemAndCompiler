#!/bin/bash

# ./fio iodepth size numjobs


#sudo rm -f /mnt/nvm0/*
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
 -rw=read -ioengine=libaio -bs=$2k -size=$3G -numjobs=$4 \
 -runtime=1000 -group_reporting -name=my_nvm_read_test \
 > ./output/nvm/nvm-readSeq-$1-$2k-$3G-test;

#sudo rm -f /mnt/nvm0/*;
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
-rw=write -ioengine=libaio -bs=$2k -size=$3G -numjobs=$4 \
-runtime=1000 -group_reporting -name=my_nvm_write_test \
> ./output/nvm/nvm-writeSeq-$1-$2k-$3G-test;