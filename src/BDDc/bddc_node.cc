/*****************************************
*  BDD Package (SAPPORO-1.94)   - Node  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

void var_enlarge()
{
  bddvar i, newSpc;
  struct B_VarTable *newVar;
  unsigned int *newVarID;

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

  /* Enlarge space */
  newVar = 0;
  newVarID = 0;
  newVar = B_MALLOC(struct B_VarTable, newSpc);
  newVarID = B_MALLOC(unsigned int, newSpc);
  if(newVar && newVarID)
  {
    for(i=0; i<VarSpc; i++)
    {
      newVar[i].hashSpc = Var[i].hashSpc;
      newVar[i].hashUsed = Var[i].hashUsed;
      newVar[i].lev = Var[i].lev;
      newVarID[i] = VarID[i];
#ifdef B_EXTEND
      newVar[i].hash_64 = Var[i].hash_64;
#elif defined(B_32)
      newVar[i].hash_32 = Var[i].hash_32;
#else
      newVar[i].hash_32 = Var[i].hash_32;
      newVar[i].hash_h8 = Var[i].hash_h8;
#endif
    }
    free(Var);
    free(VarID);
    Var = newVar;
    VarID = newVarID;
  }
  else
  {
    if(newVar) free(newVar);
    if(newVarID) free(newVarID);
    err("var_enlarge: memory allocation failed", VarSpc, ExceptionType::OutOfMemory);
  }

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

  /* Enlarge space */
  newNode = 0;
  newNode = B_MALLOC(struct B_NodeTable, newSpc);
  if(newNode)
  {
    for(i=0; i<NodeSpc; i++)
    {
#ifdef B_EXTEND
      newNode[i].f0_64 = Node[i].f0_64;
      newNode[i].f1_64 = Node[i].f1_64;
      newNode[i].nx_64 = Node[i].nx_64;
      newNode[i].varrfc = Node[i].varrfc;
#elif defined(B_32)
      newNode[i].varrfc = Node[i].varrfc;
      newNode[i].f0_32 = Node[i].f0_32;
      newNode[i].f1_32 = Node[i].f1_32;
      newNode[i].nx_32 = Node[i].nx_32;
#else
      newNode[i].varrfc = Node[i].varrfc;
      newNode[i].f0_32 = Node[i].f0_32;
      newNode[i].f1_32 = Node[i].f1_32;
      newNode[i].nx_32 = Node[i].nx_32;
      newNode[i].f0_h8 = Node[i].f0_h8;
      newNode[i].f1_h8 = Node[i].f1_h8;
      newNode[i].nx_h8 = Node[i].nx_h8;
#endif
    }
    free(Node);
    Node = newNode;
  }
  else return 1; /* Not enough memory */

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

int hash_enlarge(bddvar v)
/* Throws an exception if not enough memory */
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
    return 0; /*  Cancel enlarging */
  newSpc = oldSpc << 1U;

  /* Enlarge space */
#ifdef B_EXTEND
  newhash_64 = 0;
  newhash_64 = B_MALLOC(bddp_64, newSpc);
  if(newhash_64)
  {
    for(i=0; i<varp->hashSpc; i++) newhash_64[i] = varp->hash_64[i];
    free(varp->hash_64);
    varp->hash_64 = newhash_64;
  }
  else {
    bddp memsize = sizeof(bddp_64) * newSpc;
    throw BDDOutOfMemoryException("hash_enlarge: not enough memory for hash table", memsize);
  }
#elif defined(B_32)
  newhash_32 = 0;
  newhash_32 = B_MALLOC(bddp_32, newSpc);
  if(newhash_32)
  {
    for(i=0; i<varp->hashSpc; i++) newhash_32[i] = varp->hash_32[i];
    free(varp->hash_32);
    varp->hash_32 = newhash_32;
  }
  else {
    bddp memsize = sizeof(bddp_32) * newSpc;
    throw BDDOutOfMemoryException("hash_enlarge: not enough memory for hash table", memsize);
  }
#else
  newhash_32 = 0;
  newhash_h8 = 0;
  newhash_32 = B_MALLOC(bddp_32, newSpc);
  newhash_h8 = B_MALLOC(bddp_h8, newSpc);
  if(newhash_32 && newhash_h8)
  {
    for(i=0; i<varp->hashSpc; i++)
    {
      newhash_32[i] = varp->hash_32[i];
      newhash_h8[i] = varp->hash_h8[i];
    }
    free(varp->hash_32);
    free(varp->hash_h8);
    varp->hash_32 = newhash_32;
    varp->hash_h8 = newhash_h8;
  }
  else
  {
    bddp memsize = 0;
    if(newhash_32) {
      free(newhash_32);
    } else {
      memsize += sizeof(bddp_32) * newSpc;
    }
    if(newhash_h8) {
      free(newhash_h8);
    } else {
      memsize += sizeof(bddp_h8) * newSpc;
    }
    throw BDDOutOfMemoryException("hash_enlarge: not enough memory for hash table", memsize);
  }
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
  return 0;
}

bddp getnode(bddvar v, bddp f0, bddp f1)
/* Throws an exception if not enough memory */
{
  /* After checking elimination rule & negative edge rule */
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

  varp = &Var[v];
  if(varp->hashSpc == 0)
  /* Create hash-table */
  {
#ifdef B_EXTEND
    varp->hash_64 = 0;
    varp->hash_64 = B_MALLOC(bddp_64, B_HASH_SPC0);
    if(!varp->hash_64) throw BDDOutOfMemoryException("getnode: not enough memory for hash table", sizeof(bddp_64) * B_HASH_SPC0);
#elif defined(B_32)
    varp->hash_32 = 0;
    varp->hash_32 = B_MALLOC(bddp_32, B_HASH_SPC0);
    if(!varp->hash_32) throw BDDOutOfMemoryException("getnode: not enough memory for hash table", sizeof(bddp_32) * B_HASH_SPC0);
#else
    varp->hash_32 = 0;
    varp->hash_32 = B_MALLOC(bddp_32, B_HASH_SPC0);
    if(!varp->hash_32) throw BDDOutOfMemoryException("getnode: not enough memory for hash table", sizeof(bddp_32) * B_HASH_SPC0);
    varp->hash_h8 = 0;
    varp->hash_h8 = B_MALLOC(bddp_h8, B_HASH_SPC0);
    if(!varp->hash_h8)
    {
      free(varp->hash_32);
      throw BDDOutOfMemoryException("getnode: not enough memory for hash table", sizeof(bddp_h8) * B_HASH_SPC0);
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
        /* Sharing equivalent node */
        if(!B_CST(f0)) { fp = B_NP(f0); B_RFC_DEC_NP(fp); }
        if(!B_CST(f1)) { fp = B_NP(f1); B_RFC_DEC_NP(fp); }
        B_RFC_INC_NP(np);
        return B_BDDP_NP(np);
      }
      nx = B_GET_BDDP(np->nx);
    }
  }

  /* Check hash-table overflow */
  if(++ varp->hashUsed >= varp->hashSpc)
  {
    if(hash_enlarge(v)) throw BDDOutOfMemoryException("getnode: "
      "not enough memory for hash table", sizeof(bddp_32) * varp->hashSpc); /* Hash-table overflow */
    key = B_HASHKEY(f0, f1, varp->hashSpc); /* Enlarge success */
  }

  /* Check node-table overflow */
  if(NodeUsed >= NodeSpc-1U)
  {
    if(node_enlarge())
    {
      if(bddgc()) throw BDDOutOfMemoryException("getnode: "
        "not enough memory for node table", 0); /* Node-table overflow */
      key = B_HASHKEY(f0, f1, varp->hashSpc);
    }
    /* Node-table enlarged or GC succeeded */
  }
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
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check elimination rule */
  if(f0 == f1)
  {
    if(!B_CST(f0)) { fp = B_NP(f0); B_RFC_DEC_NP(fp); }
    return f0;
  }

  /* Negative edge constraint */
  if(B_NEG(f0))
  {
    bddp h;

    h = getnode(v, B_NOT(f0), B_NOT(f1));
    if(h == bddnull) return bddnull;
    return B_NOT(h);
  }
  return getnode(v, f0, f1);
}

bddp getzddp(bddvar v, bddp f0, bddp f1)
/* Returns bddnull if not enough memory */
{
  /* Check elimination rule */
  if(f1 == bddfalse) return f0;

  /* Negative edge constraint */
  if(B_NEG(f0))
  {
    bddp h;

    h = getnode(v, f0, f1);
    if(h == bddnull) return bddnull;
    return B_NOT(h);
  }
  return getnode(v, B_NOT(f0), f1);
}

} // namespace sapporobdd
