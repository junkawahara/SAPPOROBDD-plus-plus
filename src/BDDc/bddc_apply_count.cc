/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Count Ops *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)          *
*  Counting operations: CARD, CARD2, LIT, LEN, SUPPORT
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply_count(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Counting operations: BC_CARD, BC_CARD2, BC_LIT, BC_LEN, BC_SUPPORT */
/* Returns numeric value or bddnull if overflow */
{
  struct B_NodeTable *fp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, h0, h1, h;
  bddvar v;
  char z;

  /* Check terminal case */
  if(!skip) switch(op)
  {
  case BC_SUPPORT:
    if(B_CST(f)) return bddfalse;
    if(B_NEG(f)) f = B_NOT(f);
    break;

  case BC_CARD:
    if(B_CST(f)) return (f == bddfalse)? 0: 1;
    if(B_NEG(f))
    {
      h = apply(B_NOT(f), bddfalse, op, 1);
      return (h >= bddnull)? bddnull: h+1;
    }
    break;

  case BC_CARD2:
    if(B_CST(f)) return (f == bddfalse)? 0: 1;
    break;

  case BC_LIT:
    if(B_CST(f)) return 0;
    if(B_NEG(f)) f = B_NOT(f);
    break;

  case BC_LEN:
    if(B_CST(f)) return 0;
    if(B_NEG(f)) f = B_NOT(f);
    break;

  default:
    err("apply_count: unknown opcode", op, ExceptionType::InternalError);
    break;
  }

  /* Non-trivial operations */
  fp = B_NP(f);

  switch(op)
  {
  case BC_SUPPORT:
    if(B_RFC_ONE_NP(fp)) key = bddnull;
    else
    {
      /* Checking Cache */
      key = B_CACHEKEY(op, f, g);
      cachep = Cache + key;
      if(cachep->op == op &&
         f == B_GET_BDDP(cachep->f) &&
         g == B_GET_BDDP(cachep->g))
      {
        /* Hit */
        h = B_GET_BDDP(cachep->h);
        if(!B_CST(h) && h != bddnull) { fp = B_NP(h); B_RFC_INC_NP(fp); }
        return h;
      }
    }
    /* Get (f0, f1) */
    APPLY_GET_CHILDREN_UNARY(f, fp, f0, f1, v, z);
    break;

  case BC_CARD:
  case BC_LIT:
  case BC_LEN:
    if(B_RFC_ONE_NP(fp)) key = bddnull;
    else
    {
      /* Checking Cache */
      key = B_CACHEKEY(op, f, bddfalse);
      cachep = Cache + key;
      if(cachep->op == op &&
         f == B_GET_BDDP(cachep->f) &&
         bddfalse == B_GET_BDDP(cachep->g))
      {
        /* Hit */
        return B_GET_BDDP(cachep->h);
      }
    }
    /* Get (f0, f1) */
    APPLY_GET_CHILDREN_COUNT(f, fp, f0, f1);
    break;

  case BC_CARD2:
    if(B_RFC_ONE_NP(fp)) key = bddnull;
    else
    {
      /* Checking Cache */
      key = B_CACHEKEY(BC_CARD, f, bddfalse);
      cachep = Cache + key;
      if(cachep->op == BC_CARD &&
         f == B_GET_BDDP(cachep->f) &&
         bddfalse == B_GET_BDDP(cachep->g))
      {
        /* Hit */
        h = B_GET_BDDP(cachep->h);
        if(h != bddnull) return h;
      }
    }
    /* Get (f0, f1) */
    APPLY_GET_CHILDREN_COUNT(f, fp, f0, f1);
    break;

  default:
    err("apply_count: unknown opcode", op, ExceptionType::InternalError);
  }

  /* Stack overflow limitter */
  BDD_RECUR_INC;

  /* Get result */
  switch(op)
  {
  case BC_SUPPORT:
    h0 = apply(f0, bddfalse, op, 0);
    if(h0 == bddnull) { h = h0; break; }
    h1 = apply(f1, bddfalse, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; }
    h = z? apply(h0, h1, BC_UNION, 0):
           apply(B_NOT(h0), B_NOT(h1), BC_AND, 0);
    bddfree(h0); bddfree(h1);
    if(h == bddnull) break;
    h0 = h;
    h = z? getzddp(v, h0, bddtrue):
           getbddp(v, B_NOT(h0), bddtrue);
    if(h == bddnull) bddfree(h0);
    break;

  case BC_CARD:
    h0 = apply(f0, bddfalse, op, 0);
    if(h0 == bddnull) { h = h0; break; }
    h1 = apply(f1, bddfalse, op, 0);
    if(h1 == bddnull) { h = h1; break; }
    h = h0 + h1;
    if(h >= bddnull) h = bddnull;
    break;

  case BC_CARD2:
    h0 = apply(B_ABS(f0), bddfalse, op, 0);
    if(h0 == B_MP_NULL) { h = h0; break; }
    h1 = apply(B_ABS(f1), bddfalse, op, 0);
    if(h1 == B_MP_NULL) { h = h1; break; }
    {
      struct B_MP mp;
      struct B_MPTable *mpt;
      bddp i, size2;
      bddp *wp;

      mp.len = 1;
      mp.word[0] = 0;
      if(B_NEG(f0)) mp.word[0]++;
      if(B_NEG(f1)) mp.word[0]++;
      mp_add(&mp, h0);
      mp_add(&mp, h1);
      if(mp.len == 1 && mp.word[0] <= bddnull)
        { h = mp.word[0]; break; }
      mpt = mptable + mp.len-1;
      if(mpt->word == 0)
      {
        mpt->size = 16;
        mpt->used = 0;
        mpt->word = B_MALLOC(bddp, mp.len * mpt->size);
        if (!mpt->word) {
          err("apply: not enough memory for mp table", sizeof(bddp) * mp.len * mpt->size,
          ExceptionType::OutOfMemory);
        }
      }
      if(mpt->size == mpt->used)
      {
        size2 = mpt->size << 1;
        if(size2 > (B_CST_MASK>>B_MP_LWID)) { h = B_MP_NULL; break; }
        wp = 0;
        wp = B_MALLOC(bddp, mp.len * size2);
        if(!wp) {
          err("apply: not enough memory for mp table", sizeof(bddp) * mp.len * size2,
          ExceptionType::OutOfMemory);
        }
        for(i=0; i<mp.len*(mpt->size); i++) wp[i] = mpt->word[i];
        mpt->size = size2;
        free(mpt->word);
        mpt->word = wp;
      }
      wp = mpt->word;

      for(i=0; i<(bddp)mp.len; i++) wp[mp.len*(mpt->used)+i] = mp.word[i];
      h = (((bddp)mp.len-1)<<B_MP_LPOS) + B_CST_MASK + (mpt->used++);
      break;
    }

  case BC_LIT:
    h = apply(f0, bddfalse, op, 0)
      + apply(f1, bddfalse, op, 0);
    if(h >= bddnull) h = bddnull;
    h += apply(f1, bddfalse, BC_CARD, 0);
    if(h >= bddnull) h = bddnull;
    break;

  case BC_LEN:
    h0 = apply(f0, bddfalse, op, 0);
    h1 = apply(f1, bddfalse, op, 0) + 1;
    h = (h0 < h1)? h1: h0;
    break;

  default:
    err("apply_count: unknown opcode", op, ExceptionType::InternalError);
    h = bddnull;
    break;
  }

  /* Stack overflow limitter */
  BDD_RECUR_DEC;

  /* Saving to Cache */
  if(key != bddnull)
  {
    cachep = Cache + key;
    if(op == BC_CARD2)
      cachep->op = BC_CARD;
    else
      cachep->op = op;
    B_SET_BDDP(cachep->f, f);
    if(op == BC_SUPPORT)
      B_SET_BDDP(cachep->g, g);
    else
      B_SET_BDDP(cachep->g, bddfalse);
    B_SET_BDDP(cachep->h, h);
  }

  return h;
}

} // namespace sapporobdd
