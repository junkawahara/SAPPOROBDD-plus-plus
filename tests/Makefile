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

# B_FORCE_ITERATIVE mode flags (force iterative apply for testing)
CXXFLAGS_ITERATIVE = $(CXXFLAGS) -DB_FORCE_ITERATIVE
CFLAGS_ITERATIVE = $(CFLAGS) -DB_FORCE_ITERATIVE

# Source files
BDDC_SRCS = ../src/BDDc/bddc_init.cc ../src/BDDc/bddc_node.cc ../src/BDDc/bddc_gc.cc \
            ../src/BDDc/bddc_apply.cc ../src/BDDc/bddc_apply_binary.cc \
            ../src/BDDc/bddc_apply_unary.cc ../src/BDDc/bddc_apply_count.cc \
            ../src/BDDc/bddc_apply_special.cc ../src/BDDc/bddc_apply_iterative.cc \
            ../src/BDDc/bddc_operations.cc ../src/BDDc/bddc_io.cc ../src/BDDc/bddc_util.cc
# The source of the ZDD class: its current name, or the old one when only that
# one is present.  The compile rules below name this variable both as their
# prerequisite and as the file they compile, so that editing the source really
# does rebuild the object.
ifneq ($(wildcard ../src/BDD+/ZDD.cc),)
    ZDD_SRC = ../src/BDD+/ZDD.cc
else
    ZDD_SRC = ../src/BDD+/ZBDD.cc
endif

SOURCES = test_ZBDD.cpp test_ZBDD_Hash.cpp ../src/BDD+/BDD.cc $(BDDC_SRCS) $(ZDD_SRC)

HEADERS = ../include/BDD.h ../include/bddc.h ../include/BDDException.h
# Dynamically add the appropriate ZDD source file
ifneq ($(wildcard ../include/ZDD.h),)
    HEADERS += ../include/ZDD.h
else
    HEADERS += ../include/ZBDD.h
endif

# Object files
OBJ_ZDD = test_ZBDD.o ZDD.o BDD.o bddc.o
OBJ_HASH = test_ZBDD_Hash.o ZDD.o BDD.o BDDHASH.o ZDDHASH.o bddc.o
OBJ_RECOVERY = test_recovery.o ZDD.o BDD.o bddc.o
OBJ_OPERATOR = test_operator.o ZDD.o BDD.o bddc.o
OBJ_ITERATIVE_TEST = test_iterative.o ZDD.o BDD.o bddc.o
OBJ_MP_OOM = test_mp_oom.o ZDD.o BDD.o bddc.o
OBJ_BDDCT = test_BDDCT.o BDDCT.o ZDD.o BDD.o bddc.o

# B_EXTEND mode object files
OBJ_ZDD_EXTEND = test_ZBDD_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o
OBJ_OPERATOR_EXTEND = test_operator_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o
OBJ_ITERATIVE_TEST_EXTEND = test_iterative_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o
OBJ_MP_OOM_EXTEND = test_mp_oom_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o
OBJ_BDDCT_EXTEND = test_BDDCT_extend.o BDDCT_extend.o ZDD_extend.o BDD_extend.o bddc_extend.o

# B_FORCE_ITERATIVE mode object files
OBJ_ZDD_ITERATIVE = test_ZBDD_iterative.o ZDD_iterative.o BDD_iterative.o bddc_iterative.o
OBJ_OPERATOR_ITERATIVE = test_operator_iterative.o ZDD_iterative.o BDD_iterative.o bddc_iterative.o
OBJ_MP_OOM_ITERATIVE = test_mp_oom_iterative.o ZDD_iterative.o BDD_iterative.o bddc_iterative.o

# Target executables
TARGETS = test_ZBDD test_ZBDD_Hash test_recovery test_bddc_cache test_operator test_iterative_deep test_mp_oom test_BDDCT
TARGETS_EXTEND = test_ZBDD_extend test_operator_extend test_bddc_cache_extend test_iterative_deep_extend test_mp_oom_extend test_BDDCT_extend
TARGETS_ITERATIVE = test_ZBDD_iterative test_operator_iterative test_bddc_cache_iterative test_mp_oom_iterative

# Default target
all: $(TARGETS)

# B_EXTEND mode targets
extend: $(TARGETS_EXTEND)

# B_FORCE_ITERATIVE mode targets (force iterative apply for testing)
iterative: $(TARGETS_ITERATIVE)

# Build test_ZBDD
test_ZBDD: $(OBJ_ZDD)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_ZDD)

# Build test_ZBDD_Hash
test_ZBDD_Hash: $(OBJ_HASH)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_HASH)

# Build test_recovery (usability after caught exceptions)
test_recovery: $(OBJ_RECOVERY)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_RECOVERY)

# Build test_bddc_cache
test_bddc_cache: test_bddc_cache.o bddc.o
	$(CXX) $(CXXFLAGS) -o $@ test_bddc_cache.o bddc.o

# Build test_operator
test_operator: $(OBJ_OPERATOR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_OPERATOR)

# Build test_iterative_deep (tests recursive vs iterative apply with high variable count)
test_iterative_deep: $(OBJ_ITERATIVE_TEST)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_ITERATIVE_TEST)

# Build test_mp_oom (allocation fault injection; --wrap redirects the
# library's malloc/realloc calls to the test's __wrap_malloc/__wrap_realloc)
test_mp_oom: $(OBJ_MP_OOM)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_MP_OOM) -Wl,--wrap=malloc -Wl,--wrap=realloc

# Build test_BDDCT (cost table and its caches)
test_BDDCT: $(OBJ_BDDCT)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_BDDCT)

# Build B_EXTEND mode test programs
test_ZBDD_extend: $(OBJ_ZDD_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_ZDD_EXTEND)

test_operator_extend: $(OBJ_OPERATOR_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_OPERATOR_EXTEND)

test_bddc_cache_extend: test_bddc_cache_extend.o bddc_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ test_bddc_cache_extend.o bddc_extend.o

test_iterative_deep_extend: $(OBJ_ITERATIVE_TEST_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_ITERATIVE_TEST_EXTEND)

test_mp_oom_extend: $(OBJ_MP_OOM_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_MP_OOM_EXTEND) -Wl,--wrap=malloc -Wl,--wrap=realloc

test_BDDCT_extend: $(OBJ_BDDCT_EXTEND)
	$(CXX) $(CXXFLAGS_EXTEND) -o $@ $(OBJ_BDDCT_EXTEND)

# Build B_FORCE_ITERATIVE mode test programs
test_ZBDD_iterative: $(OBJ_ZDD_ITERATIVE)
	$(CXX) $(CXXFLAGS_ITERATIVE) -o $@ $(OBJ_ZDD_ITERATIVE)

test_operator_iterative: $(OBJ_OPERATOR_ITERATIVE)
	$(CXX) $(CXXFLAGS_ITERATIVE) -o $@ $(OBJ_OPERATOR_ITERATIVE)

test_bddc_cache_iterative: test_bddc_cache_iterative.o bddc_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -o $@ test_bddc_cache_iterative.o bddc_iterative.o

test_mp_oom_iterative: $(OBJ_MP_OOM_ITERATIVE)
	$(CXX) $(CXXFLAGS_ITERATIVE) -o $@ $(OBJ_MP_OOM_ITERATIVE) -Wl,--wrap=malloc -Wl,--wrap=realloc

# Compile rules
test_ZBDD.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_ZBDD.cpp

test_bddc_cache.o: test_bddc_cache.cpp $(HEADERS) ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c test_bddc_cache.cpp

test_operator.o: test_operator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_operator.cpp

test_iterative.o: test_iterative.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_iterative.cpp

test_mp_oom.o: test_mp_oom.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c test_mp_oom.cpp

test_BDDCT.o: test_BDDCT.cpp $(HEADERS) ../include/BDDCT.h
	$(CXX) $(CXXFLAGS) -c test_BDDCT.cpp

# B_EXTEND mode compile rules
test_ZBDD_extend.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_ZBDD.cpp -o test_ZBDD_extend.o

test_operator_extend.o: test_operator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_operator.cpp -o test_operator_extend.o

test_bddc_cache_extend.o: test_bddc_cache.cpp $(HEADERS) ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c test_bddc_cache.cpp -o test_bddc_cache_extend.o

test_iterative_extend.o: test_iterative.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_iterative.cpp -o test_iterative_extend.o

test_mp_oom_extend.o: test_mp_oom.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c test_mp_oom.cpp -o test_mp_oom_extend.o

test_BDDCT_extend.o: test_BDDCT.cpp $(HEADERS) ../include/BDDCT.h
	$(CXX) $(CXXFLAGS_EXTEND) -c test_BDDCT.cpp -o test_BDDCT_extend.o

# B_FORCE_ITERATIVE mode compile rules
test_ZBDD_iterative.o: test_ZBDD.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_ITERATIVE) -c test_ZBDD.cpp -o test_ZBDD_iterative.o

test_operator_iterative.o: test_operator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_ITERATIVE) -c test_operator.cpp -o test_operator_iterative.o

test_bddc_cache_iterative.o: test_bddc_cache.cpp $(HEADERS) ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c test_bddc_cache.cpp -o test_bddc_cache_iterative.o

test_mp_oom_iterative.o: test_mp_oom.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_ITERATIVE) -c test_mp_oom.cpp -o test_mp_oom_iterative.o

test_ZBDD_Hash.o: test_ZBDD_Hash.cpp ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c test_ZBDD_Hash.cpp

test_recovery.o: test_recovery.cpp ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c test_recovery.cpp

ZDD.o: $(ZDD_SRC) $(HEADERS)
	@echo "Using $(ZDD_SRC)"
	$(CXX) $(CXXFLAGS) -c $(ZDD_SRC) -o ZDD.o

BDD.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/BDD.cc -o BDD.o

BDDHASH.o: ../src/BDD+/BDDHASH.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/BDDHASH.cc -o BDDHASH.o

ZDDHASH.o: ../src/BDD+/ZDDHASH.cc ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/ZDDHASH.cc -o ZDDHASH.o

BDDCT.o: ../src/BDD+/BDDCT.cc ../include/BDDCT.h $(HEADERS)
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/BDDCT.cc -o BDDCT.o

bddc.o: ../include/bddc.h ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h $(BDDC_SRCS)
	@echo "Compiling split bddc source files..."
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_init.cc -o bddc_init.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_node.cc -o bddc_node.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_gc.cc -o bddc_gc.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply.cc -o bddc_apply.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply_binary.cc -o bddc_apply_binary.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply_unary.cc -o bddc_apply_unary.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply_count.cc -o bddc_apply_count.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply_special.cc -o bddc_apply_special.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_apply_iterative.cc -o bddc_apply_iterative.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_operations.cc -o bddc_operations.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_io.cc -o bddc_io.o
	$(CXX) $(CXXFLAGS) -I../src/BDDc -c ../src/BDDc/bddc_util.cc -o bddc_util.o
	ld -r bddc_init.o bddc_node.o bddc_gc.o bddc_apply.o bddc_apply_binary.o bddc_apply_unary.o bddc_apply_count.o bddc_apply_special.o bddc_apply_iterative.o bddc_operations.o bddc_io.o bddc_util.o -o bddc.o

# B_EXTEND mode object compilation
ZDD_extend.o: $(ZDD_SRC) $(HEADERS)
	@echo "Using $(ZDD_SRC) with B_EXTEND"
	$(CXX) $(CXXFLAGS_EXTEND) -c $(ZDD_SRC) -o ZDD_extend.o

BDD_extend.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS_EXTEND) -c ../src/BDD+/BDD.cc -o BDD_extend.o

BDDCT_extend.o: ../src/BDD+/BDDCT.cc ../include/BDDCT.h $(HEADERS)
	$(CXX) $(CXXFLAGS_EXTEND) -c ../src/BDD+/BDDCT.cc -o BDDCT_extend.o

bddc_extend.o: ../include/bddc.h ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h $(BDDC_SRCS)
	@echo "Compiling split bddc source files with B_EXTEND..."
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_init.cc -o bddc_init_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_node.cc -o bddc_node_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_gc.cc -o bddc_gc_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply.cc -o bddc_apply_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply_binary.cc -o bddc_apply_binary_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply_unary.cc -o bddc_apply_unary_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply_count.cc -o bddc_apply_count_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply_special.cc -o bddc_apply_special_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_apply_iterative.cc -o bddc_apply_iterative_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_operations.cc -o bddc_operations_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_io.cc -o bddc_io_extend.o
	$(CXX) $(CXXFLAGS_EXTEND) -I../src/BDDc -c ../src/BDDc/bddc_util.cc -o bddc_util_extend.o
	ld -r bddc_init_extend.o bddc_node_extend.o bddc_gc_extend.o bddc_apply_extend.o bddc_apply_binary_extend.o bddc_apply_unary_extend.o bddc_apply_count_extend.o bddc_apply_special_extend.o bddc_apply_iterative_extend.o bddc_operations_extend.o bddc_io_extend.o bddc_util_extend.o -o bddc_extend.o

# B_FORCE_ITERATIVE mode object compilation
ZDD_iterative.o: $(ZDD_SRC) $(HEADERS)
	@echo "Using $(ZDD_SRC) with B_FORCE_ITERATIVE"
	$(CXX) $(CXXFLAGS_ITERATIVE) -c $(ZDD_SRC) -o ZDD_iterative.o

BDD_iterative.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS_ITERATIVE) -c ../src/BDD+/BDD.cc -o BDD_iterative.o

bddc_iterative.o: ../include/bddc.h ../src/BDDc/bddc_internal.h ../src/BDDc/bddc_apply_common.h $(BDDC_SRCS)
	@echo "Compiling split bddc source files with B_FORCE_ITERATIVE..."
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_init.cc -o bddc_init_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_node.cc -o bddc_node_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_gc.cc -o bddc_gc_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply.cc -o bddc_apply_iterative_forced.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply_binary.cc -o bddc_apply_binary_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply_unary.cc -o bddc_apply_unary_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply_count.cc -o bddc_apply_count_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply_special.cc -o bddc_apply_special_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_apply_iterative.cc -o bddc_apply_iterative_impl.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_operations.cc -o bddc_operations_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_io.cc -o bddc_io_iterative.o
	$(CXX) $(CXXFLAGS_ITERATIVE) -I../src/BDDc -c ../src/BDDc/bddc_util.cc -o bddc_util_iterative.o
	ld -r bddc_init_iterative.o bddc_node_iterative.o bddc_gc_iterative.o bddc_apply_iterative_forced.o bddc_apply_binary_iterative.o bddc_apply_unary_iterative.o bddc_apply_count_iterative.o bddc_apply_special_iterative.o bddc_apply_iterative_impl.o bddc_operations_iterative.o bddc_io_iterative.o bddc_util_iterative.o -o bddc_iterative.o

# SBDD_helper test

# http://codeforces.com/blog/entry/15547  -Wshift-overflow=2 -Wduplicated-cond
SBDDH_OPT = -Wall -Wextra -pedantic -Wshadow -Wformat=2 -Wfloat-equal \
-Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
-D_FORTIFY_SOURCE=2 -fstack-protector -Wlogical-op -Wconversion

testsbddh: SBDD_helper_testc_cpp.h SBDD_helper_testcpp.cpp \
        SBDD_helper_testc.c ../sbdd_helper/SBDD_helper.h Makefile
	g++ -O3 -I../include -DB_64 $(SBDDH_OPT) SBDD_helper_testcpp.cpp ../lib/BDD64.a -o testsbddh

# The test drivers above are shared with the classic SAPPOROBDD and thus
# compile SBDD_helper in its default configuration.  This one is the
# upstream smoke test of the SAPPOROBDD_PLUS_PLUS configuration, which
# the header selects only when that macro is defined (the test source
# defines it itself).
testsbddhspp: SBDD_helper_testspp.cpp ../sbdd_helper/SBDD_helper.h Makefile
	g++ -O3 -I../include -DB_64 $(SBDDH_OPT) SBDD_helper_testspp.cpp ../lib/BDD64.a -o testsbddhspp

# The B_EXTEND counterparts of the two targets above.  That mode is the
# one whose bddvarmax (2^32 - 2) does not fit in an int, which
# the level range checks of SBDD_helper have to allow for.  No library
# archive is built for the mode, so these link the object files that the
# extend target compiles here; build them first with "make extend", or
# let make build the four objects these rules depend on.
SBDDH_EXTEND_OBJ = bddc_extend.o ZDD_extend.o BDD_extend.o BDDCT_extend.o

testsbddh_extend: SBDD_helper_testc_cpp.h SBDD_helper_testcpp.cpp \
        SBDD_helper_testc.c ../sbdd_helper/SBDD_helper.h Makefile \
        $(SBDDH_EXTEND_OBJ)
	g++ -O3 -I../include -DB_64 -DB_EXTEND $(SBDDH_OPT) SBDD_helper_testcpp.cpp $(SBDDH_EXTEND_OBJ) -o testsbddh_extend

testsbddhspp_extend: SBDD_helper_testspp.cpp ../sbdd_helper/SBDD_helper.h \
        Makefile $(SBDDH_EXTEND_OBJ)
	g++ -O3 -I../include -DB_64 -DB_EXTEND $(SBDDH_OPT) SBDD_helper_testspp.cpp $(SBDDH_EXTEND_OBJ) -o testsbddhspp_extend


# Run tests
test: $(TARGETS)
	@echo "Running test_ZBDD..."
	@./test_ZBDD
	@echo ""
	@echo "Running test_ZBDD_Hash..."
	@./test_ZBDD_Hash
	@echo ""
	@echo "Running test_recovery..."
	@./test_recovery
	@echo ""
	@echo "Running test_bddc_cache..."
	@./test_bddc_cache
	@echo ""
	@echo "Running test_operator..."
	@./test_operator
	@echo ""
	@echo "Running test_iterative_deep (recursive vs iterative apply)..."
	@./test_iterative_deep
	@echo ""
	@echo "Running test_mp_oom (multi-precision table OOM)..."
	@./test_mp_oom
	@echo ""
	@echo "Running test_BDDCT (cost table and its caches)..."
	@./test_BDDCT

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
	@echo "Running test_iterative_deep_extend..."
	@./test_iterative_deep_extend
	@echo ""
	@echo "Running test_mp_oom_extend..."
	@./test_mp_oom_extend
	@echo ""
	@echo "Running test_BDDCT_extend..."
	@./test_BDDCT_extend
	@echo ""

# Run B_FORCE_ITERATIVE mode tests
test_iterative: $(TARGETS_ITERATIVE)
	@echo "Running B_FORCE_ITERATIVE mode tests (iterative apply forced)..."
	@echo "Running test_ZBDD_iterative..."
	@./test_ZBDD_iterative
	@echo ""
	@echo "Running test_operator_iterative..."
	@./test_operator_iterative
	@echo ""
	@echo "Running test_bddc_cache_iterative..."
	@./test_bddc_cache_iterative
	@echo ""
	@echo "Running test_mp_oom_iterative..."
	@./test_mp_oom_iterative
	@echo ""

# Clean
clean:
	rm -f *.o $(TARGETS) $(TARGETS_EXTEND) $(TARGETS_ITERATIVE) test_ZBDD_results.txt test_ZBDD_Hash_results.txt test_zdd_export.dat test_bddc_cache testsbddh testsbddh.exe testsbddhspp testsbddhspp.exe testsbddh_extend testsbddh_extend.exe testsbddhspp_extend testsbddhspp_extend.exe test_operator test_iterative_deep test_iterative_deep_extend
	rm -f bddc_init.o bddc_node.o bddc_gc.o bddc_apply.o bddc_apply_binary.o bddc_apply_unary.o bddc_apply_count.o bddc_apply_special.o bddc_apply_iterative.o bddc_operations.o bddc_io.o bddc_util.o
	rm -f bddc_init_extend.o bddc_node_extend.o bddc_gc_extend.o bddc_apply_extend.o bddc_apply_binary_extend.o bddc_apply_unary_extend.o bddc_apply_count_extend.o bddc_apply_special_extend.o bddc_apply_iterative_extend.o bddc_operations_extend.o bddc_io_extend.o bddc_util_extend.o
	rm -f bddc_init_iterative.o bddc_node_iterative.o bddc_gc_iterative.o bddc_apply_iterative_forced.o bddc_apply_binary_iterative.o bddc_apply_unary_iterative.o bddc_apply_count_iterative.o bddc_apply_special_iterative.o bddc_apply_iterative_impl.o bddc_operations_iterative.o bddc_io_iterative.o bddc_util_iterative.o

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

.PHONY: all test clean extend test_extend iterative test_iterative
