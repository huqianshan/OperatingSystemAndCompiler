#!/bin/bash

for i in `seq 1 10`
do
    sudo cat ramdevice.o > /dev/nvm0
done