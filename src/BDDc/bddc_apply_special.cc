/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Special Ops *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)            *
*  Special operations: COFACTOR, UNIV
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply_special(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Special operations: BC_COFACTOR, BC_UNIV.  Both are BDD operations; the
   public entry points refuse ZDD nodes, and a ZDD node reaching the
   recursion is reported below rather than combined with getbddp().
   Memory exhaustion is reported by BDDOutOfMemoryException from getnode();
   the function never returns bddnull (the bddnull tests below are only a
   defence).  skip is always 0 here: no negation rule of these operations
   re-enters with skip = 1, and with skip = 1 a constant operand would reach
   the child extraction. */
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  bddp key, f0 = 0, f1 = 0, g0 = 0, g1 = 0, h0, h1, h;
  bddvar v = 0, flev, glev;
  char z = 0;

  if(skip)
    err("apply_special: skip is not defined", op, ExceptionType::InternalError);

  /* Check terminal case */
  switch(op)
  {
  case BC_COFACTOR:
    /* Check trivial cases */
    if(B_CST(f)) return f;
    if(g == bddfalse || f == B_NOT(g)) return bddfalse;
    if(f == g) return bddtrue;
    if(g == bddtrue) { fp = B_NP(f); B_RFC_INC_NP(fp); return f; }
    break;

  case BC_UNIV:
    /* Check trivial cases */
    if(B_CST(f)) return f;
    if(B_CST(g)) { fp = B_NP(f); B_RFC_INC_NP(fp); return f; }
    if(B_NEG(g)) g = B_NOT(g);
    break;

  default:
    err("apply_special: unknown opcode", op, ExceptionType::InternalError);
  }

  /* Non-trivial operations */
  /* Try cache? */
  if((B_CST(f) || B_RFC_ONE_NP(B_NP(f))) &&
     (B_CST(g) || B_RFC_ONE_NP(B_NP(g)))) key = bddnull;
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

  /* Get (f0, f1) and (g0, g1) */
  APPLY_GET_CHILDREN_BINARY(f, g, fp, gp, flev, glev, f0, f1, g0, g1, v, z);
  if(z)
    err("apply_special: ZDD node in a BDD operation", f, ExceptionType::InternalError);

  /* Stack overflow limiter */
  BDD_RECUR_INC;

  /* Get result node.  The recursions and getbddp() report memory exhaustion
     by exception, which would skip the bddfree() calls below; the references
     held in h0/h1 are released on the way out instead of leaking and pinning
     their nodes against bddgc() forever.  See apply_binary() for what this
     relies on. */
  h0 = bddnull;
  h1 = bddnull;
  try
  {
  switch(op)
  {
  case BC_COFACTOR:
    if(g0 == bddfalse && g1 != bddfalse)
    {
      h = apply(f1, g1, op, 0);
    }
    else if(g1 == bddfalse && g0 != bddfalse)
    {
      h = apply(f0, g0, op, 0);
    }
    else
    {
      h0 = apply(f0, g0, op, 0);
      if(h0 == bddnull) { h = h0; break; }
      h1 = apply(f1, g1, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; }
      h = getbddp(v, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); }
    }
    break;

  case BC_UNIV:
    /* g is the disjunction of the variables to quantify (bdduniv() checks
       the shape), so only its 0-edge chain is followed: g0 is the rest of
       the set, and g1 == g0 exactly when v is not in it. */
    if(f0 == f1)
    {
      /* f does not depend on v, so both branches would recur on the same
         (f0, g0) pair.  Both combining rules collapse to that single result:
         apply(h, h, BC_AND) returns h, and getbddp(v, h, h) returns h by the
         elimination rule; the reference count works out the same either way. */
      h = apply(f0, g0, op, 0);
    }
    else if(g0 != g1)
    {
      h0 = apply(f0, g0, op, 0);
      if(h0 == bddnull) { h = h0; break; }
      h1 = apply(f1, g0, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; }
      h = apply(h0, h1, BC_AND, 0);
      bddfree(h0); bddfree(h1);
      h0 = bddnull; h1 = bddnull;
    }
    else
    {
      h0 = apply(f0, g0, op, 0);
      if(h0 == bddnull) { h = h0; break; }
      h1 = apply(f1, g0, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; }
      h = getbddp(v, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); }
    }
    break;

  default:
    err("apply_special: unknown opcode", op, ExceptionType::InternalError);
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

  return h;
}

} // namespace sapporobdd
