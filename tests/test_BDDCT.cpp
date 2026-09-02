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
#include <climits>
#include <cstdio>
#include <sstream>
#include <string>
#include <locale>
#include <type_traits>

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

/* =====================================================================
   Additional tests from BDDCT-tests.md.
   ===================================================================== */

/* the valid single-cost range of the implementation */
static const bddcost kCostMax = bddcost_null - 1;
static const bddcost kCostMin = -bddcost_null + 1;

/* the single set described by mask, as a ZDD */
static ZDD MaskToSet(unsigned mask)
{
  ZDD s = 1;
  for(int i=0; i<NV; i++) if(mask & (1U << i)) s = s.Change(var[i]);
  return s;
}

/* the sets of f within the bound, built by plain enumeration */
static ZDD ExpectedLE(const BDDCT& ct, const ZDD& f, bddcost bound)
{
  vector<unsigned> sets;
  Enumerate(f, 0, sets);
  ZDD e = 0;
  for(size_t i=0; i<sets.size(); i++)
    if(MaskCost(ct, sets[i]) <= bound) e += MaskToSet(sets[i]);
  return e;
}

static int SetVarCost(BDDCT& ct, int i, bddcost cost)
{
  return ct.SetCostOfLev(BDD_LevOfVar(var[i]), cost);
}

/* redirects cout into a string and always restores it */
class CoutCapture
{
  std::streambuf* _old;
  std::ostringstream _oss;
public:
  CoutCapture(void) { _old = cout.rdbuf(_oss.rdbuf()); }
  ~CoutCapture(void) { cout.rdbuf(_old); }
  std::string Str(void) const { return _oss.str(); }
};

/* groups digits in threes, as a great many locales do */
class GroupingPunct : public std::numpunct<char>
{
protected:
  virtual std::string do_grouping(void) const { return "\3"; }
  virtual char do_thousands_sep(void) const { return ','; }
};

/* feeds text to Import() through a temporary file that is always closed */
static int ImportFromString(BDDCT& ct, const std::string& text)
{
  FILE* fp = tmpfile();
  if(!fp) return -2;
  if(text.size() &&
     fwrite(text.c_str(), 1, text.size(), fp) != text.size())
  {
    fclose(fp);
    return -2;
  }
  rewind(fp);
  int r = ct.Import(fp);
  fclose(fp);
  return r;
}

/* outcome of a call that may leave the cost range: 0 = returned a value,
   1 = BDDOutOfRangeException, 2 = some other BDDException */
static int TryMinCost(BDDCT& ct, const ZDD& f, bddcost& out)
{
  try { out = ct.MinCost(f); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

static int TryMaxCost(BDDCT& ct, const ZDD& f, bddcost& out)
{
  try { out = ct.MaxCost(f); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

static int TryCostLE(BDDCT& ct, const ZDD& f, const bddcost b, ZDD& out)
{
  try { bddcost aw, rb; out = ct.ZDD_CostLE(f, b, aw, rb); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

/* the same with the two costs the four-argument form reports */
static int TryCostLEB(BDDCT& ct, const ZDD& f, const bddcost b, ZDD& out,
                      bddcost& aw, bddcost& rb)
{
  try { out = ct.ZDD_CostLE(f, b, aw, rb); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

static int TryCostLE2(BDDCT& ct, const ZDD& f, const bddcost b, ZDD& out)
{
  try { out = ct.ZDD_CostLE(f, b); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

static int TryCostLE0(BDDCT& ct, const ZDD& f, const bddcost b, ZDD& out)
{
  try { out = ct.ZDD_CostLE0(f, b); return 0; }
  catch(const BDDOutOfRangeException&) { return 1; }
  catch(const BDDException&) { return 2; }
}

/* ---- P0-01..P0-03: the filters checked as families, not counts ---- */

static void test_filter_contents(void)
{
  cout << endl << "--- P0-01..P0-03: filter results as families ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  SetVarCost(ct, 2, 30);

  ZDD f = Set(0) + Set(1) * Set(2);          /* costs 10 and 50 */
  test_result("P0-01: ZDD_CostLE(10) is exactly { {v0} }",
              ct.ZDD_CostLE(f, 10) == Set(0));
  test_result("P0-01: ZDD_CostLE0(10) is exactly { {v0} }",
              ct.ZDD_CostLE0(f, 10) == Set(0));

  static const bddcost bounds[] = { 49, 9, 50, 10, 9, 50, 49, 10, 50, 9 };
  bool le_ok = true, le0_ok = true;
  for(size_t i=0; i<sizeof(bounds)/sizeof(bounds[0]); i++)
  {
    ZDD e = ExpectedLE(ct, f, bounds[i]);
    if(ct.ZDD_CostLE(f, bounds[i]) != e) le_ok = false;
    if(ct.ZDD_CostLE0(f, bounds[i]) != e) le0_ok = false;
  }
  test_result("P0-02: ZDD_CostLE() is stable over repeated shuffled bounds", le_ok);
  test_result("P0-02: ZDD_CostLE0() is stable over repeated shuffled bounds", le0_ok);

  BDDCT rct;
  rct.Alloc(NV);
  for(int i=0; i<NV; i++) SetVarCost(rct, i, (i * 7) % 23 + 1);
  srand(20260810);
  int bad_le = 0, bad_le0 = 0;
  for(int t=0; t<100; t++)
  {
    ZDD g = RandomFamily(1 + (rand() % 5));
    vector<unsigned> sets;
    Enumerate(g, 0, sets);
    if(sets.empty()) continue;
    bddcost mn = MaskCost(rct, sets[0]), mx = mn;
    for(size_t i=1; i<sets.size(); i++)
    {
      bddcost c = MaskCost(rct, sets[i]);
      if(c < mn) mn = c;
      if(c > mx) mx = c;
    }
    bddcost bound = mn - 3 + (bddcost)(rand() % (mx - mn + 7));
    ZDD e = ExpectedLE(rct, g, bound);
    if(rct.ZDD_CostLE(g, bound) != e) bad_le++;
    if(rct.ZDD_CostLE0(g, bound) != e) bad_le0++;
  }
  test_result("P0-03: ZDD_CostLE() returns exactly the enumerated sets", bad_le == 0);
  test_result("P0-03: ZDD_CostLE0() returns exactly the enumerated sets", bad_le0 == 0);
}

/* ---- P0-04..P0-07: zero and negative costs ---- */

static void test_zero_negative_costs(void)
{
  cout << endl << "--- P0-04..P0-07: zero and negative costs ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, -10);
  SetVarCost(ct, 1, 0);
  SetVarCost(ct, 2, 20);

  ZDD f = Set(0) + Set(1) + Set(2) + Set(0) * Set(2);  /* -10, 0, 20, 10 */
  test_result("P0-04: MinCost() handles negative costs", ct.MinCost(f) == -10);
  test_result("P0-04: MaxCost() handles negative costs", ct.MaxCost(f) == 20);

  ZDD e0 = Set(0) + Set(1);
  test_result("P0-05: ZDD_CostLE(0) is exactly { {v0}, {v1} }",
              ct.ZDD_CostLE(f, 0) == e0);
  test_result("P0-05: ZDD_CostLE0(0) is exactly { {v0}, {v1} }",
              ct.ZDD_CostLE0(f, 0) == e0);

  static const bddcost bounds[] = { -11, -10, 0, 10, 19, 20 };
  bool le_ok = true, le0_ok = true;
  for(size_t i=0; i<sizeof(bounds)/sizeof(bounds[0]); i++)
  {
    ZDD e = ExpectedLE(ct, f, bounds[i]);
    if(ct.ZDD_CostLE(f, bounds[i]) != e) le_ok = false;
    if(ct.ZDD_CostLE0(f, bounds[i]) != e) le0_ok = false;
  }
  test_result("P0-06: ZDD_CostLE() honours every boundary bound", le_ok);
  test_result("P0-06: ZDD_CostLE0() honours every boundary bound", le0_ok);

  ZDD g = ZDD(1) + Set(2);
  test_result("P0-07: the empty set is rejected at bound -1",
              ct.ZDD_CostLE(g, -1) == ZDD(0) && ct.ZDD_CostLE0(g, -1) == ZDD(0));
  test_result("P0-07: the empty set is accepted at bound 0",
              ct.ZDD_CostLE(g, 0) == ZDD(1) && ct.ZDD_CostLE0(g, 0) == ZDD(1));
}

/* ---- P0-08, P0-09, P1-06: the bddcost range at the table ---- */

static void test_cost_boundaries(void)
{
  cout << endl << "--- P0-08, P0-09, P1-06: bddcost range at the table ---" << endl;
  BDDCT ct;
  ct.Alloc(3);
  test_result("P0-08: SetCost() takes the largest valid cost",
              ct.SetCost(0, kCostMax) == 0 && ct.Cost(0) == kCostMax);
  test_result("P0-08: SetCost() takes the smallest valid cost",
              ct.SetCost(1, kCostMin) == 0 && ct.Cost(1) == kCostMin);

  BDDCT at;
  bool amax = at.Alloc(2, kCostMax) == 0 &&
              at.Cost(0) == kCostMax && at.Cost(1) == kCostMax;
  bool amin = at.Alloc(2, kCostMin) == 0 &&
              at.Cost(0) == kCostMin && at.Cost(1) == kCostMin;
  test_result("P0-08: Alloc() takes both extreme default costs", amax && amin);

  BDDCT rt;
  rt.Alloc(3, 4);
  rt.SetLabel(1, "keep");
  static const bddcost bad[] = { bddcost_null, -bddcost_null, INT_MIN };
  bool rejected = true, intact = true;
  for(size_t i=0; i<sizeof(bad)/sizeof(bad[0]); i++)
  {
    if(rt.SetCost(0, bad[i]) == 0) rejected = false;
    if(rt.Alloc(5, bad[i]) == 0) rejected = false;
    if(rt.Size() != 3) intact = false;
    for(int j=0; j<3; j++) if(rt.Cost(j) != 4) intact = false;
    if(!rt.Label(1) || strcmp(rt.Label(1), "keep") != 0) intact = false;
  }
  test_result("P0-09: SetCost() and Alloc() reject the out-of-range costs",
              rejected);
  test_result("P0-09/P1-06: a rejected cost leaves size, costs and labels alone",
              intact);
}

/* ---- P0-10..P0-12: path sums beyond the bddcost range ----
   A cost the caller is given back has to be a bddcost, and a set whose cost
   is not one is the range error below.  The two filters report no cost, so
   they answer such a family instead of refusing it: the sums themselves are
   taken in the wider type the recursions work in. */

static void test_overflow_and_bounds(void)
{
  cout << endl << "--- P0-10..P0-12: path sums beyond the bddcost range ---" << endl;
  bddcost c;
  ZDD z;
  {
    BDDCT ct;
    ct.Alloc(NV);
    SetVarCost(ct, 0, kCostMax);
    SetVarCost(ct, 1, kCostMax);
    ZDD f = Set(0) * Set(1);
    test_result("P0-10: MinCost() reports the positive overflow",
                TryMinCost(ct, f, c) == 1);
    test_result("P0-10: MaxCost() reports the positive overflow",
                TryMaxCost(ct, f, c) == 1);
    /* the one set costs 2 * kCostMax, so the bound rejects it and its cost
       is what rej_best would have to report */
    test_result("P0-10: ZDD_CostLE() reports the cost it cannot express",
                TryCostLE(ct, f, 100, z) == 1);
    test_result("P0-10: the forms that report no cost filter the family",
                TryCostLE2(ct, f, 100, z) == 0 && z == ZDD(0) &&
                TryCostLE0(ct, f, 100, z) == 0 && z == ZDD(0));
  }
  {
    BDDCT ct;
    ct.Alloc(NV);
    SetVarCost(ct, 0, kCostMin);
    SetVarCost(ct, 1, kCostMin);
    ZDD f = Set(0) * Set(1);
    test_result("P0-11: MinCost() reports the negative overflow",
                TryMinCost(ct, f, c) == 1);
    test_result("P0-11: MaxCost() reports the negative overflow",
                TryMaxCost(ct, f, c) == 1);
    test_result("P0-11: ZDD_CostLE() reports the cost it cannot express",
                TryCostLE(ct, f, 0, z) == 1);
    /* 2 * kCostMin is below every bound, so the set is accepted */
    test_result("P0-11: the forms that report no cost keep the family",
                TryCostLE2(ct, f, 0, z) == 0 && z == f &&
                TryCostLE0(ct, f, 0, z) == 0 && z == f);
  }
  {
    BDDCT ct;
    ct.Alloc(NV);
    SetVarCost(ct, 0, 10);
    SetVarCost(ct, 1, 20);
    ZDD f = Set(0) + Set(1);
    static const bddcost bads[] = { INT_MIN, -bddcost_null };
    bool ok = true;
    for(size_t i=0; i<sizeof(bads)/sizeof(bads[0]); i++)
    {
      int r;
      z = ZDD(1);
      r = TryCostLE(ct, f, bads[i], z);
      if(!(r == 1 || (r == 0 && z == ZDD(0)))) ok = false;
      z = ZDD(1);
      r = TryCostLE2(ct, f, bads[i], z);
      if(!(r == 1 || (r == 0 && z == ZDD(0)))) ok = false;
      z = ZDD(1);
      r = TryCostLE0(ct, f, bads[i], z);
      if(!(r == 1 || (r == 0 && z == ZDD(0)))) ok = false;
    }
    test_result("P0-12: an unnegatable bound is rejected or rejects every set",
                ok);
  }
}

/* ---- sums that leave the bddcost range and come back ----
   The cost of a set, the bound that is left after the cost of a variable has
   been taken off it, and the smallest and largest cost of a sub-ZDD are three
   different things, and only the first has to be a bddcost.  The recursions
   used to work in bddcost throughout and reported a range error for any of
   them, so a table whose costs cancel refused families it prices perfectly
   well -- and the answer depended on what was in the cache, as a bound the
   cache answered never reached the arithmetic. */

static void test_wide_sums(void)
{
  cout << endl << "--- intermediate sums outside the bddcost range ---" << endl;
  ZDD z;
  bddcost aw, rb;
  {
    /* the bound at the top of the range and a negative cost: what is left of
       the bound for the 1-branch is kCostMax + 1, while the one set of the
       family costs -1 */
    BDDCT cold;
    cold.Alloc(NV);
    for(int i=0; i<NV; i++) SetVarCost(cold, i, -1);
    ZDD f = Set(0);
    bddcost caw = 0, crb = 0;
    int r = TryCostLEB(cold, f, kCostMax, z, caw, crb);
    test_result("the widest bound is a bound, not a cost of the table",
                r == 0 && z == f && caw == -1 && crb == bddcost_null);

    /* the same call on a table that has already answered another bound for
       the same family: the cache used to decide whether this threw */
    BDDCT warm;
    warm.Alloc(NV);
    for(int i=0; i<NV; i++) SetVarCost(warm, i, -1);
    warm.ZDD_CostLE(f, 0, aw, rb);
    ZDD w;
    bddcost waw = 0, wrb = 0;
    int rw = TryCostLEB(warm, f, kCostMax, w, waw, wrb);
    test_result("the answer is the same on a cold and on a warm cache",
                rw == r && w == z && waw == caw && wrb == crb);

    test_result("ZDD_CostLE0() answers the widest bound alike",
                TryCostLE0(cold, f, kCostMax, z) == 0 && z == f);
  }
  {
    /* costs that cancel: the one set of the family costs 1, and the sum of
       the two variables below the top passes through kCostMax + 1 */
    BDDCT ct;
    ct.Alloc(NV);
    for(int i=0; i<NV; i++) SetVarCost(ct, i, 0);
    SetVarCost(ct, 0, 1);
    SetVarCost(ct, 1, kCostMax);
    SetVarCost(ct, 2, kCostMin);
    ZDD f = Set(0) * Set(1) * Set(2);
    bddcost mn = 0, mx = 0;
    bool ok = TryMinCost(ct, f, mn) == 0 && mn == 1;
    ok = ok && TryMaxCost(ct, f, mx) == 0 && mx == 1;
    test_result("MinCost() and MaxCost() price the set the table can express",
                ok);
    ok = TryCostLE(ct, f, 1, z) == 0 && z == f;
    ok = ok && ct.ZDD_CostLE(f, 1, aw, rb) == f &&
         aw == 1 && rb == bddcost_null;
    test_result("ZDD_CostLE() accepts it under its own cost", ok);
    ok = ct.ZDD_CostLE(f, 0, aw, rb) == ZDD(0) &&
         aw == bddcost_null && rb == 1;
    test_result("ZDD_CostLE() rejects it below its own cost", ok);
    test_result("ZDD_CostLE0() answers both bounds alike",
                TryCostLE0(ct, f, 1, z) == 0 && z == f &&
                TryCostLE0(ct, f, 0, z) == 0 && z == ZDD(0));
  }
  {
    /* -bddcost_null is a cost no table stores but two stored costs reach.
       The cache keys used to be negated bddcost values, which left no room
       for it: the entry was given up, and the manual said so. */
    BDDCT ct;
    ct.Alloc(NV);
    for(int i=0; i<NV; i++) SetVarCost(ct, i, 0);
    SetVarCost(ct, 0, -1);
    SetVarCost(ct, 1, kCostMin);
    ZDD f = Set(0) * Set(1);
    bool ok = ct.ZDD_CostLE(f, -bddcost_null, aw, rb) == f &&
              aw == -bddcost_null && rb == bddcost_null;
    ZDD again = ct.ZDD_CostLE(f, -bddcost_null, aw, rb);
    ok = ok && again == f && aw == -bddcost_null && rb == bddcost_null;
    test_result("a composite cost of -bddcost_null goes through the cache",
                ok && ct.CallCount() == 1);
    bddcost c = 0;
    test_result("MinCost() reports the same composite cost",
                TryMinCost(ct, f, c) == 0 && c == -bddcost_null);
  }
}

/* ---- P0-13..P0-16: the error ZDD ---- */

static void test_error_zdd_all(void)
{
  cout << endl << "--- P0-13..P0-16: the error ZDD ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  ZDD bad(-1);
  bddcost aw, rb;
  bool ok;

  ok = false;
  try { ct.MinCost(bad); }
  catch(const BDDInvalidBDDValueException&) { ok = true; }
  catch(const BDDException&) { }
  test_result("P0-13: MinCost() throws BDDInvalidBDDValueException", ok);

  ok = false;
  try { ct.MaxCost(bad); }
  catch(const BDDInvalidBDDValueException&) { ok = true; }
  catch(const BDDException&) { }
  test_result("P0-14: MaxCost() throws BDDInvalidBDDValueException", ok);

  ok = false;
  try { ct.ZDD_CostLE(bad, 10); }
  catch(const BDDInvalidBDDValueException&) { ok = true; }
  catch(const BDDException&) { }
  test_result("P0-15: ZDD_CostLE(f, b) throws BDDInvalidBDDValueException", ok);

  ok = false;
  try { ct.ZDD_CostLE(bad, 10, aw, rb); }
  catch(const BDDInvalidBDDValueException&) { ok = true; }
  catch(const BDDException&) { }
  test_result("P0-15: ZDD_CostLE(f, b, aw, rb) throws BDDInvalidBDDValueException",
              ok);

  ok = false;
  try { ct.ZDD_CostLE0(bad, 10); }
  catch(const BDDInvalidBDDValueException&) { ok = true; }
  catch(const BDDException&) { }
  test_result("P0-16: ZDD_CostLE0() throws BDDInvalidBDDValueException", ok);
}

/* ---- P0-17, P0-18: copy and move policy ---- */

static void test_copy_move_policy(void)
{
  cout << endl << "--- P0-17, P0-18: copy and move policy ---" << endl;
  /* BDDCT owns raw arrays and an implicitly generated copy would make two
     owners of them, so the contract is P0-17A: no copies (and no implicit
     move, which would be the same shallow copy).  A runtime copy would
     double-free, so the contract is checked as a type property. */
  test_result("P0-17A: BDDCT is not copy constructible",
              !std::is_copy_constructible<BDDCT>::value);
  test_result("P0-17A: BDDCT is not copy assignable",
              !std::is_copy_assignable<BDDCT>::value);
  test_result("P0-18: BDDCT has no implicit shallow move",
              !std::is_move_constructible<BDDCT>::value &&
              !std::is_move_assignable<BDDCT>::value);
}

/* ---- P1-01..P1-07: Alloc(), Cost() and the level APIs ---- */

static void test_alloc_accessors(void)
{
  cout << endl << "--- P1-01..P1-07: Alloc(), Cost() and the level APIs ---" << endl;
  {
    BDDCT ct;
    test_result("P1-01: a fresh table is empty and answers by its contract",
                ct.Size() == 0 && ct.Cost(0) == bddcost_null &&
                ct.Cost(-1) == bddcost_null && ct.Label(0) == 0);
  }
  {
    BDDCT ct;
    bool ok = ct.Alloc(5, 7) == 0 && ct.Size() == 5;
    for(int i=0; i<5; i++)
      if(ct.Cost(i) != 7 || !ct.Label(i) || ct.Label(i)[0]) ok = false;
    test_result("P1-02: Alloc(N, 7) fills every cost and empties every label", ok);
    /* an index outside the table answers with the mark on both sides; a
       negative one, that is a variable above the table, used to answer 1 */
    test_result("P1-03: Cost() is the bddcost_null mark on both sides of the table",
                ct.Cost(-1) == bddcost_null &&
                ct.Cost(ct.Size()) == bddcost_null &&
                ct.CostOfLev(0) == bddcost_null &&
                ct.CostOfLev(ct.Size() + 1) == bddcost_null);
    test_result("P1-04: Alloc(-1) yields the empty table",
                ct.Alloc(-1) == 0 && ct.Size() == 0);
  }
  {
    BDDCT ct;
    ct.Alloc(NV);
    SetVarCost(ct, 0, 10);
    SetVarCost(ct, 1, 20);
    ct.SetLabel(0, "old");
    ZDD f = Set(0) + Set(1);
    ct.MinCost(f);
    ct.ZDD_CostLE(f, 15);
    bddcost aw, rb;
    bool hit = ct.Cache0Ref(4, f) == 10 &&
               ct.CacheRef(f, 15, aw, rb) == Set(0);
    test_result("P1-05: the caches hold the results before the re-Alloc()", hit);
    bool ok = ct.Alloc(4, 2) == 0 && ct.Size() == 4;
    for(int i=0; i<4; i++)
      if(ct.Cost(i) != 2 || !ct.Label(i) || ct.Label(i)[0]) ok = false;
    test_result("P1-05: re-Alloc() replaces every cost and label", ok);
    test_result("P1-05: re-Alloc() drops both caches",
                ct.Cache0Ref(4, f) == bddcost_null &&
                ct.CacheRef(f, 15, aw, rb) == ZDD(-1));
  }
  {
    BDDCT ct;
    ct.Alloc(NV);
    bool set_ok = true, get_ok = true;
    for(int lev=1; lev<=NV; lev++)
      if(ct.SetCostOfLev(lev, lev * 3 + 1) != 0) set_ok = false;
    for(int lev=1; lev<=NV; lev++)
    {
      if(ct.CostOfLev(lev) != lev * 3 + 1) get_ok = false;
      if(ct.Cost(NV - lev) != lev * 3 + 1) get_ok = false;
    }
    test_result("P1-07: SetCostOfLev() reaches every level", set_ok);
    test_result("P1-07: levels map onto the reversed table indices", get_ok);
  }
}

/* ---- P1-08..P1-12: labels ---- */

static void test_labels(void)
{
  cout << endl << "--- P1-08..P1-12: labels ---" << endl;
  BDDCT ct;
  ct.Alloc(4);
  ct.SetLabel(0, "aa");
  ct.SetLabel(3, "dd");
  test_result("P1-08: SetLabel() rejects the out-of-range indices",
              ct.SetLabel(-1, "x") != 0 && ct.SetLabel(4, "x") != 0);
  test_result("P1-08: the rejected labels changed nothing",
              strcmp(ct.Label(0), "aa") == 0 && strcmp(ct.Label(3), "dd") == 0);

  bool ok = ct.SetLabel(1, "") == 0 && ct.Label(1)[0] == 0;
  ok = ok && ct.SetLabel(1, "ab") == 0 && strcmp(ct.Label(1), "ab") == 0;
  ok = ok && ct.SetLabel(1, "cd") == 0 && strcmp(ct.Label(1), "cd") == 0;
  test_result("P1-09: empty, short and overwritten labels keep the latest value",
              ok);

  const char* s15 = "123456789012345";           /* CT_STRLEN chars */
  const char* s16 = "1234567890123456";
  const char* s40 = "1234567890123456789012345678901234567890";
  ok = ct.SetLabel(2, s15) == 0 && strcmp(ct.Label(2), s15) == 0;
  test_result("P1-10: a CT_STRLEN-char label is stored whole", ok);
  /* a longer label used to be cut down to CT_STRLEN characters and reported
     as stored whole */
  ok = ct.SetLabel(2, s16) != 0 && strcmp(ct.Label(2), s15) == 0;
  ok = ok && ct.SetLabel(2, s40) != 0 && strcmp(ct.Label(2), s15) == 0;
  test_result("P1-10: a longer label is refused and changes nothing", ok);

  bool lev_ok = true;
  for(int lev=1; lev<=4; lev++)
  {
    char buf[8];
    sprintf(buf, "L%d", lev);
    if(ct.SetLabelOfLev(lev, buf) != 0) lev_ok = false;
  }
  for(int lev=1; lev<=4; lev++)
  {
    char buf[8];
    sprintf(buf, "L%d", lev);
    if(strcmp(ct.LabelOfLev(lev), buf) != 0) lev_ok = false;
    if(ct.LabelOfLev(lev) != ct.Label(4 - lev)) lev_ok = false;
  }
  test_result("P1-11: the level labels mirror the index labels", lev_ok);

  test_result("P1-12: Label() is null outside the table",
              ct.Label(-1) == 0 && ct.Label(4) == 0);

  /* a label that is not there at all -- what a C interface or an optional
     value hands over when it holds nothing -- used to be read from and take
     the process down, in a method that answers every other label it cannot
     store by returning 1 */
  const std::string kept = ct.Label(0);
  ok = ct.SetLabel(0, (const char*)0) != 0 && kept == ct.Label(0);
  ok = ok && ct.SetLabelOfLev(4, (const char*)0) != 0 && kept == ct.Label(0);
  test_result("a null label is refused and changes nothing", ok);

  /* the buffer behind the pointer belongs to the table: a plain char* let a
     caller write CT_STRLEN or more characters into it and wreck the heap */
  test_result("Label() hands out a pointer that cannot be written through",
              (std::is_same<decltype(ct.Label(0)), const char*>::value) &&
              (std::is_same<decltype(ct.LabelOfLev(1)), const char*>::value));
}

/* ---- P1-13..P1-20: Export() / Import() ---- */

static void test_export_import(void)
{
  cout << endl << "--- P1-13..P1-20: Export() / Import() ---" << endl;
  {
    BDDCT src;
    src.Alloc(4);
    src.SetCost(0, 5);
    src.SetCost(1, 0);
    src.SetCost(2, -7);
    src.SetCost(3, 123);
    src.SetLabel(0, "alpha");
    src.SetLabel(2, "gamma");
    std::string dump;
    { CoutCapture cap; src.Export(); dump = cap.Str(); }
    BDDCT dst;
    bool ok = ImportFromString(dst, dump) == 0 && dst.Size() == 4;
    if(ok)
      for(int i=0; i<4; i++)
      {
        if(dst.Cost(i) != src.Cost(i)) ok = false;
        if(strcmp(dst.Label(i), src.Label(i)) != 0) ok = false;
      }
    test_result("P1-13: a labelled table survives the Export/Import round trip",
                ok);
  }
  {
    BDDCT src;
    src.Alloc(3, 9);
    std::string dump;
    { CoutCapture cap; src.Export(); dump = cap.Str(); }
    BDDCT dst;
    bool ok = ImportFromString(dst, dump) == 0 && dst.Size() == 3;
    if(ok)
      for(int i=0; i<3; i++)
        if(dst.Cost(i) != 9 || dst.Label(i)[0]) ok = false;
    test_result("P1-14: an unlabelled table survives the round trip", ok);
  }
  {
    BDDCT src;
    src.Alloc(0);
    std::string dump;
    { CoutCapture cap; src.Export(); dump = cap.Str(); }
    BDDCT dst;
    test_result("P1-15: the empty table survives the round trip",
                ImportFromString(dst, dump) == 0 && dst.Size() == 0);
  }
  {
    BDDCT dst;
    bool ok = ImportFromString(dst,
                "#made-by-hand\n#n 3\n10 #lab0\n#note\n20\n30\n") == 0;
    ok = ok && dst.Size() == 3 && dst.Cost(0) == 10 && dst.Cost(1) == 20 &&
         dst.Cost(2) == 30 && strcmp(dst.Label(0), "lab0") == 0 &&
         dst.Label(1)[0] == 0 && dst.Label(2)[0] == 0;
    test_result("P1-16: comment tokens are read over", ok);
  }
  {
    /* the comment rule is about tokens, not lines -- it cannot be about
       lines, as the size follows the "#n" comment on the first line -- so a
       free-text comment is refused rather than read over silently */
    BDDCT a;
    test_result("P1-16: a comment token does not reach over the rest of a line",
                ImportFromString(a, "#to be revised\n2\n1\n2\n") != 0 &&
                a.Size() == 0);
    BDDCT b;
    bool ok = ImportFromString(b, "#to-be-revised\n2\n1\n2\n") == 0 &&
              b.Size() == 2 && b.Cost(0) == 1 && b.Cost(1) == 2;
    test_result("P1-16: the same comment written as one token is read over", ok);
  }
  {
    BDDCT a;
    bool ok = ImportFromString(a, "1 5 #123456789012345\n") == 0 &&
              a.Size() == 1 && strcmp(a.Label(0), "123456789012345") == 0;
    test_result("P1-16: a CT_STRLEN-char label is imported whole", ok);
    BDDCT b;
    test_result("P1-16: a longer label is a format error",
                ImportFromString(b, "1 5 #1234567890123456\n") != 0 &&
                b.Size() == 0);
  }
  {
    BDDCT a, b;
    test_result("P1-17: a non-numeric size is refused",
                ImportFromString(a, "abc 5\n") != 0);
    test_result("P1-17: a non-numeric cost is refused",
                ImportFromString(b, "2 xyz 7\n") != 0);
  }
  {
    BDDCT a, b;
    test_result("P1-18: a size beyond int is refused",
                ImportFromString(a, "4294967299\n1 2 3\n") != 0);
    test_result("P1-18: a cost beyond int is refused",
                ImportFromString(b, "1 4294967301\n") != 0);
  }
  {
    BDDCT a, b;
    test_result("P1-19: a cost shortfall is refused",
                ImportFromString(a, "3 1 2") != 0);
    test_result("P1-19: an EOF right after a label is refused",
                ImportFromString(b, "2 5 #lab") != 0);
  }
  {
    /* the guarantee chosen for the class: a failed Import() leaves the
       empty table, never a half-imported one */
    BDDCT a;
    a.Alloc(2, 8);
    bool fail_a = ImportFromString(a, "3 1 2") != 0;
    test_result("P1-20: the cost-shortfall failure leaves the empty table",
                fail_a && a.Size() == 0);
    BDDCT b;
    b.Alloc(2, 8);
    bool fail_b = ImportFromString(b, "3\n") != 0;
    test_result("P1-20: the EOF-after-size failure leaves the empty table",
                fail_b && b.Size() == 0);
    BDDCT c;
    c.Alloc(2, 8);
    bool fail_c = ImportFromString(c, "") != 0;
    test_result("P1-20: the empty-input failure leaves the empty table",
                fail_c && c.Size() == 0);
  }
  {
    /* Data behind the table is not part of the format, and the token that
       holds it is read from the stream to find out whether it is the last
       variable's label: accepting it both took a broken file for a good one
       and swallowed a token of whatever follows the table in the stream. */
    BDDCT a;
    a.Alloc(2, 8);
    test_result("data behind the table is a format error",
                ImportFromString(a, "1 5 trailing\n") != 0 && a.Size() == 0);
    BDDCT b;
    test_result("a comment behind the table is read over",
                ImportFromString(b, "1 5 #lab #note\n") == 0 &&
                b.Size() == 1 && strcmp(b.Label(0), "lab") == 0);
    BDDCT c;
    test_result("data behind the empty table is a format error",
                ImportFromString(c, "0 1\n") != 0 && c.Size() == 0);
    BDDCT d;
    test_result("a comment behind the empty table is read over",
                ImportFromString(d, "0 #done\n") == 0 && d.Size() == 0);
  }
  {
    /* a token longer than any this format has is a format error, and only
       the first few characters of it are ever held: without a limit one
       token of a hostile file exhausts the memory of the process before
       anything looks at what is in it */
    std::string huge = "1 ";
    huge.append(200000, '9');
    huge += "\n";
    BDDCT a;
    test_result("an unbounded numeric token is a format error",
                ImportFromString(a, huge) != 0 && a.Size() == 0);
    std::string label = "1 5 #";
    label.append(200000, 'x');
    label += "\n";
    BDDCT b;
    test_result("an unbounded label token is a format error",
                ImportFromString(b, label) != 0 && b.Size() == 0);
    std::string comment = "#";
    comment.append(200000, 'c');
    comment += "\n2 1 2\n";
    BDDCT c;
    test_result("an unbounded comment token is a format error",
                ImportFromString(c, comment) != 0 && c.Size() == 0);
  }
  {
    /* a stream that is not there used to be handed to fgetc() */
    BDDCT a;
    a.Alloc(2, 8);
    test_result("a null stream is refused and leaves the empty table",
                a.Import((FILE*)0) != 0 && a.Size() == 0);
  }
  {
    /* The format is plain decimal, and everything cout carries that could
       write something else is the caller's: a locale that groups digits
       used to write "#n 1,000", which Import() then refused. */
    BDDCT src;
    src.Alloc(4, 1000);
    src.SetCost(1, -1234567);
    std::string dump;
    {
      CoutCapture cap;
      std::locale saved = cout.getloc();
      cout.imbue(std::locale(cout.getloc(), new GroupingPunct));
      src.Export();
      cout.imbue(saved);
      dump = cap.Str();
    }
    BDDCT dst;
    bool ok = dump.find(',') == std::string::npos;
    ok = ok && ImportFromString(dst, dump) == 0 && dst.Size() == 4;
    if(ok)
      for(int i=0; i<4; i++) if(dst.Cost(i) != src.Cost(i)) ok = false;
    test_result("Export() writes the format under any locale of cout", ok);
  }
}

/* ---- P1-21..P1-25: AllocRand() ---- */

static void test_allocrand(void)
{
  cout << endl << "--- P1-21..P1-25: AllocRand() ---" << endl;
  srand(13579);
  BDDCT ct;
  bool ok = ct.AllocRand(10, 3, 7) == 0 && ct.Size() == 10;
  if(ok)
    for(int i=0; i<10; i++)
      if(ct.Cost(i) < 3 || ct.Cost(i) > 7) ok = false;
  test_result("P1-21: the costs stay inside the closed range", ok);

  ok = ct.AllocRand(10, 5, 5) == 0 && ct.Size() == 10;
  if(ok) for(int i=0; i<10; i++) if(ct.Cost(i) != 5) ok = false;
  test_result("P1-22: a one-value range yields only that value", ok);

  test_result("P1-23: a zero-size table is allowed",
              ct.AllocRand(0, 1, 2) == 0 && ct.Size() == 0);

  /* an empty range is a mistake -- swapped arguments -- and used to be
     answered with a table of the min everywhere */
  ct.Alloc(3, 8);
  ok = ct.AllocRand(5, 10, 3) != 0 && ct.Size() == 3;
  if(ok) for(int i=0; i<3; i++) if(ct.Cost(i) != 8) ok = false;
  test_result("P1-24: min > max is refused and leaves the table alone", ok);

  ok = ct.AllocRand(5, 1, bddcost_null) != 0 &&
       ct.AllocRand(5, -bddcost_null, 1) != 0 && ct.Size() == 3;
  if(ok) for(int i=0; i<3; i++) if(ct.Cost(i) != 8) ok = false;
  test_result("P1-24: an invalid range bound is refused the same way", ok);

  /* the full valid range; its width does not fit in bddcost, which the
     range arithmetic has to survive without an overflow */
  ok = ct.AllocRand(6, kCostMin, kCostMax) == 0 && ct.Size() == 6;
  if(ok)
    for(int i=0; i<6; i++)
      if(ct.Cost(i) < kCostMin || ct.Cost(i) > kCostMax) ok = false;
  test_result("P1-25: the widest range never stores an invalid cost", ok);
}

/* ---- P1-26..P1-28: cache invalidation by SetCost() and Alloc() ---- */

static void test_cache_invalidation(void)
{
  cout << endl << "--- P1-26..P1-28: cache invalidation ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  ZDD f = Set(0) + Set(1);

  bool primed = ct.MinCost(f) == 10 && ct.MaxCost(f) == 20 &&
                ct.ZDD_CostLE(f, 15) == Set(0) && ct.ZDD_CostLE0(f, 15) == Set(0);
  test_result("P1-26: the caches are primed with the first costs", primed);

  bool changed = SetVarCost(ct, 0, 100) == 0;
  bool after = changed &&
               ct.MinCost(f) == 20 && ct.MaxCost(f) == 100 &&
               ct.ZDD_CostLE(f, 15) == ZDD(0) && ct.ZDD_CostLE0(f, 15) == ZDD(0) &&
               ct.ZDD_CostLE(f, 20) == Set(1) && ct.ZDD_CostLE0(f, 20) == Set(1) &&
               ct.ZDD_CostLE(f, 100) == f && ct.ZDD_CostLE0(f, 100) == f;
  test_result("P1-26: SetCost() invalidates every cached result", after);

  bool re = ct.Alloc(NV) == 0 &&        /* every cost back to 1 */
            ct.MinCost(f) == 1 && ct.MaxCost(f) == 1 &&
            ct.ZDD_CostLE(f, 0) == ZDD(0) && ct.ZDD_CostLE0(f, 0) == ZDD(0) &&
            ct.ZDD_CostLE(f, 1) == f && ct.ZDD_CostLE0(f, 1) == f;
  test_result("P1-27: re-Alloc() recomputes on the new table", re);

  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  ct.MinCost(f);
  ct.MaxCost(f);
  ct.ZDD_CostLE(f, 15);
  int r = ct.SetCost(NV - 1, bddcost_null);     /* index of var[0] */
  bool unchanged = r != 0 && ct.Cost(NV - 1) == 10 &&
                   ct.MinCost(f) == 10 && ct.MaxCost(f) == 20 &&
                   ct.ZDD_CostLE(f, 15) == Set(0) &&
                   ct.ZDD_CostLE0(f, 15) == Set(0);
  test_result("P1-28: a rejected SetCost() changes neither costs nor results",
              unchanged);
}

/* ---- P1-29..P1-31, P1-33: the cache tables themselves ---- */

static void test_cache_mechanics(void)
{
  cout << endl << "--- P1-29..P1-31, P1-33: the cache tables themselves ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);

  /* 20 distinct keys: more than half of the fresh 16-slot tables, so the
     enlarge paths run, and more than 16, so two of them share a slot */
  vector<ZDD> keys, vals;
  for(int i=0; i<NV && (int)keys.size() < 20; i++)
    for(int j=i+1; j<NV && (int)keys.size() < 20; j++)
    {
      keys.push_back(Set(i) + Set(j));
      vals.push_back(MaskToSet((unsigned)keys.size()));
    }

  ct.CacheClear();
  bool ent_ok = true;
  for(size_t i=0; i<keys.size(); i++)
    if(ct.CacheEnt(keys[i], vals[i], (bddcost)(100 + i), bddcost_null) != 0)
      ent_ok = false;
  bool ref_ok = true;
  for(size_t i=0; i<keys.size(); i++)
  {
    bddcost aw = 0, rb = 0;
    ZDD r = ct.CacheRef(keys[i], (bddcost)(100 + i), aw, rb);
    if(r != vals[i] || aw != (bddcost)(100 + i) || rb != bddcost_null)
      ref_ok = false;
  }
  test_result("P1-29: CacheEnlarge() keeps every earlier entry",
              ent_ok && ref_ok);

  ct.Cache0Clear();
  ent_ok = true;
  for(size_t i=0; i<keys.size(); i++)
  {
    if(ct.Cache0Ent(4, keys[i], (bddcost)(100 + i)) != 0) ent_ok = false;
    if(ct.Cache0Ent(5, keys[i], (bddcost)(200 + i)) != 0) ent_ok = false;
  }
  ref_ok = true;
  for(size_t i=0; i<keys.size(); i++)
  {
    if(ct.Cache0Ref(4, keys[i]) != (bddcost)(100 + i)) ref_ok = false;
    if(ct.Cache0Ref(5, keys[i]) != (bddcost)(200 + i)) ref_ok = false;
  }
  test_result("P1-30: Cache0Enlarge() keeps every earlier entry",
              ent_ok && ref_ok);

  /* P1-31: two keys probing the same slot of the fresh 16-entry tables;
     the hash is id*1234567 masked, so equal products mod 16 collide */
  int ia = -1, ib = -1;
  for(size_t i=0; i<keys.size() && ia < 0; i++)
    for(size_t j=i+1; j<keys.size() && ia < 0; j++)
      if(((bddword)(keys[i].GetID() * (bddword)1234567) & 15) ==
         ((bddword)(keys[j].GetID() * (bddword)1234567) & 15))
      { ia = (int)i; ib = (int)j; }
  bool col_ok = ia >= 0;
  if(col_ok)
  {
    ct.CacheClear();
    ct.Cache0Clear();
    col_ok = ct.CacheEnt(keys[ia], vals[ia], 7, bddcost_null) == 0 &&
             ct.CacheEnt(keys[ib], vals[ib], 7, bddcost_null) == 0;
    bddcost aw, rb;
    col_ok = col_ok && ct.CacheRef(keys[ia], 7, aw, rb) == vals[ia];
    col_ok = col_ok && ct.CacheRef(keys[ib], 7, aw, rb) == vals[ib];
    col_ok = col_ok && ct.Cache0Ent(4, keys[ia], 111) == 0 &&
             ct.Cache0Ent(4, keys[ib], 222) == 0 &&
             ct.Cache0Ref(4, keys[ia]) == 111 &&
             ct.Cache0Ref(4, keys[ib]) == 222;
  }
  test_result("P1-31: colliding keys keep their own results", col_ok);

  BDDCT ct2;
  ct2.Alloc(NV);
  SetVarCost(ct2, 0, 10);
  SetVarCost(ct2, 1, 20);
  ZDD f = Set(0) + Set(1);
  bddcost aw1, rb1, aw2, rb2;
  ZDD r1 = ct2.ZDD_CostLE(f, 15, aw1, rb1);
  bddcost mn1 = ct2.MinCost(f), mx1 = ct2.MaxCost(f);
  bool cleared = ct2.CacheClear() == 0 && ct2.Cache0Clear() == 0;
  ZDD r2 = ct2.ZDD_CostLE(f, 15, aw2, rb2);
  test_result("P1-33: the results recompute identically after the clears",
              cleared && r1 == r2 && aw1 == aw2 && rb1 == rb2 &&
              ct2.MinCost(f) == mn1 && ct2.MaxCost(f) == mx1);
}

/* ---- the edges of a cache entry ----
   CacheRef() used to answer an entry whose map is empty by stepping off the
   front of that map, and to look for the "every set was rejected" mark by
   walking back from the end, where it can never be. */

static void test_cache_entry_edges(void)
{
  cout << endl << "--- the edges of a cache entry ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  ZDD f = Set(0) + Set(1);
  bddcost aw = 0, rb = 0;

  ct.CacheClear();
  test_result("CacheEnt() takes an entry with no cost bounds at all",
              ct.CacheEnt(f, f, bddcost_null, bddcost_null) == 0);
  test_result("CacheRef() misses on an entry that holds nothing",
              ct.CacheRef(f, 10, aw, rb) == ZDD(-1));

  /* the entry that says every set was rejected, and the bounds it answers */
  ct.CacheClear();
  test_result("CacheEnt() takes the all-rejected entry",
              ct.CacheEnt(f, ZDD(0), bddcost_null, 30) == 0);
  aw = 0; rb = 0;
  ZDD h = ct.CacheRef(f, 10, aw, rb);
  test_result("CacheRef() serves the all-rejected entry under its bound",
              h == ZDD(0) && aw == bddcost_null && rb == 30);
  aw = 0; rb = 0;
  test_result("CacheRef() misses above the rejected cost",
              ct.CacheRef(f, 35, aw, rb) == ZDD(-1));
}

/* ---- P1-32, P1-34..P1-38: acc_worst and rej_best ---- */

static void test_acc_rej_bounds(void)
{
  cout << endl << "--- P1-32, P1-34..P1-38: acc_worst and rej_best ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  ZDD f = Set(0) + Set(1);
  bddcost aw, rb;

  test_result("P1-34: the empty family reports two bddcost_null marks",
              ct.ZDD_CostLE(ZDD(0), 5, aw, rb) == ZDD(0) &&
              aw == bddcost_null && rb == bddcost_null);

  ZDD h = ct.ZDD_CostLE(f, 25, aw, rb);
  test_result("P1-35: full acceptance reports the worst accepted cost only",
              h == f && aw == 20 && rb == bddcost_null);

  h = ct.ZDD_CostLE(f, 5, aw, rb);
  test_result("P1-36: full rejection reports the best rejected cost only",
              h == ZDD(0) && aw == bddcost_null && rb == 10);

  h = ct.ZDD_CostLE(f, 15, aw, rb);
  test_result("P1-37: partial acceptance reports both boundary costs",
              h == Set(0) && aw == 10 && rb == 20);

  /* P1-32/P1-38: the same ZDD, every kind of bound, repeated out of order;
     the expectations come from plain enumeration and must survive the hits */
  static const bddcost bounds[] = { 15, 5, 25, 9, 10, 20, 50, 15, 5, 25 };
  vector<unsigned> sets;
  Enumerate(f, 0, sets);
  bool ok = true;
  for(int round=0; round<3 && ok; round++)
    for(size_t i=0; i<sizeof(bounds)/sizeof(bounds[0]); i++)
    {
      bddcost b = bounds[i];
      bddcost eaw = bddcost_null, erb = bddcost_null;
      for(size_t s=0; s<sets.size(); s++)
      {
        bddcost c = MaskCost(ct, sets[s]);
        if(c <= b) { if(eaw == bddcost_null || c > eaw) eaw = c; }
        else       { if(erb == bddcost_null || c < erb) erb = c; }
      }
      ZDD e = ExpectedLE(ct, f, b);
      ZDD r = ct.ZDD_CostLE(f, b, aw, rb);
      if(r != e || aw != eaw || rb != erb) ok = false;
    }
  test_result("P1-32/P1-38: every bound answers consistently, hit or miss", ok);
}

/* ---- P2-01, P2-02: the backward-compatible names ---- */

static void test_compat_api(void)
{
  cout << endl << "--- P2-01, P2-02: the ZBDD_* compatibility names ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  SetVarCost(ct, 0, 10);
  SetVarCost(ct, 1, 20);
  ZDD f = Set(0) + Set(1);

  bddcost aw1, rb1, aw2, rb2;
  ZDD a = ct.ZDD_CostLE(f, 15, aw1, rb1);
  ZDD b = ct.ZBDD_CostLE(f, 15, aw2, rb2);
  test_result("P2-01: ZBDD_CostLE() matches ZDD_CostLE() in all outputs",
              a == b && aw1 == aw2 && rb1 == rb2 &&
              ct.ZBDD_CostLE(f, 15) == a);
  test_result("P2-02: ZBDD_CostLE0() matches ZDD_CostLE0()",
              ct.ZBDD_CostLE0(f, 15) == ct.ZDD_CostLE0(f, 15));
}

/* ---- P2-05: a longer fixed-seed run with collections in between ---- */

static void test_stress_fixed_seed(void)
{
  cout << endl << "--- P2-05: fixed-seed stress with collections ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  for(int i=0; i<NV; i++)
    SetVarCost(ct, i, (i % 3 == 0)? -(5 * i + 3): (7 * i + 1));

  srand(424242);
  int rounds = 0;
  int bad_min = 0, bad_max = 0, bad_le = 0, bad_le0 = 0;
  for(int t=0; t<400; t++)
  {
    ZDD g = RandomFamily(1 + (rand() % 10));
    vector<unsigned> sets;
    Enumerate(g, 0, sets);
    if(sets.empty()) continue;
    rounds++;

    bddcost mn = MaskCost(ct, sets[0]), mx = mn;
    for(size_t i=1; i<sets.size(); i++)
    {
      bddcost c = MaskCost(ct, sets[i]);
      if(c < mn) mn = c;
      if(c > mx) mx = c;
    }
    bddcost bound = mn - 3 + (bddcost)(rand() % (mx - mn + 7));
    ZDD e = ExpectedLE(ct, g, bound);

    if(ct.MinCost(g) != mn) bad_min++;
    if(ct.MaxCost(g) != mx) bad_max++;
    if(ct.ZDD_CostLE(g, bound) != e) bad_le++;
    if(ct.ZDD_CostLE0(g, bound) != e) bad_le0++;

    if((t % 25) == 24) BDD_GC();
  }
  test_result("P2-05: the stress rounds all ran", rounds == 400);
  test_result("P2-05: MinCost() matches the enumeration throughout", bad_min == 0);
  test_result("P2-05: MaxCost() matches the enumeration throughout", bad_max == 0);
  test_result("P2-05: ZDD_CostLE() matches the enumeration throughout", bad_le == 0);
  test_result("P2-05: ZDD_CostLE0() matches the enumeration throughout", bad_le0 == 0);
}

/* ---- the stack overflow limitter in the cost recursions ----
   CLE/CLE0/MinC/MaxC recurse once per ZDD level; without BDD_RECUR_INC a
   diagram deeper than the stack crashed the process instead of raising the
   usual limit error.  BDDerr() resets the recursion counter when the guard
   throws; the resets below only make the tests independent of that. */

static void test_deep_recursion_guard(void)
{
  cout << endl << "--- deep ZDD: BDDException instead of a stack overflow ---" << endl;

  const int depth = BDD_RecurLimit + 1000;
  vector<int> deep_var(depth);
  for(int i=0; i<depth; i++) deep_var[i] = BDD_NewVar();
  ZDD f = 1;
  for(int i=0; i<depth; i++) f = f.Change(deep_var[i]);

  BDDCT ct;
  /* the chain sits on the levels above the NV variables of the other tests,
     and a table that does not cover every level of the diagram is refused
     before the recursion even starts */
  ct.Alloc(NV + depth);

  bool threw = false;
  try { ct.MinCost(f); } catch(const BDDException&) { threw = true; }
  BDD_RecurCount = 0;
  test_result("MinCost() on a too-deep chain throws BDDException", threw);

  threw = false;
  try { ct.MaxCost(f); } catch(const BDDException&) { threw = true; }
  BDD_RecurCount = 0;
  test_result("MaxCost() on a too-deep chain throws BDDException", threw);

  threw = false;
  bddcost aw, rb;
  try { ct.ZDD_CostLE(f, 0, aw, rb); } catch(const BDDException&) { threw = true; }
  BDD_RecurCount = 0;
  test_result("ZDD_CostLE() on a too-deep chain throws BDDException", threw);

  threw = false;
  try { ct.ZDD_CostLE0(f, 0); } catch(const BDDException&) { threw = true; }
  BDD_RecurCount = 0;
  test_result("ZDD_CostLE0() on a too-deep chain throws BDDException", threw);

  /* the guard must not get in the way of an ordinary shallow run */
  BDDCT ct2;
  ct2.Alloc(NV);
  SetVarCost(ct2, 0, 10);
  SetVarCost(ct2, 1, 20);
  ZDD g = Set(0) + Set(1);
  test_result("a shallow MinCost() still works after the failures",
              ct2.MinCost(g) == 10 && BDD_RecurCount == 0);
}

/* ---- a diagram the cost table does not cover ----
   Cost() answered 1 for a variable above the table, and no recursion looked
   at what came back, so a family over more variables than the table describes
   was filtered as if every unknown variable cost 1. */

static void test_variable_outside_table(void)
{
  cout << endl << "--- a variable the cost table has no entry for ---" << endl;
  BDDCT ct;
  ct.Alloc(2);                    /* levels 1 and 2 only */
  SetVarCost(ct, 0, 10);          /* var[0] is on level 1 */
  SetVarCost(ct, 1, 20);          /* var[1] is on level 2 */

  ZDD in = Set(0) + Set(1);
  ZDD out = Set(0) + Set(7);      /* var[7] is on level 8, above the table */
  bddcost c;
  ZDD z;

  test_result("the family inside the table is still answered",
              ct.MinCost(in) == 10 && ct.MaxCost(in) == 20 &&
              ct.ZDD_CostLE(in, 15) == Set(0) &&
              ct.ZDD_CostLE0(in, 15) == Set(0));

  test_result("MinCost() refuses a variable outside the table",
              TryMinCost(ct, out, c) == 1);
  test_result("MaxCost() refuses a variable outside the table",
              TryMaxCost(ct, out, c) == 1);
  test_result("ZDD_CostLE() refuses a variable outside the table",
              TryCostLE(ct, out, 15, z) == 1 && TryCostLE2(ct, out, 15, z) == 1);
  test_result("ZDD_CostLE0() refuses a variable outside the table",
              TryCostLE0(ct, out, 15, z) == 1);

  /* the empty table covers no level at all, so even a one-variable family
     is outside it, while the constants stay answerable */
  BDDCT empty;
  test_result("the empty table refuses every family with a variable",
              TryMinCost(empty, Set(0), c) == 1 &&
              TryCostLE0(empty, Set(0), 0, z) == 1);
  test_result("the empty table still answers for the constants",
              empty.MinCost(ZDD(1)) == 0 && empty.MaxCost(ZDD(0)) == bddcost_null);
}

/* ---- CallCount(): the recursion counter of the operation that ran last ----
   The counter used to be kept by ZDD_CostLE() alone, so MinCost(), MaxCost()
   and ZDD_CostLE0() reported whatever the last ZDD_CostLE() had left. */

static void test_call_count(void)
{
  cout << endl << "--- CallCount() ---" << endl;
  BDDCT ct;
  ct.Alloc(NV);
  for(int i=0; i<NV; i++) SetVarCost(ct, i, (i + 1) * 10);
  ZDD f = Set(0) * Set(1) + Set(2) * Set(3) + Set(4);

  bddcost mn = ct.MinCost(f);
  bddword deep_min = ct.CallCount();
  ct.MaxCost(f);
  bddword deep_max = ct.CallCount();
  bddcost aw, rb;
  ct.ZDD_CostLE(f, mn, aw, rb);
  bddword deep_le = ct.CallCount();
  ct.ZDD_CostLE0(f, mn);
  bddword deep_le0 = ct.CallCount();
  test_result("every cost operation counts its own recursive calls",
              deep_min > 1 && deep_max > 1 && deep_le > 1 && deep_le0 > 1);

  /* one call each: the recursion returns at once on the empty family, and the
     count of the operation before it is gone */
  ct.MinCost(ZDD(0));
  bool one = ct.CallCount() == 1;
  ct.MaxCost(ZDD(0));
  one = one && ct.CallCount() == 1;
  ct.ZDD_CostLE(ZDD(0), 5, aw, rb);
  one = one && ct.CallCount() == 1;
  ct.ZDD_CostLE0(ZDD(0), 5);
  one = one && ct.CallCount() == 1;
  test_result("every entry point resets the count before it recurses", one);

  /* repeating an operation stops at the root entry of the cache */
  ct.MinCost(f);
  bool hit = ct.CallCount() == 1;
  ct.MaxCost(f);
  hit = hit && ct.CallCount() == 1;
  ct.ZDD_CostLE(f, mn, aw, rb);
  hit = hit && ct.CallCount() == 1;
  test_result("a repeated operation counts the one call the cache serves", hit);

  /* the count describes the operation that ran last, and an operation that
     threw ran: it used to be checked for the error ZDD before the count was
     reset, so a caller looking at the count of the call that just failed was
     given the count of the one before it */
  ZDD bad(-1);
  bool reset = true;
  ct.MinCost(f);
  try { ct.MinCost(bad); } catch(const BDDException&) { }
  if(ct.CallCount() != 0) reset = false;
  ct.MaxCost(f);
  try { ct.MaxCost(bad); } catch(const BDDException&) { }
  if(ct.CallCount() != 0) reset = false;
  ct.ZDD_CostLE(f, mn, aw, rb);
  try { ct.ZDD_CostLE(bad, 5, aw, rb); } catch(const BDDException&) { }
  if(ct.CallCount() != 0) reset = false;
  try { ct.ZDD_CostLE(bad, 5); } catch(const BDDException&) { }
  if(ct.CallCount() != 0) reset = false;
  ct.ZDD_CostLE0(f, mn);
  try { ct.ZDD_CostLE0(bad, 5); } catch(const BDDException&) { }
  if(ct.CallCount() != 0) reset = false;
  test_result("a failed operation leaves the count of no other one", reset);
}

/* ---- a variable inserted below the top level ----
   The recursions price a node through the level its variable has at call
   time, but the caches are keyed by node IDs alone.  BDD_NewVarOfLev()
   below the top moves every variable above the insertion point one level
   up; an entry from before the move then answered with the costs of the
   old levels, so all four cost operations returned stale results, without
   a word, until the caches were cleared by hand.  The table now snapshots
   which variable sat on each of its levels when the caches were filled,
   and drops both caches when a level it covers has changed hands -- and
   only then: an insertion at the top or above the table moves nothing the
   table prices, so those keep the caches.  This test runs last of all:
   the variables of var[] do not come back to their old levels. */

static void test_variable_order_change(void)
{
  cout << endl << "--- a variable inserted below the top level ---" << endl;

  BDDCT ct;
  ct.Alloc(2);
  ct.SetCostOfLev(1, 10);         /* var[0], for now */
  ct.SetCostOfLev(2, 20);         /* var[1], for now */

  ZDD g = Set(0);                 /* the one set {var[0]}: costs 10 now */
  ZDD f = Set(0) + Set(1);

  bddcost aw, rb;
  bool primed = ct.MinCost(g) == 10 && ct.MaxCost(g) == 10 &&
                ct.ZDD_CostLE(g, 15, aw, rb) == g &&
                ct.ZDD_CostLE0(g, 15) == g &&
                ct.MinCost(f) == 10 && ct.MaxCost(f) == 20;
  test_result("the caches are primed under the first variable order", primed);

  /* everything moves one level up: var[0] to level 2, whose cost is 20,
     and var[1] to level 3, above the two-level table */
  BDD_NewVarOfLev(1);

  test_result("MinCost() prices the moved variable by its new level",
              ct.MinCost(g) == 20);
  test_result("MaxCost() prices the moved variable by its new level",
              ct.MaxCost(g) == 20);
  test_result("ZDD_CostLE() prices the moved variable by its new level",
              ct.ZDD_CostLE(g, 15) == ZDD(0) && ct.ZDD_CostLE(g, 20) == g);
  test_result("ZDD_CostLE0() prices the moved variable by its new level",
              ct.ZDD_CostLE0(g, 15) == ZDD(0) && ct.ZDD_CostLE0(g, 20) == g);

  /* var[1] sits above the table now, so the family the caches know must be
     refused like any other family with a variable outside the table */
  bddcost c;
  ZDD z;
  test_result("a variable pushed above the table is refused, not served",
              TryMinCost(ct, f, c) == 1 && TryMaxCost(ct, f, c) == 1 &&
              TryCostLE(ct, f, 15, z) == 1 && TryCostLE2(ct, f, 15, z) == 1 &&
              TryCostLE0(ct, f, 15, z) == 1);

  /* the caches driven directly: an entry from the old order is a miss */
  BDDCT ct2;
  ct2.Alloc(4);
  bool stored = ct2.CacheEnt(g, g, 10, bddcost_null) == 0 &&
                ct2.Cache0Ent(4, g, 10) == 0 &&
                ct2.Cache0Ref(4, g) == 10 &&
                ct2.CacheRef(g, 10, aw, rb) == g;
  BDD_NewVarOfLev(1);             /* var[0] to level 3 */
  test_result("Cache0Ref() misses across the order change",
              stored && ct2.Cache0Ref(4, g) == bddcost_null);
  test_result("CacheRef() misses across the order change",
              ct2.CacheRef(g, 10, aw, rb) == -1);

  /* a variable on top of the order moves nothing that exists, so the
     caches stay: the repeated operation is the one call the cache serves */
  BDDCT ct3;
  ct3.Alloc(4);                   /* levels 1..4; var[0] is on level 3 */
  ct3.SetCostOfLev(3, 30);
  bool warm = ct3.MinCost(g) == 30;
  ct3.MinCost(g);
  bddword calls_warm = ct3.CallCount();
  BDD_NewVar();
  test_result("a new variable on top leaves the caches in place",
              warm && ct3.MinCost(g) == 30 &&
              ct3.CallCount() == calls_warm);

  /* an insertion above the table moves only levels the table never
     prices, so the caches stay as well */
  BDD_NewVarOfLev(5);
  test_result("an insertion above the table leaves the caches in place",
              ct3.MinCost(g) == 30 && ct3.CallCount() == calls_warm);

  /* an insertion at a level the table covers changes hands on level 2:
     the caches go, and var[0] is priced by its new level 4, whose cost is
     the Alloc() default 1 */
  BDD_NewVarOfLev(2);
  bool recomputed = ct3.MinCost(g) == 1 && ct3.CallCount() > calls_warm;
  bool recached = ct3.MinCost(g) == 1 && ct3.CallCount() == calls_warm;
  test_result("an insertion inside the table drops and recomputes",
              recomputed && recached);
}

/* ---- P2-06, P2-07: out of reach in this build ---- */

static void test_env_notes(void)
{
  cout << endl << "--- P2-06, P2-07 ---" << endl;
  cout << "[INFO] P2-06 not applicable: the BDD core is single-threaded, so"
          " concurrent use is outside the contract (the recursions themselves"
          " no longer pass their context through file statics)." << endl;
  cout << "[INFO] P2-07 not applicable: BDDCT allocates with the nothrow"
          " operator new and reports a failure as the library's own"
          " out-of-memory error, but this build has no hook to make an"
          " allocation fail on demand, and a table large enough to exhaust"
          " memory for real cannot be asked for safely from a test." << endl;
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

    test_filter_contents();
    test_zero_negative_costs();
    test_cost_boundaries();
    test_overflow_and_bounds();
    test_wide_sums();
    test_error_zdd_all();
    test_copy_move_policy();
    test_alloc_accessors();
    test_labels();
    test_export_import();
    test_allocrand();
    test_cache_invalidation();
    test_cache_mechanics();
    test_cache_entry_edges();
    test_acc_rej_bounds();
    test_compat_api();
    test_stress_fixed_seed();
    test_deep_recursion_guard();
    test_variable_outside_table();
    test_call_count();
    test_variable_order_change();
    test_env_notes();
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
