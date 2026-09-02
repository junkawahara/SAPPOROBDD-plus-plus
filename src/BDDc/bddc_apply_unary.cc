/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Unary Ops *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)          *
*  Unary operations: AT0, AT1, OFFSET, ONSET, CHANGE, LSHIFT, RSHIFT
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply_unary(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Unary operations: BC_AT0, BC_AT1, BC_OFFSET, BC_ONSET, BC_CHANGE,
   BC_LSHIFT, BC_RSHIFT.  g is a variable index (or a shift count), not a
   BDD.  Memory exhaustion is reported by BDDOutOfMemoryException from
   getnode(); the function never returns bddnull (the bddnull tests below
   are only a defence).  skip = 1 is the second half of a negation rule and
   is defined for AT0/AT1/OFFSET and LSHIFT/RSHIFT only; f is then a node.
   BC_AT0/BC_AT1 apply the BDD negative-edge rule (a negated f negates the
   1-edge); bddat0()/bddat1() refuse ZDD nodes for that reason. */
{
  struct B_NodeTable *fp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, h0, h1, h;
  bddvar v, flev, glev;
  char z;

  switch(op)
  {
  case BC_AT0: case BC_AT1: case BC_OFFSET: case BC_LSHIFT: case BC_RSHIFT:
    break;
  case BC_ONSET: case BC_CHANGE:
    if(skip)
      err("apply_unary: skip is not defined for this opcode", op, ExceptionType::InternalError);
    break;
  default:
    err("apply_unary: unknown opcode", op, ExceptionType::InternalError);
  }

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
    /* Check negation.  The frame is counted while the positive operand is
       computed, see apply_binary(). */
    if(B_NEG(f))
    {
      BDD_RECUR_INC;
      h = apply(B_NOT(f), g, op, 1);
      BDD_RECUR_DEC;
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
      /* getnode() reports memory exhaustion by exception; release the
         reference taken above instead of leaking it */
      try { h = getzddp((bddvar)g, bddfalse, f); }
      catch(...) { bddfree(f); throw; }
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
      try { h = getzddp((bddvar)g, h0, h1); }
      catch(...) { bddfree(h1); bddfree(h0); throw; }
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
      BDD_RECUR_INC;
      h = apply(B_NOT(f), g, op, 1);
      BDD_RECUR_DEC;
      if(h == bddnull) return bddnull;
      return B_NOT(h);
    }
    break;

  default:
    break;
  }

  /* Non-trivial operations.  f is a node here: the terminal cases above
     answer every constant, and skip = 1 only arrives with the node of a
     negation rule. */
  assert(!B_CST(f));
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

  /* Stack overflow limiter */
  BDD_RECUR_INC;

  /* Get result node.  The recursions and get{b,z}ddp() report memory
     exhaustion by exception, which would skip the bddfree() calls below;
     the references held in h0/h1 are released on the way out instead of
     leaking and pinning their nodes against bddgc() forever.  See
     apply_binary() for what this relies on. */
  h0 = bddnull;
  h1 = bddnull;
  try
  {
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
          err("apply_unary: Invalid shift", newlev, ExceptionType::OutOfRange);
      }
      else
      {
        newlev = flev - (bddvar)g;
        if(newlev == 0 || newlev > flev)
          err("apply_unary: Invalid shift", newlev, ExceptionType::OutOfRange);
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
  }
  }
  catch(...)
  {
    bddfree(h1);
    bddfree(h0);
    throw;
  }

  /* Stack overflow limiter */
  BDD_RECUR_DEC;

  /* Saving to Cache */
  APPLY_CACHE_STORE(key, op, f, g, h, cachep);
  if(key != bddnull && h != bddnull)
  {
    /* Additional cache entries for related operations: at0(f, v) == f means
       f does not depend on v, so at1(f, v) == f as well; offset(f, v) == f
       means no set contains v, so onset(f, v) is empty, and the converse. */
    if(h == f) switch(op)
    {
    case BC_AT0:
      APPLY_CACHE_STORE(key, BC_AT1, f, g, h, cachep);
      break;
    case BC_AT1:
      APPLY_CACHE_STORE(key, BC_AT0, f, g, h, cachep);
      break;
    case BC_OFFSET:
      APPLY_CACHE_STORE(key, BC_ONSET, f, g, bddfalse, cachep);
      break;
    default:
      break;
    }
    if(h == bddfalse && op == BC_ONSET)
      APPLY_CACHE_STORE(key, BC_OFFSET, f, g, f, cachep);
  }

  return h;
}

} // namespace sapporobdd
