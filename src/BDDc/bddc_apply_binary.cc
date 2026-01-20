/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Binary Ops *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)           *
*  Binary operations: AND, XOR, INTERSEC, UNION, SUBTRACT
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply_binary(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Binary operations: BC_AND, BC_XOR, BC_INTERSEC, BC_UNION, BC_SUBTRACT */
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, g0 = 0, g1 = 0, h0, h1, h;
  bddvar v = 0, flev, glev;
  char z = 0;

  /* Check terminal case */
  if(!skip) switch(op)
  {
  case BC_AND:
    /* Check trivial cases */
    if(f == bddfalse || g == bddfalse || f == B_NOT(g))
      return bddfalse;
    if(f == g)
    {
      if(f != bddtrue) { fp = B_NP(f); B_RFC_INC_NP(fp); }
      return f;
    }
    if(f == bddtrue) { fp = B_NP(g); B_RFC_INC_NP(fp); return g; }
    if(g == bddtrue) { fp = B_NP(f); B_RFC_INC_NP(fp); return f; }
    /* Check operand swap */
    if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */
    break;

  case BC_XOR:
    /* Check trivial cases */
    if(f == g) return bddfalse;
    if(f == B_NOT(g)) return bddtrue;
    if(f == bddfalse) { fp = B_NP(g); B_RFC_INC_NP(fp); return g; }
    if(g == bddfalse) { fp = B_NP(f); B_RFC_INC_NP(fp); return f; }
    if(f == bddtrue) {fp=B_NP(g); B_RFC_INC_NP(fp); return B_NOT(g);}
    if(g == bddtrue) {fp=B_NP(f); B_RFC_INC_NP(fp); return B_NOT(f);}
    /* Check negation */
    if(B_NEG(f) && B_NEG(g)) { f = B_NOT(f); g = B_NOT(g); }
    else if(B_NEG(f) || B_NEG(g))
    {
      f = B_ABS(f); g = B_ABS(g);
      /* Check operand swap */
      h = (f < g)? apply(g, f, op, 1): apply(f, g, op, 1);
      if(h == bddnull) return bddnull;
      return B_NOT(h);
    }
    /* Check operand swap */
    if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */
    break;

  case BC_INTERSEC:
    /* Check trivial cases */
    if(f == bddfalse || g == bddfalse) return bddfalse;
    if(f == bddtrue) return B_NEG(g)? bddtrue: bddfalse;
    if(g == bddtrue) return B_NEG(f)? bddtrue: bddfalse;
    if(f == g) { fp = B_NP(f); B_RFC_INC_NP(fp); return f; }
    if(f == B_NOT(g)) {fp=B_NP(f); B_RFC_INC_NP(fp); return B_ABS(f); }
    /* Check operand swap */
    if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */
    break;

  case BC_UNION:
    /* Check trivial cases */
    if(f == bddfalse)
    {
      if(!B_CST(g)) {fp=B_NP(g); B_RFC_INC_NP(fp); }
      return g;
    }
    if(f == bddtrue)
    {
      if(!B_CST(g)) {fp=B_NP(g); B_RFC_INC_NP(fp); }
      return B_NEG(g)? g: B_NOT(g);
    }
    if(g == bddfalse || f == g)
      { fp=B_NP(f); B_RFC_INC_NP(fp); return f; }
    if(g == bddtrue || f == B_NOT(g))
    {
      fp=B_NP(f); B_RFC_INC_NP(fp);
      return B_NEG(f)? f: B_NOT(f);
    }
    /* Check operand swap */
    if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */
    break;

  case BC_SUBTRACT:
    /* Check trivial cases */
    if(f == bddfalse || f == g) return bddfalse;
    if(f == bddtrue || f == B_NOT(g))
      return B_NEG(g)? bddfalse: bddtrue;
    if(g == bddfalse) { fp=B_NP(f); B_RFC_INC_NP(fp); return f; }
    if(g == bddtrue) { fp=B_NP(f); B_RFC_INC_NP(fp); return B_ABS(f); }
    break;

  default:
    err("apply_binary: unknown opcode", op, ExceptionType::InternalError);
    break;
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

  /* Get (f0, f1) and (g0, g1)*/
  APPLY_GET_CHILDREN_BINARY(f, g, fp, gp, flev, glev, f0, f1, g0, g1, v, z);

  /* Stack overflow limitter */
  BDD_RECUR_INC;

  /* Get result node */
  h0 = apply(f0, g0, op, 0);
  if(h0 == bddnull) { h = h0; }
  else
  {
    h1 = apply(f1, g1, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; }
    else
    {
      h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); }
    }
  }

  /* Stack overflow limitter */
  BDD_RECUR_DEC;

  /* Saving to Cache */
  APPLY_CACHE_STORE(key, op, f, g, h, cachep);

  return h;
}

} // namespace sapporobdd
