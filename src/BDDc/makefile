CC    = g++
DIR   = ../..
INCL  = $(DIR)/include
OPT   = -O3 -Wall -Wextra -Wshadow -I$(INCL) -I.
OPT32 = $(OPT) -DB_32

SRCS = bddc_init.cc bddc_node.cc bddc_gc.cc bddc_apply.cc \
       bddc_apply_binary.cc bddc_apply_unary.cc bddc_apply_count.cc \
       bddc_apply_special.cc bddc_operations.cc bddc_io.cc bddc_util.cc

OBJS_64 = $(SRCS:.cc=_64.o)
OBJS_32 = $(SRCS:.cc=_32.o)

all: bddc_64.o

32: bddc_32.o

64: bddc_64.o

bddc_32.o: $(OBJS_32)
	ld -r $(OBJS_32) -o bddc_32.o

bddc_64.o: $(OBJS_64)
	ld -r $(OBJS_64) -o bddc_64.o

HDRS = bddc_internal.h bddc_apply_common.h $(INCL)/bddc.h

%_64.o: %.cc $(HDRS)
	$(CC) $(OPT) -c $< -o $@

%_32.o: %.cc $(HDRS)
	$(CC) $(OPT32) -c $< -o $@

clean:
	rm -f *.o *.a *~

