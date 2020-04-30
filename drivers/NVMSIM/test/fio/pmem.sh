#!/bin/bash

# ./pmem iodepth size numjobs


sudo fio -filename=/dev/pmem0 -direct=1 -iodepth $1 \
 -rw=read -ioengine=libaio -bs=$2 -size=$3G -numjobs=$4 \
 -runtime=1000 -group_reporting -name=mytest \
 > ./output/pmem/pmem-readSeq-$1-$2-$3G-test;


sudo fio -filename=/dev/pmem0 -direct=1 -iodepth $1  \
-rw=write -ioengine=libaio -bs=$2k -size=$3G -numjobs=$4 \
-runtime=1000 -group_reporting -name=mytest  -allow_mounted_write=1\
> ./output/pmem/pmem-writeSeq-$1-$2-$3G-test;