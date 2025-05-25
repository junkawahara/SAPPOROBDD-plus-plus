/*********************************************
 * ZDD Class Comprehensive Test Program      *
 * Tests all member functions of ZDD class   *
 *********************************************/

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <set>
#include <algorithm>
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

// Test initialization
void test_init() {
    std::cout << "=== ZDD Class Comprehensive Test Results ===" << endl;
    std::cout << "Start Time: " << __DATE__ << " " << __TIME__ << endl;
    std::cout << "============================================" << endl << endl;
    
    // Initialize BDD package
    BDD_Init(256, 1024 * 1024);

    for (int i = 0; i < 100; i++) {
        BDD_NewVar(); // Pre-create some variables
    }
}

// Test cleanup
void test_cleanup() {
    std::cout << endl << "============================================" << endl;
    std::cout << "Total Tests: " << test_count << endl;
    std::cout << "Passed: " << pass_count << endl;
    std::cout << "Failed: " << fail_count << endl;
    std::cout << "============================================" << endl;
}

// Test constructors
void test_constructors() {
    std::cout << "=== Testing Constructors ===" << endl;
    
    // Default constructor
    ZDD z1;
    test_result("Default constructor creates empty set", z1 == ZDD(0));
    
    // Integer constructor
    ZDD z2(0);
    test_result("ZDD(0) creates empty set", z2 == ZDD());
    
    ZDD z3(1);
    test_result("ZDD(1) creates unit set", z3.Card() == 1);
    test_result("The root of ZDD(1) is variable 0", z3.Top() == 0);
    test_result("ZDD(1) is equal to ZDD(1)", z3 == ZDD(1));
    test_result("ZDD(1) is not equal to ZDD(0)", z3 != z2);
    
    ZDD z4(-1);
    test_result("ZDD(-1) creates null", z4 == ZDD(-1));
    test_result("ZDD(-1) is not equal to ZDD(0)", z4 != z2);
    
    // Copy constructor
    ZDD z5(z3);
    test_result("Copy constructor", z5 == z3);
    ZDD z6(1);
    z6 = z6.Change(1);
    z6 = z6.Change(2);
    z6 = z6.Change(3);
    ZDD z7(z6);
    test_result("Copy constructor", z7 == z6);
    
    std::cout << endl;
}

// Test assignment operators
void test_assignment_operators() {
    std::cout << "=== Testing Assignment Operators ===" << endl;
    
    // Define various sets for testing
    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    vector<vector<int>> sets2 = {{2, 3}, {4}, {3, 4}};
    
    // operator=
    ZDD z1 = buildZDDFromSets(sets1);
    ZDD z2 = buildZDDFromSets(sets2);
    ZDD origZ2 = z2;  // Save original value for verification
    z2 = z1;
    test_result("operator= assignment", z1 == z2);
    test_result("operator= changes target", z2 != origZ2);
    test_result("operator= changes target", z2 == buildZDDFromSets(sets1));
    test_result("operator= preserves source", z1 == buildZDDFromSets(sets1));
    
    
    // operator&=
    z1 = buildZDDFromSets(sets1);
    z2 = buildZDDFromSets(sets2);
    ZDD expectedAnd = buildZDDFromSets({{3, 4}});  // Intersection of sets1 and sets2
    
    ZDD origZ1 = z1;
    z1 &= z2;
    test_result("operator&= (intersection) result", z1 == expectedAnd);
    test_result("operator&= changes target", z1 != origZ1);
    test_result("operator&= preserves source", z2 == buildZDDFromSets(sets2));
    
    // operator+=
    z1 = buildZDDFromSets(sets1);
    z2 = buildZDDFromSets(sets2);
    ZDD expectedUnion = buildZDDFromSets({{1, 2}, {3, 4}, {1}, {1, 4, 5}, {2, 3}, {4}});  // Union of sets1 and sets2
    
    origZ1 = z1;
    z1 += z2;
    test_result("operator+= (union) result", z1 == expectedUnion);
    test_result("operator+= changes target", z1 != origZ1);
    test_result("operator+= preserves source", z2 == buildZDDFromSets(sets2));
    
    // operator-=
    z1 = buildZDDFromSets(sets1);
    z2 = buildZDDFromSets(sets2);
    ZDD expectedDiff = buildZDDFromSets({{1, 2}, {1}, {1, 4, 5}});  // Difference of sets1 and sets2
    
    origZ1 = z1;
    z1 -= z2;
    test_result("operator-= (difference) result", z1 == expectedDiff);
    test_result("operator-= changes target", z1 != origZ1);
    test_result("operator-= preserves source", z2 == buildZDDFromSets(sets2));
    
    // operator<<=
    z1 = buildZDDFromSets(sets1);
    origZ1 = z1;
    z1 <<= 1;
    
    // Left shift should increment all variable numbers by 1
    vector<vector<int>> shiftedSets = {{2, 3}, {4, 5}, {2}, {2, 5, 6}};
    ZDD expectedShift = buildZDDFromSets(shiftedSets);
    
    test_result("operator<<= (left shift) result", z1 == expectedShift);
    test_result("operator<<= changes target", z1 != origZ1);
    
    // operator>>=
    z1 = buildZDDFromSets(sets2);
    origZ1 = z1;
    z1 >>= 1;
    
    // Right shift should decrement all variable numbers by 1
    vector<vector<int>> rightShiftedSets = {{1, 2}, {3}, {2, 3}};
    ZDD expectedRShift = buildZDDFromSets(rightShiftedSets);
    
    test_result("operator>>= (right shift) result", z1 == expectedRShift);
    test_result("operator>>= changes target", z1 != origZ1);
    
    // Additional compound operators
    
    // operator*=
    z1 = buildZDDFromSets(sets1);
    z2 = buildZDDFromSets(sets2);
    // Product operation creates all combinations of sets from z1 and z2
    vector<vector<int>> productSets;
    for (auto& set1 : sets1) {
        for (auto& set2 : sets2) {
            // Use set to eliminate duplicates
            std::set<int> combinedSet;
            // Insert all elements from set1 and set2
            combinedSet.insert(set1.begin(), set1.end());
            combinedSet.insert(set2.begin(), set2.end());
            
            // Convert back to vector for buildZDDFromSets
            vector<int> combined(combinedSet.begin(), combinedSet.end());
            productSets.push_back(combined);
        }
    }
    ZDD expectedProduct = buildZDDFromSets(productSets);  // Product creates combined sets
    
    origZ1 = z1;
    z1 *= z2;
    test_result("operator*= (product) result", z1 == expectedProduct);
    test_result("operator*= changes target", z1 != origZ1);
    test_result("operator*= preserves source", z2 == buildZDDFromSets(sets2));
    
    // operator/=
    vector<vector<int>> setsDiv1 = {{1, 2, 3}, {1, 2, 3, 4}, {1, 5}, {1, 4, 5}, {2, 3, 4}, {2}};
    vector<vector<int>> setsDiv2 = {{2, 3}, {5}};
    z1 = buildZDDFromSets(setsDiv1);
    z2 = buildZDDFromSets(setsDiv2);
    ZDD expectedDiv = buildZDDFromSets({{1}, {1, 4}});
    
    origZ1 = z1;
    z1 /= z2;
    test_result("operator/= (division) result", z1 == expectedDiv);
    test_result("operator/= changes target", z1 != origZ1);
    test_result("operator/= preserves source", z2 == buildZDDFromSets(setsDiv2));
    
    // operator%=
    z1 = buildZDDFromSets(setsDiv1);
    z2 = buildZDDFromSets(setsDiv2);
    ZDD expectedRem = buildZDDFromSets({{2, 3, 4}, {2}});
    
    origZ1 = z1;
    z1 %= z2;
    test_result("operator%= (modulo) result", z1 == expectedRem);
    test_result("operator%= changes target", z1 != origZ1);
    test_result("operator%= preserves source", z2 == buildZDDFromSets(setsDiv2));
    
    std::cout << endl;
}

// Test binary operators
void test_binary_operators() {
    std::cout << "=== Testing Binary Operators ===" << endl;
    
    // Define various sets for testing
    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    vector<vector<int>> sets2 = {{2, 3}, {4}, {3, 4}};
    vector<vector<int>> setsDiv1 = {{1, 2, 3}, {1, 2, 3, 4}, {1, 5}, {1, 4, 5}, {2, 3, 4}, {2}};
    vector<vector<int>> setsDiv2 = {{2, 3}, {5}};
    
    // Empty and unit sets for special cases
    ZDD empty(0);
    ZDD unit(1);
    
    // operator&
    ZDD z1 = buildZDDFromSets(sets1);
    ZDD z2 = buildZDDFromSets(sets2);
    ZDD andResult = z1 & z2;
    ZDD expectedAnd = buildZDDFromSets({{3, 4}});
    test_result("operator& (intersection) result", andResult == expectedAnd);
    test_result("operator& preserves left operand", z1 == buildZDDFromSets(sets1));
    test_result("operator& preserves right operand", z2 == buildZDDFromSets(sets2));
    
    // Special cases for operator&
    test_result("operator& with empty set (left)", (empty & z1) == ZDD(0));
    test_result("operator& with empty set (right)", (z1 & empty) == ZDD(0));
    test_result("operator& with unit set (left)", (unit & z1) == ZDD(0));
    test_result("operator& with unit set (right)", (z1 & unit) == ZDD(0));
    test_result("operator& with both empty sets", (empty & empty) == ZDD(0));
    test_result("operator& with both unit sets", (unit & unit) == unit);
    
    // operator+
    ZDD unionResult = z1 + z2;
    ZDD expectedUnion = buildZDDFromSets({{1, 2}, {3, 4}, {1}, {1, 4, 5}, {2, 3}, {4}});
    test_result("operator+ (union) result", unionResult == expectedUnion);
    test_result("operator+ preserves left operand", z1 == buildZDDFromSets(sets1));
    test_result("operator+ preserves right operand", z2 == buildZDDFromSets(sets2));
    
    // Special cases for operator+
    test_result("operator+ with empty set (left)", (empty + z1) == z1);
    test_result("operator+ with empty set (right)", (z1 + empty) == z1);
    test_result("operator+ with unit set (left)", (unit + z1).Card() == 5);
    test_result("operator+ with unit set (right)", (z1 + unit).Card() == 5);
    test_result("operator+ with both empty sets", (empty + empty) == empty);
    test_result("operator+ with both unit sets", (unit + unit) == unit);
    
    // operator-
    ZDD diffResult = z1 - z2;
    ZDD expectedDiff = buildZDDFromSets({{1, 2}, {1}, {1, 4, 5}});
    test_result("operator- (difference) result", diffResult == expectedDiff);
    test_result("operator- preserves left operand", z1 == buildZDDFromSets(sets1));
    test_result("operator- preserves right operand", z2 == buildZDDFromSets(sets2));
    
    // Special cases for operator-
    test_result("operator- with empty set (left)", (empty - z1) == empty);
    test_result("operator- with empty set (right)", (z1 - empty) == z1);
    test_result("operator- with unit set (left)", (unit - z1) == unit);
    test_result("operator- with unit set (right)", (z1 - unit) == z1);
    test_result("operator- with both empty sets", (empty - empty) == empty);
    test_result("operator- with both unit sets", (unit - unit) == empty);
    
    // operator*
    ZDD prodResult = z1 * z2;
    // Product operation creates all combinations of sets from z1 and z2
    vector<vector<int>> productSets;
    for (auto& set1 : sets1) {
        for (auto& set2 : sets2) {
            // Use set to eliminate duplicates
            std::set<int> combinedSet;
            // Insert all elements from set1 and set2
            combinedSet.insert(set1.begin(), set1.end());
            combinedSet.insert(set2.begin(), set2.end());
            
            // Convert back to vector for buildZDDFromSets
            vector<int> combined(combinedSet.begin(), combinedSet.end());
            productSets.push_back(combined);
        }
    }
    ZDD expectedProd = buildZDDFromSets(productSets);
    
    test_result("operator* (product) result", prodResult == expectedProd);
    test_result("operator* preserves left operand", z1 == buildZDDFromSets(sets1));
    test_result("operator* preserves right operand", z2 == buildZDDFromSets(sets2));
    
    // Special cases for operator*
    test_result("operator* with empty set (left)", (empty * z1) == empty);
    test_result("operator* with empty set (right)", (z1 * empty) == empty);
    test_result("operator* with unit set (left)", (unit * z1) == z1);
    test_result("operator* with unit set (right)", (z1 * unit) == z1);
    test_result("operator* with both empty sets", (empty * empty) == empty);
    test_result("operator* with both unit sets", (unit * unit) == unit);
    
    // operator/
    ZDD z3 = buildZDDFromSets(setsDiv1);
    ZDD z4 = buildZDDFromSets(setsDiv2);
    ZDD divResult = z3 / z4;
    ZDD expectedDiv = buildZDDFromSets({{1}, {1, 4}});
    test_result("operator/ (quotient) result", divResult == expectedDiv);
    test_result("operator/ preserves left operand", z3 == buildZDDFromSets(setsDiv1));
    test_result("operator/ preserves right operand", z4 == buildZDDFromSets(setsDiv2));
    
    // Special cases for operator/
    test_result("operator/ with empty set (left)", (empty / z1) == empty);
    test_result("operator/ with unit set (left)", (unit / z1) == empty);
    test_result("operator/ with unit set (right)", (z1 / unit) == z1);
    test_result("operator/ with both unit sets", (unit / unit) == unit);
    
    // operator%
    ZDD modResult = z3 % z4;
    ZDD expectedRem = buildZDDFromSets({{2, 3, 4}, {2}});
    test_result("operator% (remainder) result", modResult == expectedRem);
    test_result("operator% preserves left operand", z3 == buildZDDFromSets(setsDiv1));
    test_result("operator% preserves right operand", z4 == buildZDDFromSets(setsDiv2));
    
    // Special cases for operator%
    test_result("operator% with empty set (left)", (empty % z1) == empty);
    test_result("operator% with unit set (left)", (unit % z1) == unit);
    test_result("operator% with unit set (right)", (z1 % unit) == empty);
    test_result("operator% with both unit sets", (unit % unit) == empty);
    
    // operator<<
    ZDD leftShift = z1 << 1;
    vector<vector<int>> shiftedSets = {{2, 3}, {4, 5}, {2}, {2, 5, 6}};
    ZDD expectedLeftShift = buildZDDFromSets(shiftedSets);
    test_result("operator<< (left shift) result", leftShift == expectedLeftShift);
    test_result("operator<< preserves operand", z1 == buildZDDFromSets(sets1));
    
    // Special cases for operator<<
    test_result("operator<< with empty set", (empty << 1) == empty);
    test_result("operator<< with unit set", (unit << 1) == unit);
    
    // operator>>
    ZDD rightShift = z2 >> 1;
    vector<vector<int>> rightShiftedSets = {{1, 2}, {3}, {2, 3}};
    ZDD expectedRightShift = buildZDDFromSets(rightShiftedSets);
    test_result("operator>> (right shift) result", rightShift == expectedRightShift);
    test_result("operator>> preserves operand", z2 == buildZDDFromSets(sets2));
    
    // Special cases for operator>>
    test_result("operator>> with empty set", (empty >> 1) == empty);
    test_result("operator>> with unit set", (unit >> 1) == unit);
    
    // operator==
    ZDD a = buildZDDFromSets(sets1);
    ZDD b = buildZDDFromSets(sets1);
    ZDD c = buildZDDFromSets(sets2);
    test_result("operator== (equality for same sets)", a == b);
    test_result("operator== (inequality for different sets)", !(a == c));
    
    // Special cases for operator==
    test_result("operator== with empty sets", empty == empty);
    test_result("operator== with unit sets", unit == unit);
    test_result("operator== empty set != unit set", !(empty == unit));
    
    // operator!=
    test_result("operator!= (inequality for different sets)", a != c);
    test_result("operator!= (equality for same sets)", !(a != b));
    
    // Special cases for operator!=
    test_result("operator!= with empty sets", !(empty != empty));
    test_result("operator!= with unit sets", !(unit != unit));
    test_result("operator!= empty set != unit set", empty != unit);

    // ZDD_Meet
    vector<vector<int>> meetSets;
    for (auto& set1 : sets1) {
        for (auto& set2 : sets2) {
            std::set<int> commonSet;
            for (size_t i = 0; i < set1.size(); ++i) {
                auto it = std::find(set2.begin(), set2.end(), set1[i]);
                if (it != set2.end()) {
                    commonSet.insert(set1[i]);
                }
            }
            // Convert back to vector for buildZDDFromSets
            vector<int> common(commonSet.begin(), commonSet.end());
            meetSets.push_back(common);
        }
    }
    ZDD meetResult = ZDD_Meet(z1, z2);
    ZDD expectedMeet = buildZDDFromSets(meetSets);
    test_result("ZDD_Meet() meet operation result", meetResult == expectedMeet); 
    test_result("ZDD_Meet() with empty set", ZDD_Meet(empty, z1) == empty);
    test_result("ZDD_Meet() with empty set", ZDD_Meet(z1, empty) == empty);
    test_result("ZDD_Meet() with unit set", ZDD_Meet(unit, z1) == unit);
    test_result("ZDD_Meet() with unit set", ZDD_Meet(z1, unit) == unit);

    std::cout << endl;
}

// Test basic query functions
void test_query_functions() {
    std::cout << "=== Testing Query Functions ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    ZDD zdd1 = buildZDDFromSets(sets1);
    ZDD singletonZDD = buildZDDFromSets({{1}}); // A ZDD with a single set {1}
    
    // Empty and unit sets for special cases
    ZDD null(-1);
    ZDD empty(0);
    ZDD unit(1);
    
    // Top() - returns highest variable
    test_result("Top() for zdd1 returns correct variable", zdd1.Top() == 5);
    // 
    test_result("Top() for zdd1.OnSet0(5) returns correct variable", zdd1.OnSet0(5).Top() == 4);
    test_result("Top() for empty set returns 0", empty.Top() == 0);
    test_result("Top() for unit set returns 0", unit.Top() == 0);
    test_result("Top() for null set returns 0", null.Top() == 0);
    
    // GetID()
    test_result("GetID() for zdd1 returns the same ID", zdd1.GetID() == buildZDDFromSets(sets1).GetID());
    test_result("GetID() for zdd1 returns a different ID from a different ZDD", zdd1.GetID() != buildZDDFromSets({{1}}).GetID());
    test_result("GetID() for zdd1 + ZDD(1)", zdd1.GetID() + 1 == (zdd1 + ZDD(1)).GetID());
    test_result("GetID() for empty set returns valid ID", empty.GetID() == bddempty);
    test_result("GetID() for unit set returns valid ID", unit.GetID() == bddsingle);
    test_result("GetID() for null set returns valid ID", null.GetID() == bddnull);

    // ZDD_ID
    bddword id = zdd1.GetID();
    ZDD zddIdResult = ZDD_ID(bddcopy(id));
    test_result("ZDD_ID() creates from ID", zddIdResult == zdd1);
    id = singletonZDD.GetID();
    zddIdResult = ZDD_ID(bddcopy(id));
    test_result("ZDD_ID() creates from ID with different sets", zddIdResult != zdd1);
    id = empty.GetID();
    zddIdResult = ZDD_ID(bddcopy(id));
    test_result("ZDD_ID() creates from empty set ID", zddIdResult == empty);
    id = unit.GetID();
    zddIdResult = ZDD_ID(bddcopy(id));
    test_result("ZDD_ID() creates from unit set ID", zddIdResult == unit);
    id = null.GetID();
    zddIdResult = ZDD_ID(bddcopy(id));
    test_result("ZDD_ID() creates from null set ID", zddIdResult == null);
    
    // Size()
    test_result("Size() for a singleton returns 1", buildZDDFromSets({{1}}).Size() == 1);
    test_result("Size() for a zdd with two nodes returns 2", buildZDDFromSets({{1}, {2}}).Size() == 2);
    // If the negative edge representation were used, it would return 3.
    test_result("Size() for a zdd with negative edges", buildZDDFromSets({{}, {1}, {1, 2}}).Size() == 2);
    test_result("Size() for zdd1 returns node count", zdd1.Size() == 6);
    test_result("Size() for empty set returns 0", empty.Size() == 0);
    test_result("Size() for unit set returns 0", unit.Size() == 0);
    test_result("Size() for null set returns 0", null.Size() == 0);
    
    // Card() - returns element count (number of sets)
    test_result("Card() for zdd1 returns correct element count", zdd1.Card() == 4); // matches sets1 size
    test_result("Card() for empty set returns 0", empty.Card() == 0);
    test_result("Card() for unit set returns 1", unit.Card() == 1);
    test_result("Card() for null set returns 0", null.Card() == 0);

    // CardMP16
    char buffer[256];
    char* resultCardMP16 = zdd1.CardMP16(buffer);
    test_result("CardMP16() returns string of 4", std::string(resultCardMP16) == "4");
    test_result("CardMP16() for empty set returns string of 0", ZDD(0).CardMP16(buffer) == std::string("0"));
    test_result("CardMP16() for unit set returns string of 1", ZDD(1).CardMP16(buffer) == std::string("1"));
    test_result("CardMP16() for null set returns string of 0", ZDD(-1).CardMP16(buffer) == std::string("0"));
    // CardMP16 for a singleton set
    ZDD singletonSet = buildZDDFromSets({{1}});
    resultCardMP16 = singletonSet.CardMP16(buffer);
    test_result("CardMP16() for singleton set returns string of 1", std::string(resultCardMP16) == "1");

    ZDD powerSet(1);
    for (int i = 1; i <= 70; ++i) {
        powerSet += powerSet.Change(i);
    }
    resultCardMP16 = powerSet.CardMP16(buffer);
    test_result("CardMP16() for large power set returns correct string", std::string(resultCardMP16) == "400000000000000000");
    
    // Lit() - returns literal count (total number of items in all sets)
    test_result("Lit() for zdd1 returns correct literal count", zdd1.Lit() == 8);
    test_result("Lit() for empty set returns 0", empty.Lit() == 0);
    test_result("Lit() for unit set returns 0", unit.Lit() == 0);
    test_result("Lit() for null set returns 0", null.Lit() == 0);
    
    // Len()
    // the length of {1, 4, 5}
    test_result("Len() for zdd1 returns valid length", zdd1.Len() == 3);
    test_result("Len() for empty set returns 0", empty.Len() == 0);
    test_result("Len() for unit set returns 0", unit.Len() == 0);
    test_result("Len() for null set returns 0", null.Len() == 0);
    
    // IsPoly()
    test_result("IsPoly() for zdd1 with multiple items", zdd1.IsPoly());
    test_result("IsPoly() for ZDD with multiple items", buildZDDFromSets({{1}, {2}}).IsPoly());
    test_result("IsPoly() for ZDD with a single item", !buildZDDFromSets({{1}}).IsPoly());
    test_result("IsPoly() for ZDD with a single item", !buildZDDFromSets({{1, 2}}).IsPoly());
    test_result("IsPoly() for empty set returns 0", !empty.IsPoly());
    test_result("IsPoly() for unit set returns 0", !unit.IsPoly());
    
    // Support() - returns used variables
    test_result("Support() for zdd1", zdd1.Support() == buildZDDFromSets({{1}, {2}, {3}, {4}, {5}}));
    test_result("Support() for ZDD with single item", buildZDDFromSets({{1}}).Support() == buildZDDFromSets({{1}}));
    test_result("Support() for ZDD with multiple items", buildZDDFromSets({{1, 3}, {6}}).Support() == buildZDDFromSets({{1}, {3}, {6}}));
    test_result("Support() for empty set returns empty set", empty.Support() == empty);
    test_result("Support() for unit set returns empty set", unit.Support() == empty);

    std::cout << endl;
}

// Test set operations
void test_set_operations() {
    std::cout << "=== Testing Set Operations ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    ZDD z1 = buildZDDFromSets(sets1);
    
    // Empty and unit sets for special cases
    ZDD empty(0);
    ZDD unit(1);
    
    // OffSet() tests
    ZDD z2 = z1.OffSet(4);
    ZDD expectedOffSet = buildZDDFromSets({{1, 2}, {1}});
    test_result("OffSet() removes sets with variable", z2 == expectedOffSet);
    test_result("OffSet() on empty set returns empty", empty.OffSet(1) == empty);
    test_result("OffSet() on unit set returns unit", unit.OffSet(1) == unit);
    
    // OnSet() tests
    ZDD z3 = z1.OnSet(4);
    ZDD expectedOnSet = buildZDDFromSets({{3, 4}, {1, 4, 5}});
    test_result("OnSet() keeps sets with variable", z3 == expectedOnSet);
    test_result("OnSet() on empty set returns empty", empty.OnSet(1) == empty);
    test_result("OnSet() on unit set returns empty", unit.OnSet(1) == empty);
    
    // OnSet0() tests
    ZDD z4 = z1.OnSet0(4);
    ZDD expectedOnSet0 = buildZDDFromSets({{3}, {1, 5}});
    test_result("OnSet0() keeps sets without variable", z4 == expectedOnSet0);
    test_result("OnSet0() on empty set returns empty", empty.OnSet0(1) == empty);
    test_result("OnSet0() on unit set returns unit", unit.OnSet0(1) == empty);
    
    // Change() tests
    ZDD z5 = z1.Change(4);
    ZDD expectedChange = buildZDDFromSets({{1, 2, 4}, {3}, {1, 4}, {1, 5}});
    test_result("Change() changes variable in sets", z5 == expectedChange);
    test_result("Change() on empty set returns empty", empty.Change(1) == empty);
    test_result("Change() on unit set returns unit set with variable", unit.Change(1) == buildZDDFromSets({{1}}));
    
    std::cout << endl;
}

// Test advanced operations
void test_advanced_operations() {
    std::cout << "=== Testing Advanced Operations ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    ZDD zdd1 = buildZDDFromSets(sets1);
    
    // Empty and unit sets for special cases
    ZDD empty(0);
    ZDD unit(1);

    // Swap() tests
    ZDD swapResult = zdd1.Swap(1, 2);
    ZDD expectedSwap = buildZDDFromSets({{2, 1}, {3, 4}, {2}, {2, 4, 5}});
    test_result("Swap() with zdd1 exchanges variables 1 and 2", swapResult == expectedSwap);
    test_result("Swap() on empty set returns empty", empty.Swap(1, 2) == empty);
    test_result("Swap() on unit set returns unit", unit.Swap(1, 2) == unit);
    
    // Restrict() tests
    ZDD restrictFilter = buildZDDFromSets({{1}});
    ZDD restrictResult = zdd1.Restrict(restrictFilter);
    ZDD expectedRestrict = buildZDDFromSets({{1}, {1, 2}, {1, 4, 5}});
    test_result("Restrict() on zdd1 filters sets containing {1}", restrictResult == expectedRestrict);
    restrictFilter = buildZDDFromSets({{1, 4}, {3}});
    restrictResult = zdd1.Restrict(restrictFilter);
    expectedRestrict = buildZDDFromSets({{1, 4, 5}, {3, 4}});
    test_result("Restrict() on zdd1 filters sets containing {1, 4} or {3}", restrictResult == expectedRestrict);
    
    test_result("Restrict() on empty set returns empty", empty.Restrict(zdd1) == empty);
    test_result("Restrict() with unit set (filter)", zdd1.Restrict(unit) == zdd1);
    test_result("Restrict() on unit set itself", unit.Restrict(zdd1) == empty);
    
    // Permit() tests
    ZDD permitFilter = buildZDDFromSets({{1, 4, 5, 7}, {2, 3, 4}});
    ZDD permitResult = zdd1.Permit(permitFilter);
    ZDD expectedPermit = buildZDDFromSets({{3, 4}, {1}, {1, 4, 5}});
    test_result("Permit() on zdd1 filters sets containing {1, 4, 5, 7} or {2, 3, 4}", permitResult == expectedPermit);
    test_result("Permit() on empty set returns empty", empty.Permit(zdd1) == empty);
    test_result("Permit() with unit set (filter)", zdd1.Permit(unit) == empty);
    test_result("Permit() on unit set", unit.Permit(zdd1) == unit);
    
    // PermitSym() tests
    ZDD permitSymResult = zdd1.PermitSym(1);
    test_result("PermitSym(1) on zdd1 returns sets of size at most 1", permitSymResult == buildZDDFromSets({{1}}));
    permitSymResult = zdd1.PermitSym(2);
    test_result("PermitSym(2) on zdd1 returns sets of size at most 2", permitSymResult == buildZDDFromSets({{1, 2}, {3, 4}, {1}}));
    permitSymResult = zdd1.PermitSym(3);
    test_result("PermitSym(3) on zdd1 with no sets of size at most 3", permitSymResult == buildZDDFromSets(sets1));
    test_result("PermitSym(4) on zdd1 with no sets of size at most 4", zdd1.PermitSym(4) == zdd1.PermitSym(3));
    test_result("PermitSym() on empty set returns empty", empty.PermitSym(1) == empty);
    test_result("PermitSym() on unit set returns unit", unit.PermitSym(1) == unit);
    
    // Always() tests
    ZDD alwaysResult = zdd1.Always();
    test_result("Always() on zdd1 returns set of common variables", alwaysResult == empty);
    alwaysResult = buildZDDFromSets({{1, 2, 3}, {2, 3, 4}, {2, 3}, {2, 3, 5, 7}});
    test_result("Always() on a ZDD with common variables", alwaysResult.Always() == buildZDDFromSets({{2}, {3}}));
    test_result("Always() on empty set returns empty", empty.Always() == empty);
    test_result("Always() on unit set returns empty", unit.Always() == empty);
    
    // Divisor() tests
    ZDD divisorResult = zdd1.Divisor();
    test_result("Divisor() on zdd1 returns non-empty", divisorResult == buildZDDFromSets({{1, 5}, {3}}));
    test_result("Divisor() on empty set returns empty", empty.Divisor() == empty);
    test_result("Divisor() on unit set returns unit", unit.Divisor() == unit);

    zdd1.SetZSkip(); // for ZLev() and Intersec() tests
    
    // ZLev() tests
    ZDD zlevResult = zdd1.ZLev(1);
    test_result("ZLev(1) on zdd1", zlevResult == buildZDDFromSets({{1}}));
    zlevResult = zdd1.ZLev(2);
    test_result("ZLev(2) on zdd1", zlevResult == buildZDDFromSets({{1}, {1, 2}}));
    zlevResult = zdd1.ZLev(4);
    test_result("ZLev(4) on zdd1", zlevResult == buildZDDFromSets({{1}, {1, 2}, {3, 4}}));
    zlevResult = zdd1.ZLev(5);
    test_result("ZLev(5) on zdd1", zlevResult == buildZDDFromSets(sets1));
    zlevResult = zdd1.ZLev(3);
    test_result("ZLev(3) on zdd1", zlevResult == zdd1.ZLev(2));
    zlevResult = zdd1.ZLev(3, 1);
    test_result("ZLev(3, 1) on zdd1", zlevResult == zdd1.ZLev(4));
    test_result("ZLev() on empty set returns empty", empty.ZLev(2) == empty);
    test_result("ZLev() on unit set returns unit", unit.ZLev(2) == unit);
    
    // Intersec() tests
    ZDD intersecFilter = buildZDDFromSets({{1}, {3, 4}, {5}});
    ZDD intersecResult = zdd1.Intersec(intersecFilter);
    ZDD expectedIntersec = buildZDDFromSets({{1}, {3, 4}});
    test_result("Intersec() on zdd1 returns intersection of family sets", intersecResult == expectedIntersec);
    test_result("Intersec() on empty set returns empty", zdd1.Intersec(empty) == empty);
    test_result("Intersec() with unit set", zdd1.Intersec(unit) == empty);
    test_result("Intersec() on empty set returns empty", empty.Intersec(zdd1) == empty);
    test_result("Intersec() with unit set", unit.Intersec(zdd1) == empty);
    test_result("Intersec() with both unit sets", unit.Intersec(unit) == unit);
    
    std::cout << endl;
}

// Test symmetry operations
void test_symmetry_operations() {
    std::cout << "=== Testing Symmetry Operations ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    ZDD zdd1 = buildZDDFromSets(sets1);
    vector<vector<int>> sets2 = {{1, 2}, {1}, {2}};
    ZDD zdd2 = buildZDDFromSets(sets2);
    vector<vector<int>> sets3 = {{1, 2}, {1}, {2}, {3}, {4}, {5}, {4, 5}};
    ZDD zdd3 = buildZDDFromSets(sets3);
    vector<vector<int>> sets4 = {{1, 2, 3}, {1}, {2}, {3}, {4}, {5}, {4, 5}};
    ZDD zdd4 = buildZDDFromSets(sets4);
    
    // Empty and unit sets for special cases
    ZDD empty(0);
    ZDD unit(1);
    
    // SymChk() tests
    int symChk1 = zdd1.SymChk(1, 5);
    test_result("SymChk() on zdd1 for variables 1 and 5", symChk1 == 0);
    int symChk2 = zdd2.SymChk(1, 2);
    test_result("SymChk() on zdd2 for variables 1 and 2", symChk2 != 0);
    test_result("SymChk() on empty set returns 1", empty.SymChk(1, 2) == 1);
    test_result("SymChk() on unit set returns 1", unit.SymChk(1, 2) == 1);

    // SymGrp() tests
    ZDD symGrpResult = zdd1.SymGrp();
    test_result("SymGrp() on zdd1 returns symmetry groups", symGrpResult == empty);
    ZDD symGrp2Result = zdd2.SymGrp();
    test_result("SymGrp() on zdd2 returns symmetry groups", symGrp2Result == buildZDDFromSets({{1, 2}}));
    ZDD symGrp3Result = zdd3.SymGrp();
    test_result("SymGrp() on zdd3 returns symmetry groups", symGrp3Result == buildZDDFromSets({{1, 2}, {4, 5}}));
    test_result("SymGrp() on empty set returns empty set", empty.SymGrp() == empty);
    test_result("SymGrp() on unit set returns empty set", unit.SymGrp() == empty);

    // SymGrpNaive() tests
    ZDD symGrpNaiveResult = zdd1.SymGrpNaive();
    test_result("SymGrpNaive() on zdd1 returns symmetry groups", symGrpNaiveResult == buildZDDFromSets({{1}, {2}, {3}, {4}, {5}}));
    ZDD symGrpNaive2Result = zdd2.SymGrpNaive();
    test_result("SymGrpNaive() on zdd2 returns symmetry groups", symGrpNaive2Result == buildZDDFromSets({{1, 2}}));
    ZDD symGrpNaive3Result = zdd3.SymGrpNaive();
    test_result("SymGrpNaive() on zdd3 returns symmetry groups", symGrpNaive3Result == buildZDDFromSets({{1, 2}, {3}, {4, 5}}));
    test_result("SymGrpNaive() on empty set returns empty set", empty.SymGrpNaive() == empty);
    test_result("SymGrpNaive() on unit set returns empty set", unit.SymGrpNaive() == empty);

    // SymSet() tests
    ZDD symSetResult1 = zdd1.SymSet(1);
    test_result("SymSet() on zdd1 for variable 1 returns symset", symSetResult1 == empty);
    ZDD symSetResult2 = zdd2.SymSet(1);
    test_result("SymSet() on zdd2 for variable 1 returns symset", symSetResult2 == buildZDDFromSets({{2}}));
    ZDD symSetResult3 = zdd3.SymSet(1);
    test_result("SymSet() on zdd3 for variable 1 returns symset", symSetResult3 == buildZDDFromSets({{2}}));
    ZDD symSetResult4 = zdd4.SymSet(2);
    test_result("SymSet() on zdd4 for variable 1 returns symset", symSetResult4 == buildZDDFromSets({{1}, {3}}));
    test_result("SymSet() on empty set returns empty set", empty.SymSet(1) == empty);
    test_result("SymSet() on unit set returns empty set", unit.SymSet(1) == empty);

    std::cout << endl;
}

// Test implication operations
void test_implication_operations() {
    std::cout << "=== Testing Implication Operations ===" << endl;

    vector<vector<int>> sets1 = {{1, 2}, {3, 4}, {1}, {1, 4, 5}};
    ZDD zdd1 = buildZDDFromSets(sets1);
    vector<vector<int>> sets2 = {{1, 2, 3, 4}, {1, 2, 3, 5}};
    ZDD zdd2 = buildZDDFromSets(sets2);
    
    // Empty and unit sets for special cases
    ZDD empty(0);
    ZDD unit(1);
    
    // ImplyChk() tests
    int implyChkResult1 = zdd1.ImplyChk(1, 2);
    test_result("ImplyChk(1, 2) on zdd1", implyChkResult1 == 0);
    int implyChkResult2 = zdd1.ImplyChk(2, 1);
    test_result("ImplyChk(2, 1) on zdd1", implyChkResult2 != 0);
    test_result("ImplyChk() on empty set returns 1", empty.ImplyChk(1, 2) == 1);
    test_result("ImplyChk() on unit set returns 1", unit.ImplyChk(1, 2) == 1);

    // CoImplyChk() tests
    int coImplyChkResult1 = zdd1.CoImplyChk(1, 2);
    test_result("CoImplyChk(1, 2) on zdd1", coImplyChkResult1 == 0);
    int coImplyChkResult2 = zdd1.CoImplyChk(2, 1);
    test_result("CoImplyChk(2, 1) on zdd1", coImplyChkResult2 != 0);
    test_result("CoImplyChk() on empty set returns 1", empty.CoImplyChk(1, 2) == 1);
    test_result("CoImplyChk() on unit set returns 1", unit.CoImplyChk(1, 2) == 1);

    // ImplySet() tests
    ZDD implySetResult1 = zdd1.ImplySet(1);
    test_result("ImplySet() on zdd1 for variable 1", implySetResult1 == empty);
    ZDD implySetResult2 = zdd1.ImplySet(6);
    test_result("ImplySet() on zdd1 for variable 6", implySetResult2 == buildZDDFromSets({{1}, {2}, {3}, {4}, {5}}));
    ZDD implySetResult3 = zdd2.ImplySet(1);
    test_result("ImplySet() on zdd2 for variable 1", implySetResult3 == buildZDDFromSets({{2}, {3}}));
    test_result("ImplySet() on empty set returns empty set", empty.ImplySet(1) == empty);
    test_result("ImplySet() on unit set returns empty set", unit.ImplySet(1) == empty);

    // CoImplySet() tests
    ZDD coImplySetResult1 = zdd1.CoImplySet(1);
    test_result("CoImplySet() on zdd1 for variable 1", coImplySetResult1 == empty);
    ZDD coImplySetResult2 = zdd1.CoImplySet(6);
    test_result("CoImplySet() on zdd1 for variable 6", coImplySetResult2 == buildZDDFromSets({{1}, {2}, {3}, {4}, {5}}));
    ZDD coImplySetResult3 = zdd2.CoImplySet(1);
    test_result("CoImplySet() on zdd2 for variable 1", coImplySetResult3 == buildZDDFromSets({{2}, {3}}));
    test_result("CoImplySet() on empty set returns empty set", empty.CoImplySet(1) == empty);
    test_result("CoImplySet() on unit set returns empty set", unit.CoImplySet(1) == empty);
    
    std::cout << endl;
}

// Test I/O operations
void test_io_operations() {
    std::cout << "=== Testing I/O Operations ===" << endl;
    
    // Export and Import
    vector<ZDD> zs = {
        ZDD(0),
        ZDD(1),
        ZDD(1).Change(1),
        buildZDDFromSets({{1, 2}, {3, 4}, {1}, {1, 4, 5}}),
    };
    for (size_t i = 0; i < zs.size(); ++i) {
        FILE* fp = fopen("./test_zdd_export.dat", "w");
        if (fp) {
            zs[i].Export(fp);
            fclose(fp);
            
            fp = fopen("./test_zdd_export.dat", "r");
            if (fp) {
                ZDD z = ZDD_Import(fp);
                fclose(fp);
                test_result("Export/Import round trip", zs[i] == z);
            } else {
                test_result("Export/Import file read", false);
            }
        } else {
            test_result("Export/Import file write", false);
        }
    }
    
    // Print functions (just test they don't crash)
    bool print_ok = true;
    try {
        for (size_t i = 0; i < zs.size(); ++i) {
            zs[i].Print();
            zs[i].PrintPla();
            // XPrint() is not defined in the implementation
            // z1.XPrint();
        }
    } catch (...) {
        print_ok = false;
    }
    test_result("Print functions execute", print_ok);
    
    std::cout << endl;
}

// Test external functions
void test_external_functions() {
    std::cout << "=== Testing External Functions ===" << endl;

    // Initialize random seed
    srand(1);

    // ZDD_Random
    for (int i = 1; i <= 10; ++i) {
        ZDD z = ZDD_Random(i, 10 * i);
        test_result((std::string("ZDD_Random() creates random ZDD with ") + std::to_string(i) + std::string(" variables")).c_str(), z.Top() <= i);
    }
    
    std::cout << endl;
}


// Test edge cases
void test_edge_cases() {
    std::cout << "=== Testing Edge Cases ===" << endl;
    
    // Operations on null
    ZDD z1(-1);
    ZDD z2(1);
    
    test_result("Null + ZDD = Null", (z1 + z2) == ZDD(-1));
    test_result("Null & ZDD = Null", (z1 & z2) == ZDD(-1));
    test_result("Null - ZDD = Null", (z1 - z2) == ZDD(-1));
    
    // Operations on empty set
    ZDD z3(0);
    test_result("Empty & ZDD(1) = Empty", (z3 & z2) == ZDD(0));
    test_result("Empty + ZDD(1) = ZDD(1)", (z3 + z2) == z2);
    
    // Self operations
    int v1 = 1;
    ZDD z4 = ZDD(1).Change(v1);
    test_result("ZDD & ZDD = ZDD", (z4 & z4) == z4);
    test_result("ZDD - ZDD = Empty", (z4 - z4) == ZDD(0));
    
    std::cout << endl;
}

// Main test function
int main() {
    test_init();
    
    std::cout << "Starting ZDD Class Comprehensive Tests..." << endl << endl;
    
    test_constructors();
    test_assignment_operators();
    test_binary_operators();
    test_query_functions();
    test_set_operations();
    test_advanced_operations();
    test_symmetry_operations();
    test_implication_operations();
    test_io_operations();
    test_external_functions();
    test_edge_cases();
    
    test_cleanup();
    
    cout << "ZDD test completed." << endl;
    cout << "Total tests: " << test_count << endl;
    cout << "Passed: " << pass_count << endl;
    cout << "Failed: " << fail_count << endl;
    
    return 0;
}
