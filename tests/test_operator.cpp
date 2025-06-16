/*********************************************
 * ZDD Operator+ Performance Test Program    *
 * Tests operator+ with large ZDDs           *
 *********************************************/

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>
#include <algorithm>
#include <set>
#include <unordered_set>
#define BDD_CPP
#include "../include/bddc.h"
#include "../include/BDD.h"

// This macro is defined in bddc.h
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
#define ZDD_Meet ZBDD_Meet
#define ZDD_Random ZBDD_Random
#define ZDD_Export ZBDD_Export
#define ZDD_Import ZBDD_Import
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
ZDD buildZDDFromSets(const vector<vector<int> >& sets) {
    ZDD result(0); // Empty ZDD
    
    // For each set in the family
    for (size_t i = 0; i < sets.size(); ++i) {
        ZDD setZDD(1); // Unit ZDD
        
        // For each element in the set
        for (size_t j = 0; j < sets[i].size(); ++j) {
            int elem = sets[i][j];
            // Make sure the variable exists in the BDD system
            while (BDD_VarUsed() < elem) {
                BDD_NewVar();
            }
            
            // Add the element to the set ZDD
            setZDD = setZDD.Change(elem);
        }
        
        // Add the set to the result
        result += setZDD;
    }
    
    return result;
}

// Helper function to convert vector to string for hashing
std::string vectorToString(const vector<int>& vec) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += ",";
        result += std::to_string(vec[i]);
    }
    return result;
}

// Helper function to compute union of two set families (optimized with hash)
vector<vector<int> > computeSetUnion(const vector<vector<int> >& sets1, const vector<vector<int> >& sets2) {
    std::unordered_set<std::string> seen;
    vector<vector<int> > result;
    
    // Add all sets from sets1
    for (size_t i = 0; i < sets1.size(); ++i) {
        // Sort the set for consistent hashing
        vector<int> sorted_set = sets1[i];
        std::sort(sorted_set.begin(), sorted_set.end());
        
        std::string key = vectorToString(sorted_set);
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            result.push_back(sets1[i]);
        }
    }
    
    // Add sets from sets2 that are not already in result
    for (size_t i = 0; i < sets2.size(); ++i) {
        // Sort the set for consistent hashing
        vector<int> sorted_set = sets2[i];
        std::sort(sorted_set.begin(), sorted_set.end());
        
        std::string key = vectorToString(sorted_set);
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            result.push_back(sets2[i]);
        }
    }
    
    return result;
}

// Helper function to compute intersection of two set families (optimized with hash)
vector<vector<int> > computeSetIntersection(const vector<vector<int> >& sets1, const vector<vector<int> >& sets2) {
    std::unordered_set<std::string> set2_hashes;
    vector<vector<int> > result;
    
    // First, hash all sets from sets2
    for (size_t i = 0; i < sets2.size(); ++i) {
        vector<int> sorted_set = sets2[i];
        std::sort(sorted_set.begin(), sorted_set.end());
        std::string key = vectorToString(sorted_set);
        set2_hashes.insert(key);
    }
    
    // Then check which sets from sets1 are also in sets2
    for (size_t i = 0; i < sets1.size(); ++i) {
        vector<int> sorted_set = sets1[i];
        std::sort(sorted_set.begin(), sorted_set.end());
        std::string key = vectorToString(sorted_set);
        
        if (set2_hashes.find(key) != set2_hashes.end()) {
            result.push_back(sets1[i]);
        }
    }
    
    return result;
}

// Test initialization
void test_init(double cache_ratio) {
    std::cout << "=== ZDD Operator Validation Test ===" << endl;
    std::cout << "Start Time: " << __DATE__ << " " << __TIME__ << endl;
    std::cout << "=====================================" << endl << endl;
    
    // Initialize BDD package
    size_t memory_bytes = (size_t)(1024 * 1024 * 1024);
    BDD_Init(256, memory_bytes, cache_ratio);
}

// Test cleanup
void test_cleanup() {
    std::cout << endl << "=======================================" << endl;
    std::cout << "Total Tests: " << test_count << endl;
    std::cout << "Passed: " << pass_count << endl;
    std::cout << "Failed: " << fail_count << endl;
    std::cout << "=======================================" << endl;
}

// Create a large ZDD for testing and return both ZDD and sets
pair<ZDD, vector<vector<int> > > createLargeZDD1() {
    std::cout << "Creating large ZDD1..." << endl;
    
    // Create variables up to 100
    for (int i = 1; i <= 100; ++i) {
        BDD_NewVar();
    }
    
    vector<vector<int> > largeSets;
    
    // Add sets with different patterns
    for (int i = 1; i <= 50; ++i) {
        vector<int> set1;
        set1.push_back(i);
        set1.push_back(i + 50);
        largeSets.push_back(set1);
    }
    
    // Add some singleton sets
    for (int i = 1; i <= 20; ++i) {
        vector<int> set2;
        set2.push_back(i * 2);
        largeSets.push_back(set2);
    }
    
    // Add some larger sets
    for (int i = 1; i <= 10; ++i) {
        vector<int> set3;
        for (int j = 1; j <= 5; ++j) {
            set3.push_back(i * 10 + j);
        }
        largeSets.push_back(set3);
    }
    
    ZDD result = buildZDDFromSets(largeSets);
    std::cout << "ZDD1 created with " << result.Card() << " sets and " << result.Size() << " nodes" << endl;
    
    return make_pair(result, largeSets);
}

// Create another large ZDD for testing and return both ZDD and sets
pair<ZDD, vector<vector<int> > > createLargeZDD2() {
    std::cout << "Creating large ZDD2..." << endl;
    
    vector<vector<int> > largeSets;
    
    // Add sets with different patterns from ZDD1
    for (int i = 25; i <= 75; ++i) {
        vector<int> set1;
        set1.push_back(i);
        set1.push_back(i + 25);
        largeSets.push_back(set1);
    }
    
    // Add some triplet sets
    for (int i = 1; i <= 15; ++i) {
        vector<int> set2;
        set2.push_back(i * 3);
        set2.push_back(i * 3 + 1);
        set2.push_back(i * 3 + 2);
        largeSets.push_back(set2);
    }
    
    // Add some quadruplet sets
    for (int i = 1; i <= 8; ++i) {
        vector<int> set3;
        for (int j = 1; j <= 4; ++j) {
            set3.push_back(i * 12 + j);
        }
        largeSets.push_back(set3);
    }
    
    ZDD result = buildZDDFromSets(largeSets);
    std::cout << "ZDD2 created with " << result.Card() << " sets and " << result.Size() << " nodes" << endl;
    
    return make_pair(result, largeSets);
}

// Test simple cases with small sets
void test_simple_operator_validation() {
    std::cout << "=== Testing Simple Operator Cases ===" << endl;
    
    // Create simple test sets
    vector<vector<int> > sets1;
    sets1.push_back(vector<int>());
    sets1[0].push_back(1); sets1[0].push_back(2);
    sets1.push_back(vector<int>());
    sets1[1].push_back(3);
    sets1.push_back(vector<int>());
    sets1[2].push_back(1); sets1[2].push_back(3);
    
    vector<vector<int> > sets2;
    sets2.push_back(vector<int>());
    sets2[0].push_back(2); sets2[0].push_back(3);
    sets2.push_back(vector<int>());
    sets2[1].push_back(1); sets2[1].push_back(2);
    sets2.push_back(vector<int>());
    sets2[2].push_back(4);
    
    ZDD zdd1 = buildZDDFromSets(sets1);
    ZDD zdd2 = buildZDDFromSets(sets2);
    
    std::cout << "ZDD1 has " << zdd1.Card() << " sets" << endl;
    std::cout << "ZDD2 has " << zdd2.Card() << " sets" << endl;
    
    // Test operator+
    ZDD union_result = zdd1 + zdd2;
    vector<vector<int> > vector_union = computeSetUnion(sets1, sets2);
    ZDD expected_union = buildZDDFromSets(vector_union);
    
    std::cout << "Union: ZDD has " << union_result.Card() << " sets, vector has " << vector_union.size() << " sets" << endl;
    test_result("Simple operator+ matches vector union", union_result == expected_union);
    
    // Test operator&
    ZDD intersection_result = zdd1 & zdd2;
    vector<vector<int> > vector_intersection = computeSetIntersection(sets1, sets2);
    ZDD expected_intersection = buildZDDFromSets(vector_intersection);
    
    std::cout << "Intersection: ZDD has " << intersection_result.Card() << " sets, vector has " << vector_intersection.size() << " sets" << endl;
    test_result("Simple operator& matches vector intersection", intersection_result == expected_intersection);
    
    std::cout << endl;
}

// Generate random subsets of {1, 2, ..., n}
vector<vector<int> > generateRandomSubsets(int n, int count, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 1);
    vector<vector<int> > result;
    
    for (int i = 0; i < count; ++i) {
        vector<int> subset;
        // Each element has 50% chance of being included
        for (int j = 1; j <= n; ++j) {
            if (dist(rng) == 1) {
                subset.push_back(j);
            }
        }
        result.push_back(subset);
    }
    
    return result;
}

// Generate two sets of random subsets with some overlap
pair<vector<vector<int> >, vector<vector<int> > > generateRandomSubsetsWithOverlap(int n, int count, std::mt19937& rng) {
    std::uniform_int_distribution<int> element_dist(0, 1);
    std::uniform_int_distribution<int> overlap_dist(0, 2);  // 0, 1, 2 for roughly 1/3 probability
    
    vector<vector<int> > sets1, sets2;
    
    // Generate first set of random subsets
    for (int i = 0; i < count; ++i) {
        vector<int> subset;
        for (int j = 1; j <= n; ++j) {
            if (element_dist(rng) == 1) {
                subset.push_back(j);
            }
        }
        sets1.push_back(subset);
    }
    
    // Generate second set with some overlap
    int overlap_count = count / 3;  // About 1/3 overlap
    
    // First, copy some subsets from sets1 to create overlap
    for (int i = 0; i < overlap_count; ++i) {
        std::uniform_int_distribution<int> index_dist(0, sets1.size() - 1);
        int random_index = index_dist(rng);
        sets2.push_back(sets1[random_index]);
    }
    
    // Then generate remaining unique subsets for sets2
    for (int i = overlap_count; i < count; ++i) {
        vector<int> subset;
        for (int j = 1; j <= n; ++j) {
            if (element_dist(rng) == 1) {
                subset.push_back(j);
            }
        }
        sets2.push_back(subset);
    }
    
    // Shuffle sets2 to randomize the order
    std::shuffle(sets2.begin(), sets2.end(), rng);
    
    return make_pair(sets1, sets2);
}

// Test with random subsets
void test_random_subsets(int n, int count) {
    std::cout << "=== Testing with Random Subsets ===" << endl;
    
    const int seed = 1;
    
    // Initialize random number generator with fixed seed
    std::mt19937 rng(seed);
    
    // Generate two sets of random subsets with overlap
    pair<vector<vector<int> >, vector<vector<int> > > sets_pair = generateRandomSubsetsWithOverlap(n, count, rng);
    vector<vector<int> > sets1 = sets_pair.first;
    vector<vector<int> > sets2 = sets_pair.second;
    
    std::cout << "Generated " << sets1.size() << " random subsets for set1 (n=" << n << ")" << endl;
    std::cout << "Generated " << sets2.size() << " random subsets for set2 (n=" << n << ")" << endl;
    std::cout << "Expected overlap: approximately " << count / 3 << " common subsets" << endl;
    
    // Build ZDDs from the random sets
    clock_t build_start = clock();
    ZDD zdd1 = buildZDDFromSets(sets1);
    ZDD zdd2 = buildZDDFromSets(sets2);
    clock_t build_end = clock();
    
    std::cout << "ZDD1 has " << zdd1.Card() << " sets and " << zdd1.Size() << " nodes" << endl;
    std::cout << "ZDD2 has " << zdd2.Card() << " sets and " << zdd2.Size() << " nodes" << endl;
    std::cout << "ZDD construction took " << double(build_end - build_start) / CLOCKS_PER_SEC << " seconds" << endl;
    
    // Test operator+
    std::cout << "Testing operator+ with random subsets..." << endl;
    clock_t union_start = clock();
    ZDD zdd_union = zdd1 + zdd2;
    clock_t union_end = clock();

    std::cout << "operator+ took " << double(union_end - union_start) / CLOCKS_PER_SEC << " seconds" << endl;
    std::cout << "zdd_union has " << zdd_union.Card() << " sets and " << zdd_union.Size() << " nodes" << endl;

    clock_t build_expected_start = clock();
    vector<vector<int> > vector_union = computeSetUnion(sets1, sets2);
    ZDD expected_union = buildZDDFromSets(vector_union);
    clock_t build_expected_end = clock();

    std::cout << "buildZDDFromSets for expected_union took " << double(build_expected_end - build_expected_start) / CLOCKS_PER_SEC << " seconds" << endl;
    
    std::cout << "Union: ZDD has " << zdd_union.Card() << " sets, vector has " << vector_union.size() << " sets" << endl;
    test_result("Random subsets operator+ matches vector union", zdd_union == expected_union);
    test_result("Random subsets operator+ cardinality correct", zdd_union.Card() == vector_union.size());
    
    // Test operator&
    std::cout << "Testing operator& with random subsets..." << endl;
    clock_t intersection_start = clock();
    ZDD zdd_intersection = zdd1 & zdd2;
    clock_t intersection_end = clock();

    std::cout << "operator& took " << double(intersection_end - intersection_start) / CLOCKS_PER_SEC << " seconds" << endl;
    std::cout << "zdd_intersection has " << zdd_intersection.Card() << " sets and " << zdd_intersection.Size() << " nodes" << endl;

    clock_t build_expected_intersection_start = clock();
    vector<vector<int> > vector_intersection = computeSetIntersection(sets1, sets2);
    ZDD expected_intersection = buildZDDFromSets(vector_intersection);
    clock_t build_expected_intersection_end = clock();

    std::cout << "buildZDDFromSets for expected_intersection took " << double(build_expected_intersection_end - build_expected_intersection_start) / CLOCKS_PER_SEC << " seconds" << endl;
    
    std::cout << "Intersection: ZDD has " << zdd_intersection.Card() << " sets, vector has " << vector_intersection.size() << " sets" << endl;
    test_result("Random subsets operator& matches vector intersection", zdd_intersection == expected_intersection);
    test_result("Random subsets operator& cardinality correct", zdd_intersection.Card() == vector_intersection.size());
    
    // Additional validation
    test_result("Random subsets union contains all from set1", (zdd_union & zdd1) == zdd1);
    test_result("Random subsets union contains all from set2", (zdd_union & zdd2) == zdd2);
    test_result("Random subsets intersection subset of set1", zdd_intersection.Card() <= zdd1.Card());
    test_result("Random subsets intersection subset of set2", zdd_intersection.Card() <= zdd2.Card());
    
    std::cout << endl;
}

// Test operator+ with vector set comparison
void test_operator_plus_validation() {
    std::cout << "=== Testing operator+ with Vector Set Comparison ===" << endl;
    
    // Create two ZDDs and their corresponding vector sets
    pair<ZDD, vector<vector<int> > > zdd1_pair = createLargeZDD1();
    pair<ZDD, vector<vector<int> > > zdd2_pair = createLargeZDD2();
    
    ZDD zdd1 = zdd1_pair.first;
    ZDD zdd2 = zdd2_pair.first;
    vector<vector<int> > sets1 = zdd1_pair.second;
    vector<vector<int> > sets2 = zdd2_pair.second;
    
    std::cout << "Starting operator+ operation..." << endl;
    clock_t start_time = clock();
    
    // Perform ZDD union operation
    ZDD zdd_result = zdd1 + zdd2;
    
    // Perform vector set union operation
    vector<vector<int> > vector_result = computeSetUnion(sets1, sets2);
    
    clock_t end_time = clock();
    double elapsed_time = double(end_time - start_time) / CLOCKS_PER_SEC;
    
    std::cout << "Operator+ completed in " << elapsed_time << " seconds" << endl;
    std::cout << "ZDD result has " << zdd_result.Card() << " sets and " << zdd_result.Size() << " nodes" << endl;
    std::cout << "Vector result has " << vector_result.size() << " sets" << endl;
    
    // Build ZDD from vector result for comparison
    ZDD expected_zdd = buildZDDFromSets(vector_result);
    
    // Compare results
    test_result("operator+ matches vector set union", zdd_result == expected_zdd);
    test_result("operator+ cardinality matches vector union size", zdd_result.Card() == vector_result.size());
    
    // Additional validation tests
    test_result("operator+ result is not empty", zdd_result.Card() > 0);
    test_result("operator+ result has at least as many sets as zdd1", zdd_result.Card() >= zdd1.Card());
    test_result("operator+ result has at least as many sets as zdd2", zdd_result.Card() >= zdd2.Card());
    
    std::cout << endl;
}

// Test operator& (intersection) with vector set comparison
void test_operator_and_validation() {
    std::cout << "=== Testing operator& with Vector Set Comparison ===" << endl;
    
    // Create two ZDDs and their corresponding vector sets
    pair<ZDD, vector<vector<int> > > zdd1_pair = createLargeZDD1();
    pair<ZDD, vector<vector<int> > > zdd2_pair = createLargeZDD2();
    
    ZDD zdd1 = zdd1_pair.first;
    ZDD zdd2 = zdd2_pair.first;
    vector<vector<int> > sets1 = zdd1_pair.second;
    vector<vector<int> > sets2 = zdd2_pair.second;
    
    std::cout << "Starting operator& operation..." << endl;
    clock_t start_time = clock();
    
    // Perform ZDD intersection operation
    ZDD zdd_result = zdd1 & zdd2;
    
    // Perform vector set intersection operation
    vector<vector<int> > vector_result = computeSetIntersection(sets1, sets2);
    
    clock_t end_time = clock();
    double elapsed_time = double(end_time - start_time) / CLOCKS_PER_SEC;
    
    std::cout << "Operator& completed in " << elapsed_time << " seconds" << endl;
    std::cout << "ZDD result has " << zdd_result.Card() << " sets and " << zdd_result.Size() << " nodes" << endl;
    std::cout << "Vector result has " << vector_result.size() << " sets" << endl;
    
    // Build ZDD from vector result for comparison
    ZDD expected_zdd = buildZDDFromSets(vector_result);
    
    // Compare results
    test_result("operator& matches vector set intersection", zdd_result == expected_zdd);
    test_result("operator& cardinality matches vector intersection size", zdd_result.Card() == vector_result.size());
    
    // Additional validation tests
    test_result("operator& result has at most as many sets as zdd1", zdd_result.Card() <= zdd1.Card());
    test_result("operator& result has at most as many sets as zdd2", zdd_result.Card() <= zdd2.Card());
    
    std::cout << endl;
}

// Test operator+ with different sized ZDDs
void test_operator_plus_mixed_sizes() {
    std::cout << "=== Testing operator+ with Mixed Size ZDDs ===" << endl;
    
    // Small ZDD
    vector<vector<int> > smallSets;
    smallSets.push_back(vector<int>()); // empty set first
    smallSets[0].push_back(1); smallSets[0].push_back(2);
    smallSets.push_back(vector<int>());
    smallSets[1].push_back(3); smallSets[1].push_back(4);
    smallSets.push_back(vector<int>());
    smallSets[2].push_back(5);
    
    ZDD smallZDD = buildZDDFromSets(smallSets);
    
    // Medium ZDD
    pair<ZDD, vector<vector<int> > > large_pair = createLargeZDD1();
    ZDD largeZDD = large_pair.first;
    
    std::cout << "Testing small + large..." << endl;
    ZDD result1 = smallZDD + largeZDD;
    test_result("small + large produces valid result", result1.Card() >= largeZDD.Card());
    
    std::cout << "Testing large + small..." << endl;
    ZDD result2 = largeZDD + smallZDD;
    test_result("large + small produces valid result", result2.Card() >= largeZDD.Card());
    
    // Results should be the same (commutativity)
    test_result("operator+ is commutative", result1 == result2);
    
    std::cout << endl;
}

// Test operator+ edge cases
void test_operator_plus_edge_cases() {
    std::cout << "=== Testing operator+ Edge Cases ===" << endl;
    
    pair<ZDD, vector<vector<int> > > large_pair = createLargeZDD1();
    ZDD largeZDD = large_pair.first;
    ZDD empty(0);
    ZDD unit(1);
    
    // Test with empty set
    ZDD result1 = largeZDD + empty;
    test_result("large + empty = large", result1 == largeZDD);
    
    ZDD result2 = empty + largeZDD;
    test_result("empty + large = large", result2 == largeZDD);
    
    // Test with unit set
    ZDD result3 = largeZDD + unit;
    test_result("large + unit has more sets", result3.Card() == largeZDD.Card() + 1);
    
    // Test self-union
    ZDD result4 = largeZDD + largeZDD;
    test_result("large + large = large (idempotency)", result4 == largeZDD);
    
    std::cout << endl;
}

// Test operator+ performance with multiple operations
void test_operator_plus_multiple() {
    std::cout << "=== Testing Multiple operator+ Operations ===" << endl;
    
    vector<ZDD> zdds;
    
    // Create several medium-sized ZDDs
    for (int i = 0; i < 5; ++i) {
        vector<vector<int> > sets;
        for (int j = 1; j <= 20; ++j) {
            vector<int> set;
            set.push_back(i * 20 + j);
            set.push_back(i * 20 + j + 100);
            sets.push_back(set);
        }
        zdds.push_back(buildZDDFromSets(sets));
    }
    
    std::cout << "Performing sequential operator+ operations..." << endl;
    clock_t start_time = clock();
    
    ZDD result = zdds[0];
    for (size_t i = 1; i < zdds.size(); ++i) {
        result = result + zdds[i];
        std::cout << "After adding ZDD " << i << ": " << result.Card() << " sets, " << result.Size() << " nodes" << endl;
    }
    
    clock_t end_time = clock();
    double elapsed_time = double(end_time - start_time) / CLOCKS_PER_SEC;
    
    std::cout << "Multiple operator+ completed in " << elapsed_time << " seconds" << endl;
    std::cout << "Final result has " << result.Card() << " sets and " << result.Size() << " nodes" << endl;
    
    test_result("Multiple operator+ produces valid result", result.Card() > 0);
    
    std::cout << endl;
}

// Main test function
int main(int argc, char* argv[]) {
    // Default values
    int n = 30;
    int count = 1000;
    double cache_ratio = 1.0;
    
    // Parse command line arguments
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n <= 0) {
            std::cout << "Error: n must be a positive integer" << endl;
            std::cout << "Usage: " << argv[0] << " [n] [count] [cache_ratio]" << endl;
            std::cout << "  n: size of the universal set {1, 2, ..., n} (default: 30)" << endl;
            std::cout << "  count: number of random subsets to generate (default: 1000)" << endl;
            std::cout << "  cache ratio: cache ratio for BDD package (default: 1.0)" << endl;
            return 1;
        }
    }
    
    if (argc >= 3) {
        count = atoi(argv[2]);
        if (count <= 0) {
            std::cout << "Error: count must be a positive integer" << endl;
            std::cout << "Usage: " << argv[0] << " [n] [count] [cache_ratio]" << endl;
            std::cout << "  n: size of the universal set {1, 2, ..., n} (default: 30)" << endl;
            std::cout << "  count: number of random subsets to generate (default: 1000)" << endl;
            std::cout << "  cache ratio: cache ratio for BDD package (default: 1.0)" << endl;
            return 1;
        }
    }
    
    if (argc >= 4) {
        cache_ratio = atof(argv[3]);
        if (cache_ratio <= 0.0) {
            std::cout << "Error: cache_ratio must be a positive number" << endl;
            std::cout << "Usage: " << argv[0] << " [n] [count] [cache_ratio]" << endl;
            std::cout << "  n: size of the universal set {1, 2, ..., n} (default: 30)" << endl;
            std::cout << "  count: number of random subsets to generate (default: 1000)" << endl;
            std::cout << "  cache ratio: cache ratio for BDD package (default: 1.0)" << endl;
            return 1;
        }
    }
    
    if (argc > 4) {
        std::cout << "Warning: Extra arguments ignored" << endl;
    }
    
    std::cout << "Parameters: n=" << n << ", count=" << count << ", cache_ratio=" << cache_ratio << endl;
    
    test_init(cache_ratio);
    
    std::cout << "Starting ZDD Operator Validation Tests..." << endl << endl;
    
    try {
        test_simple_operator_validation();
        test_random_subsets(n, count);
        test_operator_plus_validation();
        test_operator_and_validation();
        test_operator_plus_mixed_sizes();
        test_operator_plus_edge_cases();
        test_operator_plus_multiple();
    } catch (const std::exception& e) {
        std::cout << "Exception occurred: " << e.what() << endl;
        test_result("Test completed without fatal errors", false);
    }
    
    test_cleanup();
    
    cout << "ZDD operator validation test completed." << endl;
    cout << "Total tests: " << test_count << endl;
    cout << "Passed: " << pass_count << endl;
    cout << "Failed: " << fail_count << endl;
    
    return (fail_count > 0) ? 1 : 0;
}
