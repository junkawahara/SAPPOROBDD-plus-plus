/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Unary Ops *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)          *
*  Unary operations: AT0, AT1, OFFSET, ONSET, CHANGE, LSHIFT, RSHIFT
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply_unary(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Unary operations: BC_AT0, BC_AT1, BC_OFFSET, BC_ONSET, BC_CHANGE, BC_LSHIFT, BC_RSHIFT */
/* g is typically a variable index, not a BDD */
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, h0, h1, h;
  bddvar v, flev, glev;
  char z;

  /* Check terminal case */
  if(!skip) switch(op)
  {
  case BC_AT0:
  case BC_AT1:
  case BC_OFFSET:
    /* Check trivial cases */
    if(B_CST(f)) return f;
    /* special cases */
    fp = B_NP(f); flev = Var[B_VAR_NP(fp)].lev;
    glev = Var[(bddvar)g].lev;
    if(flev < glev) { B_RFC_INC_NP(fp); return f; }
    if(flev == glev)
    {
      if(op != BC_AT1)
      {
        h = B_GET_BDDP(fp->f0);
        if(B_NEG(f)^B_NEG(h)) h = B_NOT(h);
      }
      else
      {
        h = B_GET_BDDP(fp->f1);
        if(B_NEG(f)) h = B_NOT(h);
      }
      if(!B_CST(h)) { fp = B_NP(h); B_RFC_INC_NP(fp); }
      return h;
    }
    /* Check negation */
    if(B_NEG(f))
    {
      h = apply(B_NOT(f), g, op, 1);
      if(h == bddnull) return bddnull;
      return B_NOT(h);
    }
    break;

  case BC_ONSET:
    /* Check trivial cases */
    if(B_CST(f)) return bddfalse;
    /* special cases */
    fp = B_NP(f); flev = Var[B_VAR_NP(fp)].lev;
    glev = Var[(bddvar)g].lev;
    if(flev < glev)  return bddfalse;
    if(flev == glev)
    {
      h = B_GET_BDDP(fp->f1);
      if(!B_CST(h)) { fp = B_NP(h); B_RFC_INC_NP(fp); }
      return h;
    }
    /* Check negation */
    if(B_NEG(f)) f = B_NOT(f);
    break;

  case BC_CHANGE:
    /* Check trivial cases */
    if(f == bddfalse) return f;
    if(B_CST(f)) return getzddp((bddvar)g, bddfalse, f);
    /* special cases */
    fp = B_NP(f); flev = Var[B_VAR_NP(fp)].lev;
    glev = Var[(bddvar)g].lev;
    if(flev < glev)
    {
      B_RFC_INC_NP(fp);
      h = getzddp((bddvar)g, bddfalse, f);
      if(h == bddnull) bddfree(f);
      return h;
    }
    if(flev == glev)
    {
      h0 = B_GET_BDDP(fp->f1);
      h1 = B_GET_BDDP(fp->f0);
      if(B_NEG(f)^B_NEG(h1)) h1 = B_NOT(h1);
      if(!B_CST(h0)) { fp = B_NP(h0); B_RFC_INC_NP(fp); }
      if(!B_CST(h1)) { fp = B_NP(h1); B_RFC_INC_NP(fp); }
      h = getzddp((bddvar)g, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); }
      return h;
    }
    break;

  case BC_LSHIFT:
  case BC_RSHIFT:
    /* Check trivial cases */
    if(B_CST(f)) return f;

    /* Check negation */
    if(B_NEG(f))
    {
      h = apply(B_NOT(f), g, op, 1);
      if(h == bddnull) return bddnull;
      return B_NOT(h);
    }
    break;

  default:
    err("apply_unary: unknown opcode", op, ExceptionType::InternalError);
    break;
  }

  /* Non-trivial operations */
  fp = B_NP(f);
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

  /* Stack overflow limitter */
  BDD_RECUR_INC;

  /* Get result node */
  switch(op)
  {
  case BC_AT0:
  case BC_AT1:
  case BC_OFFSET:
  case BC_ONSET:
  case BC_CHANGE:
    h0 = apply(f0, g, op, 0);
    if(h0 == bddnull) { h = h0; break; }
    h1 = apply(f1, g, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; }
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
    if(h == bddnull) { bddfree(h0); bddfree(h1); }
    break;

  case BC_LSHIFT:
  case BC_RSHIFT:
    /* Get VarID of new level */
    {
      bddvar newlev;

      flev = bddlevofvar(v);
      if(op == BC_LSHIFT)
      {
        newlev = flev + (bddvar)g;
        if(newlev > VarUsed || newlev < flev)
          err("apply: Invald shift", newlev, ExceptionType::OutOfRange);
      }
      else
      {
        newlev = flev - (bddvar)g;
        if(newlev == 0 || newlev > flev)
          err("apply: Invald shift", newlev, ExceptionType::OutOfRange);
      }
      v = bddvaroflev(newlev);
    }
    h0 = apply(f0, g, op, 0);
    if(h0 == bddnull) { h = h0; break; }
    h1 = apply(f1, g, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; }
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
    if(h == bddnull) { bddfree(h0); bddfree(h1); }
    break;

  default:
    err("apply_unary: unknown opcode", op, ExceptionType::InternalError);
    h = bddnull;
    break;
  }

  /* Stack overflow limitter */
  BDD_RECUR_DEC;

  /* Saving to Cache */
  /* h == bddnull means out of memory; not a property of (op, f, g),
     so it must not be cached (see APPLY_CACHE_STORE). */
  if(key != bddnull && h != bddnull)
  {
    cachep = Cache + key;
    cachep->op = op;
    B_SET_BDDP(cachep->f, f);
    B_SET_BDDP(cachep->g, g);
    B_SET_BDDP(cachep->h, h);
    /* Additional cache entries for related operations */
    if(h == f) switch(op)
    {
    case BC_AT0:
      key = B_CACHEKEY(BC_AT1, f, g);
      cachep = Cache + key;
      cachep->op = BC_AT1;
      B_SET_BDDP(cachep->f, f);
      B_SET_BDDP(cachep->g, g);
      B_SET_BDDP(cachep->h, h);
      break;
    case BC_AT1:
      key = B_CACHEKEY(BC_AT0, f, g);
      cachep = Cache + key;
      cachep->op = BC_AT0;
      B_SET_BDDP(cachep->f, f);
      B_SET_BDDP(cachep->g, g);
      B_SET_BDDP(cachep->h, h);
      break;
    case BC_OFFSET:
      key = B_CACHEKEY(BC_ONSET, f, g);
      cachep = Cache + key;
      cachep->op = BC_ONSET;
      B_SET_BDDP(cachep->f, f);
      B_SET_BDDP(cachep->g, g);
      B_SET_BDDP(cachep->h, bddfalse);
      break;
    default:
      break;
    }
    if(h == bddfalse && op == BC_ONSET)
    {
      key = B_CACHEKEY(BC_OFFSET, f, g);
      cachep = Cache + key;
      cachep->op = BC_OFFSET;
      B_SET_BDDP(cachep->f, f);
      B_SET_BDDP(cachep->g, g);
      B_SET_BDDP(cachep->h, f);
    }
  }

  return h;
}

} // namespace sapporobdd
