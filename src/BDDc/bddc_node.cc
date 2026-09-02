/*****************************************
*  BDD Package (SAPPORO-1.94)   - Node  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

/* The three enlargements below grow their tables with B_REALLOC rather than
   with a fresh allocation and a copy.  realloc() extends the block in place
   when it can, and otherwise copies it itself; either way the old and the
   new table are never both needed at once, which the copy loops required
   (three times the old node table for a doubling), and that at exactly the
   moment the tables are enlarged because memory is running short.  A failed
   realloc() leaves the old block untouched, so every table stays valid when
   the enlargement is refused.  The error value reported with an enlargement
   failure is the number of entries the table was to have. */

void var_enlarge()
{
  bddvar i, newSpc;
  struct B_VarTable *newVar;
  bddvar *newVarID;

  /* Get new size */
  if(VarSpc == bddvarmax+1U)
    err("var_enlarge: var index range full", VarSpc, ExceptionType::OutOfRange);
#ifdef B_EXTEND
  // avoid overflow
  if((((unsigned long long)VarSpc) << 2U) > (unsigned long long)bddvarmax+1ULL)
    newSpc = bddvarmax+1U;
  else
    newSpc = VarSpc << 2U;
#else
  newSpc = VarSpc << 2U;
  if(newSpc > bddvarmax+1) newSpc = bddvarmax+1U;
#endif

  /* Enlarge space.  If the second table cannot be grown after the first
     one was, the first keeps its extra capacity: VarSpc still describes the
     smaller size and the next attempt simply grows both again. */
  newVar = B_REALLOC(Var, struct B_VarTable, newSpc);
  if(!newVar)
    err("var_enlarge: memory allocation failed", newSpc, ExceptionType::OutOfMemory);
  Var = newVar;
  newVarID = B_REALLOC(VarID, bddvar, newSpc);
  if(!newVarID)
    err("var_enlarge: memory allocation failed", newSpc, ExceptionType::OutOfMemory);
  VarID = newVarID;

  /* Initialize new space */
  for(i=VarSpc; i<newSpc; i++)
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
  VarSpc = newSpc;
}

int node_enlarge()
/* Returns 1 if not enough memory */
{
  bddp i, newSpc;
  struct B_NodeTable *newNode;

  /* Get new size */
  if(NodeSpc >= NodeLimit) return 1; /* Cannot enlarge */
  newSpc = NodeSpc << 1U;
  if(newSpc > NodeLimit) newSpc = NodeLimit;

  /* Enlarge space.  The free slots carry indeterminate f0/f1 fields (only
     varrfc and nx are initialized); realloc() moves the bytes as they are,
     which is well defined, whereas the field-by-field copy this replaces
     read those indeterminate values. */
  newNode = B_REALLOC(Node, struct B_NodeTable, newSpc);
  if(!newNode) return 1; /* Not enough memory */
  Node = newNode;

  /* Initialize new space */
  Node[newSpc-1U].varrfc = 0;
  B_SET_BDDP(Node[newSpc-1U].nx, Avail);
  for(i=NodeSpc; i<newSpc-1U; i++)
  {
    Node[i].varrfc = 0;
    B_SET_BDDP(Node[i].nx, i+1U);
  }
  Avail = NodeSpc;
  NodeSpc = newSpc;

  /* Realloc Cache */
  allocatecache();
  /* if allocatecache returned false, only NodeTable has been enlarged */
  return 0;
}

void hash_enlarge(bddvar v)
/* Doubles the hash table of variable v and rehashes its chains.  Throws
   BDDOutOfMemoryException, with the table unchanged, if the new table cannot
   be allocated; a table that has reached the largest size is left alone. */
{
  struct B_NodeTable *np, *np0;
  struct B_VarTable *varp;
  bddp i, oldSpc, newSpc, nx, key, f0, f1;
#ifdef B_EXTEND
  bddp_64 *newhash_64, *p_64;
#elif defined(B_32)
  bddp_32 *newhash_32, *p_32;
#else
  bddp_32 *newhash_32, *p_32;
  bddp_h8 *newhash_h8, *p_h8;
#endif

  varp = &Var[v];
  /* Get new size */
  oldSpc = varp->hashSpc;
  if(oldSpc == B_NODE_MAX + 1U)
    return; /*  Cancel enlarging */
  newSpc = oldSpc << 1U;

  /* Enlarge space */
#ifdef B_EXTEND
  newhash_64 = B_REALLOC(varp->hash_64, bddp_64, newSpc);
  if(!newhash_64)
    err("hash_enlarge: not enough memory for hash table", newSpc, ExceptionType::OutOfMemory);
  varp->hash_64 = newhash_64;
#elif defined(B_32)
  newhash_32 = B_REALLOC(varp->hash_32, bddp_32, newSpc);
  if(!newhash_32)
    err("hash_enlarge: not enough memory for hash table", newSpc, ExceptionType::OutOfMemory);
  varp->hash_32 = newhash_32;
#else
  /* as in var_enlarge(): a first table grown ahead of a failed second one
     keeps its capacity, hashSpc still describes the smaller size */
  newhash_32 = B_REALLOC(varp->hash_32, bddp_32, newSpc);
  if(!newhash_32)
    err("hash_enlarge: not enough memory for hash table", newSpc, ExceptionType::OutOfMemory);
  varp->hash_32 = newhash_32;
  newhash_h8 = B_REALLOC(varp->hash_h8, bddp_h8, newSpc);
  if(!newhash_h8)
    err("hash_enlarge: not enough memory for hash table", newSpc, ExceptionType::OutOfMemory);
  varp->hash_h8 = newhash_h8;
#endif
  varp->hashSpc = newSpc;

  /* Initialize new hash entry */
  for(i=oldSpc; i<newSpc; i++)
  {
    B_SET_NXP(p, varp->hash, i);
    B_SET_BDDP(*p, bddnull);
  }

  /* restore hash entry */
  for(i=0; i<oldSpc; i++)
  {
    np0 = 0;
    B_SET_NXP(p, varp->hash, i);
    nx = B_GET_BDDP(*p);
    while(nx != bddnull)
    {
      np = Node + nx;
      f0 = B_GET_BDDP(np->f0);
      f1 = B_GET_BDDP(np->f1);
      key = B_HASHKEY(f0, f1, newSpc);
      if(key == i) np0 = np;
      else
      {
        if(np0) B_CPY_BDDP(np0->nx, np->nx);
        else { B_SET_NXP(p, varp->hash, i); B_CPY_BDDP(*p, np->nx); }
        B_SET_NXP(p, varp->hash, key);
        B_CPY_BDDP(np->nx, *p);
        B_SET_BDDP(*p, nx);
      }
      if(np0) nx = B_GET_BDDP(np0->nx);
      else { B_SET_NXP(p, varp->hash, i); nx = B_GET_BDDP(*p); }
    }
  }
}

bddp getnode(bddvar v, bddp f0, bddp f1)
/* Returns the node (v, f0, f1), shared with an existing one when there is
   one.  Throws BDDOutOfMemoryException if neither a hash table nor a node
   can be secured.  Preconditions (see bddc_internal.h): v is a variable in
   use and f0/f1 are valid bddp values carrying one reference each; the
   elimination and negative-edge rules have been applied by the caller.  The
   levels of f0 and f1 are not checked here: bddpush() builds SeqBDD nodes,
   whose children may carry the same or a higher variable. */
{
  struct B_NodeTable *np, *fp;
  struct B_VarTable *varp;
  bddp ix, nx, key;
#ifdef B_EXTEND
  bddp_64 *p_64;
#elif defined(B_32)
  bddp_32 *p_32;
#else
  bddp_32 *p_32;
  bddp_h8 *p_h8;
#endif

  assert(v != 0 && v <= VarUsed);
  assert(f0 != bddnull && f1 != bddnull);
  assert(B_CST(f0) || !B_BAD_NODE(f0));
  assert(B_CST(f1) || !B_BAD_NODE(f1));

  varp = &Var[v];
  if(varp->hashSpc == 0)
  /* Create hash-table */
  {
#ifdef B_EXTEND
    varp->hash_64 = B_MALLOC(bddp_64, B_HASH_SPC0);
    if(!varp->hash_64) err("getnode: not enough memory for hash table", B_HASH_SPC0, ExceptionType::OutOfMemory);
#elif defined(B_32)
    varp->hash_32 = B_MALLOC(bddp_32, B_HASH_SPC0);
    if(!varp->hash_32) err("getnode: not enough memory for hash table", B_HASH_SPC0, ExceptionType::OutOfMemory);
#else
    varp->hash_32 = B_MALLOC(bddp_32, B_HASH_SPC0);
    if(!varp->hash_32) err("getnode: not enough memory for hash table", B_HASH_SPC0, ExceptionType::OutOfMemory);
    varp->hash_h8 = B_MALLOC(bddp_h8, B_HASH_SPC0);
    if(!varp->hash_h8)
    {
      /* The pointer has to be cleared along with the block: bddinit() frees
         every non-null hash pointer when it starts over, which is the
         documented way to recover from this exception, and a dangling one
         was freed a second time there. */
      free(varp->hash_32);
      varp->hash_32 = 0;
      err("getnode: not enough memory for hash table", B_HASH_SPC0, ExceptionType::OutOfMemory);
    }
#endif
    for(ix=0; ix<B_HASH_SPC0; ix++)
    {
      B_SET_NXP(p, varp->hash, ix);
      B_SET_BDDP(*p, bddnull);
    }
    varp->hashSpc = B_HASH_SPC0;
    key = B_HASHKEY(f0, f1, varp->hashSpc);
  }
  else
  /* Looking for equivalent existing node */
  {
    key = B_HASHKEY(f0, f1, varp->hashSpc);
    B_SET_NXP(p, varp->hash, key);
    nx = B_GET_BDDP(*p);
    while(nx != bddnull)
    {
      np = Node + nx;
      if(f0 == B_GET_BDDP(np->f0) &&
         f1 == B_GET_BDDP(np->f1) )
      {
        /* Sharing equivalent node.  The new reference to np is taken
           before the references to f0 and f1 are handed back: taking it can
           throw (a saturated counter needs an RFC-table entry, and the table
           may fail to grow), and the caller then releases f0 and f1 itself,
           so they must not have been released here already.  Releasing a
           valid reference cannot throw. */
        B_RFC_INC_NP(np);
        if(!B_CST(f0)) { fp = B_NP(f0); B_RFC_DEC_NP(fp); }
        if(!B_CST(f1)) { fp = B_NP(f1); B_RFC_DEC_NP(fp); }
        return B_BDDP_NP(np);
      }
      nx = B_GET_BDDP(np->nx);
    }
  }

  /* Check hash-table overflow.  hashUsed counts the entries that are really
     in the table, so it is incremented only once both the hash table and the
     node table are secured; an exception thrown on the way out would
     otherwise leave the counter permanently too high. */
  if(varp->hashUsed + 1U >= varp->hashSpc)
  {
    hash_enlarge(v); /* throws when it cannot grow the table */
    key = B_HASHKEY(f0, f1, varp->hashSpc);
  }

  /* Check node-table overflow */
  if(NodeUsed >= NodeSpc-1U)
  {
    if(node_enlarge())
    {
      /* The table cannot grow, so collect garbage.  bddgc() answers 1 when
         nothing was freed -- and also when fewer nodes than the GC threshold
         were freed, although those are free now.  The threshold is the
         caller's instruction (bddsetgcthreshold()) to give up at that point
         rather than to go on at the limit with a full sweep of the tables
         for every handful of nodes, so a collection below it is reported as
         out of memory even though a slot is available. */
      if(bddgc())
        err("getnode: not enough memory for node table", NodeLimit,
            ExceptionType::OutOfMemory); /* Node-table overflow */
      key = B_HASHKEY(f0, f1, varp->hashSpc);
    }
    /* Node-table enlarged or GC succeeded */
  }
  varp->hashUsed++;
  NodeUsed++;

  /* Creating a new node */
  nx = Avail;
  np = Node + nx;
  Avail = B_GET_BDDP(np->nx);
  B_SET_NXP(p, varp->hash, key);
  B_CPY_BDDP(np->nx, *p);
  B_SET_BDDP(*p, nx);
  B_SET_BDDP(np->f0, f0);
  B_SET_BDDP(np->f1, f1);
  np->varrfc = v;
  B_RFC_INC_NP(np);
  return B_BDDP_NP(np);
}

bddp getbddp(bddvar v, bddp f0, bddp f1)
/* BDD node (v, f0, f1) after the elimination rule and the negative-edge
   rule.  Never returns bddnull: memory exhaustion is thrown by getnode(). */
{
  struct B_NodeTable *fp;

  /* Check elimination rule */
  if(f0 == f1)
  {
    if(!B_CST(f0)) { fp = B_NP(f0); B_RFC_DEC_NP(fp); }
    return f0;
  }

  /* Negative edge constraint */
  if(B_NEG(f0)) return B_NOT(getnode(v, B_NOT(f0), B_NOT(f1)));
  return getnode(v, f0, f1);
}

bddp getzddp(bddvar v, bddp f0, bddp f1)
/* ZDD node (v, f0, f1) after the elimination rule and the negative-edge
   rule.  Never returns bddnull: memory exhaustion is thrown by getnode(). */
{
  /* Check elimination rule */
  if(f1 == bddfalse) return f0;

  /* Negative edge constraint */
  if(B_NEG(f0)) return B_NOT(getnode(v, f0, f1));
  return getnode(v, B_NOT(f0), f1);
}

} // namespace sapporobdd
