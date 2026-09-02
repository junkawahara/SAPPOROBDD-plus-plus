/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Common Macros *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)              *
*  Split from bddc_apply.cc for modularity            *
******************************************/

#ifndef BDDC_APPLY_COMMON_H
#define BDDC_APPLY_COMMON_H

#include "bddc_internal.h"

namespace sapporobdd {

/*
 * Macros shared by the recursive apply_*.cc files.  Only the macros that the
 * implementations really use are kept here: a set of cache-check and
 * recursion helpers that nothing referred to used to sit beside them, and
 * they had drifted from the code (no clamping of a BC_CARD2 table reference
 * read as a count, no release of h0/h1 when getnode() throws), so a reader
 * who reused them would have reintroduced fixed bugs.
 *
 * Every macro argument is evaluated more than once, so the arguments have to
 * be plain variables or constants without side effects.
 *
 * Error reporting: get{b,z}ddp() and the recursions report memory exhaustion
 * by BDDOutOfMemoryException, never by returning bddnull.  The bddnull tests
 * that remain in the apply_*.cc files are a second line of defence (the
 * count operations do use bddnull, as a saturated count), not the way a
 * failure is expected to arrive.
 */

/* Save result to cache */
/* An h of bddnull means the operation ran out of memory.  That is a property
   of the current memory state, not of (op, f, g), so it must not be cached:
   the entry would survive bddgc() (B_NP(bddnull) is outside the node table,
   so the cache sweep never clears it) and keep reporting a stale failure
   even after memory has been reclaimed.
   The slot is recomputed here rather than taken from the key computed
   before the recursion: a getnode() inside the recursion may have enlarged
   the cache, and the old key then names a slot the lookup for (op, f, g)
   never visits, so the entry would have been written where nothing finds
   it.  key is still needed to tell whether the result is to be cached at
   all (bddnull: both operands are referenced only once). */
#define APPLY_CACHE_STORE(key_, op_, f_, g_, h_, cachep_) \
  do { \
    if((key_) != bddnull && (h_) != bddnull) { \
      (cachep_) = Cache + B_CACHEKEY((op_), (f_), (g_)); \
      (cachep_)->op = (op_); \
      B_SET_BDDP((cachep_)->f, (f_)); \
      B_SET_BDDP((cachep_)->g, (g_)); \
      B_SET_BDDP((cachep_)->h, (h_)); \
    } \
  } while(0)

/* Get child nodes for binary operations.
   Preconditions: f and g are not both constant, neither is bddnull, and
   both are of the same kind (BDD or ZDD) -- the public entry points check
   the kind, and a BDD node next to a ZDD node would have the z flag of one
   operand govern the negative-edge handling of the other.  The node pointer
   of a constant operand is never formed.  (The macro parameters carry a
   trailing underscore so that they cannot be mistaken for the node table
   fields of the same names inside the macro body.) */
#define APPLY_GET_CHILDREN_BINARY(f_, g_, fp_, gp_, flev_, glev_, f0_, f1_, g0_, g1_, v_, z_) \
  do { \
    (z_) = 0; \
    (fp_) = B_CST(f_)? 0: B_NP(f_); \
    (flev_) = (fp_)? Var[B_VAR_NP(fp_)].lev: 0; \
    (gp_) = B_CST(g_)? 0: B_NP(g_); \
    (glev_) = (gp_)? Var[B_VAR_NP(gp_)].lev: 0; \
    (f0_) = (f_); (f1_) = (f_); \
    (g0_) = (g_); (g1_) = (g_); \
    if((flev_) <= (glev_)) { \
      (v_) = B_VAR_NP(gp_); \
      if(B_Z_NP(gp_)) { \
        (z_) = 1; \
        if((flev_) < (glev_)) (f1_) = bddfalse; \
      } \
      (g0_) = B_GET_BDDP((gp_)->f0); \
      (g1_) = B_GET_BDDP((gp_)->f1); \
      if(B_NEG(g_)^B_NEG(g0_)) (g0_) = B_NOT(g0_); \
      if(B_NEG(g_) && !(z_)) (g1_) = B_NOT(g1_); \
    } \
    if((flev_) >= (glev_)) { \
      (v_) = B_VAR_NP(fp_); \
      if(B_Z_NP(fp_)) { \
        (z_) = 1; \
        if((flev_) > (glev_)) (g1_) = bddfalse; \
      } \
      (f0_) = B_GET_BDDP((fp_)->f0); \
      (f1_) = B_GET_BDDP((fp_)->f1); \
      if(B_NEG(f_)^B_NEG(f0_)) (f0_) = B_NOT(f0_); \
      if(B_NEG(f_) && !(z_)) (f1_) = B_NOT(f1_); \
    } \
  } while(0)

/* Get child nodes for unary operations.  Precondition: f is a node. */
#define APPLY_GET_CHILDREN_UNARY(f_, fp_, f0_, f1_, v_, z_) \
  do { \
    (fp_) = B_NP(f_); \
    (v_) = B_VAR_NP(fp_); \
    (z_) = B_Z_NP(fp_)? 1: 0; \
    (f0_) = B_GET_BDDP((fp_)->f0); \
    (f1_) = B_GET_BDDP((fp_)->f1); \
    if(B_NEG(f_)^B_NEG(f0_)) (f0_) = B_NOT(f0_); \
    if(B_NEG(f_) && !(z_)) (f1_) = B_NOT(f1_); \
  } while(0)

/* Get child nodes for count operations (no negation on f1).
   Precondition: f is a node. */
#define APPLY_GET_CHILDREN_COUNT(f_, fp_, f0_, f1_) \
  do { \
    (fp_) = B_NP(f_); \
    (f0_) = B_GET_BDDP((fp_)->f0); \
    (f1_) = B_GET_BDDP((fp_)->f1); \
    if(B_NEG(f_)^B_NEG(f0_)) (f0_) = B_NOT(f0_); \
  } while(0)

/* Declarations of category-specific apply functions */
bddp apply_binary(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_unary(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_count(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_special(bddp f, bddp g, unsigned char op, unsigned char skip);

/* ============================================================
 * Iterative (non-recursive) apply implementation
 * Used when the recursive version would not fit into the recursion budget,
 * see b_recursion_fits() in bddc_internal.h: with no other recursion under
 * way that is VarUsed >= BDD_RecurLimit (a graph over exactly that many
 * variables reaches that depth).
 * ============================================================ */

/* Stack frame for iterative apply */
struct ApplyStackFrame {
    /* Input operands */
    bddp f, g;
    unsigned char op;
    unsigned char skip;

    /* State machine: 0 = init, 1 = after h0, 2 = after h1,
       3 = waiting for the result to negate (the XOR, AT0/AT1/OFFSET,
       LSHIFT/RSHIFT negation paths, and the +1 of a negated BC_CARD),
       4 = waiting for the single recursion of BC_COFACTOR and BC_UNIV */
    unsigned char state;

    /* Extracted child nodes */
    bddp f0, f1, g0, g1;
    bddvar v;
    char z;

    /* Intermediate results */
    bddp h0, h1;
    bddp key;

    /* Result to return to parent frame */
    bddp result;
};

/* Initial stack capacity */
#define APPLY_STACK_INIT_SIZE 256

/* Stack structure for iterative apply */
struct ApplyStack {
    struct ApplyStackFrame *frames;
    bddp top;       /* Number of frames in use (0 = empty) */
    bddp capacity;  /* Current capacity */
};

/* Declarations of iterative apply functions */
bddp apply_binary_iterative(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_unary_iterative(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_count_iterative(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_special_iterative(bddp f, bddp g, unsigned char op, unsigned char skip);

} // namespace sapporobdd

#endif /* BDDC_APPLY_COMMON_H */
