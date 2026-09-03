CC	= g++

DIR	= ../..
INCL    = $(DIR)/include
LIB	= $(DIR)/lib/BDD32.a
LIB64	= $(DIR)/lib/BDD64.a
XLIB64  =
#XLIB64  = /usr/X11R6/lib64/libX11.a

PRG     = gpm32
PRG64   = gpm
PRG_ST  = gpm32-static
PRG64_ST = gpm-static

OPT 	= -O3 -Wall -Wextra -Wshadow -DYY_ALWAYS_INTERACTIVE -I$(INCL)
# -DB_32 must match lib/BDD32.a: without it the application would be built
# with a 64-bit bddp and linked against a library using a 32-bit one.
OPT32   = $(OPT) -DB_32
OPT64   = $(OPT) -DB_64
OPT_ST  = $(OPT32) -DB_STATIC -static
OPT64_ST = $(OPT64) -DB_STATIC -static

OBJ	= gpm.o
OBJ64   = gpm_64.o
OBJ_ST  = gpm_st.o
OBJ64_ST = gpm_64_st.o

# gpm.cc includes BDDCT.h as well, and a change to a header changes what
# the object may call: without it here a stale object hid a header the
# source no longer compiles against.
HDRS	= gpm.h $(INCL)/ZDD.h $(INCL)/BDD.h $(INCL)/BDDCT.h

all:	$(PRG64)    

32:	$(PRG)    

static:	$(PRG64_ST)    

32static: $(PRG_ST)    

$(PRG): $(OBJ) $(LIB)
	$(CC) $(OPT32) $(OBJ) $(LIB) -o $(PRG)

$(PRG64): $(OBJ64) $(LIB64)
	$(CC) $(OPT64) $(OBJ64) $(LIB64) $(XLIB64) -o $(PRG64)

$(PRG_ST): $(OBJ_ST) $(LIB)
	$(CC) $(OPT_ST) $(OBJ_ST) $(LIB) -o $(PRG_ST)

$(PRG64_ST): $(OBJ64_ST) $(LIB64)
	$(CC) $(OPT64_ST) $(OBJ64_ST) $(LIB64) -o $(PRG64_ST)

gpm.o: gpm.cc $(HDRS)
	$(CC) $(OPT32) -c gpm.cc

gpm_64.o: gpm.cc $(HDRS)
	$(CC) $(OPT64) -c gpm.cc -o gpm_64.o

gpm_st.o: gpm.cc $(HDRS)
	$(CC) $(OPT_ST) -c gpm.cc -o gpm_st.o

gpm_64_st.o: gpm.cc $(HDRS)
	$(CC) $(OPT64_ST) -c gpm.cc -o gpm_64_st.o

clean:
	rm -f *.o
	rm -f lex.yy.*
	rm -f y.tab.*

