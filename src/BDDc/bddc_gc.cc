/*****************************************
*  BDD Package (SAPPORO-1.94)   - GC    *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

/* Frees one node whose reference count has reached 0: unlinks it from the
   hash chain of its variable, puts its slot on the free list and drops the
   references it holds on its children.  The children whose count reaches 0
   in turn are returned through c0/c1 (0 when there is none); it is the
   caller's job to free them. */
static void gc_free_node(struct B_NodeTable *np,
                         struct B_NodeTable **c0, struct B_NodeTable **c1)
{
  bddp key, nx1, f0, f1;
  struct B_VarTable *varp;
  struct B_NodeTable *np1, *np2;
#ifdef B_EXTEND
  bddp_64 *p_64;
#elif defined(B_32)
  bddp_32 *p_32;
#else
  bddp_32 *p_32;
  bddp_h8 *p_h8;
#endif

  *c0 = 0;
  *c1 = 0;

  /* remove the node from hash list.  The chain is walked by index, and the
     pointer is formed only for an index that names a node: Node + bddnull
     is outside the table and undefined to compute even when never
     dereferenced. */
  varp = Var + B_VAR_NP(np);
  f0 = B_GET_BDDP(np->f0);
  f1 = B_GET_BDDP(np->f1);
  key = B_HASHKEY(f0, f1, varp->hashSpc);
  B_SET_NXP(p, varp->hash, key);
  nx1 = B_GET_BDDP(*p);
  if(nx1 == bddnull)
    err("gc1: Fail to find the node to be deleted", np-Node, ExceptionType::InternalError);
  np1 = Node + nx1;

  if(np1 == np) B_CPY_BDDP(*p, np->nx);
  else
  {
    while(np1 != np)
    {
      np2 = np1;
      nx1 = B_GET_BDDP(np2->nx);
      if(nx1 == bddnull)
        err("gc1: Fail to find the node to be deleted", np-Node, ExceptionType::InternalError);
      np1 = Node + nx1;
    }
    B_CPY_BDDP(np2->nx, np->nx);
  }
  varp->hashUsed--;

  /* append the node to avail list */
  B_SET_BDDP(np->nx, Avail);
  Avail = np - Node;

  NodeUsed--;
  np->varrfc = 0;

  /* Drop the references to the sub-graphs */
  if(!B_CST(f0))
  {
    np1 = B_NP(f0);
    B_RFC_DEC_NP(np1);
    if(B_RFC_ZERO_NP(np1)) *c0 = np1;
  }
  if(!B_CST(f1))
  {
    np1 = B_NP(f1);
    B_RFC_DEC_NP(np1);
    /* the two edges may share a child (a ZDD node may have f0 == f1); its
       count reached 0 on one of the two decrements, so it is reported once */
    if(B_RFC_ZERO_NP(np1) && np1 != *c0) *c1 = np1;
  }
}

/* The recursive form of gc1(), used only when the explicit stack below
   cannot be grown.  It descends one machine frame per node and is bounded by
   the recursion limiter. */
static void gc1_recursive(struct B_NodeTable *np)
{
  struct B_NodeTable *c0, *c1;

  gc_free_node(np, &c0, &c1);
  if(c0) { BDD_RECUR_INC; gc1_recursive(c0); BDD_RECUR_DEC; }
  if(c1) { BDD_RECUR_INC; gc1_recursive(c1); BDD_RECUR_DEC; }
}

#define GC_STACK_SPC0 256

void gc1(struct B_NodeTable *np)
{
  /* np is a node ptr to be collected. (refc == 0).  The nodes it releases
     are collected in turn, through a work list on the heap rather than by
     recursion: the recursion went one frame deeper per level of the graph,
     so on a graph deeper than the recursion limit it was cut off by the
     limiter in the middle of a collection -- from inside getnode(), with
     the operation cache still pointing at the nodes freed so far -- and
     with a limit set below the depth of the graph a collection could never
     complete at all.  If the work list cannot grow, the child is freed by
     the recursive form instead, so the collection never stalls. */
  struct B_NodeTable **stack, **newstack;
  struct B_NodeTable *c0, *c1;
  bddp top, spc;

  stack = B_MALLOC(struct B_NodeTable *, GC_STACK_SPC0);
  if(!stack) { gc1_recursive(np); return; }
  spc = GC_STACK_SPC0;
  top = 0;
  stack[top++] = np;

  try
  {
    while(top > 0)
    {
      np = stack[--top];
      gc_free_node(np, &c0, &c1);
      if(c0 == 0) c0 = c1, c1 = 0;
      if(c0 == 0) continue;
      if(top + 2 > spc)
      {
        newstack = B_REALLOC(stack, struct B_NodeTable *, spc << 1U);
        if(!newstack)
        {
          /* no room to defer the children: free them right away */
          gc1_recursive(c0);
          if(c1) gc1_recursive(c1);
          continue;
        }
        stack = newstack;
        spc <<= 1U;
      }
      stack[top++] = c0;
      if(c1) stack[top++] = c1;
    }
  }
  catch(...)
  {
    free(stack);
    throw;
  }
  free(stack);
}

/* Rebuilds the RFC-table with only the entries that are still needed: an
   entry serves a node whose inline counter is saturated, and once that
   counter has come down again (or the node has been freed and its slot
   reused) the entry is dead.  Nothing used to remove the dead entries, so
   the table only ever grew, by a factor of 4 each time, with every node that
   had once been shared 4095 times (the inline counter of the default build
   has 12 bits).  A rebuild that cannot allocate its table keeps the old one:
   dead entries are harmless, they only cost space. */
static void rfc_table_pack(void)
{
  struct B_RFC_Table *newRFCT;
  bddp ix, nx, key, live, newSpc;

  if(RFCT_Spc == 0) return;
  live = 0;
  for(ix=0; ix<RFCT_Spc; ix++)
  {
    nx = B_GET_BDDP((RFCT+ix)->nx);
    if(nx != bddnull && (Node+nx)->varrfc >= B_RFC_MASK) live++;
  }
  /* the smallest power of 2 that keeps the load below one half */
  for(newSpc = B_RFCT_SPC0; newSpc < (live << 1U) + 1U; newSpc <<= 1U) ;
  if(newSpc == RFCT_Spc && live == RFCT_Used) return;

  newRFCT = B_MALLOC(struct B_RFC_Table, newSpc);
  if(!newRFCT) return;
  for(ix=0; ix<newSpc; ix++)
  {
    B_SET_BDDP((newRFCT+ix)->nx, bddnull);
    B_SET_BDDP((newRFCT+ix)->rfc, (bddp)0);
  }
  for(ix=0; ix<RFCT_Spc; ix++)
  {
    bddp rfc;
    nx = B_GET_BDDP((RFCT+ix)->nx);
    if(nx == bddnull || (Node+nx)->varrfc < B_RFC_MASK) continue;
    key = nx & (newSpc-1);
    while(B_GET_BDDP((newRFCT+key)->nx) != bddnull) key = (key+1) & (newSpc-1);
    B_SET_BDDP((newRFCT+key)->nx, nx);
    rfc = B_GET_BDDP((RFCT+ix)->rfc);
    B_SET_BDDP((newRFCT+key)->rfc, rfc);
  }
  free(RFCT);
  RFCT = newRFCT;
  RFCT_Spc = newSpc;
  RFCT_Used = live;
}

/* Sweeps the operation cache and the multi-precision count table after
   nodes have been freed: an entry naming a freed node would otherwise hand
   the slot's next occupant out as a result. */
static void cache_sweep(void)
{
  bddp i, f;
  struct B_NodeTable *fp;
  struct B_CacheTable *cachep;

  /* An entry is dropped when a node it names is free -- or is not in the
     table at all: such an entry cannot arise, but a check that keeps the
     impossible entry instead of dropping it, as the pointer comparison this
     replaces did, is no defence. */
#define GC_DEAD_REF(f) \
  (!B_CST(f) && (B_NDX(f) >= NodeSpc || (fp = B_NP(f))->varrfc == 0))

  for(cachep=Cache; cachep<Cache+CacheSpc; cachep++)
  {
    switch(cachep->op)
    {
    case BC_NULL:
      break;
    /* f, g and h are node references (BC_SUPPORT keeps bddfalse in g,
       which passes as a constant) */
    case BC_AND:
    case BC_XOR:
    case BC_INTERSEC:
    case BC_UNION:
    case BC_SUBTRACT:
    case BC_COFACTOR:
    case BC_UNIV:
    case BC_SUPPORT:
      f = B_GET_BDDP(cachep->f);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      f = B_GET_BDDP(cachep->g);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      f = B_GET_BDDP(cachep->h);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      break;
    /* g holds a VarID or a shift count, not a bddp, so only f and h are
       node references */
    case BC_AT0:
    case BC_AT1:
    case BC_OFFSET:
    case BC_ONSET:
    case BC_CHANGE:
    case BC_LSHIFT:
    case BC_RSHIFT:
      f = B_GET_BDDP(cachep->f);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      f = B_GET_BDDP(cachep->h);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      break;
    /* h is a number; a value above bddnull is a BC_CARD2 reference into the
       multi-precision table, which is cleared below */
    case BC_CARD:
    case BC_LIT:
    case BC_LEN:
      f = B_GET_BDDP(cachep->f);
      if(GC_DEAD_REF(f)) { cachep->op = BC_NULL; break; }
      f = B_GET_BDDP(cachep->h);
      if(f > bddnull) { cachep->op = BC_NULL; break; }
      break;
    /* user-defined operations (bddwcache()): the meaning of f, g and h is
       the user's, so the entry is dropped whatever it holds */
    default:
      cachep->op = BC_NULL;
      break;
    }
  }
#undef GC_DEAD_REF

  /* MP-Count table clear */
  for(i=0; i<B_MP_LMAX; i++)
  {
    mptable[i].size = 0;
    mptable[i].used = 0;
    free(mptable[i].word);
    mptable[i].word = 0;
  }
}

int bddgc()
/* Collects the nodes that are no longer referenced.  Returns 1 if nothing
   could be freed, or if fewer nodes than the GC threshold were freed (see
   bddsetgcthreshold()); returns 0 otherwise.  In the second case the freed
   slots are usable all the same; the threshold is a policy of the caller,
   which getnode() honours by reporting the operation as out of memory
   rather than thrashing at the node limit. */
{
  bddp i, n;
  struct B_NodeTable *fp;
  struct B_NodeTable *np;
  struct B_VarTable *varp;
  bddvar v;
  bddp oldSpc, newSpc, nx, key;
#ifdef B_EXTEND
  bddp_64 *newhash_64, *p_64, *p2_64;
#elif defined(B_32)
  bddp_32 *newhash_32, *p_32, *p2_32;
#else
  bddp_32 *newhash_32, *p_32, *p2_32;
  bddp_h8 *newhash_h8, *p_h8, *p2_h8;
#endif

  n = NodeUsed;
  /* Once any node has been collected, its slot may be reused by a later
     allocation, so the operation caches and the MP-Count table, which hold
     raw bddp values, must be swept -- even when the collection is cut short
     by an exception (the recursive fallback of gc1() can hit the recursion
     limit, and a corrupt reference count is reported from here), and even
     when the GC is reported as failed below because of GCThreshold.  Only
     the hash-table packing, a pure memory optimization, may be skipped.  A
     collection that was cut short leaves nodes with a zero count behind;
     they are consistent and the next collection takes them. */
  try
  {
    for(fp=Node; fp<Node+NodeSpc; fp++)
      if(fp->varrfc != 0 && B_RFC_ZERO_NP(fp))
        gc1(fp);
  }
  catch(...)
  {
    if(n != NodeUsed) cache_sweep();
    throw;
  }

  bddp freedNodes = n - NodeUsed;
  if(freedNodes == 0) return 1; /* No free node */

  cache_sweep();

  /* Report the GC as failed when it freed too few nodes: at the node limit,
     getnode() then gives up instead of thrashing on tiny GCs. */
  if(GCThreshold > 0 && freedNodes <= GCThreshold) {
    return 1;
  }

  rfc_table_pack();

  /* Hash-table packing.  A table that cannot be reallocated is skipped;
     the next variable's table may well be small enough. */
  for(v=1; v<=VarUsed; v++)
  {
    varp = &Var[v];

    /* Get new size */
    oldSpc = varp->hashSpc;
    newSpc = oldSpc;
    while(newSpc > B_HASH_SPC0)
    {
      if(newSpc>>2 < varp->hashUsed) break;
      newSpc >>= 1;
    }
    if(newSpc == oldSpc) continue;

    /* Reduce space */
#ifdef B_EXTEND
    newhash_64 = B_MALLOC(bddp_64, newSpc);
    if(!newhash_64) continue; /* Not enough memory */
#elif defined(B_32)
    newhash_32 = B_MALLOC(bddp_32, newSpc);
    if(!newhash_32) continue; /* Not enough memory */
#else
    newhash_32 = B_MALLOC(bddp_32, newSpc);
    newhash_h8 = B_MALLOC(bddp_h8, newSpc);
    if(!newhash_32 || !newhash_h8)
    {
      if(newhash_32) free(newhash_32);
      if(newhash_h8) free(newhash_h8);
      continue; /* Not enough memory */
    }
#endif

    /* Initialize new hash entry */
    for(i=0; i<newSpc; i++)
    {
      B_SET_NXP(p, newhash, i);
      B_SET_BDDP(*p, bddnull);
    }

    /* restore hash entry.  The old buckets that fall into new bucket "key"
       are exactly key, key+newSpc, key+2*newSpc, ..., so walking them in that
       order lets the tail of the chain assembled so far be carried from one
       old bucket to the next.  Rescanning the merged chain from its head for
       every old bucket, as an index-major loop has to, costs time quadratic
       in the length of the merged chain.  The concatenation order, and hence
       the resulting chains, are the same as before. */
    for(key=0; key<newSpc; key++)
    {
      np = 0; /* last node of the chain built in newhash[key] so far */
      for(i=key; i<oldSpc; i+=newSpc)
      {
        B_SET_NXP(p2, varp->hash, i);
        nx = B_GET_BDDP(*p2);
        if(nx == bddnull) continue; /* nothing to concatenate */
        if(np) B_CPY_BDDP(np->nx, *p2);
        else { B_SET_NXP(p, newhash, key); B_CPY_BDDP(*p, *p2); }
        do
        {
          np = Node + nx;
          nx = B_GET_BDDP(np->nx);
        } while(nx != bddnull);
      }
    }
    varp->hashSpc = newSpc;
#ifdef B_EXTEND
    free(varp->hash_64);
    varp->hash_64 = newhash_64;
#elif defined(B_32)
    free(varp->hash_32);
    varp->hash_32 = newhash_32;
#else
    free(varp->hash_32);
    varp->hash_32 = newhash_32;
    free(varp->hash_h8);
    varp->hash_h8 = newhash_h8;
#endif
  }
  return 0;
}

/* Grows the RFC-table to 4 times its size.  Returns 0 on success and 1 if
   the new table cannot be allocated, in which case the old table and every
   global describing it are left as they were.  The size used to be
   quadrupled and the pointer cleared before the allocation was attempted,
   so a failure left RFCT null under a non-zero RFCT_Spc, and the next
   saturated counter dereferenced the null pointer instead of finding the
   exception it had been promised. */
static int rfc_table_enlarge(void)
{
  struct B_RFC_Table *newRFCT;
  bddp ix, nx, nx2, key, rfc, newSpc;

  newSpc = RFCT_Spc << 2;
  newRFCT = B_MALLOC(struct B_RFC_Table, newSpc);
  if(!newRFCT) return 1;
  for(ix=0; ix<newSpc; ix++)
  {
    B_SET_BDDP((newRFCT+ix)->nx, bddnull);
    B_SET_BDDP((newRFCT+ix)->rfc, (bddp)0);
  }
  for(ix=0; ix<RFCT_Spc; ix++)
  {
    nx = B_GET_BDDP((RFCT+ix)->nx);
    if(nx == bddnull) continue;
    key = nx & (newSpc-1);
    nx2 = B_GET_BDDP((newRFCT+key)->nx);
    while(nx2 != bddnull)
    {
      key = (key+1) & (newSpc-1);
      nx2 = B_GET_BDDP((newRFCT+key)->nx);
    }
    B_SET_BDDP((newRFCT+key)->nx, nx);
    rfc = B_GET_BDDP((RFCT+ix)->rfc);
    B_SET_BDDP((newRFCT+key)->rfc, rfc);
  }
  free(RFCT);
  RFCT = newRFCT;
  RFCT_Spc = newSpc;
  return 0;
}

int rfc_inc_ovf(struct B_NodeTable *np)
/* Increments the reference count of a node whose inline counter is
   saturated (or about to be), keeping the excess in the RFC-table.  Throws
   BDDOutOfMemoryException, with the count unchanged, if the table cannot be
   created or is full and cannot grow. */
{
  bddp ix, nx, nx2, key, rfc;

  if(RFCT_Spc == 0)
  {
    /* Create RFC-table */
    RFCT = B_MALLOC(struct B_RFC_Table, B_RFCT_SPC0);
    if(!RFCT)
      err("B_RFC_INC_NP: rfc memory over flow", np-Node, ExceptionType::OutOfMemory);
    for(ix=0; ix<B_RFCT_SPC0; ix++)
    {
      B_SET_BDDP((RFCT+ix)->nx, bddnull);
      B_SET_BDDP((RFCT+ix)->rfc, (bddp)0);
    }
    RFCT_Spc = B_RFCT_SPC0;
  }

  nx = np - Node;
  key = nx & (RFCT_Spc-1);
  nx2 = B_GET_BDDP((RFCT+key)->nx);
  while(nx2 != bddnull)
  {
    if(nx == nx2)
    {
      if(np->varrfc < B_RFC_MASK)
      {
        rfc = 0;
        np->varrfc += B_RFC_UNIT;
      }
      else
      {
        rfc = B_GET_BDDP((RFCT+key)->rfc);
        /* the field is narrower than a count could in theory become; a
           wrapped count would free a live node later on */
        if(rfc >= B_RFCT_MAX)
          err("B_RFC_INC_NP: rfc over flow", np-Node, ExceptionType::OutOfRange);
        rfc++;
      }
      B_SET_BDDP((RFCT+key)->rfc, rfc);
      return 0;
    }
    key = (key+1) & (RFCT_Spc-1);
    nx2 = B_GET_BDDP((RFCT+key)->nx);
  }

  /* new rfc entry.  The table is grown before the entry goes in, so that a
     refused enlargement can still be reported before anything has changed;
     it is only fatal when the table would otherwise become full, since the
     linear probing above needs an empty slot to stop at. */
  if(((RFCT_Used + 1) << 1) >= RFCT_Spc)
  {
    if(rfc_table_enlarge())
    {
      if(RFCT_Used + 1 >= RFCT_Spc)
        err("B_RFC_INC_NP: rfc memory over flow", np-Node, ExceptionType::OutOfMemory);
    }
    else
    {
      key = nx & (RFCT_Spc-1);
      while(B_GET_BDDP((RFCT+key)->nx) != bddnull) key = (key+1) & (RFCT_Spc-1);
    }
  }
  B_SET_BDDP((RFCT+key)->nx, nx);
  B_SET_BDDP((RFCT+key)->rfc, (bddp)0);
  np->varrfc += B_RFC_UNIT;
  RFCT_Used++;

  return 0;
}

int rfc_dec_ovf(struct B_NodeTable *np)
/* Decrements the reference count of a node whose inline counter is
   saturated.  The node then has an RFC-table entry by construction; a
   missing table or entry means the counts are corrupt, and is reported
   rather than silently dropping the decrement (which used to leave the
   node uncollectable for good). */
{
  bddp nx, key, nx2, rfc;

  if(RFCT_Spc == 0)
    err("B_RFC_DEC_NP: rfc table missing", np-Node, ExceptionType::InternalError);
  nx = np - Node;
  key = nx & (RFCT_Spc-1);
  nx2 = B_GET_BDDP((RFCT+key)->nx);
  while(nx2 != bddnull)
  {
    if(nx == nx2)
    {
      rfc = B_GET_BDDP((RFCT+key)->rfc);
      if(rfc == 0)
      {
        np->varrfc -= B_RFC_UNIT;
        return 0;
      }
      B_SET_BDDP((RFCT+key)->rfc, rfc-1);
      return 0;
    }
    key = (key+1) & (RFCT_Spc-1);
    nx2 = B_GET_BDDP((RFCT+key)->nx);
  }
  err("B_RFC_DEC_NP: rfc entry missing", np-Node, ExceptionType::InternalError);
}

} // namespace sapporobdd
