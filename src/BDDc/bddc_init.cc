/*****************************************
*  BDD Package (SAPPORO-1.94)   - Init  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

/* ------- Definition of global variables ------- */
/* Stack overflow limiter */
const int BDD_RecurLimit = 8192;
int BDD_RecurCount = 0;

/* Node table */
struct B_NodeTable *Node = 0;
bddp NodeLimit = 0;
bddp NodeUsed = 0;
bddp Avail = bddnull;
bddp NodeSpc = 0;

/* Variable table */
struct B_VarTable *Var = 0;
bddvar *VarID = 0;
bddvar VarUsed = 0;
bddvar VarSpc = 0;

/* Operation Cache */
struct B_CacheTable *Cache = 0;
bddp CacheSpc = 0;
double CacheRatio = 0.5;
bddp GCThreshold = 0;
int ShiftCacheUsed = 0;

/* RFC-table */
struct B_RFC_Table *RFCT = 0;
bddp RFCT_Spc = 0;
bddp RFCT_Used = 0;

/* MP-Count Table */
struct B_MPTable mptable[B_MP_LMAX] = {{0, 0, NULL}};
bddp MPAllocFailSize = 0;
int MPCountOverflowed = 0;

/* ------------------ External functions ------------------ */
int bddinit(bddp initsize, bddp limitsize, double cacheRatio)
/* Throws BDDOutOfMemoryException when a table cannot be allocated and
   BDDOutOfRangeException for an illegal cacheRatio: a positive value that is
   not a power of 2, one outside 1/CACHE_RATIO_MAX..CACHE_RATIO_MAX, or NaN.
   A cacheRatio of 0 or below selects the default ratio of 0.5.  The return
   value is always 0; it only remains for source compatibility with the old
   interface, which returned 1 instead of throwing. */
{
  bddp   ix;
  bddvar i;
  bool cacheallocated = false;

  /* Set cache ratio if specified.  This validates its argument and throws for
     an illegal one, so it runs before any global is touched: a rejected ratio
     has to leave the running environment exactly as it was.  NaN compares
     false with 0.0 and used to slip through to the default as if it were a
     request for it; setcacheratiovalue() rejects it. */
  if(cacheRatio > 0.0 || isnan(cacheRatio)) {
    setcacheratiovalue(cacheRatio, "bddinit");
  } else {
    CacheRatio = 0.5; /* Default cache ratio */
  }

  /* Nothing of the old environment survives, so the globals that describe it
     start over as well: the recursion depth, the shift-cache flag and the GC
     threshold set by bddsetgcthreshold().  They are reset here rather than at
     the top of the function because the check above may throw, and clearing
     ShiftCacheUsed while the old cache still holds the shift entries it
     stands for would keep bddnewvaroflev() from ever sweeping them. */
  BDD_RecurCount = 0;
  ShiftCacheUsed = 0;
  GCThreshold = 0;

  /* Check dupulicate initialization */
  if(Node){ free(Node); Node = 0; }
  if(Var)
  {
    for(i=0; i<VarSpc; i++)
    {
#ifdef B_EXTEND
      if(Var[i].hash_64) free(Var[i].hash_64);
#elif defined(B_32)
      if(Var[i].hash_32) free(Var[i].hash_32);
#else
      if(Var[i].hash_32) free(Var[i].hash_32);
      if(Var[i].hash_h8) free(Var[i].hash_h8);
#endif
    }
    free(Var); Var = 0;
  }
  if(VarID){ free(VarID); VarID = 0; }
  if(Cache){ free(Cache); Cache = 0; }

  /* Set NodeLimit */
  if(limitsize < B_NODE_SPC0) NodeLimit = B_NODE_SPC0;
  else if(limitsize > B_NODE_MAX) NodeLimit = B_NODE_MAX;
  else NodeLimit = limitsize;

  /* Set NodeSpc */
  if(initsize < B_NODE_SPC0) NodeSpc = B_NODE_SPC0;
  else if(initsize > NodeLimit) NodeSpc = NodeLimit;
  else NodeSpc = initsize;

  /* Set VarSpc */
  VarSpc = B_VAR_SPC0;

  /* Memory allocation */
  Node = B_MALLOC(struct B_NodeTable, NodeSpc);
  Var = B_MALLOC(struct B_VarTable, VarSpc);
  VarID = B_MALLOC(bddvar, VarSpc);
  CacheSpc = 0;
  cacheallocated = allocatecache();

  /* Check overflow */
  if(Node == 0 || Var == 0 || VarID == 0 || !cacheallocated)
  {
    if(Cache){ free(Cache); Cache = 0; CacheSpc = 0; }
    if(VarID){ free(VarID); VarID = 0; }
    if(Var){ free(Var); Var = 0; }
    if(Node){ free(Node); Node = 0; }
    /* The tables are gone, so every size and counter that describes them has
       to go with them.  What normally resets those is the initialization
       below, which the throw skips: an application that catches the exception
       and keeps calling the library would otherwise find NodeSpc, VarSpc and
       the used counters still holding the previous session's values, making
       the freed tables look alive and turning a clean error into a
       null-pointer crash.  The tables that survive a table-less state (RFC
       and MP-Count) are released here for the same reason. */
    NodeLimit = 0;
    NodeSpc = 0;
    NodeUsed = 0;
    Avail = bddnull;
    VarSpc = 0;
    VarUsed = 0;
    if(RFCT){ free(RFCT); RFCT = 0; }
    RFCT_Spc = 0;
    RFCT_Used = 0;
    for(i=0; i<B_MP_LMAX; i++)
    {
      mptable[i].size = 0;
      mptable[i].used = 0;
      if(mptable[i].word){ free(mptable[i].word); mptable[i].word = 0; }
    }
    MPAllocFailSize = 0;
    MPCountOverflowed = 0;
    err("bddinit: Memory allocation failed", 0, ExceptionType::OutOfMemory);
  }

  /* Initialize */
  NodeUsed = 0;
  Node[NodeSpc-1U].varrfc = 0;
  B_SET_BDDP(Node[NodeSpc-1U].nx, bddnull);
  for(ix=0; ix<NodeSpc-1U; ix++)
  {
    Node[ix].varrfc = 0;
    B_SET_BDDP(Node[ix].nx, ix+1U);
  }
  Avail = 0;

  VarUsed = 0;
  for(i=0; i<VarSpc; i++)
  {
    Var[i].hashSpc = 0;
    Var[i].hashUsed = 0;
    Var[i].lev = i;
    VarID[i] = i;
#ifdef B_EXTEND
    Var[i].hash_64 = 0;
#elif defined(B_32)
    Var[i].hash_32 = 0;
#else
    Var[i].hash_32 = 0;
    Var[i].hash_h8 = 0;
#endif
  }

  /* Init RFC Table */
  if(RFCT){ free(RFCT); RFCT = 0; }
  RFCT_Spc = 0;
  RFCT_Used = 0;

  /* Init MP-Count Table */
  for(i=0; i<B_MP_LMAX; i++)
  {
    mptable[i].size = 0;
    mptable[i].used = 0;
    if(mptable[i].word) { free(mptable[i].word); mptable[i].word = 0; }
  }
  MPAllocFailSize = 0;
  MPCountOverflowed = 0;

  return 0;
}

/* The same validation serves bddsetcacheratio() and the cacheRatio argument
   of bddinit(), so the diagnostics name the function the user actually
   called instead of always blaming bddsetcacheratio(). */
[[noreturn]] static void ratio_err(const char *caller, const char *detail)
{
  char msg[128];
  snprintf(msg, sizeof(msg), "%s: %s", caller, detail);
  err(msg, 0, ExceptionType::OutOfRange);
}

void setcacheratiovalue(double ratio, const char *caller)
{
  const double epsilon = 1e-9;

  /* NaN compares false against every bound below, so it would pass all three
     range checks and reach the float-to-int conversion, which is undefined
     for a value no int can represent.  It has to be rejected first. */
  if (isnan(ratio)) {
    ratio_err(caller, "ratio is not a number");
  }

 /* Check if ratio is a power of 2 */
  if (ratio <= 0.0) {
    ratio_err(caller, "ratio must be positive");
  } else if (ratio > static_cast<double>(CACHE_RATIO_MAX)) {
    ratio_err(caller, "ratio exceeds maximum");
  } else if (ratio < 1.0 / static_cast<double>(CACHE_RATIO_MAX)) {
    ratio_err(caller, "ratio is too small");
  }

  if (ratio >= 1.0) {
    /* Rounded, not truncated, exactly as the inverse branch below does it:
       a ratio computed rather than written down can land just under the
       power of 2 it means (2.0 - 1ulp), and truncation would reject it. */
    int ratio_integer = static_cast<int>(ratio + epsilon);
    if (fabs(static_cast<double>(ratio_integer) - ratio) > epsilon
        || (ratio_integer & (ratio_integer - 1)) != 0) {
      ratio_err(caller, "ratio must be a power of 2");
    }
    CacheRatio = static_cast<double>(ratio_integer);
  } else {
    /* For ratios less than 1, we check if the inverse is a power of 2 */
    double inverse_ratio = 1.0 / ratio;
    int ratio_integer = static_cast<int>(inverse_ratio + epsilon);
    if (fabs(static_cast<double>(ratio_integer) - inverse_ratio) > epsilon
        || (ratio_integer & (ratio_integer - 1)) != 0) {
      ratio_err(caller, "ratio must be a power of 2");
    }
    CacheRatio = 1.0 / static_cast<double>(ratio_integer);
  }
}

/* Sizes the operation cache at CacheRatio times the node table size, rounded
   up to a power of 2, and moves the current entries over.  The cache never
   grows past 2^37 entries (the smallest power of 2 not below B_NODE_MAX/2,
   where the search below stops), so a large node table combined with a
   large ratio silently gets a smaller cache than asked for.
   Returns true if the cache is allocated successfully. */
bool allocatecache()
{
  bddp oldCacheSpc = 0;
  bddp newCacheSpc;
  struct B_CacheTable *newCache;
  bddp ix;
  struct B_CacheTable *cp, *cp1;

  if (Cache != NULL) {
    oldCacheSpc = CacheSpc;
  }

  /* Calculate new cache size */
  bddp targetCacheSize;
  double targetCacheSizeDouble = static_cast<double>(NodeSpc) * CacheRatio;
  /* The search below stops at B_NODE_MAX/2, so the target is clamped there
     too: clamping it to B_NODE_MAX instead would name a size the loop can
     never reach. */
  if (targetCacheSizeDouble > static_cast<double>(B_NODE_MAX >> 1U)) {
    targetCacheSize = B_NODE_MAX >> 1U;
  } else if (targetCacheSizeDouble < B_NODE_SPC0) {
    targetCacheSize = B_NODE_SPC0;
  } else {
    targetCacheSize = static_cast<bddp>(targetCacheSizeDouble);
  }

  /* Find the smallest power of 2 not less than targetCacheSize */
  for (newCacheSpc = B_NODE_SPC0; newCacheSpc < targetCacheSize
       && newCacheSpc < (B_NODE_MAX >> 1U);
        newCacheSpc <<= 1U) ;

  /* newCacheSpc must be a power of 2 */
  assert((newCacheSpc & (newCacheSpc - 1)) == 0);

  /* If size is different, reallocate cache */
  if (newCacheSpc != oldCacheSpc || Cache == NULL) {
    /* Allocate new cache */
    newCache = B_MALLOC(struct B_CacheTable, newCacheSpc);
    if (newCache == NULL) {
      return false;
    }

    if (Cache != NULL) { /* reallocate cache */
      /* Copy old cache to new cache */
      for (ix = 0; ix < oldCacheSpc && ix < newCacheSpc; ++ix) {
        cp = newCache + ix;
        cp1 = Cache + ix;
        cp->op = cp1->op;
        B_CPY_BDDP(cp->f, cp1->f);
        B_CPY_BDDP(cp->g, cp1->g);
        B_CPY_BDDP(cp->h, cp1->h);
      }
      if (newCacheSpc > oldCacheSpc) {
        /* assume that oldCacheSpc is the power of 2 */
        assert((oldCacheSpc & (oldCacheSpc - 1)) == 0);
        for (ix = oldCacheSpc; ix < newCacheSpc; ++ix) {
          cp = newCache + ix;
          cp1 = newCache + ix - oldCacheSpc;
          cp->op = cp1->op;
          B_CPY_BDDP(cp->f, cp1->f);
          B_CPY_BDDP(cp->g, cp1->g);
          B_CPY_BDDP(cp->h, cp1->h);
        }
      }
      free(Cache);
    } else {
      /* Initialize new cache.  f, g and h are cleared along with op because
         an enlargement copies whole entries, empty ones included, and reading
         the indeterminate contents of a fresh malloc to copy them is
         undefined behaviour (and a MemorySanitizer report) even though every
         reader tests op first. */
      for(ix=0; ix<newCacheSpc; ix++)
      {
        newCache[ix].op = BC_NULL;
        B_SET_BDDP(newCache[ix].f, bddnull);
        B_SET_BDDP(newCache[ix].g, bddnull);
        B_SET_BDDP(newCache[ix].h, bddnull);
      }
    }

    /* Update pointers */
    Cache = newCache;
    CacheSpc = newCacheSpc;
  }
  return true;
}

} // namespace sapporobdd
