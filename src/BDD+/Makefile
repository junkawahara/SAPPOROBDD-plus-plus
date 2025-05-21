CC	= g++
DIR	= ../..
INCL    = $(DIR)/include
OPT	= -O3 -Wall -Wextra -Wshadow -I$(INCL)
OPT32  = $(OPT) -DB_32

LIB32	= $(DIR)/lib/BDD32.a
LIB64	= $(DIR)/lib/BDD64.a
OBJC32	= $(DIR)/src/BDDc/*_32.o
OBJC64	= $(DIR)/src/BDDc/*_64.o
OBJX32	= $(DIR)/src/BDDXc/*_32.o
OBJX64	= $(DIR)/src/BDDXc/*_64.o

all:    $(LIB64)

32:     $(LIB32)

64:     $(LIB64)

$(LIB64): BDD_64.o BDDX11_64.o BDDHASH_64.o ZDD_64.o ZDDX11_64.o \
	  ZDDHASH_64.o MLZDDV_64.o SOP_64.o BtoI_64.o \
	  CtoI_64.o CtoIX11_64.o BDDDG_64.o ZDDDG_64.o \
	  PiDD_64.o RotPiDD_64.o SeqBDD_64.o GBase_64.o BDDCT_64.o
	  rm -f $(LIB64)
	  touch $(DIR)/src/BDDXc/graph_64.o
	  ar cr $(LIB64) *_64.o $(OBJC64) $(OBJX64)
	  ranlib $(LIB64)

$(LIB32): BDD_32.o BDDX11_32.o BDDHASH_32.o ZDD_32.o ZDDX11_32.o \
	  ZDDHASH_32.o MLZDDV_32.o SOP_32.o BtoI_32.o \
	  CtoI_32.o CtoIX11_32.o BDDDG_32.o ZDDDG_32.o \
	  PiDD_32.o RotPiDD_32.o SeqBDD_32.o GBase_32.o BDDCT_32.o
	  rm -f $(LIB32)
	  touch $(DIR)/src/BDDXc/graph_32.o
	  ar cr $(LIB32) *_32.o $(OBJC32) $(OBJX32)
	  ranlib $(LIB32)

clean:
	rm -f *.o *~

BDD_32.o: BDD.cc $(INCL)/BDD.h
	$(CC) $(OPT32) -c BDD.cc -o BDD_32.o

BDD_64.o: BDD.cc $(INCL)/BDD.h
	$(CC) $(OPT) -c BDD.cc -o BDD_64.o

BDDX11_32.o: BDDX11.cc $(INCL)/BDD.h
	$(CC) $(OPT32) -c BDDX11.cc -o BDDX11_32.o

BDDX11_64.o: BDDX11.cc $(INCL)/BDD.h
	$(CC) $(OPT) -c BDDX11.cc -o BDDX11_64.o

BDDHASH_32.o: BDDHASH.cc $(INCL)/BDD.h
	$(CC) $(OPT32) -c BDDHASH.cc -o BDDHASH_32.o

BDDHASH_64.o: BDDHASH.cc $(INCL)/BDD.h
	$(CC) $(OPT) -c BDDHASH.cc -o BDDHASH_64.o

ZDD_32.o: ZDD.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c ZDD.cc -o ZDD_32.o

ZDD_64.o: ZDD.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c ZDD.cc -o ZDD_64.o

ZDDX11_32.o: ZDDX11.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c ZDDX11.cc -o ZDDX11_32.o

ZDDX11_64.o: ZDDX11.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c ZDDX11.cc -o ZDDX11_64.o

ZDDHASH_32.o: ZDDHASH.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c ZDDHASH.cc -o ZDDHASH_32.o

ZDDHASH_64.o: ZDDHASH.cc $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c ZDDHASH.cc -o ZDDHASH_64.o

MLZDDV_32.o: MLZDDV.cc $(INCL)/MLZDDV.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c MLZDDV.cc -o MLZDDV_32.o

MLZDDV_64.o: MLZDDV.cc $(INCL)/MLZDDV.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c MLZDDV.cc -o MLZDDV_64.o

SOP_32.o: SOP.cc $(INCL)/SOP.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c SOP.cc -o SOP_32.o

SOP_64.o: SOP.cc $(INCL)/SOP.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c SOP.cc -o SOP_64.o

BtoI_32.o: BtoI.cc $(INCL)/BtoI.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c BtoI.cc -o BtoI_32.o

BtoI_64.o: BtoI.cc $(INCL)/BtoI.h $(INCL)/BDD.h
	$(CC) $(OPT) -c BtoI.cc -o BtoI_64.o

CtoI_32.o: CtoI.cc $(INCL)/CtoI.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c CtoI.cc -o CtoI_32.o

CtoI_64.o: CtoI.cc $(INCL)/CtoI.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c CtoI.cc -o CtoI_64.o

CtoIX11_32.o: CtoIX11.cc $(INCL)/CtoI.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c CtoIX11.cc -o CtoIX11_32.o

CtoIX11_64.o: CtoIX11.cc $(INCL)/CtoI.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c CtoIX11.cc -o CtoIX11_64.o

BDDDG_32.o: BDDDG.cc $(INCL)/BDDDG.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c BDDDG.cc -o BDDDG_32.o

BDDDG_64.o: BDDDG.cc $(INCL)/BDDDG.h $(INCL)/BDD.h
	$(CC) $(OPT) -c BDDDG.cc -o BDDDG_64.o

ZDDDG_32.o: ZDDDG.cc $(INCL)/ZDDDG.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c ZDDDG.cc -o ZDDDG_32.o

ZDDDG_64.o: ZDDDG.cc $(INCL)/ZDDDG.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c ZDDDG.cc -o ZDDDG_64.o

PiDD_32.o: PiDD.cc $(INCL)/PiDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c PiDD.cc -o PiDD_32.o

PiDD_64.o: PiDD.cc $(INCL)/PiDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c PiDD.cc -o PiDD_64.o

RotPiDD_32.o: RotPiDD.cc $(INCL)/RotPiDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -std=c++11 -c RotPiDD.cc -o RotPiDD_32.o

RotPiDD_64.o: RotPiDD.cc $(INCL)/RotPiDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -std=c++11 -c RotPiDD.cc -o RotPiDD_64.o

SeqBDD_32.o: SeqBDD.cc $(INCL)/SeqBDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c SeqBDD.cc -o SeqBDD_32.o

SeqBDD_64.o: SeqBDD.cc $(INCL)/SeqBDD.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c SeqBDD.cc -o SeqBDD_64.o

GBase_32.o: GBase.cc $(INCL)/GBase.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c GBase.cc -o GBase_32.o

GBase_64.o: GBase.cc $(INCL)/GBase.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c GBase.cc -o GBase_64.o

BDDCT_32.o: BDDCT.cc $(INCL)/BDDCT.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT32) -c BDDCT.cc -o BDDCT_32.o

BDDCT_64.o: BDDCT.cc $(INCL)/BDDCT.h $(INCL)/ZDD.h $(INCL)/BDD.h
	$(CC) $(OPT) -c BDDCT.cc -o BDDCT_64.o

