# Makefile for ZDD test programs
# For Windows with MinGW or Linux/Unix with g++

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I. -I../include

# Define B_64 for 64-bit mode (optional)
# CXXFLAGS += -DB_64

# Source files
SOURCES = test_ZDD.cpp test_ZDD_Hash.cpp ../src/BDD+/ZDD.cc ../src/BDD+/BDD.cc ../src/BDDc/bddc.cc

# Object files
OBJ_ZDD = test_ZDD.o ZDD.o BDD.o bddc.o
OBJ_HASH = test_ZDD_Hash.o ZDD.o BDD.o bddc.o

# Target executables
TARGETS = test_ZDD test_ZDD_Hash

# Default target
all: $(TARGETS)

# Build test_ZDD
test_ZDD: $(OBJ_ZDD)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_ZDD)

# Build test_ZDD_Hash
test_ZDD_Hash: $(OBJ_HASH)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_HASH)

# Compile rules
test_ZDD.o: test_ZDD.cpp ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c test_ZDD.cpp

test_ZDD_Hash.o: test_ZDD_Hash.cpp ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c test_ZDD_Hash.cpp

ZDD.o: ../src/BDD+/ZDD.cc ../include/ZDD.h ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/ZDD.cc -o ZDD.o

BDD.o: ../src/BDD+/BDD.cc ../include/BDD.h ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDD+/BDD.cc -o BDD.o

bddc.o: ../src/BDDc/bddc.cc ../include/bddc.h
	$(CXX) $(CXXFLAGS) -c ../src/BDDc/bddc.cc -o bddc.o

# Run tests
test: $(TARGETS)
	@echo "Running test_ZDD..."
	@./test_ZDD
	@echo ""
	@echo "Running test_ZDD_Hash..."
	@./test_ZDD_Hash

# Clean
clean:
	rm -f *.o $(TARGETS) test_ZDD_results.txt test_ZDD_Hash_results.txt test_zdd_export.dat

# For Visual Studio nmake
# Use: nmake /f Makefile.vc
Makefile.vc:
	@echo "# Makefile for Visual Studio" > Makefile.vc
	@echo "CXX = cl" >> Makefile.vc
	@echo "CXXFLAGS = /EHsc /std:c++11 /I." >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "all: test_ZDD test_ZDD_Hash" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "test_ZDD: test_ZDD.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "	$(CXX) $(CXXFLAGS) /Fe:test_ZDD test_ZDD.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "test_ZDD_Hash: test_ZDD_Hash.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "	$(CXX) $(CXXFLAGS) /Fe:test_ZDD_Hash test_ZDD_Hash.cpp ZDD.cc BDD.cc bddc.cc" >> Makefile.vc
	@echo "" >> Makefile.vc
	@echo "clean:" >> Makefile.vc
	@echo "	del *.obj * test_ZDD_results.txt test_ZDD_Hash_results.txt test_zdd_export.dat" >> Makefile.vc

.PHONY: all test clean
