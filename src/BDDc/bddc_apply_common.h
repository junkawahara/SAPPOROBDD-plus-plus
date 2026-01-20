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
 * Common macros for apply operations
 * These macros reduce code duplication across apply_*.cc files
 */

/* Cache check for binary operations (two BDD operands) */
#define APPLY_CACHE_CHECK_BINARY(op, f, g, key, cachep, result) \
  do { \
    if((B_CST(f) || B_RFC_ONE_NP(B_NP(f))) && \
       (B_CST(g) || B_RFC_ONE_NP(B_NP(g)))) { \
      key = bddnull; \
    } else { \
      key = B_CACHEKEY(op, f, g); \
      cachep = Cache + key; \
      if(cachep->op == op && \
         f == B_GET_BDDP(cachep->f) && \
         g == B_GET_BDDP(cachep->g)) { \
        result = B_GET_BDDP(cachep->h); \
        if(!B_CST(result) && result != bddnull) { \
          struct B_NodeTable *_np = B_NP(result); \
          B_RFC_INC_NP(_np); \
        } \
        return result; \
      } \
    } \
  } while(0)

/* Cache check for unary operations (one BDD operand) */
#define APPLY_CACHE_CHECK_UNARY(op, f, g, key, cachep, result) \
  do { \
    struct B_NodeTable *_fp = B_NP(f); \
    if(B_RFC_ONE_NP(_fp)) { \
      key = bddnull; \
    } else { \
      key = B_CACHEKEY(op, f, g); \
      cachep = Cache + key; \
      if(cachep->op == op && \
         f == B_GET_BDDP(cachep->f) && \
         g == B_GET_BDDP(cachep->g)) { \
        result = B_GET_BDDP(cachep->h); \
        if(!B_CST(result) && result != bddnull) { \
          struct B_NodeTable *_np = B_NP(result); \
          B_RFC_INC_NP(_np); \
        } \
        return result; \
      } \
    } \
  } while(0)

/* Cache check for count operations (numeric result, no RFC increment) */
#define APPLY_CACHE_CHECK_COUNT(op, f, key, cachep, result) \
  do { \
    struct B_NodeTable *_fp = B_NP(f); \
    if(B_RFC_ONE_NP(_fp)) { \
      key = bddnull; \
    } else { \
      key = B_CACHEKEY(op, f, bddfalse); \
      cachep = Cache + key; \
      if(cachep->op == op && \
         f == B_GET_BDDP(cachep->f) && \
         bddfalse == B_GET_BDDP(cachep->g)) { \
        return B_GET_BDDP(cachep->h); \
      } \
    } \
  } while(0)

/* Save result to cache */
#define APPLY_CACHE_STORE(key, op, f, g, h, cachep) \
  do { \
    if(key != bddnull) { \
      cachep = Cache + key; \
      cachep->op = op; \
      B_SET_BDDP(cachep->f, f); \
      B_SET_BDDP(cachep->g, g); \
      B_SET_BDDP(cachep->h, h); \
    } \
  } while(0)

/* Get child nodes for binary operations */
#define APPLY_GET_CHILDREN_BINARY(f, g, fp, gp, flev, glev, f0, f1, g0, g1, v, z) \
  do { \
    z = 0; \
    fp = B_NP(f); \
    flev = B_CST(f)? 0: Var[B_VAR_NP(fp)].lev; \
    gp = B_NP(g); \
    glev = B_CST(g)? 0: Var[B_VAR_NP(gp)].lev; \
    f0 = f; f1 = f; \
    g0 = g; g1 = g; \
    if(flev <= glev) { \
      v = B_VAR_NP(gp); \
      if(B_Z_NP(gp)) { \
        z = 1; \
        if(flev < glev) f1 = bddfalse; \
      } \
      g0 = B_GET_BDDP(gp->f0); \
      g1 = B_GET_BDDP(gp->f1); \
      if(B_NEG(g)^B_NEG(g0)) g0 = B_NOT(g0); \
      if(B_NEG(g) && !z) g1 = B_NOT(g1); \
    } \
    if(flev >= glev) { \
      v = B_VAR_NP(fp); \
      if(B_Z_NP(fp)) { \
        z = 1; \
        if(flev > glev) g1 = bddfalse; \
      } \
      f0 = B_GET_BDDP(fp->f0); \
      f1 = B_GET_BDDP(fp->f1); \
      if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0); \
      if(B_NEG(f) && !z) f1 = B_NOT(f1); \
    } \
  } while(0)

/* Get child nodes for unary operations */
#define APPLY_GET_CHILDREN_UNARY(f, fp, f0, f1, v, z) \
  do { \
    fp = B_NP(f); \
    v = B_VAR_NP(fp); \
    z = B_Z_NP(fp)? 1: 0; \
    f0 = B_GET_BDDP(fp->f0); \
    f1 = B_GET_BDDP(fp->f1); \
    if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0); \
    if(B_NEG(f) && !z) f1 = B_NOT(f1); \
  } while(0)

/* Get child nodes for count operations (no negation on f1) */
#define APPLY_GET_CHILDREN_COUNT(f, fp, f0, f1) \
  do { \
    fp = B_NP(f); \
    f0 = B_GET_BDDP(fp->f0); \
    f1 = B_GET_BDDP(fp->f1); \
    if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0); \
  } while(0)

/* Recursive call with overflow check (for operations returning BDD) */
#define APPLY_RECURSE_CHECK(h, call) \
  do { \
    h = call; \
    if(h == bddnull) break; \
  } while(0)

/* Helper for standard binary recursion pattern */
#define APPLY_BINARY_RECURSE(h, h0, h1, f0, f1, g0, g1, op, v, z) \
  do { \
    h0 = apply(f0, g0, op, 0); \
    if(h0 == bddnull) { h = h0; break; } \
    h1 = apply(f1, g1, op, 0); \
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } \
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1); \
    if(h == bddnull) { bddfree(h0); bddfree(h1); } \
  } while(0)

/* Helper for unary recursion with variable g */
#define APPLY_UNARY_RECURSE(h, h0, h1, f0, f1, g, op, v, z) \
  do { \
    h0 = apply(f0, g, op, 0); \
    if(h0 == bddnull) { h = h0; break; } \
    h1 = apply(f1, g, op, 0); \
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } \
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1); \
    if(h == bddnull) { bddfree(h0); bddfree(h1); } \
  } while(0)

/* Declarations of category-specific apply functions */
bddp apply_binary(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_unary(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_count(bddp f, bddp g, unsigned char op, unsigned char skip);
bddp apply_special(bddp f, bddp g, unsigned char op, unsigned char skip);

} // namespace sapporobdd

#endif /* BDDC_APPLY_COMMON_H */
