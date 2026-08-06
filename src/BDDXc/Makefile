CC    = g++
DIR   = ../..
INCL  = $(DIR)/include
OPT   = -O3 -Wall -Wextra -Wshadow -I$(INCL) -I. -I/opt/X11/include
OPT32 = $(OPT) -DB_32

WINDOWDEF = -DLINE

SRCS = graph.cc interface.cc location.cc parts11.cc \
       reorder.cc train.cc window11.cc

OBJS_64 = $(SRCS:.cc=_64.o)
OBJS_32 = $(SRCS:.cc=_32.o)

HDRS = bddxc_internal.h defs.h train.h interface.h reorder.h \
       $(INCL)/bddc.h $(INCL)/BDDException.h

all:	$(OBJS_64)

32:	$(OBJS_32)

64:	$(OBJS_64)

clean:
	rm -f *.o *.a *~

%_64.o: %.cc $(HDRS)
	$(CC) $(OPT) $(WINDOWDEF) -c $< -o $@

%_32.o: %.cc $(HDRS)
	$(CC) $(OPT32) $(WINDOWDEF) -c $< -o $@

X11HDRS = x11globals.h yubi.ic yubi_mask.ic wait.ic wait_mask.ic

parts11_64.o parts11_32.o: x11globals.h
window11_64.o window11_32.o: $(X11HDRS)
