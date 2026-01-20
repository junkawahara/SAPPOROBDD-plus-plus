# Makefile for ZDD test programs
# For Windows with MinGW or Linux/Unix with g++

CC = gcc
CFLAGS = -Wall -Wextra -I. -I../include -DB_64
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I. -I../include -DB_64

# Define B_64 for 64-bit mode (optional)
# -DB_64 is not needed for SAPPOROBDD++, but needed for SAPPOROBDD
CXXFLAGS += -DB_64

# B_EXTEND mode flags
CXXFLAGS_EXTEND = $(CXXFLAGS) -DB_EXTEND
CFLAGS_EXTEND = $(CFLAGS) -DB_EXTEND

# Source files
BDDC_SRCS = ../src/BDDc/bddc_init.cc ../src/BDDc/bddc_node.cc ../src/BDDc/bddc_gc.cc \
            ../src/BDDc/bddc_apply.cc ../src/BDDc/bddc_operations.cc \
            ../src/BDDc/bddc_io.cc ../src/BDDc/bddc_util.cc
SOURCES = test_ZBDD.cpp test_ZBDD_Hash.cpp ../src/BDD+/BDD.cc $(BDDC_SRCS)
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
OBJ_OPERATOR = test_operator.o ZDD.o BDD.o bddc.o

# B_EXTEND mode object files
OBJ_ZDD_EXTEND = test_ZBDD_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o
OBJ_OPERATOR_EXTEND = test_operator_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o

# Target executables
#TARGETS = test_ZBDD test_ZBDD_Hash
TARGETS = test_ZBDD test_bddc_cache test_operator
TARGETS_EXTEND = test_ZBDD_extend test_operator_extend test_bddc_cache_extend

# Default target
all: $(TARGETS)

# B_EXTEND mode targets
extend: $(TARGETS_EXTEND)

# Build test_ZBDD
test_ZBDD: $(OBJ_ZDD)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_ZDD)

# Build test_ZBDD_Hash
test_ZBDD_Hash: $(OBJ_HASH)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_HASH)

# Build test_bddc_cache
test_bddc_cache: test_bddc_cache.o bddc.o
	$(CXX) $(CXXFLAGS) -o $@ test_bddc_cache.o bddc.o

# Build test_operator
test_operator: $(OBJ_OPERATOR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_OPERATOR)

# Build B_EXTEND mode test programs
test_ZBDD_extend: $(OBJ_ZDD_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_ZDD_EXTEND)

test_operator_extend: $(OBJ_OPERATOR_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_OPERATOR_EXTEND)

test_bddc_cache_extend: test_bddc_cache_extend.o bddc_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ test_bddc_cache_extend.o bddc_extend.o

# Compile rules
test_ZBDD.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_ZBDD.cpp

test_bddc_cache.o: test_bddc_cache.cpp $(HEADERS) ../src/BDDc/bddc_internal.h
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c test_bddc_cache.cpp

test_operator.o: test_operator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_operator.cpp

# B_EXTEND mode compile rules
test_ZBDD_extend.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_ZBDD.cpp -o test_ZBDD_extend.o

test_operator_extend.o: test_operator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_operator.cpp -o test_operator_extend.o

test_bddc_cache_extend.o: test_bddc_cache.cpp $(HEADERS) ../src/BDDc/bddc_internal.h
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c test_bddc_cache.cpp -o test_bddc_cache_extend.o

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

bddc.o: ../include/bddc.h ../src/BDDc/bddc_internal.h $(BDDC_SRCS)
	@echo "Compiling split bddc source files..."
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_init.cc -o bddc_init.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_node.cc -o bddc_node.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_gc.cc -o bddc_gc.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply.cc -o bddc_apply.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_operations.cc -o bddc_operations.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_io.cc -o bddc_io.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_util.cc -o bddc_util.o
	ld -r bddc_init.o bddc_node.o bddc_gc.o bddc_apply.o bddc_operations.o bddc_io.o bddc_util.o -o bddc.o

# B_EXTEND mode object compilation
ZDD_extend.o: $(HEADERS)
	@if [ -f ../src/BDD+/ZDD.cc ]; then \
		echo "Using ZDD.cc with B_EXTEND"; \
		$(CXX) $(CXXFLAGS_EXTEND) -c ../src/BDD+/ZDD.cc -o ZDD_extend.o; \
	else \
		echo "ZDD.cc not found, using ZBDD.cc instead with B_EXTEND"; \
		$(CXX) $(CXXFLAGS_EXTEND) -c ../src/BDD+/ZBDD.cc -o ZDD_extend.o; \
	fi

BDD_extend.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS_EXTEND) -c ../src/BDD+/BDD.cc -o BDD_extend.o

bddc_extend.o: ../include/bddc.h ../src/BDDc/bddc_internal.h $(BDDC_SRCS)
	@echo "Compiling split bddc source files with B_EXTEND..."
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_init.cc -o bddc_init_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_node.cc -o bddc_node_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_gc.cc -o bddc_gc_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply.cc -o bddc_apply_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_operations.cc -o bddc_operations_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_io.cc -o bddc_io_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_util.cc -o bddc_util_extend.o
	ld -r bddc_init_extend.o bddc_node_extend.o bddc_gc_extend.o bddc_apply_extend.o bddc_operations_extend.o bddc_io_extend.o bddc_util_extend.o -o bddc_extend.o

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
	@echo "Running test_operator..."
	@./test_operator

# Run B_EXTEND mode tests
test_extend: $(TARGETS_EXTEND)
	@echo "Running B_EXTEND mode tests..."
	@echo "Running test_ZBDD_extend..."
	@./test_ZBDD_extend
	@echo ""
	@echo "Running test_operator_extend..."
	@./test_operator_extend
	@echo ""
	@echo "Running test_bddc_cache_extend..."
	@./test_bddc_cache_extend
	@echo ""

# Clean
clean:
	rm -f *.o $(TARGETS) $(TARGETS_EXTEND) test_ZBDD_results.txt test_ZBDD_Hash_results.txt test_ZBDD_export.dat test_bddc_cache testsbddh testsbddh.exe test_operator
	rm -f bddc_init.o bddc_node.o bddc_gc.o bddc_apply.o bddc_operations.o bddc_io.o bddc_util.o
	rm -f bddc_init_extend.o bddc_node_extend.o bddc_gc_extend.o bddc_apply_extend.o bddc_operations_extend.o bddc_io_extend.o bddc_util_extend.o

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

.PHONY: all test clean extend test_extend
