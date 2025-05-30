/*********************************************
 * BDDC Cache Functions Test Program         *
 * Tests setcacheratiovalue and allocatecache *
 * functions from bddc.cc                     *
 *********************************************/

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <set>
#include <algorithm>
#include <exception>

// Define BDD_CPP to use C++ interface
#define BDD_CPP

// Include bddc.cc directly first to access static functions
#include "../src/BDDc/bddc.cc"

// Only include the minimal headers we need, avoid BDD.h to prevent macro conflicts
#include "../include/bddc.h"

using namespace std;


// Include bddc.cc directly to access static functions
// Note: This will include all the static variables and functions
// (Already included above before headers to avoid macro conflicts)

// Test counter
int test_count = 0;
int pass_count = 0;
int fail_count = 0;

// Test result recording
void test_result(const char* test_name, bool passed) {
    test_count++;
    if (passed) {
        pass_count++;
        std::cout << "[PASS] " << test_name << endl;
    } else {
        fail_count++;
        std::cout << "[FAIL] " << test_name << endl;
    }
}

using namespace sapporobdd;

// Test setcacheratiovalue function
void test_setcacheratiovalue() {
    std::cout << "\n=== Testing setcacheratiovalue function ===" << endl;
    
    // Test valid power of 2 values (integers)
    try {
        setcacheratiovalue(2.0);
        test_result("setcacheratiovalue(2.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(2.0) - power of 2", false);
    }
    
    try {
        setcacheratiovalue(4.0);
        test_result("setcacheratiovalue(4.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(4.0) - power of 2", false);
    }
    
    try {
        setcacheratiovalue(8.0);
        test_result("setcacheratiovalue(8.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(8.0) - power of 2", false);
    }
    
    try {
        setcacheratiovalue(16.0);
        test_result("setcacheratiovalue(16.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(16.0) - power of 2", false);
    }
    
    try {
        setcacheratiovalue(32.0);
        test_result("setcacheratiovalue(32.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(32.0) - power of 2", false);
    }
    
    // Test valid fractional power of 2 values
    try {
        setcacheratiovalue(0.5);
        test_result("setcacheratiovalue(0.5) - fractional power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.5) - fractional power of 2", false);
    }
    
    try {
        setcacheratiovalue(0.25);
        test_result("setcacheratiovalue(0.25) - fractional power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.25) - fractional power of 2", false);
    }
    
    try {
        setcacheratiovalue(0.125);
        test_result("setcacheratiovalue(0.125) - fractional power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.125) - fractional power of 2", false);
    }
    
    try {
        setcacheratiovalue(0.0625);
        test_result("setcacheratiovalue(0.0625) - fractional power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.0625) - fractional power of 2", false);
    }
    
    try {
        setcacheratiovalue(1.0);
        test_result("setcacheratiovalue(1.0) - power of 2", true);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(1.0) - power of 2", false);
    }
    
    // Test invalid non-power of 2 values - should throw exceptions
    try {
        setcacheratiovalue(3.0);
        test_result("setcacheratiovalue(3.0) - not power of 2, should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(3.0) - not power of 2, should throw exception", true);
    }
    
    try {
        setcacheratiovalue(0.3);
        test_result("setcacheratiovalue(0.3) - not power of 2, should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.3) - not power of 2, should throw exception", true);
    }
    
    try {
        setcacheratiovalue(0.6);
        test_result("setcacheratiovalue(0.6) - not power of 2, should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.6) - not power of 2, should throw exception", true);
    }
    
    try {
        setcacheratiovalue(1.5);
        test_result("setcacheratiovalue(1.5) - not power of 2, should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(1.5) - not power of 2, should throw exception", true);
    }
    
    try {
        setcacheratiovalue(2.5);
        test_result("setcacheratiovalue(2.5) - not power of 2, should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(2.5) - not power of 2, should throw exception", true);
    }
    
    // Test zero and negative values
    try {
        setcacheratiovalue(0.0);
        test_result("setcacheratiovalue(0.0) - should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(0.0) - should throw exception", true);
    }
    
    try {
        setcacheratiovalue(-0.5);
        test_result("setcacheratiovalue(-0.5) - should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(-0.5) - should throw exception", true);
    }
}

// Test allocatecache function
void test_allocatecache() {
    std::cout << "\n=== Testing allocatecache function ===" << endl;
    
    // Test various combinations of NodeSpc and CacheRatio
    std::vector<bddp> nodeSpcValues = {256, 511, 512, 513, 1024, 2048, 4096, 8192};
    std::vector<double> cacheRatioValues = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
    
    for (bddp nodeSpc : nodeSpcValues) {
        for (double cacheRatio : cacheRatioValues) {
            NodeSpc = nodeSpc; // Set global NodeSpc for the test
            setcacheratiovalue(cacheRatio); // Set cache ratio using the function
            allocatecache();
            
            std::cout << "NodeSpc: " << nodeSpc 
                      << ", CacheRatio: " << cacheRatio 
                      << ", new NodeSpc: " << NodeSpc
                      << ", new CacheSpc: " << CacheSpc << endl;
        }
    }
}

// Main test function
int main() {
    std::cout << "=== BDDC Cache Functions Test ===" << endl;
    std::cout << "Testing setcacheratiovalue and allocatecache functions" << endl;
    
    // Initialize BDD system
    if (bddinit(1000, 10000)) {
        std::cerr << "BDD initialization failed" << endl;
        return 1;
    }
    
    try {
        test_setcacheratiovalue();
        test_allocatecache();
        
        std::cout << "\n=== Test Summary ===" << endl;
        std::cout << "Total tests: " << test_count << endl;
        std::cout << "Passed: " << pass_count << endl;
        std::cout << "Failed: " << fail_count << endl;
        
        if (fail_count == 0) {
            std::cout << "All tests passed!" << endl;
            return 0;
        } else {
            std::cout << "Some tests failed!" << endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << endl;
        return 1;
    }
}
