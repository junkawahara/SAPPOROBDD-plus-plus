/*****************************************
*  BDD Package (SAPPORO-1.94)   - GC    *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

void gc1(struct B_NodeTable *np)
{
  /* np is a node ptr to be collected. (refc == 0) */
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

  /* remove the node from hash list */
  varp = Var + B_VAR_NP(np);
  f0 = B_GET_BDDP(np->f0);
  f1 = B_GET_BDDP(np->f1);
  key = B_HASHKEY(f0, f1, varp->hashSpc);
  B_SET_NXP(p, varp->hash, key);
  nx1 = B_GET_BDDP(*p);
  np1 = Node + nx1;

  if(np1 == np) B_CPY_BDDP(*p, np->nx);
  else
  {
    while(np1 != np)
    {
      if(nx1 == bddnull)
        err("gc1: Fail to find the node to be deleted", np-Node, ExceptionType::InternalError);
      np2 = np1;
      nx1 = B_GET_BDDP(np2->nx);
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

  /* Check sub-graphs recursively */
  if(!B_CST(f0))
  {
    np1 = B_NP(f0);
    B_RFC_DEC_NP(np1);
    if(B_RFC_ZERO_NP(np1))
    {  BDD_RECUR_INC; gc1(np1); BDD_RECUR_DEC; }
  }
  if(!B_CST(f1))
  {
    np1 = B_NP(f1);
    B_RFC_DEC_NP(np1);
    if(B_RFC_ZERO_NP(np1))
    {  BDD_RECUR_INC; gc1(np1); BDD_RECUR_DEC; }
  }
}

int bddgc()
/* Returns 1 if there are no free node (usually 0) */
{
  bddp i, n, f;
  struct B_NodeTable *fp;
  struct B_CacheTable *cachep;
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
  for(fp=Node; fp<Node+NodeSpc; fp++)
    if(fp->varrfc != 0 && B_RFC_ZERO_NP(fp))
      gc1(fp);

  bddp freedNodes = n - NodeUsed;
  if(freedNodes == 0) return 1; /* No free node */

  /* Check if freed nodes count is below threshold */
  if(GCThreshold > 0 && freedNodes <= GCThreshold) {
    return 1;
  }

  /* Cache clear */
  for(cachep=Cache; cachep<Cache+CacheSpc; cachep++)
  {
    switch(cachep->op)
    {
    case BC_NULL:
      break;
    case BC_AND:
    case BC_XOR:
    case BC_INTERSEC:
    case BC_UNION:
    case BC_SUBTRACT:
      f = B_GET_BDDP(cachep->f);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      f = B_GET_BDDP(cachep->g);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      f = B_GET_BDDP(cachep->h);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      break;
    /* g holds a VarID, not a bddp, so only f and h are node references */
    case BC_AT0:
    case BC_AT1:
    case BC_OFFSET:
    case BC_ONSET:
    case BC_CHANGE:
      f = B_GET_BDDP(cachep->f);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      f = B_GET_BDDP(cachep->h);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      break;
    case BC_CARD:
    case BC_LIT:
    case BC_LEN:
      f = B_GET_BDDP(cachep->f);
      if(!B_CST(f) && (fp=B_NP(f))<Node+NodeSpc && fp->varrfc == 0)
      {
        cachep->op = BC_NULL;
        break;
      }
      f = B_GET_BDDP(cachep->h);
      if(f > bddnull)
      {
        cachep->op = BC_NULL;
        break;
      }
      break;
    default:
      cachep->op = BC_NULL;
      break;
    }
  }

  /* MP-Count table clear */
  for(i=0; i<B_MP_LMAX; i++)
  {
    mptable[i].size = 0;
    mptable[i].used = 0;
    free(mptable[i].word);
    mptable[i].word = 0;
  }

  /* Hash-table packing */
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
    newhash_64 = 0;
    newhash_64 = B_MALLOC(bddp_64, newSpc);
    if(!newhash_64) break; /* Not enough memory */
#elif defined(B_32)
    newhash_32 = 0;
    newhash_32 = B_MALLOC(bddp_32, newSpc);
    if(!newhash_32) break; /* Not enough memory */
#else
    newhash_32 = 0;
    newhash_h8 = 0;
    newhash_32 = B_MALLOC(bddp_32, newSpc);
    newhash_h8 = B_MALLOC(bddp_h8, newSpc);
    if(!newhash_32 || !newhash_h8)
    {
      if(newhash_32) free(newhash_32);
      if(newhash_h8) free(newhash_h8);
      break; /* Not enough memory */
    }
#endif

    /* Initialize new hash entry */
    for(i=0; i<newSpc; i++)
    {
      B_SET_NXP(p, newhash, i);
      B_SET_BDDP(*p, bddnull);
    }

    /* restore hash entry */
    for(i=0; i<oldSpc; i++)
    {
      key = i & (newSpc-1U);
      np = 0;
      B_SET_NXP(p, newhash, key);
      nx = B_GET_BDDP(*p);
      while(nx != bddnull)
      {
        np = Node + nx;
        nx = B_GET_BDDP(np->nx);
      }
      if(np) { B_SET_NXP(p2, varp->hash, i); B_CPY_BDDP(np->nx, *p2); }
      else
      {
        B_SET_NXP(p, newhash, key);
        B_SET_NXP(p2, varp->hash, i);
        B_CPY_BDDP(*p, *p2);
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

int rfc_inc_ovf(struct B_NodeTable *np)
{
  bddp ix, nx, nx2, key, rfc, oldSpc;
  struct B_RFC_Table *oldRFCT;

/* printf("rfc_inc %d (u:%d)\n", np-Node, RFCT_Used); */
  if(RFCT_Spc == 0)
  {
    /* Create RFC-table */
    RFCT = 0;
    RFCT = B_MALLOC(struct B_RFC_Table, B_RFCT_SPC0);
    if(!RFCT)
    {
      err("B_RFC_INC_NP: rfc memory over flow", np-Node, ExceptionType::OutOfMemory);
    }
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
      else rfc = B_GET_BDDP((RFCT+key)->rfc) + 1;
      B_SET_BDDP((RFCT+key)->rfc, rfc);
      return 0;
    }
    key = (key+1) & (RFCT_Spc-1);
    nx2 = B_GET_BDDP((RFCT+key)->nx);
  }

  /* new rfc entry */
  B_SET_BDDP((RFCT+key)->nx, nx);
  B_SET_BDDP((RFCT+key)->rfc, (bddp)0);
  np->varrfc += B_RFC_UNIT;
  RFCT_Used++;

  if((RFCT_Used << 1) >= RFCT_Spc)
  {
    /* Enlarge RFC-table */
    oldSpc = RFCT_Spc;
    RFCT_Spc <<= 2;

    oldRFCT = RFCT;
    RFCT = 0;
    RFCT = B_MALLOC(struct B_RFC_Table, RFCT_Spc);
    if(!RFCT)
    {
      err("B_RFC_INC_NP: rfc memory over flow", np-Node, ExceptionType::OutOfMemory);
    }
    for(ix=0; ix<RFCT_Spc; ix++)
    {
      B_SET_BDDP((RFCT+ix)->nx, bddnull);
      B_SET_BDDP((RFCT+ix)->rfc, (bddp)0);
    }
    for(ix=0; ix<oldSpc; ix++)
    {
      nx = B_GET_BDDP((oldRFCT+ix)->nx);
      if(nx == bddnull) continue;
      key = nx & (RFCT_Spc-1);
      nx2 = B_GET_BDDP((RFCT+key)->nx);
      while(nx2 != bddnull)
      {
        key = (key+1) & (RFCT_Spc-1);
        nx2 = B_GET_BDDP((RFCT+key)->nx);
      }
      B_SET_BDDP((RFCT+key)->nx, nx);
      rfc = B_GET_BDDP((oldRFCT+ix)->rfc);
      B_SET_BDDP((RFCT+key)->rfc, rfc);
    }
    free(oldRFCT);
  }

  return 0;
}

int rfc_dec_ovf(struct B_NodeTable *np)
{
  bddp nx, key, nx2, rfc;

/* printf("rfc_dec %d (u:%d)\n", np-Node, RFCT_Used); */
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
  return 0;
}

} // namespace sapporobdd
