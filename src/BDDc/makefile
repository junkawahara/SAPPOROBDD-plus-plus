CC    = g++
DIR   = ../..
INCL  = $(DIR)/include
OPT   = -O3 -Wall -Wextra -Wshadow -I$(INCL) -I.
OPT32 = $(OPT) -DB_32

SRCS = bddc_init.cc bddc_node.cc bddc_gc.cc bddc_apply.cc \
       bddc_operations.cc bddc_io.cc bddc_util.cc

OBJS_64 = $(SRCS:.cc=_64.o)
OBJS_32 = $(SRCS:.cc=_32.o)

all: bddc_64.o

32: bddc_32.o

64: bddc_64.o

bddc_32.o: $(OBJS_32)
	ld -r $(OBJS_32) -o bddc_32.o

bddc_64.o: $(OBJS_64)
	ld -r $(OBJS_64) -o bddc_64.o

%_64.o: %.cc bddc_internal.h $(INCL)/bddc.h
	$(CC) $(OPT) -c $< -o $@

%_32.o: %.cc bddc_internal.h $(INCL)/bddc.h
	$(CC) $(OPT32) -c $< -o $@

clean:
	rm -f *.o *.a *~

