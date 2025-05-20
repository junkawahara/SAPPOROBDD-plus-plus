CC    = g++
DIR   = ../..
INCL  = $(DIR)/include
OPT   = -O3 -Wall -Wextra -Wshadow -I$(INCL)
OPT64 = $(OPT) -DB_64

all: bddc_64.o

32: bddc_32.o

64: bddc_64.o

bddc_32.o: bddc.cc $(INCL)/bddc.h
	$(CC) $(OPT) -c bddc.cc -o bddc_32.o

bddc_64.o: bddc.cc $(INCL)/bddc.h
	$(CC) $(OPT64) -c bddc.cc -o bddc_64.o

clean:
	rm -f *.o *.a *~

