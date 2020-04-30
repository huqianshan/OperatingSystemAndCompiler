#!/bin/bash

# ./fio iodepth size numjobs


#sudo rm -f /mnt/nvm0/*
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
 -rw=randread -ioengine=libaio -bs=$2k -size=$3G -numjobs=$4 \
 -runtime=1000 -group_reporting -name=my_nvm_read_test \
 > ./output/nvmrand/nvm-readRand-$1-$2k-$3G-test;

#sudo rm -f /mnt/nvm0/*;
sudo fio -filename=/dev/nvm0 -direct=1 -iodepth $1 -thread \
-rw=randwrite -ioengine=libaio -bs=$2k -size=$3G -numjobs=$4 \
-runtime=1000 -group_reporting -name=my_nvm_write_test \
> ./output/nvmrand/nvm-writeRand-$1-$2k-$3G-test;