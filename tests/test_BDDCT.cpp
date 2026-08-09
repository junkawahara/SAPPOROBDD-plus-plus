/*********************************************************
 * BDD cost table (BDDCT) test                            *
 * Checks the cost table itself and, above all, that the   *
 * two caches survive a garbage collection: they are keyed *
 * by a node ID, so an entry whose key node was collected  *
 * used to be returned for the unrelated function that got *
 * the recycled ID.                                        *
 *********************************************************/

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define BDD_CPP
#include "../include/bddc.h"
#include "../include/BDD.h"
#include "../include/ZDD.h"
#include "../include/BDDCT.h"
#include "../include/BDDException.h"

using namespace std;
using namespace sapporobdd;

/* ---- test bookkeeping ---- */

static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

static void test_result(const char* name, bool passed)
{
  test_count++;
  if(passed) { pass_count++; cout << "[PASS] " << name << endl; }
  else { fail_count++; cout << "[**FAIL] " << name << endl; }
}

/* ---- the variables under test ----
   var[i] is created i-th, so it sits on level i+1 and the cost table index of
   var[i] is NV-1-i. */

static const int NV = 8;
static int var[NV];

static ZDD Set(int i) { return ZDD(1).Change(var[i]); }

/* every set held by f, as a bit mask over var[] */
static void Enumerate(const ZDD& f, unsigned mask, vector<unsigned>& out)
{
  if(f == 0) return;
  if(f == 1) { out.push_back(mask); return; }
  int top = f.Top();
  int ix = 0;
  for(int i=0; i<NV; i++) if(var[i] == top) ix = i;
  Enumerate(f.OffSet(top), mask, out);
  Enumerate(f.OnSet0(top), mask | (1U << ix), out);
}

static bddcost MaskCost(const BDDCT& ct, unsigned mask)
{
  bddcost c = 0;
  for(int i=0; i<NV; i++)
    if(mask & (1U << i)) c += ct.CostOfLev(BDD_LevOfVar(var[i]));
  return c;
}

/* a family of "sets" random subsets of var[] */
static ZDD RandomFamily(int sets)
{
  ZDD f = 0;
  for(int i=0; i<sets; i++)
  {
    ZDD s = 1;
    for(int j=0; j<NV; j++) if(rand() & 1) s = s.Change(var[j]);
    f += s;
  }
  return f;
}

static void SetUpCosts(BDDCT& ct)
{
  /* var[i] costs (i+1)*10 */
  ct.Alloc(NV);
  for(int i=0; i<NV; i++) ct.SetCostOfLev(BDD_LevOfVar(var[i]), (i + 1) * 10);
}

/* ---- 1. the table itself ---- */

static void test_table(void)
{
  cout << endl << "--- cost table ---" << endl;
  BDDCT ct;

  test_result("Alloc() succeeds", ct.Alloc(NV) == 0);
  test_result("Size() reports the allocated size", ct.Size() == NV);

  bool all_one = true;
  for(int i=0; i<NV; i++) if(ct.Cost(i) != 1) all_one = false;
  test_result("Alloc() fills the table with the default cost 1", all_one);

  test_result("SetCost() succeeds", ct.SetCost(0, 42) == 0);
  test_result("Cost() returns what SetCost() stored", ct.Cost(0) == 42);
  test_result("SetCost() rejects an index below the table", ct.SetCost(-1, 1) != 0);
  test_result("SetCost() rejects an index past the table", ct.SetCost(NV, 1) != 0);
  test_result("SetCost() rejects the bddcost_null mark",
              ct.SetCost(0, bddcost_null) != 0);
  test_result("the rejected cost was not stored", ct.Cost(0) == 42);

  /* index NV-1-i belongs to var[i], i.e. to level i+1 */
  test_result("CostOfLev() addresses the table from the top level",
              ct.CostOfLev(NV) == ct.Cost(0));

  test_result("SetLabel() succeeds", ct.SetLabel(1, "abc") == 0);
  test_result("Label() returns what SetLabel() stored",
              ct.Label(1) && strcmp(ct.Label(1), "abc") == 0);
  test_result("Label() returns null outside the table", ct.Label(NV) == 0);

  test_result("Alloc(0) empties the table", ct.Alloc(0) == 0 && ct.Size() == 0);
}

/* ---- 2. the cost computations on known families ---- */

static void test_costs(void)
{
  cout << endl << "--- MinCost / MaxCost / ZDD_CostLE ---" << endl;
  BDDCT ct;
  SetUpCosts(ct);

  ZDD empty = 0;
  ZDD unit = 1;
  test_result("MinCost() of the empty family is the bddcost_null mark",
              ct.MinCost(empty) == bddcost_null);
  test_result("MaxCost() of the empty family is the bddcost_null mark",
              ct.MaxCost(empty) == bddcost_null);
  test_result("MinCost() of the empty set is 0", ct.MinCost(unit) == 0);
  test_result("MaxCost() of the empty set is 0", ct.MaxCost(unit) == 0);

  /* { {var0}, {var1,var2} }: costs 10 and 50 */
  ZDD f = Set(0) + Set(1) * Set(2);
  test_result("MinCost() over a two-set family", ct.MinCost(f) == 10);
  test_result("MaxCost() over a two-set family", ct.MaxCost(f) == 50);

  test_result("ZDD_CostLE() keeps the sets within the bound",
              ct.ZDD_CostLE(f, 10).Card() == 1);
  test_result("ZDD_CostLE() keeps every set of an ample bound",
              ct.ZDD_CostLE(f, 50).Card() == 2);
  test_result("ZDD_CostLE() drops every set below the cheapest one",
              ct.ZDD_CostLE(f, 9).Card() == 0);
  test_result("ZDD_CostLE0() agrees with ZDD_CostLE()",
              ct.ZDD_CostLE0(f, 10).Card() == 1 &&
              ct.ZDD_CostLE0(f, 50).Card() == 2 &&
              ct.ZDD_CostLE0(f, 9).Card() == 0);

  bddcost aw = 0, rb = 0;
  ct.ZDD_CostLE(f, 10, aw, rb);
  test_result("ZDD_CostLE() reports the worst accepted cost", aw == 10);
  test_result("ZDD_CostLE() reports the best rejected cost", rb == 50);

  bool threw = false;
  try { ct.MinCost(ZDD(-1)); } catch(const BDDException&) { threw = true; }
  test_result("MinCost() rejects the error ZDD", threw);
}

/* ---- 3. the caches across a garbage collection ---- */

static void test_cache_survives_gc(void)
{
  cout << endl << "--- caches across a garbage collection ---" << endl;
  BDDCT ct;
  SetUpCosts(ct);

  /* A cache entry is keyed by the node ID of its argument.  Computing over a
     family, dropping it and collecting frees its nodes, and the families built
     afterwards take the very IDs that were freed: unless the entries hold on
     to their key, one of them is then found for an unrelated family.
     The collection below drops whatever the earlier tests left behind, so
     that the families here are the ones competing for the free IDs. */
  BDD_GC();
  {
    ZDD f = Set(0) * Set(1);          /* the single set {var0,var1}, cost 30 */
    ct.MinCost(f);
    ct.MaxCost(f);
  }
  BDD_GC();

  int min_bad = 0, max_bad = 0, le0_bad = 0;
  for(int i=2; i<NV; i++)
    for(int j=i+1; j<NV; j++)
    {
      ZDD g = Set(i) * Set(j);
      bddcost cost = (i + 1) * 10 + (j + 1) * 10;
      if(ct.MinCost(g) != cost) min_bad++;
      if(ct.MaxCost(g) != cost) max_bad++;
      /* ZDD_CostLE0() prunes on the very same min/max cache */
      if(ct.ZDD_CostLE0(g, cost).Card() != 1 ||
         ct.ZDD_CostLE0(g, cost - 1).Card() != 0) le0_bad++;
    }

  test_result("MinCost() is not served from an entry of a collected family",
              min_bad == 0);
  test_result("MaxCost() is not served from an entry of a collected family",
              max_bad == 0);
  test_result("ZDD_CostLE0() is not served from an entry of a collected family",
              le0_bad == 0);

  /* The same for the ZDD_CostLE() cache.  The bound below accepts one of the
     two sets, so the result shares no node with the root of the family and
     that root is collectable once the family is dropped. */
  BDDCT ct2;
  SetUpCosts(ct2);
  BDD_GC();
  {
    ZDD f = Set(0) + Set(1);          /* {var0} costs 10, {var1} costs 20 */
    ct2.ZDD_CostLE(f, 15);
  }
  BDD_GC();

  int le_bad = 0;
  for(int i=2; i<NV; i++)
    for(int j=i+1; j<NV; j++)
    {
      ZDD g = Set(i) + Set(j);        /* both sets cost more than 15 */
      if(ct2.ZDD_CostLE(g, 15).Card() != 0) le_bad++;
    }
  test_result("ZDD_CostLE() is not served from an entry of a collected family",
              le_bad == 0);
}

/* ---- 4. random families against plain enumeration, with collections ---- */

static void test_random_against_enumeration(void)
{
  cout << endl << "--- random families against plain enumeration ---" << endl;
  BDDCT ct;
  SetUpCosts(ct);

  srand(20260809);
  int min_bad = 0, max_bad = 0, le_bad = 0, le0_bad = 0;
  int checked = 0;
  for(int t=0; t<200; t++)
  {
    ZDD f = RandomFamily(1 + (rand() % 6));
    vector<unsigned> sets;
    Enumerate(f, 0, sets);
    if(sets.empty()) continue;
    checked++;

    bddcost mn = MaskCost(ct, sets[0]);
    bddcost mx = mn;
    for(size_t i=1; i<sets.size(); i++)
    {
      bddcost c = MaskCost(ct, sets[i]);
      if(c < mn) mn = c;
      if(c > mx) mx = c;
    }
    bddcost bound = mn + ((mx - mn) / 2);
    bddword within = 0;
    for(size_t i=0; i<sets.size(); i++)
      if(MaskCost(ct, sets[i]) <= bound) within++;

    if(ct.MinCost(f) != mn) min_bad++;
    if(ct.MaxCost(f) != mx) max_bad++;
    if(ct.ZDD_CostLE(f, bound).Card() != within) le_bad++;
    if(ct.ZDD_CostLE0(f, bound).Card() != within) le0_bad++;

    /* collect while the caches are live: the families of the earlier rounds
       are gone by now, so their nodes are the ones being recycled */
    if((t % 20) == 19) BDD_GC();
  }
  test_result("the random families reached the checks", checked > 100);
  test_result("MinCost() matches the enumeration", min_bad == 0);
  test_result("MaxCost() matches the enumeration", max_bad == 0);
  test_result("ZDD_CostLE() matches the enumeration", le_bad == 0);
  test_result("ZDD_CostLE0() matches the enumeration", le0_bad == 0);
}

/* ---- 5. the pin the caches hold on their key nodes ---- */

static void test_cache_release(void)
{
  cout << endl << "--- releasing the cached nodes ---" << endl;
  BDDCT ct;
  SetUpCosts(ct);

  BDD_GC();
  bddword before = BDD_Used();
  {
    ZDD f = RandomFamily(6);
    ct.MinCost(f);
    ct.MaxCost(f);
    ct.ZDD_CostLE(f, 100);
  }
  BDD_GC();
  bddword pinned = BDD_Used();
  test_result("the caches hold their key nodes over a collection",
              pinned > before);

  test_result("CacheClear() succeeds", ct.CacheClear() == 0);
  test_result("Cache0Clear() succeeds", ct.Cache0Clear() == 0);
  BDD_GC();
  test_result("clearing the caches releases the nodes they held",
              BDD_Used() == before);

  /* SetCost() clears both caches as well */
  {
    ZDD f = RandomFamily(6);
    ct.MinCost(f);
    ct.ZDD_CostLE(f, 100);
  }
  ct.SetCost(0, 5);
  BDD_GC();
  test_result("SetCost() releases the nodes the caches held",
              BDD_Used() == before);
}

int main(void)
{
  cout << "=== BDDCT test ===" << endl;
#ifdef B_EXTEND
  BDD_Init(1024, 1024 * 1024 * 4);
#else
  BDD_Init(256, 1024 * 1024);
#endif
  for(int i=0; i<NV; i++) var[i] = BDD_NewVar();

  try
  {
    test_table();
    test_costs();
    test_cache_survives_gc();
    test_random_against_enumeration();
    test_cache_release();
  }
  catch(const std::exception& e)
  {
    cout << "Exception occurred: " << e.what() << endl;
    test_result("the tests run without an unexpected exception", false);
  }

  cout << endl;
  cout << "=======================================" << endl;
  cout << "Total Tests: " << test_count << endl;
  cout << "Passed: " << pass_count << endl;
  cout << "Failed: " << fail_count << endl;
  cout << "=======================================" << endl;

  return (fail_count == 0)? 0: 1;
}
