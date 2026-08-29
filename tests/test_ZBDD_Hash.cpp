/* Tests for BDD_Hash and ZDD_Hash.
 *
 * The two classes are near-identical independent implementations, so every
 * scenario runs against both.  The scenarios cover the defects the reviews
 * found in the original open-addressing code:
 *  - insert / update / delete / Refer / Amount basics
 *  - deleting a key that was never entered must be a no-op
 *  - repeated insert+delete of distinct keys must not fill the table with
 *    tombstones and hang the probe loop of Refer()  (used to hang)
 *  - a deleted entry must not keep a reference that pins the key's nodes
 *    against BDD_GC()                                (used to pin)
 *  - growth must keep every live entry findable
 *  - the error value (-1) works as a key like any other
 */

#include <cstdio>
#include <cstdlib>
#include "ZDD.h"

using namespace sapporobdd;

static int testCount = 0;
static int failCount = 0;

#define TEST(cond) do { \
  testCount++; \
  if(!(cond)) { \
    failCount++; \
    std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
  } \
} while(0)

static char marks[4096]; /* distinct addresses to store as values */

template<typename Hash, typename DD>
static void test_basic(DD (*key)(int))
{
  Hash h;
  TEST(h.Amount() == 0);
  TEST(h.Refer(key(0)) == 0);

  h.Enter(key(1), &marks[1]);
  h.Enter(key(2), &marks[2]);
  TEST(h.Amount() == 2);
  TEST(h.Refer(key(1)) == &marks[1]);
  TEST(h.Refer(key(2)) == &marks[2]);
  TEST(h.Refer(key(3)) == 0);

  /* update in place */
  h.Enter(key(1), &marks[3]);
  TEST(h.Amount() == 2);
  TEST(h.Refer(key(1)) == &marks[3]);

  /* delete */
  h.Enter(key(1), 0);
  TEST(h.Amount() == 1);
  TEST(h.Refer(key(1)) == 0);
  TEST(h.Refer(key(2)) == &marks[2]);

  /* deleting a key that is not in the table is a no-op */
  for(int i=0; i<100; i++) h.Enter(key(500+i), 0);
  TEST(h.Amount() == 1);
  TEST(h.Refer(key(2)) == &marks[2]);

  /* reinsert a deleted key */
  h.Enter(key(1), &marks[1]);
  TEST(h.Amount() == 2);
  TEST(h.Refer(key(1)) == &marks[1]);

  h.Clear();
  TEST(h.Amount() == 0);
  TEST(h.Refer(key(2)) == 0);
}

template<typename Hash, typename DD>
static void test_tombstone_cycling(DD (*key)(int))
{
  /* Insert and delete distinct keys far beyond the table size.  The original
     code turned every deletion into a tombstone that was never cleaned, so
     the sentinel-seeking probe loop of Refer() hung once the whole table had
     been touched.  With one key live at a time the table must not grow
     without bound either. */
  Hash h;
  for(int i=0; i<2000; i++)
  {
    h.Enter(key(i), &marks[i & 1023]);
    TEST(h.Refer(key(i)) == &marks[i & 1023]);
    h.Enter(key(i), 0);
    TEST(h.Amount() == 0);
  }
  TEST(h.Refer(key(2001)) == 0); /* used to hang here */
}

template<typename Hash, typename DD>
static void test_growth(DD (*key)(int))
{
  Hash h;
  const int n = 300;
  for(int i=0; i<n; i++) h.Enter(key(i), &marks[i]);
  TEST(h.Amount() == (bddword)n);
  for(int i=0; i<n; i++) TEST(h.Refer(key(i)) == &marks[i]);
  for(int i=0; i<n; i+=2) h.Enter(key(i), 0);
  TEST(h.Amount() == (bddword)n/2);
  for(int i=0; i<n; i++)
    TEST(h.Refer(key(i)) == ((i%2)? &marks[i]: 0));
}

template<typename Hash, typename DD>
static void test_error_key(void)
{
  Hash h;
  h.Enter(DD(-1), &marks[7]);
  TEST(h.Amount() == 1);
  TEST(h.Refer(DD(-1)) == &marks[7]);
  h.Enter(DD(-1), &marks[8]);
  TEST(h.Amount() == 1);
  TEST(h.Refer(DD(-1)) == &marks[8]);
  h.Enter(DD(-1), 0);
  TEST(h.Amount() == 0);
  TEST(h.Refer(DD(-1)) == 0);
}

static BDD bddkey(int i)
{
  /* a distinct function per i over a fixed set of variables */
  BDD f = (i & 1)? BDDvar(1): ~BDDvar(1);
  if(i & 2) f &= BDDvar(2); else f |= BDDvar(2);
  for(int b=2; b<12; b++)
    if(i & (1 << b)) f ^= BDDvar(b+1);
  return f;
}

static ZDD zddkey(int i)
{
  ZDD f = ZDD(1);
  for(int b=0; b<12; b++)
    if(i & (1 << b)) f = f.Change(b+1);
  return f + ZDD(1).Change(13); /* distinct non-constant per i */
}

static void test_gc_release_bdd(void)
{
  /* A deleted entry used to keep its key alive: the nodes of a deleted key
     could not be collected until Clear().  Build a chain, enter it, delete
     it, drop every handle, GC, and check that the nodes are gone. */
  BDD_GC();
  bddword before = BDD_Used();
  {
    BDD_Hash h;
    {
      BDD f = 1;
      for(int v=1; v<=20; v++) f &= BDDvar(v);
      h.Enter(f, &marks[0]);
      h.Enter(f, 0);
    }
    /* f destroyed here; the table still exists but the entry is deleted */
    BDD_GC();
    TEST(BDD_Used() == before);
  }
}

static void test_gc_release_zdd(void)
{
  BDD_GC();
  bddword before = BDD_Used();
  {
    ZDD_Hash h;
    {
      ZDD f = ZDD(1);
      for(int v=1; v<=20; v++) f = f.Change(v) + ZDD(1);
      h.Enter(f, &marks[0]);
      h.Enter(f, 0);
    }
    BDD_GC();
    TEST(BDD_Used() == before);
  }
}

int main()
{
  try
  {
    BDD_Init(256, 10000000);
    for(int v=0; v<20; v++) BDD_NewVar();

    test_basic<BDD_Hash, BDD>(bddkey);
    test_basic<ZDD_Hash, ZDD>(zddkey);
    test_tombstone_cycling<BDD_Hash, BDD>(bddkey);
    test_tombstone_cycling<ZDD_Hash, ZDD>(zddkey);
    test_growth<BDD_Hash, BDD>(bddkey);
    test_growth<ZDD_Hash, ZDD>(zddkey);
    test_error_key<BDD_Hash, BDD>();
    test_error_key<ZDD_Hash, ZDD>();
    test_gc_release_bdd();
    test_gc_release_zdd();
  }
  catch(const BDDException& e)
  {
    std::printf("Unexpected BDDException: %s\n", e.what());
    return 1;
  }

  std::printf("test_ZBDD_Hash: %d tests, %d failures\n", testCount, failCount);
  return (failCount == 0)? 0: 1;
}
