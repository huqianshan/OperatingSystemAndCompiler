#!/bin/bash

# ./pmem iodepth size numjobs


sudo fio -filename=/dev/pmem0 -direct=1 -iodepth $1 \
 -rw=read -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
 -runtime=1000 -group_reporting -name=mytest \
 > ./output/pmem-readSeq-$1-$2G-test;


sudo fio -filename=/dev/pmem0 -direct=1 -iodepth $1  \
-rw=write -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
-runtime=1000 -group_reporting -name=mytest  -allow_mounted_write=1\
> ./output/pmem-writeSeq-$1-$2G-test;