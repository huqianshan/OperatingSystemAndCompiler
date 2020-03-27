#!/bin/bash

for i in `seq 1 100`
do
    sudo cat ramdevice.o > /dev/nvm0
done