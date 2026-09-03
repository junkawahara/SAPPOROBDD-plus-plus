/*
Smoke test of the SAPPOROBDD_PLUS_PLUS configuration of SBDD_helper.

This is not the full test suite (the shared test drivers are written
for the classic SAPPOROBDD): it checks that the helper compiles with
the SAPPOROBDD_PLUS_PLUS macro against SAPPOROBDD++ and that a few
representative functions work.

Copyright (c) 2017 -- 2023 Jun Kawahara

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "ZBDD.h"

#define SAPPOROBDD_PLUS_PLUS
/* The only change made to the upstream test sources: this repository
   bundles the distributed single header in sbdd_helper/, and neither the
   sbdd_helper devel/ sources nor its SBDDH_TEST_RELEASE_HEADER switch
   exist here. */
#include "../sbdd_helper/SBDD_helper.h"

using namespace sapporobdd;
using namespace sbddh;

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>

static void spp_test(bool b, int line)
{
    if (!b) {
        std::cerr << "not passed (line " << line << ")" << std::endl;
        std::exit(1);
    }
}

#define test(b) spp_test((b), __LINE__)

int main()
{
    BDD_Init(1000, 1000);
    for (int i = 0; i < 20; ++i) {
        BDD_NewVar();
    }

    ZBDD f = getPowerSetWithCard(4, 2);
    test(f.Card() == 6);

    std::vector<bddvar> v;
    v.push_back(1);
    v.push_back(3);
    test(isMember(f, v));
    v.push_back(4);
    test(!isMember(f, v));

    DDIndex<int> index(f);
    test(index.count() == 6ull);
    test(index.getOrderNumber(index.getSet(3)) == 3);

    std::ostringstream oss;
    printZBDDElements(oss, f);
    test(!oss.str().empty());

    std::stringstream ss;
    exportZBDDAsGraphillion(ss, f, -1);
    ZBDD g = importZBDDAsGraphillion(ss, static_cast<int>(getLev(f)));
    test(f == g);

    printf("test passed!\n");
    return 0;
}
