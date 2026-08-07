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

/* RFC-table */
struct B_RFC_Table *RFCT = 0;
bddp RFCT_Spc = 0;
bddp RFCT_Used = 0;

/* MP-Count Table */
struct B_MPTable mptable[B_MP_LMAX] = {{0, 0, NULL}};
bddp MPAllocFailSize = 0;

/* ------------------ External functions ------------------ */
int bddinit(bddp initsize, bddp limitsize, double cacheRatio)
/* Returns 1 if not enough memory (usually 0) */
{
  bddp   ix;
  bddvar i;
  bool cacheallocated = false;

  /* Set cache ratio if specified */
  if(cacheRatio > 0.0) {
    /* throw an exeption if cacheRatio is illegal */
    setcacheratiovalue(cacheRatio);
  } else {
    CacheRatio = 0.5; /* Default cache ratio */
  }

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
    NodeLimit = 0;
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

  return 0;
}

void setcacheratiovalue(double ratio)
{
  const double epsilon = 1e-9;

 /* Check if ratio is a power of 2 */
  if (ratio <= 0.0) {
    err("bddsetcacheratio: ratio must be positive", 0, ExceptionType::OutOfRange);
  } else if (ratio > static_cast<double>(CACHE_RATIO_MAX)) {
    err("bddsetcacheratio: ratio exceeds maximum", 0, ExceptionType::OutOfRange);
  } else if (ratio < 1.0 / static_cast<double>(CACHE_RATIO_MAX)) {
    err("bddsetcacheratio: ratio is too small", 0, ExceptionType::OutOfRange);
  }

  if (ratio >= 1.0) {
    int ratio_integer = static_cast<int>(ratio);
    if (fabs(static_cast<double>(ratio_integer) - ratio) > epsilon
        || (ratio_integer & (ratio_integer - 1)) != 0) {
      err("bddsetcacheratio: ratio must be a power of 2", 0, ExceptionType::OutOfRange);
    }
    CacheRatio = static_cast<double>(ratio_integer);
  } else {
    /* For ratios less than 1, we check if the inverse is a power of 2 */
    double inverse_ratio = 1.0 / ratio;
    int ratio_integer = static_cast<int>(inverse_ratio + epsilon);
    if (fabs(static_cast<double>(ratio_integer) - inverse_ratio) > epsilon
        || (ratio_integer & (ratio_integer - 1)) != 0) {
      err("bddsetcacheratio: ratio must be a power of 2", 0, ExceptionType::OutOfRange);
    }
    CacheRatio = 1.0 / static_cast<double>(ratio_integer);
  }
}

// return true if cache is allocated successfully
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
  if (targetCacheSizeDouble > B_NODE_MAX) {
    targetCacheSize = B_NODE_MAX;
  } else if (targetCacheSizeDouble < B_NODE_SPC0) {
    targetCacheSize = B_NODE_SPC0;
  } else {
    targetCacheSize = static_cast<bddp>(targetCacheSizeDouble);
  }

  /* Find the smallest power of 2 exceeding targetCacheSize */
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
      /* Initialize new cache */
      for(ix=0; ix<newCacheSpc; ix++) newCache[ix].op = BC_NULL;
    }

    /* Update pointers */
    Cache = newCache;
    CacheSpc = newCacheSpc;
  }
  return true;
}

} // namespace sapporobdd
