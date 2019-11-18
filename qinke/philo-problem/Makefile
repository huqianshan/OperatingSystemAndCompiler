# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: philo pc

philo.o: philo.c
	$(CC) $(CFLAGS) -c philo.c

pc.o: pc.c
	$(CC) $(CFLAGS) -c pc.c

philo: philo.o 
	$(CC) $(CFLAGS) philo.o  -o philo $(LDFLAGS)

pc:  pc.o
	$(CC) $(CFLAGS) pc.o  -o pc $(LDFLAGS)
clean:
	rm -f *~ *.o pc philo  *.tar *.zip *.gzip *.bzip *.gz