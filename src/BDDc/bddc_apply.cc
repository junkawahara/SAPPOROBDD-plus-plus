/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Dispatcher *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)           *
*  Main dispatcher for apply operations            *
******************************************/

#include "bddc_apply_common.h"

namespace sapporobdd {

/* Chooses between the recursive and the iterative implementation of a
   category.  The recursive one descends at most one level per frame, so it
   is safe exactly when the remaining recursion budget covers the number of
   levels; b_recursion_fits() tests that against the current depth, which
   matters when apply() is entered from inside another recursion (a BDD+
   operation, or the AND nested in BC_UNIV).  The test used to be
   "VarUsed >= APPLY_RECURSION_THRESHOLD" with the threshold equal to
   BDD_RecurLimit, i.e. it assumed the budget was untouched.  The switch can
   therefore also happen part-way down a recursive apply; the two versions
   share the cache and compute the same result. */
#ifdef B_FORCE_ITERATIVE
#  define APPLY_USE_ITERATIVE() 1 /* Force iterative version for testing */
#else
#  define APPLY_USE_ITERATIVE() (!b_recursion_fits())
#endif

bddp apply(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Main apply dispatcher - routes to category-specific functions.
   Memory exhaustion is reported by BDDOutOfMemoryException (from getnode()
   or from a failed growth of the iterative stack); the node-producing
   operations never return bddnull.  The counting operations return a
   number, with bddnull standing for a saturated count (see apply_count()).
   The operands have been validated by the public entry points. */
{
  switch(op)
  {
  /* Binary operations */
  case BC_AND:
  case BC_XOR:
  case BC_INTERSEC:
  case BC_UNION:
  case BC_SUBTRACT:
    if(APPLY_USE_ITERATIVE()) return apply_binary_iterative(f, g, op, skip);
    return apply_binary(f, g, op, skip);

  /* Unary operations (g is typically a variable index) */
  case BC_AT0:
  case BC_AT1:
  case BC_OFFSET:
  case BC_ONSET:
  case BC_CHANGE:
  case BC_LSHIFT:
  case BC_RSHIFT:
    if(APPLY_USE_ITERATIVE()) return apply_unary_iterative(f, g, op, skip);
    return apply_unary(f, g, op, skip);

  /* Counting operations */
  case BC_CARD:
  case BC_CARD2:
  case BC_LIT:
  case BC_LEN:
  case BC_SUPPORT:
    if(APPLY_USE_ITERATIVE()) return apply_count_iterative(f, g, op, skip);
    return apply_count(f, g, op, skip);

  /* Special operations */
  case BC_COFACTOR:
  case BC_UNIV:
    if(APPLY_USE_ITERATIVE()) return apply_special_iterative(f, g, op, skip);
    return apply_special(f, g, op, skip);

  default:
    err("apply: unknown opcode", op, ExceptionType::InternalError);
  }
}

/* Tests whether f AND g is the constant 0 without building the conjunction:
   the recursion stops at the first path on which both operands are true.

   The function shares the BC_AND entries of the operation cache with
   apply_binary(): it reads them (a cached conjunction is 0 or not), and it
   writes the negative answers as (f, g) -> bddfalse, which is exactly the
   entry apply_binary() would write for the same pair.  That is only correct
   while the two agree on everything that shapes an entry -- the trivial
   cases that never reach the cache, the swap of the operands (f < g), and
   the key -- so the BC_AND case of apply_binary() and this function have to
   be changed together.  Positive answers are not cached: the result node is
   not built, so there is nothing to store.

   Preconditions: f and g are BDDs (bddimply() refuses ZDD nodes; the child
   extraction below is the BDD one), and neither is bddnull.  The recursion
   descends one level per call and counts against the recursion budget;
   when the budget does not cover the levels, the conjunction is built by
   apply() instead, which then runs iteratively -- an iterative version of
   the search itself does not exist. */
int andfalse(bddp f, bddp g)
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, g0, g1, h;
  bddvar flev, glev;
  int r;

  assert(f != bddnull && g != bddnull);

  /* Check trivial cases */
  if(f == bddfalse || g == bddfalse || f == B_NOT(g)) return 0;
  if(f == bddtrue || g == bddtrue || f == g) return 1;
  /* Check operand swap (same direction as apply's BC_AND, so that both
     share the BC_AND cache entries) */
  if(f < g) { h = f; f = g; g = h; } /* swap (f, g) */

  /* Both operands are nodes from here on: every constant was answered
     above. */
  fp = B_NP(f);
  gp = B_NP(g);

  /* Non-trivial operations */
  /* Try cache? */
  if(B_RFC_ONE_NP(fp) && B_RFC_ONE_NP(gp)) key = bddnull;
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

  if(!b_recursion_fits())
  {
    /* Not enough budget for the search: build the conjunction (iteratively,
       by the same test) and look at it. */
    h = apply(f, g, BC_AND, 0);
    r = (h != bddfalse);
    bddfree(h);
    return r;
  }

  /* Get (f0, f1) and (g0, g1) */
  flev = Var[B_VAR_NP(fp)].lev;
  glev = Var[B_VAR_NP(gp)].lev;
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

  /* Stack overflow limiter.  The recursion descends one level per call just
     like apply's, so without the limiter a deep BDD overflows the machine
     stack instead of reporting the limit. */
  BDD_RECUR_INC;

  /* Get result.  || keeps the short circuit of the original two ifs, so the
     second branch is still skipped once the first one has answered 1. */
  r = (andfalse(f0, g0) == 1 || andfalse(f1, g1) == 1);

  /* Stack overflow limiter */
  BDD_RECUR_DEC;

  if(r) return 1;

  /* Saving to Cache.  The slot is recomputed: a deeper call that fell back
     to apply() may have built nodes and enlarged the cache since the key
     was taken (see APPLY_CACHE_STORE). */
  if(key != bddnull)
  {
    cachep = Cache + B_CACHEKEY(BC_AND, f, g);
    cachep->op = BC_AND;
    B_SET_BDDP(cachep->f, f);
    B_SET_BDDP(cachep->g, g);
    B_SET_BDDP(cachep->h, bddfalse);
  }
  return 0;
}

} // namespace sapporobdd
