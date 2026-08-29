/*********************************************************
 * Multi-precision table out-of-memory test               *
 * Injects malloc failures into the multi-precision table  *
 * used by ZDD::CardMP16() and checks that the documented   *
 * contract (BDDOutOfMemoryException) is honoured and that   *
 * the library state stays usable afterwards.                *
 *********************************************************/

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#define BDD_CPP
#include "../include/bddc.h"
#include "../include/BDD.h"
#include "../include/ZDD.h"
#include "../include/BDDException.h"

using namespace std;
using namespace sapporobdd;

/* Internal state inspected by this test (defined in bddc_init.cc) */
namespace sapporobdd {
  extern int BDD_RecurCount;
  extern bddp MPAllocFailSize;
}

/* ---- malloc fault injection ----
   The test binary is linked with -Wl,--wrap=malloc (GNU ld / lld), so every
   malloc() call inside the library is routed through __wrap_malloc below. */

extern "C" void *__real_malloc(size_t size);

static int mp_fail_armed = 0;   /* fail the next multi-precision table block */
static int mp_fail_hit = 0;     /* set once a request was actually refused */

extern "C" void *__wrap_malloc(size_t size)
{
  /* Only the first block of a multi-precision table row is refused: its size
     is sizeof(bddp) * len * 16 for a row of len words.  CardMP16() needs at
     most two words here, so matching those two sizes leaves every unrelated
     allocation of the process untouched. */
  if(mp_fail_armed &&
     (size == sizeof(bddp) * 16 || size == sizeof(bddp) * 2 * 16))
  {
    mp_fail_armed = 0;
    mp_fail_hit = 1;
    return 0;
  }
  return __real_malloc(size);
}

/* ---- test bookkeeping ---- */

static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;

static void test_result(const char *name, bool passed)
{
  test_count++;
  if(passed) { pass_count++; cout << "[PASS] " << name << endl; }
  else       { fail_count++; cout << "[FAIL] " << name << endl; }
}

/* Power set of 70 items: 2^70 elements, far beyond the range of a single
   machine word, so counting it has to grow the multi-precision table. */
static ZDD LargePowerSet()
{
  ZDD f(1);
  for(int i=1; i<=70; i++) f += f.Change(BDD_NewVar());
  return f;
}

int main()
{
  cout << "=== Multi-precision table OOM test ===" << endl;

  BDD_Init(256, 1024 * 1024);

  ZDD f = LargePowerSet();
  char buffer[256];

  /* 1. A refused allocation must surface as BDDOutOfMemoryException, not as
        a successfully returned (empty) string. */
  memset(buffer, 'x', sizeof(buffer));
  mp_fail_armed = 1;
  mp_fail_hit = 0;
  bool caught_oom = false;
  bool caught_other = false;
  try
  {
    f.CardMP16(buffer);
  }
  catch(const BDDOutOfMemoryException&) { caught_oom = true; }
  catch(const BDDException&)            { caught_other = true; }
  mp_fail_armed = 0;

  test_result("malloc failure was actually injected", mp_fail_hit == 1);
  test_result("CardMP16() throws BDDOutOfMemoryException on malloc failure",
              caught_oom);
  test_result("no other BDDException type is thrown", !caught_other);

  /* 2. The failure must not leave the library in a broken state. */
  test_result("BDD_RecurCount is restored after the failure",
              BDD_RecurCount == 0);
  test_result("MPAllocFailSize is cleared after the failure",
              MPAllocFailSize == 0);

  /* 3. The very same computation must succeed once memory is available
        again, i.e. the failure must not have been cached. */
  memset(buffer, 'x', sizeof(buffer));
  string retry;
  bool retry_threw = false;
  try { retry = f.CardMP16(buffer); }
  catch(const BDDException&) { retry_threw = true; }

  test_result("retry after the failure does not throw", !retry_threw);
  test_result("retry returns the correct cardinality (2^70)",
              retry == "400000000000000000");
  test_result("BDD_RecurCount is still balanced after the retry",
              BDD_RecurCount == 0);

  /* 4. Once the table exists, further calls must keep working (the cached
        sub-results of the failed run must not poison later runs). */
  memset(buffer, 'x', sizeof(buffer));
  string again;
  bool again_threw = false;
  try { again = f.CardMP16(buffer); }
  catch(const BDDException&) { again_threw = true; }
  test_result("repeated CardMP16() keeps returning the cardinality",
              !again_threw && again == "400000000000000000");

  /* 5. The buffer size the manual asks the caller to provide has to be the
        one the widest possible result really needs: 16 words written as hex
        digits plus the terminating null.  CardMP16() takes no length, so a
        number that is too small in the manual is an overrun in the caller. */
  const size_t max_digits = 16 * sizeof(bddp) * 2; /* B_MP_LMAX words */
  const size_t doc_size = max_digits + 1;          /* 257 in a 64bit build */

  /* The largest representable cardinality is 2^(16 * 64) - 1, i.e. the power
     set of that many variables with the empty set removed. */
  ZDD full(1);
  for(size_t i = 0; i < max_digits * 4; i++) full += full.Change(BDD_NewVar());
  ZDD widest = full - ZDD(1);

  char wide_buf[doc_size + 1];
  memset(wide_buf, 'x', sizeof(wide_buf));
  string widest_str;
  bool widest_threw = false;
  try { widest_str = widest.CardMP16(wide_buf); }
  catch(const BDDException&) { widest_threw = true; }

  test_result("the widest cardinality fits in the documented buffer size",
              !widest_threw && widest_str.size() == max_digits);
  test_result("the widest cardinality is the all-ones value",
              widest_str.find_first_not_of('F') == string::npos);
  test_result("nothing is written past the documented buffer size",
              wide_buf[doc_size] == 'x');

  /* One more than that must be refused rather than written out. */
  bool over_threw = false;
  memset(wide_buf, 'x', sizeof(wide_buf));
  try { full.CardMP16(wide_buf); }
  catch(const BDDOutOfRangeException&) { over_threw = true; }
  catch(const BDDException&)           { }
  test_result("a cardinality one larger throws BDDOutOfRangeException",
              over_threw);

  cout << endl;
  cout << "=======================================" << endl;
  cout << "Total Tests: " << test_count << endl;
  cout << "Passed: " << pass_count << endl;
  cout << "Failed: " << fail_count << endl;
  cout << "=======================================" << endl;

  return (fail_count == 0)? 0: 1;
}
