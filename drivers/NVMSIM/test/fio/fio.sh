#!/bin/bash

# ./fio iodepth size numjobs


#sudo rm -f /mnt/nvm0/*
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
 -rw=read -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
 -runtime=1000 -group_reporting -name=my_nvm_read_test \
 > ./output/readSeq-$1-$2G-test;

#sudo rm -f /mnt/nvm0/*;
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
-rw=write -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
-runtime=1000 -group_reporting -name=my_nvm_write_test \
> ./output/writeSeq-$1-$2G-test;