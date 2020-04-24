obj-m:=test.o
#test-objs

CC=gcc
EXTRA_CFLAGS= -g -O0
LIBS=-lm

KVERS=$(shell uname -r)


build: test

test:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	rm -rf *.o *.ko *.mod.c Module.* modules.* \
	       *~ .*~ .\#*~ \#*~ .*.cmd .tmp*


