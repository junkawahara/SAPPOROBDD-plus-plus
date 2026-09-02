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

// Include bddc internal header to access internal functions
#include "../src/BDDc/bddc_internal.h"
// Include the apply macros to test the cache store guard
#include "../src/BDDc/bddc_apply_common.h"

// Only include the minimal headers we need, avoid BDD.h to prevent macro conflicts
#include "../include/bddc.h"

using namespace std;


// Note: Internal functions are now exposed via bddc_internal.h

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

// Regression test for review item C-5:
// A node-producing operation that runs out of memory returns bddnull.  That
// result describes the current memory state, not the operation itself, so it
// must never be written to the cache: bddgc() cannot clear such an entry
// (B_NP(bddnull) lies outside the node table, so the cache sweep skips it),
// and the operation would keep reporting a stale failure even after memory
// has been reclaimed.
void test_apply_cache_store_rejects_bddnull() {
    std::cout << "\n=== Testing APPLY_CACHE_STORE with a bddnull result ===" << endl;

    /* APPLY_CACHE_STORE recomputes the slot from (op, f, g) when it writes
       (the cache may have been enlarged since the key was taken), so the
       entry to inspect is the one that key names. */
    struct B_CacheTable *cachep = 0;
    struct B_CacheTable *entry, *entry2;
    bddp f = 2, g = 4, h;      /* arbitrary operands */
    unsigned char op = BC_AND;
    bddp key = B_CACHEKEY(op, f, g);

    entry = Cache + key;
    entry->op = BC_NULL;

    h = bddnull;
    APPLY_CACHE_STORE(key, op, f, g, h, cachep);
    test_result("bddnull result is not written to the cache",
                entry->op == BC_NULL);

    h = 6;
    APPLY_CACHE_STORE(key, op, f, g, h, cachep);
    test_result("a valid result is written to the cache",
                entry->op == BC_AND &&
                B_GET_BDDP(entry->f) == f &&
                B_GET_BDDP(entry->g) == g &&
                B_GET_BDDP(entry->h) == h);

    op = BC_UNION;
    entry2 = Cache + B_CACHEKEY(op, f, g);
    if(entry2 != entry) entry2->op = BC_NULL;
    h = bddnull;
    APPLY_CACHE_STORE(key, op, f, g, h, cachep);
    test_result("bddnull does not overwrite a valid entry",
                entry->op == BC_AND &&
                B_GET_BDDP(entry->h) == 6 &&
                (entry2 == entry || entry2->op == BC_NULL));

    entry->op = BC_NULL;
}

// Regression test for review_20260808_1 item 32:
// bddgc() collects the dead nodes with gc1() before it looks at GCThreshold,
// so the collected slots can be reused by any later allocation.  It used to
// skip the cache sweep when the threshold made it report failure, leaving
// stale entries whose bddp values silently resolved to the new, unrelated
// nodes on the next cache hit.  The sweep must run whenever nodes were
// collected, even if the GC is reported as failed.
void test_gc_threshold_sweeps_cache() {
    std::cout << "\n=== Testing cache sweep when GC is below GCThreshold ===" << endl;

    bddinit(256, 100000);
    bddsetgcthreshold(1000000); /* every GC reports failure from now on */

    for (int i = 0; i < 4; ++i) bddnewvar();
    bddp x1 = bddprime(1), x2 = bddprime(2);
    bddp x3 = bddprime(3), x4 = bddprime(4);
    bddp a = bddand(x1, x2);
    bddp b = bddand(x3, x4);

    bddp c = bddand(a, b);
    bddfree(c);
    bddgc(); /* collects the nodes only c used, then reports failure */

    /* Reuse the collected slots for different functions, then recompute:
       a stale (BC_AND, a, b, c) entry would resolve a & b to one of the
       new nodes occupying c's old slots. */
    bddp n2 = bddnot(x2);
    bddp d1 = bddand(x1, n2);
    bddp n3 = bddnot(x3);
    bddp d2 = bddand(n3, x4);
    bddp e = bddand(a, b);
    bddp r = bddat0(e, 2); /* (a & b) with x2 = 0 is constant false */
    test_result("a & b recomputed after the GC is still a & b",
                r == bddfalse);

    bddfree(r); bddfree(e); bddfree(d2); bddfree(n3);
    bddfree(d1); bddfree(n2);
    bddfree(b); bddfree(a);
    bddfree(x4); bddfree(x3); bddfree(x2); bddfree(x1);
    bddsetgcthreshold(0);
}

// Regression tests for review_20260830_3 items 1, 3, 4 and 7.
// NaN used to slip through every range check in setcacheratiovalue() and
// reach a float-to-int conversion no int can represent, leaving CacheRatio
// negative; a ratio one ulp below a power of 2 was truncated and rejected
// while its symmetric counterpart below 1 was accepted; bddinit() forgot to
// reset GCThreshold, so a new session inherited the old GC policy; and the
// diagnostics blamed bddsetcacheratio() even when bddinit() did the check.
void test_cacheratio_and_reinit_guards() {
    std::cout << "\n=== Testing cache ratio guards and re-initialization ===" << endl;

    bddinit(1000, 10000);
    setcacheratiovalue(0.5);

    try {
        setcacheratiovalue(NAN);
        test_result("setcacheratiovalue(NaN) - should throw exception", false);
    } catch (const std::exception& e) {
        test_result("setcacheratiovalue(NaN) - should throw exception", true);
    }
    test_result("a rejected NaN leaves CacheRatio untouched", CacheRatio == 0.5);

    /* 1.9999999999999998, the double just below 2.0 */
    try {
        setcacheratiovalue(nextafter(2.0, 1.0));
        test_result("a ratio one ulp below 2.0 is rounded to 2", CacheRatio == 2.0);
    } catch (const std::exception& e) {
        test_result("a ratio one ulp below 2.0 is rounded to 2", false);
    }
    /* the symmetric input below 1 has always been accepted */
    setcacheratiovalue(nextafter(0.5, 1.0));
    test_result("the symmetric ratio just above 0.5 is rounded to 0.5",
                CacheRatio == 0.5);

    bddsetgcthreshold(12345);
    bddinit(1000, 10000);
    test_result("bddinit() resets the GC threshold", bddgetgcthreshold() == 0);

    try {
        bddinit(1000, 10000, 0.3);
        test_result("bddinit() rejects a ratio that is not a power of 2", false);
    } catch (const std::exception& e) {
        test_result("bddinit() rejects a ratio that is not a power of 2", true);
        test_result("the diagnostic names bddinit(), not bddsetcacheratio()",
                    strstr(e.what(), "bddinit:") != NULL);
    }
    test_result("the rejected bddinit() leaves the running ratio in place",
                CacheRatio == 0.5);
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
    /* The loop above set NodeSpc without touching the node table it
       describes; put the manager back into a consistent state so that a
       test added after this one does not write past the real table. */
    bddinit(1000, 10000);
}

// Main test function
int main() {
    std::cout << "=== BDDC Cache Functions Test ===" << endl;
    std::cout << "Testing setcacheratiovalue and allocatecache functions" << endl;
    
    // Initialize BDD system
    /* bddinit() reports a failure by exception; its return value is
       always 0 */
    try { bddinit(1000, 10000); }
    catch (const BDDException& e) {
        std::cerr << "BDD initialization failed: " << e.what() << endl;
        return 1;
    }
    
    try {
        test_apply_cache_store_rejects_bddnull();
        test_gc_threshold_sweeps_cache();
        test_cacheratio_and_reinit_guards();
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
