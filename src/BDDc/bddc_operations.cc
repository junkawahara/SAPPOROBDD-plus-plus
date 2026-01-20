/*****************************************
*  BDD Package (SAPPORO-1.94)   - Operations  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

bddp bddcopy(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f; /* Constant */
  fp = B_NP(f);
  if(fp >= Node+NodeSpc || fp->varrfc == 0)
    err("bddcopy: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  B_RFC_INC_NP(fp);
  return f;
}

void bddfree(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return;
  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);
  if(fp >= Node+NodeSpc || fp->varrfc == 0)
    err("bddfree: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  B_RFC_DEC_NP(fp);
}

bddp bddnot(bddp f)
{
  if(f == bddnull) return bddnull;
  return B_NOT(bddcopy(f));
}

bddvar bddlevofvar(bddvar v)
{
  if(v > VarUsed)
    err("bddlevofvar: Invalid VarID", v, ExceptionType::OutOfRange);
  return Var[v].lev;
}

bddvar bddvaroflev(bddvar lev)
{
  if(lev > VarUsed)
    err("bddvaroflev: Invalid level", lev, ExceptionType::OutOfRange);
  return VarID[lev];
}

bddvar bddvarused()
{
  return VarUsed;
}

bddvar bddnewvar()
{
  if(++VarUsed == VarSpc) var_enlarge();
  return VarUsed;
}

bddvar bddnewvaroflev(bddvar lev)
{
  bddvar i;

  if(lev == 0 || lev > ++VarUsed)
    err("bddnewvaroflev: Invalid level", lev, ExceptionType::OutOfRange);
  if(VarUsed == VarSpc) var_enlarge();
  for(i=VarUsed; i>lev; i--) Var[ VarID[i] = VarID[i-1U] ].lev = i;
  Var[ VarID[lev] = VarUsed ].lev = lev;
  return VarUsed;
}

bddvar bddtop(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0; /* Constant */
  fp = B_NP(f);
  if(fp >= Node+NodeSpc || fp->varrfc == 0)
    err("bddtop: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  return B_VAR_NP(fp);
}

bddp    bddprime(bddvar v)
/* Returns bddnull if not enough memory */
{
        if(v == 0 || v > VarUsed)
		err("bddprime: Invalid VarID", v, ExceptionType::InvalidBDDValue);
        return getbddp(v, bddfalse, bddtrue);
}

bddp bddand(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddand: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddand: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddand: applying ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddand: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddand: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddand: applying ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_AND, 0);
}

bddp bddor(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  bddp h;

  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  h = bddand(B_NOT(f), B_NOT(g));
  if(h == bddnull) return bddnull;
  return B_NOT(h);
}

bddp bddxor(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddand: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddxor: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddand: applying ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddand: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddxor: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddand: applying ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_XOR, 0);
}

bddp bddnand(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  bddp h;

  h = bddand(f, g);
  if(h == bddnull) return bddnull;
  return B_NOT(h);
}

bddp bddnor(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  return bddand(B_NOT(f), B_NOT(g));
}

bddp bddxnor(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  if(g == bddnull) return bddnull;
  return bddxor(f, B_NOT(g));
}

bddp bddcofactor(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddcofactor: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddcofactor: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddcofactor: applying ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddcofactor: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddcofactor: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddcofactor: applying ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_COFACTOR, 0);
}

bddp bdduniv(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bdduniv: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bdduniv: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bdduniv: applying ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bdduniv: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bdduniv: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bdduniv: applying ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_UNIV, 0);
}

bddp bddexist(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  bddp h;

  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  h = bdduniv(B_NOT(f), g);
  if(h == bddnull) return bddnull;
  return B_NOT(h);
}

int bddimply(bddp f, bddp g)
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return 0;
  if(g == bddnull) return 0;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddimply: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddimply: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddimply: applying ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddimply: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddimply: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(B_Z_NP(fp)) err("bddimply: applying ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return ! andfalse(f, B_NOT(g));
}

bddp bddsupport(bddp f)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return bddfalse;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddsupport: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return apply(f, bddfalse, BC_SUPPORT, 0);
}

bddp bddat0(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddat0: Invalid VarID", v, ExceptionType::InvalidBDDValue);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddat0: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)v, BC_AT0, 0);
}

bddp bddat1(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddat1: Invalid VarID", v, ExceptionType::InvalidBDDValue);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddat1: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)v, BC_AT1, 0);
}

bddp bddlshift(bddp f, bddvar shift)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(shift >= VarUsed)
    err("bddlshift: Invalid shift", shift, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f;
  if(shift == 0) return bddcopy(f);
  if((fp=B_NP(f))>=Node+NodeSpc || !fp->varrfc)
    err("bddlshift: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)shift, BC_LSHIFT, 0);
}

bddp bddrshift(bddp f, bddvar shift)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(shift >= VarUsed)
    err("bddrshift: Invalid shift", shift, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f;
  if(shift == 0) return bddcopy(f);
  if((fp=B_NP(f))>=Node+NodeSpc || !fp->varrfc)
    err("bddrshift: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)shift, BC_RSHIFT, 0);
}

bddp    bddoffset(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddoffset: Invalid VarID", v, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddoffset: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  if(!B_Z_NP(fp)) err("bddoffset: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)v, BC_OFFSET, 0);
}

bddp    bddonset0(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddonset0: Invalid VarID", v, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;
  if(B_CST(f)) return bddfalse;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddonset0: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  if(!B_Z_NP(fp)) err("bddonset0: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);

  return apply(f, (bddp)v, BC_ONSET, 0);
}

bddp    bddonset(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  bddp g, h;

  g = bddonset0(f, v);
  h = bddchange(g, v);
  bddfree(g);
  return h;
}

bddp    bddchange(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddchange: Invalid VarID", v, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;
  if(!B_CST(f))
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddchange: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddchange: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);
  }

  return apply(f, (bddp)v, BC_CHANGE, 0);
}

bddp bddintersec(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddintersec: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddintersec: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddintersec: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddintersec: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddintersec: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddintersec: applying non-ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_INTERSEC, 0);
}

bddp bddunion(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddunion: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddunion: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddunion: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddunion: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddunion: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddunion: applying non-ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_UNION, 0);
}

bddp bddsubtract(bddp f, bddp g)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(f == bddnull) return bddnull;
  if(g == bddnull) return bddnull;
  if(B_CST(f))
  { if(B_ABS(f) != bddfalse) err("bddsubtract: Invalid bddp", f, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddsubtarct: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddsubtarct: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);
  }
  if(B_CST(g))
  { if(B_ABS(g) != bddfalse) err("bddsubtarct: Invalid bddp", g, ExceptionType::InvalidBDDValue); }
  else
  {
    fp = B_NP(g);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddsubtarct: Invalid bddp", g, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddsubtarct: applying non-ZDD node", g, ExceptionType::InvalidBDDValue);
  }

  return apply(f, g, BC_SUBTRACT, 0);
}

bddp bddcard(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return (f == bddfalse)? 0: 1;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddcard: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  if(!B_Z_NP(fp)) err("bddcard: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);

  return apply(f, bddfalse, BC_CARD, 0);
}

bddp bddlit(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddlit: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  if(!B_Z_NP(fp)) err("bddlit: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);

  return apply(f, bddfalse, BC_LIT, 0);
}

bddp bddlen(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0;
  fp = B_NP(f);
  if(fp>=Node+NodeSpc || !fp->varrfc)
    err("bddlen: Invalid bddp", f, ExceptionType::InvalidBDDValue);
  if(!B_Z_NP(fp)) err("bddlen: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);

  return apply(f, bddfalse, BC_LEN, 0);
}

char *bddcardmp16(bddp f, char *s)
{
  struct B_NodeTable *fp;
  int i, j, k, nz;
  struct B_MP mp;
  bddp h, d;

  mp.len = 1;
  if(f == bddnull) mp.word[0] = 0;
  else if(B_CST(f)) mp.word[0] = (f == bddtrue)? 1: 0;
  else
  {
    fp = B_NP(f);
    if(fp>=Node+NodeSpc || !fp->varrfc)
      err("bddcardmp16: Invalid bddp", f, ExceptionType::InvalidBDDValue);
    if(!B_Z_NP(fp)) err("bddcardmp16: applying non-ZDD node", f, ExceptionType::InvalidBDDValue);
    h = apply(B_ABS(f), bddfalse, BC_CARD2, 0);
    if(h == B_MP_NULL) mp.len = 0;
    else
    {
      mp.word[0] = B_NEG(f)? 1: 0;
      mp_add(&mp, h);
    }
  }
  if(!s) s = B_MALLOC(char, mp.len*sizeof(bddp)*2+1);
  if(!s)
    err("bddcardmp16: memory allocation failed", mp.len*sizeof(bddp)*2+1,
        ExceptionType::OutOfMemory);
  k = 0;
  nz = 0;
  for(i=mp.len-1; i>=0; i--)
    for(j=sizeof(bddp)*2-1; j>=0; j--)
    {
      d = (mp.word[i] >> (j*4) ) & 15;
      if(d) nz = 1;
      if(nz) s[k++] = "0123456789ABCDEF"[d];
    }
  if(!nz && mp.len) s[k++] = '0';
  s[k++] = 0;

#ifdef DEBUG
  for(i=0; i<B_MP_LMAX; i++)
  {
    printf("%d: ", i);
    printf(B_BDDP_FD, mptable[i].size);
    printf("\n");
  }
#endif

  return s;
}

int bddisbdd(bddp f)
{
  struct B_NodeTable* fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 1;
  if((fp=B_NP(f))>=Node+NodeSpc || !fp->varrfc)
    err("bddisbdd: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return (B_NEG(B_GET_BDDP(fp->f0)) ? 0 : 1);
}

int bddiszdd(bddp f)
{
  struct B_NodeTable* fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 1;
  if((fp=B_NP(f))>=Node+NodeSpc || !fp->varrfc)
    err("bddiszdd: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  return (B_NEG(B_GET_BDDP(fp->f0)) ? 1 : 0);
}

// for compatibility
int bddiszbdd(bddp f)
{
  return bddiszdd(f);
}

bddp    bddpush(bddp f, bddvar v)
/* Returns bddnull if not enough memory */
{
  struct B_NodeTable *fp;

  /* Check operands */
  if(v > VarUsed || v == 0) err("bddpush: Invalid VarID", v, ExceptionType::OutOfRange);
  if(f == bddnull) return bddnull;

  if(!B_CST(f)) { fp = B_NP(f); B_RFC_INC_NP(fp); }
  return getzddp(v, bddfalse, f);
}

void bddsetcacheratio(double cacheRatio)
/* Set cache size ratio (must be power of 2: ..., 0.25, 0.5, 1, 2, 4, ...) */
{
  /* throw an exception if cacheRatio is illegal */
  setcacheratiovalue(cacheRatio);
  if (!allocatecache()) {
    err("bddsetcacheratio: memory allocation failed", 0, ExceptionType::OutOfMemory);
  }
}

double bddgetcacheratio(void)
/* Get current cache size ratio */
{
  return CacheRatio;
}

void bddsetgcthreshold(bddp threshold)
/* Set GC threshold - minimum nodes that must be freed for successful GC */
{
  GCThreshold = threshold;
}

bddp bddgetgcthreshold(void)
/* Get current GC threshold */
{
  return GCThreshold;
}

} // namespace sapporobdd
