# Makefile for ZDD test programs
# For Windows with MinGW or Linux/Unix with g++

CC = gcc
CFLAGS = -Wall -Wextra -I. -I../include -DB_64
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I. -I../include -DB_64

# Define B_64 for 64-bit mode (optional)
# -DB_64 is not needed for SAPPOROBDD++, but needed for SAPPOROBDD
CXXFLAGS += -DB_64

# Source files
SOURCES = test_ZBDD.cpp test_ZBDD_Hash.cpp ../src/BDD+/BDD.cc ../src/BDDc/bddc.cc
# Dynamically add the appropriate ZDD source file
ifneq ($(wildcard ../src/BDD+/ZDD.cc),)
    SOURCES += ../src/BDD+/ZDD.cc
else
    SOURCES += ../src/BDD+/ZBDD.cc
endif

HEADERS = ../include/BDD.h ../include/bddc.h
# Dynamically add the appropriate ZDD source file
ifneq ($(wildcard ../include/ZDD.h),)
    HEADERS += ../include/ZDD.h
else
    HEADERS += ../include/ZBDD.h
endif

# Object files
OBJ_ZDD = test_ZBDD.o ZDD.o BDD.o bddc.o
OBJ_HASH = test_ZBDD_Hash.o ZDD.o BDD.o bddc.o

# Target executables
#TARGETS = test_ZBDD test_ZBDD_Hash
TARGETS = test_ZBDD test_bddc_cache

# Default target
all: $(TARGETS)

# Build test_ZBDD
test_ZBDD: $(OBJ_ZDD)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_ZDD)

# Build test_ZBDD_Hash
test_ZBDD_Hash: $(OBJ_HASH)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_HASH)

# Build test_bddc_cache
test_bddc_cache: test_bddc_cache.o
	$(CXX) $(CXXFLAGS) -o $@ test_bddc_cache.o

# Compile rules
test_ZBDD.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_ZBDD.cpp

test_bddc_cache.o: test_bddc_cache.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_bddc_cache.cpp

test_ZBDD_Hash.o: test_ZBDD_Hash.cpp ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c test_ZBDD_Hash.cpp

ZDD.o: $(HEADERS)
	@if [ -f ../src/BDD+/ZDD.cc ]; then \
		echo "Using ZDD.cc"; \
		$(CXX) $(CXXFLAGS) -c ../src/BDD+/ZDD.cc -o ZDD.o; \
	else \
		echo "ZDD.cc not found, using ZBDD.cc instead"; \
		$(CXX) $(CXXFLAGS) -c ../src/BDD+/ZBDD.cc -o ZDD.o; \
	fi

BDD.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/BDD.cc -o BDD.o

bddc.o: ../include/bddc.h
	@if [ -f ../src/BDDc/bddc.cc ]; then \
		echo "Using bddc.cc with g++"; \
		$(CXX) $(CXXFLAGS) -c ../src/BDDc/bddc.cc -o bddc.o; \
	elif [ -f ../src/BDDc/bddc.c ]; then \
		echo "Using ../src/BDDc/bddc.c with gcc"; \
		$(CC) $(CFLAGS) -c ../src/BDDc/bddc.c -o bddc.o; \
	else \
		echo "Error: Could not find bddc.cc or bddc.c"; \
		exit 1; \
	fi

# SBDD_helper test

# http://codeforces.com/blog/entry/15547  -Wshift-overflow=2 -Wduplicated-cond
SBDDH_OPT = -Wall -Wextra -pedantic -Wshadow -Wformat=2 -Wfloat-equal \
-Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
-D_FORTIFY_SOURCE=2 -fstack-protector -Wlogical-op -Wconversion

testsbddh: SBDD_helper_testc_cpp.h SBDD_helper_testcpp.cpp \
        SBDD_helper_testc.c ../sbdd_helper/SBDD_helper.h
	g++ -O3 -I../include $(SBDDH_OPT) SBDD_helper_testcpp.cpp ../lib/BDD64.a -o testsbddh


# Run tests
test: $(TARGETS)
	@echo "Running test_ZBDD..."
	@./test_ZBDD
	@echo ""
	@echo "Running test_bddc_cache..."
	@./test_bddc_cache
	@echo ""
	@echo "Running test_ZBDD_Hash..."
	@./test_ZBDD_Hash

# Clean
clean:
	rm -f *.o $(TARGETS) test_ZBDD_results.txt test_ZBDD_Hash_results.txt test_ZBDD_export.dat test_bddc_cache testsbddh testsbddh.exe

# For Visual Studio nmake
# Use: nmake /f Makefile.vc
Makefile.vc:
	@echo "# Makefile for Visual Studio" > Makefile.vc
	@echo "CXX = cl" >> Makefile.vc
	@echo "CXXFLAGS = /EHsc /std:c++11 /I." >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "all: test_ZBDD test_ZBDD_Hash" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "test_ZBDD: test_ZBDD.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "	$(CXX) $(CXXFLAGS) /Fe:test_ZBDD test_ZBDD.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "test_ZBDD_Hash: test_ZBDD_Hash.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "	$(CXX) $(CXXFLAGS) /Fe:test_ZBDD_Hash test_ZBDD_Hash.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "clean:" >> Makefile.vc
	@echo "	del *.obj * test_ZBDD_results.txt test_ZBDD_Hash_results.txt test_ZBDD_export.dat" >> Makefile.vc

.PHONY: all test clean
