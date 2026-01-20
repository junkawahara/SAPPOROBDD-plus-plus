/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

bddp apply(bddp f, bddp g, unsigned char op, unsigned char skip)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  /* Some initial values are not used, but
  we set them to suppress compiler warnings */
  bddp key, f0, f1, g0 = 0, g1 = 0, h0, h1, h;
  bddvar v = 0, flev, glev;
  char z = 0; /* flag to check ZDD node */

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

  case BC_SUPPORT:
    if(B_CST(f)) return bddfalse;
    if(B_NEG(f)) f = B_NOT(f);
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
    err("apply: unknown opcode", op, ExceptionType::InternalError);
    break;
  }

  /* Non-trivial operations */
  switch(op)
  {
  /* binary operation */
  case BC_AND:
  case BC_XOR:
  case BC_COFACTOR:
  case BC_UNIV:
  case BC_INTERSEC:
  case BC_UNION:
  case BC_SUBTRACT:
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
    z = 0;
    fp = B_NP(f);
    flev = B_CST(f)? 0: Var[B_VAR_NP(fp)].lev;
    gp = B_NP(g);
    glev = B_CST(g)? 0: Var[B_VAR_NP(gp)].lev;
    f0 = f; f1 = f;
    g0 = g; g1 = g;

    if(flev <= glev)
    {
      v = B_VAR_NP(gp);
      if(B_Z_NP(gp))
      {
        z = 1;
        if(flev < glev) f1 = bddfalse;
      }
      g0 = B_GET_BDDP(gp->f0);
      g1 = B_GET_BDDP(gp->f1);
      if(B_NEG(g)^B_NEG(g0)) g0 = B_NOT(g0);
      if(B_NEG(g) && !z) g1 = B_NOT(g1);
    }

    if(flev >= glev)
    {
      v = B_VAR_NP(fp);
      if(B_Z_NP(fp))
      {
        z = 1;
        if(flev > glev) g1 = bddfalse;
      }
      f0 = B_GET_BDDP(fp->f0);
      f1 = B_GET_BDDP(fp->f1);
      if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0);
      if(B_NEG(f) && !z) f1 = B_NOT(f1);
    }
    break;

  /* unary operation */
  case BC_AT0:
  case BC_AT1:
  case BC_LSHIFT:
  case BC_RSHIFT:
  case BC_SUPPORT:
  case BC_OFFSET:
  case BC_ONSET:
  case BC_CHANGE:
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
    /* Get (f0, f1)*/
    v = B_VAR_NP(fp);
    z = B_Z_NP(fp)? 1: 0;
    f0 = B_GET_BDDP(fp->f0);
    f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0);
    if(B_NEG(f) && !z) f1 = B_NOT(f1);
    break;

  case BC_CARD:
  case BC_LIT:
  case BC_LEN:
    fp = B_NP(f);
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
    /* Get (f0, f1)*/
    f0 = B_GET_BDDP(fp->f0);
    f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0);
    break;

  case BC_CARD2:
    fp = B_NP(f);
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
    /* Get (f0, f1)*/
    f0 = B_GET_BDDP(fp->f0);
    f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)^B_NEG(f0)) f0 = B_NOT(f0);
    break;

  default:
    err("apply: unknown opcode", op, ExceptionType::InternalError);
  }

  /* Stack overflow limitter */
  BDD_RECUR_INC;

  /* Get result node */
  switch(op)
  {
  case BC_AND:
  case BC_XOR:
  case BC_INTERSEC:
  case BC_UNION:
  case BC_SUBTRACT:
    h0 = apply(f0, g0, op, 0);
    if(h0 == bddnull) { h = h0; break; } /* Overflow */
    h1 = apply(f1, g1, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
    if(h == bddnull) { bddfree(h0); bddfree(h1); } /* Overflow */
    break;

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
      if(h0 == bddnull) { h = h0; break; } /* Overflow */
      h1 = apply(f1, g1, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
      h = getbddp(v, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); } /* Overflow */
    }
    break;

  case BC_UNIV:
    if(g0 != g1)
    {
      h0 = apply(f0, g0, op, 0);
      if(h0 == bddnull) { h = h0; break; } /* Overflow */
      h1 = apply(f1, g0, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
      h = apply(h0, h1, BC_AND, 0);
      bddfree(h0); bddfree(h1);
    }
    else
    {
      h0 = apply(f0, g0, op, 0);
      if(h0 == bddnull) { h = h0; break; } /* Overflow */
      h1 = apply(f1, g0, op, 0);
      if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
      h = getbddp(v, h0, h1);
      if(h == bddnull) { bddfree(h0); bddfree(h1); } /* Overflow */
    }
    break;

  case BC_AT0:
  case BC_AT1:
  case BC_OFFSET:
  case BC_ONSET:
  case BC_CHANGE:
    h0 = apply(f0, g, op, 0);
    if(h0 == bddnull) { h = h0; break; } /* Overflow */
    h1 = apply(f1, g, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
    if(h == bddnull) { bddfree(h0); bddfree(h1); } /* Overflow */
    break;

  case BC_SUPPORT:
    h0 = apply(f0, bddfalse, op, 0);
    if(h0 == bddnull) { h = h0; break; } /* Overflow */
    h1 = apply(f1, bddfalse, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
    h = z? apply(h0, h1, BC_UNION, 0):
           apply(B_NOT(h0), B_NOT(h1), BC_AND, 0);
    bddfree(h0); bddfree(h1);
    if(h == bddnull) break; /* Overflow */
    h0 = h;
    h = z? getzddp(v, h0, bddtrue):
           getbddp(v, B_NOT(h0), bddtrue);
    if(h == bddnull) bddfree(h0); /* Overflow */
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
    if(h0 == bddnull) { h = h0; break; } /* Overflow */
    h1 = apply(f1, g, op, 0);
    if(h1 == bddnull) { bddfree(h0); h = h1; break; } /* Overflow */
    h = z? getzddp(v, h0, h1): getbddp(v, h0, h1);
    if(h == bddnull) { bddfree(h0); bddfree(h1); } /* Overflow */
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
    err("apply: unknown opcode", op, ExceptionType::InternalError);
    break;
  }

  /* Stack overflow limitter */
  BDD_RECUR_DEC;

  /* Saving to Cache */
  if(key != bddnull)
  {
    cachep = Cache + key;
    cachep->op = op;
    if(op == BC_CARD2) cachep->op = BC_CARD;
    B_SET_BDDP(cachep->f, f);
    B_SET_BDDP(cachep->g, g);
    B_SET_BDDP(cachep->h, h);
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

int andfalse(bddp f, bddp g)
{
  struct B_NodeTable *fp, *gp;
  struct B_CacheTable *cachep;
  bddp key, f0, f1, g0, g1, h;
  bddvar flev, glev;

  /* Check trivial cases */
  if(f == bddfalse || g == bddfalse || f == B_NOT(g)) return 0;
  if(f == bddtrue || g == bddtrue || f == g) return 1;
  /* Check operand swap */
  if(f > g) { h = f; f = g; g = h; } /* swap (f, g) */

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
