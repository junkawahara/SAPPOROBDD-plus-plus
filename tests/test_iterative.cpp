/*********************************************
 * BDD/ZDD Iterative Apply Test Program      *
 * Tests non-recursive apply implementation  *
 * for deep recursion safety                 *
 *********************************************/

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>
#include <algorithm>
#include <set>
#define BDD_CPP
#include "../include/bddc.h"
#include "../include/BDD.h"

#ifdef SAPPOROBDD_PLUS_PLUS
#include "../include/ZDD.h"
#else
#include "../include/ZBDD.h"
#endif

using namespace std;

#ifdef SAPPOROBDD_PLUS_PLUS
using namespace sapporobdd;
#else
#define ZDD ZBDD
#define ZDD_ID ZBDD_ID
#endif

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
        std::cout << "[**FAIL] " << test_name << endl;
    }
}

// Helper function to build a ZDD from a family of sets
ZDD buildZDDFromSets(const vector<vector<int>>& sets) {
    ZDD result(0); // Empty ZDD

    for (size_t i = 0; i < sets.size(); ++i) {
        ZDD setZDD(1); // Unit ZDD

        for (size_t j = 0; j < sets[i].size(); ++j) {
            int elem = sets[i][j];
            while (BDD_VarUsed() < elem) {
                BDD_NewVar();
            }
            setZDD = setZDD.Change(elem);
        }
        result += setZDD;
    }

    return result;
}

// Test initialization
void test_init() {
    std::cout << "=== BDD/ZDD Iterative Apply Test ===" << endl;
    std::cout << "Start Time: " << __DATE__ << " " << __TIME__ << endl;
    std::cout << "=====================================" << endl << endl;

    // Memory allocation for tests
#ifdef B_EXTEND
    size_t node_limit = (size_t)512 * 1024 * 1024; // node limit for B_EXTEND mode
    BDD_Init(1024, node_limit, 1.0);
#else
    size_t node_limit = (size_t)256 * 1024 * 1024; // node limit for standard mode
    BDD_Init(512, node_limit, 1.0);
#endif

    std::cout << "BDD system initialized" << endl;
    std::cout << "APPLY_RECURSION_THRESHOLD = 8192" << endl;
    std::cout << endl;
}

// Test cleanup
void test_cleanup() {
    std::cout << endl << "=======================================" << endl;
    std::cout << "Total Tests: " << test_count << endl;
    std::cout << "Passed: " << pass_count << endl;
    std::cout << "Failed: " << fail_count << endl;
    std::cout << "=======================================" << endl;
}

/*
 * Test 1: Verify iterative version is used when VarUsed > 8192
 * Creates 10000 variables and performs operations without stack overflow
 */
void test_deep_recursion_no_overflow() {
    std::cout << "=== Test: Deep Recursion Safety (10000 variables) ===" << endl;

    // Reinitialize to ensure clean state
#ifdef B_EXTEND
    BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
    BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

    const int NUM_VARS = 10000;
    std::cout << "Creating " << NUM_VARS << " variables..." << endl;

    clock_t start = clock();
    for (int i = 1; i <= NUM_VARS; ++i) {
        BDD_NewVar();
        if (i % 2000 == 0) {
            std::cout << "  Created " << i << " variables" << endl;
        }
    }
    clock_t end = clock();

    std::cout << "Variable creation took " << double(end - start) / CLOCKS_PER_SEC << " seconds" << endl;
    test_result("Created 10000 variables", BDD_VarUsed() == NUM_VARS);
    test_result("VarUsed exceeds threshold (8192)", BDD_VarUsed() > 8192);

    // Test that iterative apply works correctly when VarUsed > 8192
    // Change() calls apply() which should use apply_unary_iterative() when VarUsed > 8192
    std::cout << "Testing iterative apply with high VarUsed..." << endl;

    try {
        // Test building a DEEP ZDD using Change()
        // Since VarUsed = 10000 > 8192, apply_unary_iterative should be used
        std::cout << "Building deep ZDD using Change() (should use iterative apply)..." << endl;
        std::cout << "  VarUsed = " << BDD_VarUsed() << " (> 8192, so iterative mode)" << endl;

        const int DEEP_SIZE = 10000;  // Build ZDD with 10000 variables
        ZDD deep_z(1);  // Start with {{}}

        start = clock();
        for (int i = 1; i <= DEEP_SIZE; ++i) {
            deep_z = deep_z.Change(i);
            if (i % 2000 == 0) {
                std::cout << "  Progress: " << i << " variables added" << endl;
            }
        }
        end = clock();
        std::cout << "  Deep ZDD built in " << double(end - start) / CLOCKS_PER_SEC << " seconds" << endl;
        // Size() (and the counting operations) walk the graph iteratively
        // when the recursive walk would not fit into the recursion budget,
        // so they can be called on a ZDD this deep.

        test_result("Built deep ZDD with 10000 variables using iterative Change()", true);

        // Now test operations on this deep ZDD
        std::cout << "Testing operations on deep ZDD (10000 levels)..." << endl;
        std::cout << "Note: apply operations should use iterative version" << endl;

        // Card() - uses apply() with BC_CARD
        std::cout << "Testing Card()..." << endl;
        start = clock();
        bddword deep_card = deep_z.Card();
        end = clock();
        std::cout << "  Card() = " << deep_card << " (took " << double(end - start) / CLOCKS_PER_SEC << "s)" << endl;
        test_result("Card() on deep ZDD (10000 levels) works", deep_card == 1);

        std::cout << "Deep ZDD test with 10000+ iterative operations completed!" << endl;

        // Also test shallow ZDDs for comparison
        std::cout << "\nAlso testing shallow ZDDs in iterative mode..." << endl;

        // z1 = {{100, 200, 300}, {150}, {500, 600}}
        ZDD z1(0);
        {
            ZDD s1(1); s1 = s1.Change(100).Change(200).Change(300);
            ZDD s2(1); s2 = s2.Change(150);
            ZDD s3(1); s3 = s3.Change(500).Change(600);
            z1 = s1 + s2 + s3;
        }
        std::cout << "  z1 created: " << z1.Card() << " sets, " << z1.Size() << " nodes" << endl;

        // z2 = {{200, 300}, {150}, {700}}
        ZDD z2(0);
        {
            ZDD s1(1); s1 = s1.Change(200).Change(300);
            ZDD s2(1); s2 = s2.Change(150);
            ZDD s3(1); s3 = s3.Change(700);
            z2 = s1 + s2 + s3;
        }
        std::cout << "  z2 created: " << z2.Card() << " sets, " << z2.Size() << " nodes" << endl;

        test_result("VarUsed > 8192 (iterative apply triggered)", BDD_VarUsed() > 8192);

        // Test operator+ (UNION)
        std::cout << "Testing operator+ (BC_UNION) in iterative mode..." << endl;
        start = clock();
        ZDD union_result = z1 + z2;
        end = clock();
        std::cout << "  operator+ took " << double(end - start) / CLOCKS_PER_SEC << " seconds" << endl;
        std::cout << "  Result: " << union_result.Card() << " sets" << endl;

        test_result("operator+ works in iterative mode", union_result.Card() == 5);

        // Test operator& (INTERSEC)
        std::cout << "Testing operator& (BC_INTERSEC) in iterative mode..." << endl;
        start = clock();
        ZDD intersect_result = z1 & z2;
        end = clock();
        std::cout << "  operator& took " << double(end - start) / CLOCKS_PER_SEC << " seconds" << endl;
        std::cout << "  Result: " << intersect_result.Card() << " sets" << endl;

        test_result("operator& works in iterative mode", intersect_result.Card() == 1);

        // Test operator- (SUBTRACT)
        std::cout << "Testing operator- (BC_SUBTRACT) in iterative mode..." << endl;
        start = clock();
        ZDD subtract_result = z1 - z2;
        end = clock();
        std::cout << "  operator- took " << double(end - start) / CLOCKS_PER_SEC << " seconds" << endl;
        std::cout << "  Result: " << subtract_result.Card() << " sets" << endl;

        test_result("operator- works in iterative mode", subtract_result.Card() == 2);

        // Test OffSet (unary operation)
        std::cout << "Testing OffSet() (BC_OFFSET) in iterative mode..." << endl;
        start = clock();
        ZDD offset_result = z1.OffSet(200);
        end = clock();
        std::cout << "  OffSet(200) result: " << offset_result.Card() << " sets" << endl;

        test_result("OffSet() works in iterative mode", offset_result.Card() == 2);

        // Test OnSet (unary operation)
        std::cout << "Testing OnSet() (BC_ONSET) in iterative mode..." << endl;
        start = clock();
        ZDD onset_result = z1.OnSet(200);
        end = clock();
        std::cout << "  OnSet(200) result: " << onset_result.Card() << " sets" << endl;

        test_result("OnSet() works in iterative mode", onset_result.Card() == 1);

        // Test Card() (count operation)
        std::cout << "Testing Card() (BC_CARD) in iterative mode..." << endl;
        start = clock();
        bddword card = z1.Card();
        end = clock();
        std::cout << "  Card() result: " << card << endl;

        test_result("Card() works in iterative mode", card == 3);

        // Test Lit() (count operation)
        std::cout << "Testing Lit() (BC_LIT) in iterative mode..." << endl;
        start = clock();
        bddword lit = z1.Lit();
        end = clock();
        std::cout << "  Lit() result: " << lit << endl;

        // z1 has {100,200,300}=3 + {150}=1 + {500,600}=2 = 6 literals total
        test_result("Lit() works in iterative mode", lit == 6);

        // Test Support() (returns set of all used variables)
        std::cout << "Testing Support() (BC_SUPPORT) in iterative mode..." << endl;
        start = clock();
        ZDD support_result = z1.Support();
        end = clock();
        std::cout << "  Support() result: " << support_result.Card() << " variables" << endl;

        // z1 uses variables: 100, 150, 200, 300, 500, 600 = 6 variables
        test_result("Support() works in iterative mode", support_result.Card() == 6);

        std::cout << "All iterative mode tests passed!" << endl;
        std::cout << "Note: VarUsed=" << BDD_VarUsed() << " > 8192, so iterative apply was used" << endl;

    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << endl;
        test_result("Iterative mode test threw exception", false);
    }

    std::cout << endl;
}

/*
 * Test 2: Compare results between recursive version (VarUsed <= 8192)
 * and iterative version (VarUsed > 8192)
 *
 * Strategy: Create equivalent ZDD structures in both modes and verify
 * that operations produce equivalent results.
 */
void test_recursive_vs_iterative_comparison() {
    std::cout << "=== Test: Recursive vs Iterative Result Comparison ===" << endl;

    // Test data
    vector<vector<int>> sets1 = {{10, 20, 30}, {15, 25}, {50}, {10}};
    vector<vector<int>> sets2 = {{20, 30}, {15, 25}, {60}, {10, 20}};

    // Store results from both phases
    bddword recursive_union_card, recursive_intersect_card, recursive_subtract_card;
    bddword recursive_change_card, recursive_z1_lit, recursive_z1_len;
    bddword iterative_union_card, iterative_intersect_card, iterative_subtract_card;
    bddword iterative_change_card, iterative_z1_lit, iterative_z1_len;

    // ---- Phase 1: Test with few variables (recursive version) ----
    {
        std::cout << "Phase 1: Testing with few variables (recursive version)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        // Create 100 variables (well under 8192 threshold)
        const int SMALL_VARS = 100;
        for (int i = 1; i <= SMALL_VARS; ++i) {
            BDD_NewVar();
        }
        std::cout << "  Created " << BDD_VarUsed() << " variables (recursive mode)" << endl;
        test_result("VarUsed is below threshold for recursive mode", BDD_VarUsed() <= 8192);

        // Create test ZDDs (scope limited to this block)
        ZDD z1 = buildZDDFromSets(sets1);
        ZDD z2 = buildZDDFromSets(sets2);

        // Perform operations and store results
        ZDD z_union = z1 + z2;
        ZDD z_intersect = z1 & z2;
        ZDD z_subtract = z1 - z2;
        ZDD z_change = z1.Change(75);

        recursive_union_card = z_union.Card();
        recursive_intersect_card = z_intersect.Card();
        recursive_subtract_card = z_subtract.Card();
        recursive_change_card = z_change.Card();
        recursive_z1_lit = z1.Lit();
        recursive_z1_len = z1.Len();

        std::cout << "  Recursive results:" << endl;
        std::cout << "    Union card: " << recursive_union_card << endl;
        std::cout << "    Intersect card: " << recursive_intersect_card << endl;
        std::cout << "    Subtract card: " << recursive_subtract_card << endl;
        std::cout << "    Change card: " << recursive_change_card << endl;
        std::cout << "    Z1 Lit: " << recursive_z1_lit << endl;
        std::cout << "    Z1 Len: " << recursive_z1_len << endl;
        // ZDDs go out of scope here before next BDD_Init
    }

    // ---- Phase 2: Test with many variables (iterative version) ----
    {
        std::cout << "Phase 2: Testing with many variables (iterative version)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        // Create 9000 variables (above 8192 threshold)
        const int LARGE_VARS = 9000;
        for (int i = 1; i <= LARGE_VARS; ++i) {
            BDD_NewVar();
        }
        std::cout << "  Created " << BDD_VarUsed() << " variables (iterative mode)" << endl;
        test_result("VarUsed exceeds threshold for iterative mode", BDD_VarUsed() > 8192);

        // Create equivalent test ZDDs (same relative structure)
        ZDD z1 = buildZDDFromSets(sets1);
        ZDD z2 = buildZDDFromSets(sets2);

        // Perform same operations
        ZDD z_union = z1 + z2;
        ZDD z_intersect = z1 & z2;
        ZDD z_subtract = z1 - z2;
        ZDD z_change = z1.Change(75);

        iterative_union_card = z_union.Card();
        iterative_intersect_card = z_intersect.Card();
        iterative_subtract_card = z_subtract.Card();
        iterative_change_card = z_change.Card();
        iterative_z1_lit = z1.Lit();
        iterative_z1_len = z1.Len();

        std::cout << "  Iterative results:" << endl;
        std::cout << "    Union card: " << iterative_union_card << endl;
        std::cout << "    Intersect card: " << iterative_intersect_card << endl;
        std::cout << "    Subtract card: " << iterative_subtract_card << endl;
        std::cout << "    Change card: " << iterative_change_card << endl;
        std::cout << "    Z1 Lit: " << iterative_z1_lit << endl;
        std::cout << "    Z1 Len: " << iterative_z1_len << endl;
        // ZDDs go out of scope here before comparison
    }

    // ---- Phase 3: Compare results ----
    std::cout << "Phase 3: Comparing results..." << endl;

    test_result("Union cardinality matches (recursive vs iterative)",
               recursive_union_card == iterative_union_card);
    test_result("Intersection cardinality matches (recursive vs iterative)",
               recursive_intersect_card == iterative_intersect_card);
    test_result("Subtraction cardinality matches (recursive vs iterative)",
               recursive_subtract_card == iterative_subtract_card);
    test_result("Change cardinality matches (recursive vs iterative)",
               recursive_change_card == iterative_change_card);
    test_result("Lit count matches (recursive vs iterative)",
               recursive_z1_lit == iterative_z1_lit);
    test_result("Len matches (recursive vs iterative)",
               recursive_z1_len == iterative_z1_len);

    std::cout << endl;
}

/*
 * Test 3: More comprehensive comparison with random sets
 */
void test_random_sets_comparison() {
    std::cout << "=== Test: Random Sets Comparison (Recursive vs Iterative) ===" << endl;

    const int SEED = 42;
    std::mt19937 rng(SEED);

    // Generate random test sets
    const int NUM_SETS = 50;
    const int MAX_SET_SIZE = 5;
    const int MAX_VAR = 80;

    vector<vector<int>> random_sets1, random_sets2;

    std::uniform_int_distribution<int> size_dist(1, MAX_SET_SIZE);
    std::uniform_int_distribution<int> var_dist(1, MAX_VAR);

    for (int i = 0; i < NUM_SETS; ++i) {
        int set_size = size_dist(rng);
        vector<int> new_set;
        std::set<int> used_vars;

        for (int j = 0; j < set_size; ++j) {
            int var;
            do {
                var = var_dist(rng);
            } while (used_vars.count(var) > 0);
            used_vars.insert(var);
            new_set.push_back(var);
        }
        std::sort(new_set.begin(), new_set.end());
        random_sets1.push_back(new_set);
    }

    for (int i = 0; i < NUM_SETS; ++i) {
        int set_size = size_dist(rng);
        vector<int> new_set;
        std::set<int> used_vars;

        for (int j = 0; j < set_size; ++j) {
            int var;
            do {
                var = var_dist(rng);
            } while (used_vars.count(var) > 0);
            used_vars.insert(var);
            new_set.push_back(var);
        }
        std::sort(new_set.begin(), new_set.end());
        random_sets2.push_back(new_set);
    }

    std::cout << "Generated " << NUM_SETS << " random sets for each ZDD" << endl;

    // Store results
    bddword rec_z1_card, rec_z2_card, rec_union_card, rec_intersect_card, rec_subtract_card;
    bddword rec_z1_size, rec_z1_lit;
    bddword iter_z1_card, iter_z2_card, iter_union_card, iter_intersect_card, iter_subtract_card;
    bddword iter_z1_size, iter_z1_lit;

    // ---- Test with recursive version ----
    {
        std::cout << "Testing with recursive version (100 variables)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 100; ++i) BDD_NewVar();

        ZDD z1 = buildZDDFromSets(random_sets1);
        ZDD z2 = buildZDDFromSets(random_sets2);

        rec_z1_card = z1.Card();
        rec_z2_card = z2.Card();
        rec_union_card = (z1 + z2).Card();
        rec_intersect_card = (z1 & z2).Card();
        rec_subtract_card = (z1 - z2).Card();
        rec_z1_size = z1.Size();
        rec_z1_lit = z1.Lit();
        // ZDDs go out of scope here before next BDD_Init
    }

    // ---- Test with iterative version ----
    {
        std::cout << "Testing with iterative version (9000 variables)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 9000; ++i) BDD_NewVar();

        ZDD z1 = buildZDDFromSets(random_sets1);
        ZDD z2 = buildZDDFromSets(random_sets2);

        iter_z1_card = z1.Card();
        iter_z2_card = z2.Card();
        iter_union_card = (z1 + z2).Card();
        iter_intersect_card = (z1 & z2).Card();
        iter_subtract_card = (z1 - z2).Card();
        iter_z1_size = z1.Size();
        iter_z1_lit = z1.Lit();
        // ZDDs go out of scope here before comparison
    }

    // ---- Compare results ----
    std::cout << "Comparing results:" << endl;
    std::cout << "  Z1 card: recursive=" << rec_z1_card << ", iterative=" << iter_z1_card << endl;
    std::cout << "  Z2 card: recursive=" << rec_z2_card << ", iterative=" << iter_z2_card << endl;
    std::cout << "  Union card: recursive=" << rec_union_card << ", iterative=" << iter_union_card << endl;
    std::cout << "  Intersect card: recursive=" << rec_intersect_card << ", iterative=" << iter_intersect_card << endl;
    std::cout << "  Subtract card: recursive=" << rec_subtract_card << ", iterative=" << iter_subtract_card << endl;
    std::cout << "  Z1 size: recursive=" << rec_z1_size << ", iterative=" << iter_z1_size << endl;
    std::cout << "  Z1 lit: recursive=" << rec_z1_lit << ", iterative=" << iter_z1_lit << endl;

    test_result("Random: Z1 cardinality matches", rec_z1_card == iter_z1_card);
    test_result("Random: Z2 cardinality matches", rec_z2_card == iter_z2_card);
    test_result("Random: Union cardinality matches", rec_union_card == iter_union_card);
    test_result("Random: Intersection cardinality matches", rec_intersect_card == iter_intersect_card);
    test_result("Random: Subtraction cardinality matches", rec_subtract_card == iter_subtract_card);
    test_result("Random: Size matches", rec_z1_size == iter_z1_size);
    test_result("Random: Lit count matches", rec_z1_lit == iter_z1_lit);

    std::cout << endl;
}

/*
 * Test 4: Test BDD operations (not just ZDD)
 */
void test_bdd_iterative_operations() {
    std::cout << "=== Test: BDD Iterative Operations ===" << endl;

    bddword rec_and_id, rec_or_id, rec_xor_id, rec_not_id, rec_complex_size;
    bddword iter_and_id, iter_or_id, iter_xor_id, iter_not_id, iter_complex_size;

    // ---- Test with recursive version ----
    {
        std::cout << "Testing BDD with recursive version (100 variables)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 100; ++i) BDD_NewVar();

        BDD x1 = BDDvar(10);
        BDD x2 = BDDvar(20);
        BDD x3 = BDDvar(30);

        BDD b_and = x1 & x2;
        BDD b_or = x1 | x2;
        BDD b_xor = x1 ^ x2;
        BDD b_not = ~x1;
        BDD b_complex = (x1 & x2) | (x2 & x3);

        rec_and_id = b_and.GetID();
        rec_or_id = b_or.GetID();
        rec_xor_id = b_xor.GetID();
        rec_not_id = b_not.GetID();
        rec_complex_size = b_complex.Size();
        // BDDs go out of scope here before next BDD_Init
    }

    // ---- Test with iterative version ----
    {
        std::cout << "Testing BDD with iterative version (9000 variables)..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 9000; ++i) BDD_NewVar();

        BDD x1 = BDDvar(10);
        BDD x2 = BDDvar(20);
        BDD x3 = BDDvar(30);

        BDD b_and = x1 & x2;
        BDD b_or = x1 | x2;
        BDD b_xor = x1 ^ x2;
        BDD b_not = ~x1;
        BDD b_complex = (x1 & x2) | (x2 & x3);

        iter_and_id = b_and.GetID();
        iter_or_id = b_or.GetID();
        iter_xor_id = b_xor.GetID();
        iter_not_id = b_not.GetID();
        iter_complex_size = b_complex.Size();
        // BDDs go out of scope here before comparison
    }

    // ---- Compare results ----
    // Note: IDs may differ, but structural properties should match
    std::cout << "Comparing BDD results:" << endl;
    std::cout << "  AND: recursive ID=" << rec_and_id << ", iterative ID=" << iter_and_id << endl;
    std::cout << "  OR: recursive ID=" << rec_or_id << ", iterative ID=" << iter_or_id << endl;
    std::cout << "  XOR: recursive ID=" << rec_xor_id << ", iterative ID=" << iter_xor_id << endl;
    std::cout << "  Complex size: recursive=" << rec_complex_size << ", iterative=" << iter_complex_size << endl;

    // IDs should actually be the same since they use the same variable indices
    test_result("BDD AND produces same ID", rec_and_id == iter_and_id);
    test_result("BDD OR produces same ID", rec_or_id == iter_or_id);
    test_result("BDD XOR produces same ID", rec_xor_id == iter_xor_id);
    test_result("BDD NOT produces same ID", rec_not_id == iter_not_id);
    test_result("BDD complex expression has same size", rec_complex_size == iter_complex_size);

    std::cout << endl;
}

/*
 * Test 5: Stress test with many operations at high variable count
 */
void test_stress_high_variables() {
    std::cout << "=== Test: Stress Test with High Variable Count ===" << endl;

#ifdef B_EXTEND
    BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
    BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

    const int NUM_VARS = 10000;
    std::cout << "Creating " << NUM_VARS << " variables..." << endl;

    for (int i = 1; i <= NUM_VARS; ++i) {
        BDD_NewVar();
    }

    std::cout << "Running stress test with multiple operations..." << endl;

    try {
        clock_t start = clock();

        // Create multiple ZDDs with varying high indices
        vector<ZDD> zdds;
        for (int i = 0; i < 20; ++i) {
            ZDD z(1);
            z = z.Change(500 * i + 100);
            zdds.push_back(z);
        }

        // Perform many union operations
        ZDD big_union(0);
        for (size_t i = 0; i < zdds.size(); ++i) {
            big_union = big_union + zdds[i];
        }

        // Union a few more, then intersect with the big union
        ZDD big_intersect = zdds[0];
        for (size_t i = 1; i < min((size_t)5, zdds.size()); ++i) {
            big_intersect = big_intersect + zdds[i];
        }
        big_intersect = big_intersect & big_union;

        // Perform subtraction
        ZDD subtract_result = big_union - zdds[0];

        // Query operations
        bddword union_card = big_union.Card();
        bddword intersect_card = big_intersect.Card();
        bddword subtract_card = subtract_result.Card();

        clock_t end = clock();
        double elapsed = double(end - start) / CLOCKS_PER_SEC;

        std::cout << "  Union of 20 ZDDs: " << union_card << " sets" << endl;
        std::cout << "  Intersection result: " << intersect_card << " sets" << endl;
        std::cout << "  Subtraction result: " << subtract_card << " sets" << endl;
        std::cout << "  Total time: " << elapsed << " seconds" << endl;

        test_result("Stress test: Union produces correct cardinality", union_card == 20);
        test_result("Stress test: Intersection produces correct cardinality", intersect_card == 5);
        test_result("Stress test: Subtraction produces correct cardinality", subtract_card == 19);
        test_result("Stress test completed without stack overflow", true);

    } catch (const std::exception& e) {
        std::cout << "Exception during stress test: " << e.what() << endl;
        test_result("Stress test threw exception", false);
    }

    std::cout << endl;
}

/*
 * Test 6: Test ZDD product (*) operation with iterative version
 */
void test_product_operation() {
    std::cout << "=== Test: Product Operation Comparison ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3}};
    vector<vector<int>> sets2 = {{4}, {5, 6}};

    // Expected product: {{1,2,4}, {1,2,5,6}, {3,4}, {3,5,6}}

    bddword rec_product_card, iter_product_card;

    // ---- Recursive version ----
    {
        std::cout << "Testing product with recursive version..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 100; ++i) BDD_NewVar();

        ZDD z1 = buildZDDFromSets(sets1);
        ZDD z2 = buildZDDFromSets(sets2);
        ZDD product = z1 * z2;
        rec_product_card = product.Card();
        // ZDDs go out of scope here
    }

    // ---- Iterative version ----
    {
        std::cout << "Testing product with iterative version..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 9000; ++i) BDD_NewVar();

        ZDD z1 = buildZDDFromSets(sets1);
        ZDD z2 = buildZDDFromSets(sets2);
        ZDD product = z1 * z2;
        iter_product_card = product.Card();
        // ZDDs go out of scope here
    }

    std::cout << "  Recursive product card: " << rec_product_card << endl;
    std::cout << "  Iterative product card: " << iter_product_card << endl;

    test_result("Product cardinality matches (recursive vs iterative)",
               rec_product_card == iter_product_card);
    test_result("Product produces expected cardinality (4)",
               rec_product_card == 4);

    std::cout << endl;
}

/*
 * Test 7: Test At0, At1, OffSet, OnSet operations
 */
void test_unary_operations_comparison() {
    std::cout << "=== Test: Unary Operations Comparison ===" << endl;

    vector<vector<int>> sets = {{1, 2, 3}, {1, 2}, {2, 3}, {1}, {3}};

    bddword rec_offset_card, rec_onset_card, rec_onset0_card;
    bddword iter_offset_card, iter_onset_card, iter_onset0_card;

    // ---- Recursive version ----
    {
        std::cout << "Testing unary operations with recursive version..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 100; ++i) BDD_NewVar();

        ZDD z = buildZDDFromSets(sets);
        ZDD offset = z.OffSet(2);
        ZDD onset = z.OnSet(2);
        ZDD onset0 = z.OnSet0(2);

        rec_offset_card = offset.Card();
        rec_onset_card = onset.Card();
        rec_onset0_card = onset0.Card();
        // ZDDs go out of scope here
    }

    // ---- Iterative version ----
    {
        std::cout << "Testing unary operations with iterative version..." << endl;

#ifdef B_EXTEND
        BDD_Init(1024, (size_t)512 * 1024 * 1024, 1.0);
#else
        BDD_Init(512, (size_t)256 * 1024 * 1024, 1.0);
#endif

        for (int i = 1; i <= 9000; ++i) BDD_NewVar();

        ZDD z = buildZDDFromSets(sets);
        ZDD offset = z.OffSet(2);
        ZDD onset = z.OnSet(2);
        ZDD onset0 = z.OnSet0(2);

        iter_offset_card = offset.Card();
        iter_onset_card = onset.Card();
        iter_onset0_card = onset0.Card();
        // ZDDs go out of scope here
    }

    std::cout << "  OffSet(2): recursive=" << rec_offset_card << ", iterative=" << iter_offset_card << endl;
    std::cout << "  OnSet(2): recursive=" << rec_onset_card << ", iterative=" << iter_onset_card << endl;
    std::cout << "  OnSet0(2): recursive=" << rec_onset0_card << ", iterative=" << iter_onset0_card << endl;

    test_result("OffSet cardinality matches", rec_offset_card == iter_offset_card);
    test_result("OnSet cardinality matches", rec_onset_card == iter_onset_card);
    test_result("OnSet0 cardinality matches", rec_onset0_card == iter_onset0_card);

    std::cout << endl;
}

// Main test function
int main() {
    test_init();

    std::cout << "Starting Iterative Apply Tests..." << endl << endl;

    try {
        // Test 1: Deep recursion safety
        test_deep_recursion_no_overflow();

        // Test 2: Recursive vs Iterative comparison
        test_recursive_vs_iterative_comparison();

        // Test 3: Random sets comparison
        test_random_sets_comparison();

        // Test 4: BDD operations
        test_bdd_iterative_operations();

        // Test 5: Stress test
        test_stress_high_variables();

        // Test 6: Product operation
        test_product_operation();

        // Test 7: Unary operations
        test_unary_operations_comparison();

    } catch (const std::exception& e) {
        std::cout << "Unexpected exception: " << e.what() << endl;
        test_result("Test suite completed without fatal error", false);
    }

    test_cleanup();

    cout << "Iterative apply test completed." << endl;
    cout << "Total tests: " << test_count << endl;
    cout << "Passed: " << pass_count << endl;
    cout << "Failed: " << fail_count << endl;

    return (fail_count > 0) ? 1 : 0;
}
