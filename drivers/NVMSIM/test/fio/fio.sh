#!/bin/bash

# ./fio iodepth size numjobs file_name


sudo rm -f $4/*
fio -directory=$4/ -direct=1 -iodepth $1 -thread \
 -rw=read -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
 -runtime=1000 -group_reporting -name=mytest \
 > ./output/pmem0-readSeq-$1-$2G-test;

sudo rm -f $4/*;
fio -directory=$4/ -direct=1 -iodepth $1 -thread \
-rw=write -ioengine=libaio -bs=4k -size=$2G -numjobs=$3 \
-runtime=1000 -group_reporting -name=mytest \
> ./output/pmem0-writeSeq-$1-$2G-test;