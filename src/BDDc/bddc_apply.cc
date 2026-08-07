/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Dispatcher *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)           *
*  Main dispatcher for apply operations            *
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

bddp apply(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Main apply dispatcher - routes to category-specific functions */
/* Returns bddnull if not enough memory */
{
  switch(op)
  {
  /* Binary operations */
  case BC_AND:
  case BC_XOR:
  case BC_INTERSEC:
  case BC_UNION:
  case BC_SUBTRACT:
#ifdef B_FORCE_ITERATIVE
    /* Force iterative version for testing */
    return apply_binary_iterative(f, g, op, skip);
#else
    /* Use iterative version when variable count exceeds threshold */
    if (VarUsed > APPLY_RECURSION_THRESHOLD) {
      return apply_binary_iterative(f, g, op, skip);
    }
    return apply_binary(f, g, op, skip);
#endif

  /* Unary operations (g is typically a variable index) */
  case BC_AT0:
  case BC_AT1:
  case BC_OFFSET:
  case BC_ONSET:
  case BC_CHANGE:
  case BC_LSHIFT:
  case BC_RSHIFT:
#ifdef B_FORCE_ITERATIVE
    /* Force iterative version for testing */
    return apply_unary_iterative(f, g, op, skip);
#else
    /* Use iterative version when variable count exceeds threshold */
    if (VarUsed > APPLY_RECURSION_THRESHOLD) {
      return apply_unary_iterative(f, g, op, skip);
    }
    return apply_unary(f, g, op, skip);
#endif

  /* Counting operations */
  case BC_CARD:
  case BC_CARD2:
  case BC_LIT:
  case BC_LEN:
  case BC_SUPPORT:
#ifdef B_FORCE_ITERATIVE
    /* Force iterative version for testing */
    return apply_count_iterative(f, g, op, skip);
#else
    /* Use iterative version when variable count exceeds threshold */
    if (VarUsed > APPLY_RECURSION_THRESHOLD) {
      return apply_count_iterative(f, g, op, skip);
    }
    return apply_count(f, g, op, skip);
#endif

  /* Special operations */
  case BC_COFACTOR:
  case BC_UNIV:
#ifdef B_FORCE_ITERATIVE
    /* Force iterative version for testing */
    return apply_special_iterative(f, g, op, skip);
#else
    /* Use iterative version when variable count exceeds threshold */
    if (VarUsed > APPLY_RECURSION_THRESHOLD) {
      return apply_special_iterative(f, g, op, skip);
    }
    return apply_special(f, g, op, skip);
#endif

  default:
    err("apply: unknown opcode", op, ExceptionType::InternalError);
    return bddnull;
  }
}

int andfalse(bddp f, bddp g)
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, g0, g1, h;
  bddvar flev, glev;

  /* Check trivial cases */
  if(f == bddfalse || g == bddfalse || f == B_NOT(g)) return 0;
  if(f == bddtrue || g == bddtrue || f == g) return 1;
  /* Check operand swap (same direction as apply's BC_AND, so that both
     share the BC_AND cache entries) */
  if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */

  /* Non-trivial operations */
  /* Try cache? */
  if((B_CST(f) || B_RFC_ONE_NP(B_NP(f))) &&
     (B_CST(g) || B_RFC_ONE_NP(B_NP(g)))) key = bddnull;
  else
  {
    /* Checking Cache */
    key = B_CACHEKEY(BC_AND, f, g);
    cachep = Cache + key;
    if(cachep->op == BC_AND &&
       f == B_GET_BDDP(cachep->f) &&
       g == B_GET_BDDP(cachep->g))
    {
      /* Hit */
      h = B_GET_BDDP(cachep->h);
      return (h==bddfalse)? 0: 1;
    }
  }
  /* Get (f0, f1) and (g0, g1)*/
  fp = B_NP(f);
  flev = B_CST(f)? 0: Var[B_VAR_NP(fp)].lev;
  gp = B_NP(g);
  glev = B_CST(g)? 0: Var[B_VAR_NP(gp)].lev;
  f0 = f; f1 = f;
  g0 = g; g1 = g;

  if(flev <= glev)
  {
    g0 = B_GET_BDDP(gp->f0);
    g1 = B_GET_BDDP(gp->f1);
    if(B_NEG(g)) { g0 = B_NOT(g0); g1 = B_NOT(g1); }
  }

  if(flev >= glev)
  {
    f0 = B_GET_BDDP(fp->f0);
    f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)) { f0 = B_NOT(f0); f1 = B_NOT(f1); }
  }

  /* Get result */
  if(andfalse(f0, g0) == 1) return 1;
  if(andfalse(f1, g1) == 1) return 1;

  /* Saving to Cache */
  if(key != bddnull)
  {
    cachep = Cache + key;
    cachep->op = BC_AND;
    B_SET_BDDP(cachep->f, f);
    B_SET_BDDP(cachep->g, g);
    B_SET_BDDP(cachep->h, bddfalse);
  }
  return 0;
}

} // namespace sapporobdd
