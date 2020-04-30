obj-m:=check.o

check-objs += test.o infotable.o bitmap.o maptable.o syncer.o
CC = gcc
FLAGS = -O -Wall 
LIBS = -lm

KVERS=$(shell uname -r)


build: check

check:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	rm -rf *.o *.ko *.mod.c Module.* modules.* \
	       *~ .*~ .\#*~ \#*~ .*.cmd .tmp*

	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean


