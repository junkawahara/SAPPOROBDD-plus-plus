/* Tests that the library stays usable after a caught exception.
 *
 *  - After an out-of-memory exception escapes from an apply recursion, the
 *    intermediate results held by the unwound frames must not keep their
 *    reference counts: once the user drops every handle and runs BDD_GC(),
 *    the node table must be empty again.  (The references used to leak and
 *    pin the nodes forever.)
 *  - After a stack-overflow (recursion limit) exception, the recursion depth
 *    counter must be back at zero, so that later shallow operations still
 *    run.  (The counter used to stay at the limit, and every later recursion
 *    failed immediately -- with no way back, not even BDD_Init().)
 */

#include <cstdio>
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

static void test_no_leak_after_oom(void)
{
  BDD_Init(64, 300); /* a table this small overflows quickly */
  const int nv = 20;
  for(int i=0; i<nv; i++) BDD_NewVar();
  int caught = 0;
  {
    ZDD f = ZDD(1);
    try
    {
      for(int round=0; round<100; round++)
      {
        ZDD g = ZDD(1);
        for(int v=1; v<=nv; v++)
          g += ZDD(1).Change(v) + ZDD(1).Change(v%nv+1).Change((v+5)%nv+1);
        f = f * g;
      }
    }
    catch(const BDDOutOfMemoryException&) { caught = 1; }
    catch(const BDDException&) { caught = 1; }
  }
  TEST(caught == 1);
  /* every handle is gone; nothing may survive the GC */
  BDD_GC();
  TEST(BDD_Used() == 0);
}

static void test_recursion_counter_recovers(void)
{
  BDD_Init(256, 40000000);
  /* 8600 variables keep count()/apply() recursive in this build only up to
     the recursion limit of 8192, so a conjunction chain this deep makes the
     BDD+ recursion of Smooth() hit the limit and throw. */
  const int n = 8600;
  for(int i=0; i<n; i++) BDD_NewVar();
  int caught = 0;
  try
  {
    BDD g = 1;
    for(int v=1; v<=n; v++) g &= BDDvar(v);
    g.Smooth(1);
  }
  catch(const BDDException&) { caught = 1; }
  TEST(caught == 1);

  /* a depth-2 recursion must succeed now */
  int ok = 0;
  try
  {
    BDD h = BDDvar(1) & BDDvar(2);
    BDD s = h.Smooth(1);
    ok = (s != -1);
  }
  catch(const BDDException&) { ok = 0; }
  TEST(ok == 1);
}

int main()
{
  test_no_leak_after_oom();
  test_recursion_counter_recovers();
  std::printf("test_recovery: %d tests, %d failures\n", testCount, failCount);
  return (failCount == 0)? 0: 1;
}
