/****************************************
 * ZDD+ Manipulator (SAPPORO-1.87)     *
 * (Main part)                          *
 * (C) Shin-ichi MINATO (May 14, 2021)  *
 ****************************************/

#include <cstdlib>
#include <memory>
#include <new>

#include "ZDD.h"

#define BDD_CPP
#include "bddc.h"
#include "BDDException.h"
#include "bddplus_internal.h"

using std::cout;

namespace sapporobdd {

static const unsigned char BC_ZDD_MULT = 20;
static const unsigned char BC_ZDD_DIV = 21;
static const unsigned char BC_ZDD_RSTR = 22;
static const unsigned char BC_ZDD_PERMIT = 23;
static const unsigned char BC_ZDD_PERMITSYM = 24;
static const unsigned char BC_ZDD_SYMCHK = 25;
static const unsigned char BC_ZDD_ALWAYS = 26;
static const unsigned char BC_ZDD_SYMSET = 27;
static const unsigned char BC_ZDD_COIMPSET = 28;
static const unsigned char BC_ZDD_MEET = 29;

static const unsigned char BC_ZDD_ZSkip = 65;
static const unsigned char BC_ZDD_INTERSEC = 66;

// class ZDD ---------------------------------------------

void ZDD::Export(FILE *strm) const
{
  /* an error ZDD used to be written out as an empty file, which re-imported
     as the constant 0: the error became a normal value for good */
  if(_zdd == bddnull)
    BDDerr("ZDD::Export: Cannot export the error ZDD.", ExceptionType::InvalidBDDValue);
  bddword p = _zdd;
  bddexport(strm, &p, 1);
}

void ZDD::Print() const
{
  /* the error value used to be printed as a huge ID with all counts 0,
     indistinguishable from a real (empty) ZDD without knowing the number */
  if(_zdd == bddnull)
  {
    cout << "[ null (error ZDD) ]\n";
    cout.flush();
    return;
  }
  cout << "[ " << GetID();
  cout << " Var:" << Top() << "(" << BDD_LevOfVar(Top()) << ")";
  cout << " Size:" << Size() << " Card:";
  cout << Card() << " Lit:" << Lit() << " Len:" << Len() << " ]\n";
  cout.flush();
}

/* returns 1 when nothing could be printed (the error ZDD, or a failure in a
   component); the void version silently printed nothing for the error ZDD,
   leaving the caller no way to tell it from a legitimate empty output */
int ZDD::PrintPla() const { return ZDDV(*this).PrintPla(); }

#define ZDD_CACHE_CHK_RETURN(op, fx, gx) \
  { ZDD h = BDD_CacheZDD(op, fx, gx); \
    if(h != -1) return h; \
    BDD_RECUR_INC; }

#define ZDD_CACHE_ENT_RETURN(op, fx, gx, h) \
  { BDD_RECUR_DEC; \
    if(h != -1) BDD_CacheEnt(op, fx, gx, h.GetID()); \
    return h; }

ZDD ZDD::Swap(int v1, int v2) const
{
  /* validate before the early return, so that v1 == v2 (or a constant
     operand inside OffSet/OnSet) does not silently skip the check */
  if(v1 <= 0 || v1 > BDD_VarUsed())
    BDDerr("ZDD::Swap: Invalid VarID.", (bddword)v1, ExceptionType::OutOfRange);
  if(v2 <= 0 || v2 > BDD_VarUsed())
    BDDerr("ZDD::Swap: Invalid VarID.", (bddword)v2, ExceptionType::OutOfRange);
  if(v1 == v2) return *this;
  ZDD f00 = this->OffSet(v1).OffSet(v2);
  ZDD f11 = this->OnSet(v1).OnSet(v2);
  ZDD h = *this - f00 - f11;
  return h.Change(v1).Change(v2) + f00 + f11;
}

ZDD ZDD::Restrict(const ZDD& g) const
{
  if(*this == -1) return -1;
  if(g == -1) return -1;
  if(*this == 0) return 0;
  if(g == 0) return 0;
  if(*this == g) return g;
  if((g & 1) == 1) return *this;
  ZDD f = *this - 1;

  int top = f.Top();
  if(BDD_LevOfVar(top) < BDD_LevOfVar(g.Top())) top = g.Top();

  bddword fx = f.GetID();
  bddword gx = g.GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_RSTR, fx, gx);

  ZDD f1 = f.OnSet0(top);
  ZDD f0 = f.OffSet(top);
  ZDD g1 = g.OnSet0(top);
  ZDD g0 = g.OffSet(top);
  ZDD h = f1.Restrict(g1 + g0).Change(top) + f0.Restrict(g0);

  ZDD_CACHE_ENT_RETURN(BC_ZDD_RSTR, fx, gx, h);
}

ZDD ZDD::Permit(const ZDD& g) const
{
  if(*this == -1) return -1;
  if(g == -1) return -1;
  if(*this == 0) return 0;
  if(g == 0) return 0;
  if(*this == g) return *this;
  if(g == 1) return *this & 1;
  if(*this == 1) return 1;

  int top = Top();
  if(BDD_LevOfVar(top) < BDD_LevOfVar(g.Top())) top = g.Top();

  bddword fx = GetID();
  bddword gx = g.GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_PERMIT, fx, gx);

  ZDD f1 = OnSet0(top);
  ZDD f0 = OffSet(top);
  ZDD g1 = g.OnSet0(top);
  ZDD g0 = g.OffSet(top);
  ZDD h = f1.Permit(g1).Change(top) + f0.Permit(g0 + g1);

  ZDD_CACHE_ENT_RETURN(BC_ZDD_PERMIT, fx, gx, h);
}

ZDD ZDD::PermitSym(int n) const
{
  if(*this == -1) return -1;
  if(*this == 0) return 0;
  /* "at most n items" leaves nothing at all for a negative n, not even the
     empty combination, whose size is 0.  A negative n used to be treated
     like n == 0 and kept the empty combination. */
  if(n < 0) return 0;
  if(*this == 1) return 1;
  if(n == 0) return *this & 1;

  int top = Top();

  bddword fx = GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_PERMITSYM, fx, n);

  ZDD f1 = OnSet0(top);
  ZDD f0 = OffSet(top);
  ZDD h = f1.PermitSym(n - 1).Change(top) + f0.PermitSym(n);

  ZDD_CACHE_ENT_RETURN(BC_ZDD_PERMITSYM, fx, n, h);
}

ZDD ZDD::Always() const
{
  if(*this == -1) return -1;
  if(*this == 0 || *this == 1) return 0;

  bddword fx = GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_ALWAYS, fx, 0);

  int t = Top();
  ZDD f1 = OnSet0(t);
  ZDD f0 = OffSet(t);
  ZDD h = f1.Always();
  if(f0 == 0) h += ZDD(1).Change(t);
  else if(h != 0) h &= f0.Always();

  ZDD_CACHE_ENT_RETURN(BC_ZDD_ALWAYS, fx, 0, h);
}

int ZDD::SymChk(int v1, int v2) const
{
  if(*this == -1) return -1;
  if(v1 <= 0 || v1 > BDD_VarUsed()) BDDerr("ZDD::SymChk(): invalid v1.", v1, ExceptionType::OutOfRange);
  if(v2 <= 0 || v2 > BDD_VarUsed()) BDDerr("ZDD::SymChk(): invalid v2.", v2, ExceptionType::OutOfRange);
  if(*this == 0 || *this == 1) return 1;
  if(v1 == v2) return 1;
  if(BDD_LevOfVar(v1) < BDD_LevOfVar(v2)) { int tmp = v1; v1 = v2; v2 = tmp; }

  ZDD S = ZDD(1).Change(v1) + ZDD(1).Change(v2);
  /* an OOM here would leave gx = bddnull below, a cache key shared by every
     variable pair whose S failed, and a later pair could hit this pair's
     cached answer */
  if(S == -1) return -1;
  bddword fx = GetID();
  bddword gx = S.GetID();
  /* The miss value of BDD_CacheInt() is bddnull, which has to be compared as
     a bddword: truncating it to int only happens to yield -1 in the 64 bit
     build, whereas in the 32 bit build bddnull is INT_MAX, so every miss would
     be taken for a hit and SymChk would answer INT_MAX. */
  bddword c = BDD_CacheInt(BC_ZDD_SYMCHK, fx, gx);
  if(c != bddnull) return (int)c;
  int Y;
  BDD_RECUR_INC;

  int t = Top();
  if(BDD_LevOfVar(t) > BDD_LevOfVar(v1))
  {
    Y = OnSet0(t).SymChk(v1, v2);
    if(Y == 1) Y = OffSet(t).SymChk(v1, v2);
  }
  else
  {
    ZDD f0 = OffSet(v1);
    ZDD f1 = OnSet0(v1);
    int t0 = f0.Top();
    int t1 = f1.Top();
    int t2 = (BDD_LevOfVar(t0) > BDD_LevOfVar(t1))? t0: t1;
    if(BDD_LevOfVar(t2) <= BDD_LevOfVar(v2))
    {
      /* compare only after checking both sides: two failed (-1) results
         would compare equal, be taken for "symmetric", and be cached */
      ZDD c0 = f0.OnSet0(v2);
      ZDD c1 = f1.OffSet(v2);
      Y = (c0 == -1 || c1 == -1)? -1: (c0 == c1);
    }
    else
    {
      ZDD g0 = f0.OffSet(t2) + f1.OffSet(t2).Change(t2);
      ZDD g1 = f0.OnSet0(t2) + f1.OnSet0(t2).Change(t2);
      Y = g1.SymChk(t2, v2);
      if(Y == 1) Y = g0.SymChk(t2, v2);
    }
  }

  BDD_RECUR_DEC;
  if(Y != -1) BDD_CacheEnt(BC_ZDD_SYMCHK, fx, gx, Y);
  return Y;
}

ZDD ZDD::SymGrp() const
{
  if(*this == -1) return -1;
  ZDD h = 0;
  ZDD g = Support();
  while(g != 0)
  {
    /* Support() and OffSet() answer -1 when they run out of memory. Entering
       the body with g == -1 would make g.Top() return 0, and the Change(0) /
       OffSet(0) below would throw "invalid VarID" instead of letting the
       failure propagate, so return -1 as the rest of this file does. */
    if(g == -1) return -1;
    int t = g.Top();
    ZDD hh = ZDD(1).Change(t);
    g = g.OffSet(t);

    ZDD g2 = g;
    while(g2 != 0)
    {
      if(g2 == -1) return -1;
      int t2 = g2.Top();
      g2 = g2.OffSet(t2);
      int y = SymChk(t, t2);
      if(y == -1) return -1;
      if(y)
      {
	hh = hh.Change(t2);
	g = g.OffSet(t2);
      }
    }
    if(hh == -1) return -1;
    if(hh.OnSet0(t) != 1) h += hh;
  }
  return h;
}

ZDD ZDD::SymGrpNaive() const
{
  if(*this == -1) return -1;
  ZDD h = 0;
  ZDD g = Support();
  while(g != 0)
  {
    if(g == -1) return -1;
    int t = g.Top();
    ZDD hh = ZDD(1).Change(t);
    g = g.OffSet(t);
    ZDD f0 = OffSet(t);
    ZDD f1 = OnSet0(t);
    if(f0 == -1 || f1 == -1) return -1;

    ZDD g2 = g;
    while(g2 != 0)
    {
      if(g2 == -1) return -1;
      int t2 = g2.Top();
      g2 = g2.OffSet(t2);
      /* Compare the two cofactors only after checking them: two failed
         (-1) results would compare equal and be taken for a symmetric pair. */
      ZDD c0 = f0.OnSet0(t2);
      ZDD c1 = f1.OffSet(t2);
      if(c0 == -1 || c1 == -1) return -1;
      if(c0 == c1)
      {
	hh = hh.Change(t2);
	g = g.OffSet(t2);
      }
    }
    if(hh == -1) return -1;
    h += hh;
  }
  return h;
}

static ZDD ZDD_SymSet(const ZDD&, const ZDD&);
static ZDD ZDD_SymSet(const ZDD& f0, const ZDD& f1)
{
  if(f0 == -1) return -1;
  if(f1 == -1) return -1;
  if(f1 == 0) return 0;
  if(f1 == 1 && (f0 == 0 || f0 == 1)) return 0;

  bddword fx = f0.GetID();
  bddword gx = f1.GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_SYMSET, fx, gx);

  int t0 = f0.Top();
  int t1 = f1.Top();
  int t = (BDD_LevOfVar(t0) > BDD_LevOfVar(t1))? t0: t1;

  ZDD f00 = f0.OffSet(t);
  ZDD f01 = f0.OnSet0(t);
  ZDD f10 = f1.OffSet(t);
  ZDD f11 = f1.OnSet0(t);
  /* as SymChk(): two error values compare equal, so a failed cofactor has
     to be caught before the comparisons below, or 0 would be cached */
  if(f00 == -1 || f01 == -1 || f10 == -1 || f11 == -1) return -1;
  
  ZDD h;
  if(f11 == 0) h = ZDD_SymSet(f00, f10) - f01.Support();
  else if(f10 == 0) h = ZDD_SymSet(f01, f11) - f00.Support();
  else
  {
    h = ZDD_SymSet(f01, f11);
    if(h != 0 && h != -1) h &= ZDD_SymSet(f00, f10);
  }
  if(h == -1) return -1;
  if(f10 == f01) h += ZDD(1).Change(t);
  if(h == -1) return -1;

  ZDD_CACHE_ENT_RETURN(BC_ZDD_SYMSET, fx, gx, h);
}

ZDD ZDD::SymSet(int v) const
{
  if(*this == -1) return -1;
  /* Only the lower bound used to be checked here; a v above the number of
     variables in use was reported later by OffSet()/OnSet0() under the name
     of the C-layer function, so the two ends of the same range answered with
     different messages. */
  if(v <= 0 || v > BDD_VarUsed())
    BDDerr("ZDD::SymSet(): invalid v.", v, ExceptionType::OutOfRange);
  ZDD f0 = OffSet(v);
  ZDD f1 = OnSet0(v);
  return ZDD_SymSet(f0, f1);
}

int ZDD::ImplyChk(int v1, int v2) const
{
  if(*this == -1) return -1;
  if(v1 <= 0 || v1 > BDD_VarUsed()) BDDerr("ZDD::ImplyChk(): invalid v1.", v1, ExceptionType::OutOfRange);
  if(v2 <= 0 || v2 > BDD_VarUsed()) BDDerr("ZDD::ImplyChk(): invalid v2.", v2, ExceptionType::OutOfRange);
  if(v1 == v2) return 1;
  if(*this == 0 || *this == 1) return 1;

  ZDD f10 = OnSet0(v1).OffSet(v2);
  if(f10 == -1) return -1;
  return (f10 == 0);
}

ZDD ZDD::ImplySet(int v) const
{
  if(*this == -1) return -1;
  /* Only the lower bound used to be checked here; a v above the number of
     variables in use was reported later by OffSet()/OnSet0() under the name
     of the C-layer function, so the two ends of the same range answered with
     different messages. */
  if(v <= 0 || v > BDD_VarUsed())
    BDDerr("ZDD::ImplySet(): invalid v.", v, ExceptionType::OutOfRange);
  ZDD f1 = OnSet0(v);
  if(f1 == 0) return Support();
  return f1.Always();
}

int ZDD::CoImplyChk(int v1, int v2) const
{
  if(*this == -1) return -1;
  if(v1 <= 0 || v1 > BDD_VarUsed()) BDDerr("ZDD::CoImplyChk(): invalid v1.", v1, ExceptionType::OutOfRange);
  if(v2 <= 0 || v2 > BDD_VarUsed()) BDDerr("ZDD::CoImplyChk(): invalid v2.", v2, ExceptionType::OutOfRange);
  if(v1 == v2) return 1;
  if(*this == 0 || *this == 1) return 1;

  ZDD f10 = OnSet0(v1).OffSet(v2);
  if(f10 == 0) return 1;

  ZDD f01 = OffSet(v1).OnSet0(v2);
  ZDD chk = f10 - f01;
  if(chk == -1) return -1;
  return (chk == 0) ;
}

static ZDD ZDD_CoImplySet(const ZDD&, const ZDD&);
static ZDD ZDD_CoImplySet(const ZDD& f0, const ZDD& f1)
{
  if(f0 == -1) return -1;
  if(f1 == -1) return -1;
  if(f1 == 0) return 0;
  if(f1 == 1 && (f0 == 0 || f0 == 1)) return 0;

  bddword fx = f0.GetID();
  bddword gx = f1.GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_COIMPSET, fx, gx);

  int t0 = f0.Top();
  int t1 = f1.Top();
  int t = (BDD_LevOfVar(t0) > BDD_LevOfVar(t1))? t0: t1;

  ZDD f00 = f0.OffSet(t);
  ZDD f01 = f0.OnSet0(t);
  ZDD f10 = f1.OffSet(t);
  ZDD f11 = f1.OnSet0(t);
  /* as ZDD_SymSet(): a failed cofactor is caught before the comparisons */
  if(f00 == -1 || f01 == -1 || f10 == -1 || f11 == -1) return -1;
  
  ZDD h;
  if(f11 == 0) h = ZDD_CoImplySet(f00, f10);
  else if(f10 == 0) h = ZDD_CoImplySet(f01, f11);
  else
  {
    h = ZDD_CoImplySet(f01, f11);
    if(h != 0 && h != -1) h &= ZDD_CoImplySet(f00, f10);
  }
  if(h == -1) return -1;
  if(f10 - f01 == 0) h += ZDD(1).Change(t);
  if(h == -1) return -1;

  ZDD_CACHE_ENT_RETURN(BC_ZDD_COIMPSET, fx, gx, h);
}

ZDD ZDD::CoImplySet(int v) const
{
  if(*this == -1) return -1;
  /* Only the lower bound used to be checked here; a v above the number of
     variables in use was reported later by OffSet()/OnSet0() under the name
     of the C-layer function, so the two ends of the same range answered with
     different messages. */
  if(v <= 0 || v > BDD_VarUsed())
    BDDerr("ZDD::CoImplySet(): invalid v.", v, ExceptionType::OutOfRange);
  ZDD f0 = OffSet(v);
  ZDD f1 = OnSet0(v);
  if(f1 == 0) return Support();
  return ZDD_CoImplySet(f0, f1);
}

int ZDD::IsPoly() const
{
  /* the error value used to be answered as 0 ("a single term"), unlike the
     other predicates of this file, which all propagate -1 */
  if(*this == -1) return -1;
  int top = Top();
  if(top == 0) return 0;
  ZDD f1 = OnSet0(top);
  ZDD f0 = OffSet(top);
  /* Check both cofactors before the test below: a failed OffSet() answers -1,
     which is != 0 and used to be reported as "more than one combination" -
     a normal answer produced by a failure. */
  if(f0 == -1 || f1 == -1) return -1;
  if(f0 != 0) return 1;
  BDD_RECUR_INC;
  int r = f1.IsPoly();
  BDD_RECUR_DEC;
  return r;
}

ZDD ZDD::Divisor() const
{
  if(*this == -1) return -1;
  if(*this == 0) return 0;
  /* IsPoly() is a three-valued predicate: -1 is a failure, and taking it for
     a bool makes it "true", so the failure used to continue as if the family
     really had several combinations. */
  int poly = IsPoly();
  if(poly == -1) return -1;
  if(!poly) return 1;
  ZDD f = *this;
  ZDD g = Support();
  int t;
  while(g != 0)
  {
    if(g == -1) return -1;
    t = g.Top();
    g = g.OffSet(t);
    ZDD f1 = f.OnSet0(t);
    if(f1 == -1) return -1;
    poly = f1.IsPoly();
    if(poly == -1) return -1;
    if(poly) f = f1;
  }
  return f;
}


//--------- External functions for ZDD ------------

ZDD operator*(const ZDD& fc, const ZDD& gc)
{
  if(fc == -1) return -1;
  if(gc == -1) return -1;
  if(fc == 0) return 0;
  if(gc == 0) return 0;
  if(fc == 1) return gc;
  if(gc == 1) return fc;

  ZDD f = fc; ZDD g = gc;
  int ftop = f.Top(); int gtop = g.Top();
  if(BDD_LevOfVar(ftop) < BDD_LevOfVar(gtop))
  {
    f = gc; g = fc;
    ftop = f.Top(); gtop = g.Top();
  }

  bddword fx = f.GetID();
  bddword gx = g.GetID();
  if(ftop == gtop && fx < gx)
  {
    f = gc; g = fc;
    fx = f.GetID(); gx = g.GetID();
  }

  ZDD_CACHE_CHK_RETURN(BC_ZDD_MULT, fx, gx);

  ZDD f1 = f.OnSet0(ftop);
  ZDD f0 = f.OffSet(ftop);
  ZDD h;
  if(ftop != gtop)
  {
    h = f1 * g;
    h = h.Change(ftop) + (f0 * g);
  }
  else
  {
    ZDD g1 = g.OnSet0(ftop);
    ZDD g0 = g.OffSet(ftop);
    h = (f1 * g1)+(f1 * g0)+(f0 * g1);
    h = h.Change(ftop) + (f0 * g0);
  }

  ZDD_CACHE_ENT_RETURN(BC_ZDD_MULT, fx, gx, h);
}

ZDD operator/(const ZDD& f, const ZDD& p)
{
  if(f == -1) return -1;
  if(p == -1) return -1;
  if(p == 1) return f;
  /* the divisor check must come before the f == p shortcut: 0/0 used to
     take the shortcut and answer 1 instead of the divide-by-zero error */
  if(p == 0) BDDerr("operator /(): Divided by zero.", ExceptionType::InvalidBDDValue);
  if(f == p) return 1;
  int top = p.Top();
  if(BDD_LevOfVar(f.Top()) < BDD_LevOfVar(top)) return 0;

  bddword fx = f.GetID();
  bddword px = p.GetID();
  ZDD_CACHE_CHK_RETURN(BC_ZDD_DIV, fx, px);
  
  ZDD q = f.OnSet0(top) / p.OnSet0(top);
  if(q != 0)
  {
    ZDD p0 = p.OffSet(top);
    if(p0 != 0) q &= f.OffSet(top) / p0;
  }

  ZDD_CACHE_ENT_RETURN(BC_ZDD_DIV, fx, px, q);
}

ZDD ZDD_Meet(const ZDD& fc, const ZDD& gc)
{
  if(fc == -1) return -1;
  if(gc == -1) return -1;
  if(fc == 0) return 0;
  if(gc == 0) return 0;
  if(fc == 1) return 1;
  if(gc == 1) return 1;

  ZDD f = fc; ZDD g = gc;
  int ftop = f.Top();
  int gtop = g.Top();
  if(BDD_LevOfVar(ftop) < BDD_LevOfVar(gtop))
  {
    f = gc; g = fc;
    ftop = f.Top(); gtop = g.Top();
  }

  bddword fx = f.GetID();
  bddword gx = g.GetID();
  if(ftop == gtop && fx < gx)
  {
    f = gc; g = fc;
    fx = f.GetID(); gx = g.GetID();
  }

  ZDD_CACHE_CHK_RETURN(BC_ZDD_MEET, fx, gx);

  ZDD f1 = f.OnSet0(ftop);
  ZDD f0 = f.OffSet(ftop);
  ZDD h;
  if(ftop != gtop)
  {
    h = ZDD_Meet(f0, g) + ZDD_Meet(f1, g);
  }
  else
  {
    ZDD g1 = g.OnSet0(ftop);
    ZDD g0 = g.OffSet(ftop);
    h = ZDD_Meet(f1, g1);
    h = h.Change(ftop) + ZDD_Meet(f0, g0)
      + ZDD_Meet(f1, g0) + ZDD_Meet(f0, g1);
  }

  ZDD_CACHE_ENT_RETURN(BC_ZDD_MEET, fx, gx, h);
}

ZDD ZDD_Random(int lev, int density)
{
  if(lev < 0) BDDerr("ZDD_Random(): lev < 0.", lev, ExceptionType::OutOfRange);
  /* in a BDDV environment the levels above BDD_TopLev() belong to the
     partitioning system variables; a lev up there used to slip past
     bddvaroflev's check and return a random family over system variables */
  if(lev > BDD_TopLev())
    BDDerr("ZDD_Random(): lev > BDD_TopLev().", lev, ExceptionType::OutOfRange);
  if(density < 0 || density > 100)
    BDDerr("ZDD_Random(): Invalid density.", density, ExceptionType::OutOfRange);
  if(lev == 0) return ((std::rand()%100) < density)? 1: 0;
  /* The first of the two calls below descends all the way to level 0 before
     anything comes back, so the machine stack is lev frames deep whatever
     the density is.  Without the limitter a lev of a few thousand crashed
     the process instead of reporting the limit like every other recursion
     in the library. */
  BDD_RECUR_INC;
  ZDD h = ZDD_Random(lev-1, density) +
          ZDD_Random(lev-1, density).Change(BDD_VarOfLev(lev));
  BDD_RECUR_DEC;
  return h;
}

ZDD ZDD_Import(FILE *strm)
{
  bddword zdd;
  // bddimportz may throw an exception on error
  if (bddimportz(strm, &zdd, 1)) {
    BDDerr("ZDD_Import(): Import failed.", ExceptionType::FileFormat);
  }
  /* a file declaring "_o 0" makes the import succeed without writing an
     output; the bddnull it leaves would escape as an error ZDD returned
     without an exception, unlike every other failure of this function */
  if (zdd == bddnull) {
    BDDerr("ZDD_Import(): No output in file.", ExceptionType::FileFormat);
  }
  return ZDD_ID(zdd);
}


// class ZDDV ---------------------------------------------

ZDDV::ZDDV(const ZDD& f, int location)
{
  if(location < 0) BDDerr("ZDDV::ZDDV(): location < 0.", location, ExceptionType::OutOfRange);
  if(location >= BDDV_MaxLen)
    BDDerr("ZDDV::ZDDV(): Too large location.", location, ExceptionType::OutOfRange);
  /* a location above 0 is encoded on the partitioning system variables that
     only BDDV_Init() creates; without them the Change(var) loop below would
     consume the user's variables 1, 2, ... as partition bits and silently
     fold the component into them */
  if(location > 0 && !BDDV_Active)
    BDDerr("ZDDV::ZDDV(): BDDV_Init() has not been run.", location, ExceptionType::InternalError);
  if(BDD_LevOfVar(f.Top()) > BDD_TopLev())
    BDDerr("ZDDV::ZDDV(): Invalid top var.", f.Top(), ExceptionType::InvalidBDDValue);
  _zdd = f;
  int var = 1;
  for(int i=location; i>0; i>>=1)
  {
    if((i & 1)!= 0) _zdd = _zdd.Change(var);
    var++;
  }
}

ZDDV ZDDV::operator<<(int shift) const
{
  ZDDV fv1 = *this;
  ZDDV fv2;
  while(fv1 != ZDDV())
  {
    if(fv1 == ZDDV(-1)) return fv1;
    int last = fv1.Last();
    fv2 += ZDDV(fv1.GetZDD(last) << shift, last);
    fv1 -= fv1.Mask(last);
  }
  return fv2;
}

ZDDV ZDDV::operator>>(int shift) const
{
  ZDDV fv1 = *this;
  ZDDV fv2;
  while(fv1 != ZDDV())
  {
    if(fv1 == ZDDV(-1)) return fv1;
    int last = fv1.Last();
    fv2 += ZDDV(fv1.GetZDD(last) >> shift, last);
    fv1 -= fv1.Mask(last);
  }
  return fv2;
}

ZDDV ZDDV::OffSet(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::OffSet(): Invalid VarID.", v, ExceptionType::OutOfRange);
  ZDDV tmp;
  tmp._zdd = _zdd.OffSet(v);
  return tmp;
}

ZDDV ZDDV::OnSet(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::OnSet(): Invalid VarID.", v, ExceptionType::OutOfRange);
  ZDDV tmp;
  tmp._zdd = _zdd.OnSet(v);
  return tmp;
}

ZDDV ZDDV::OnSet0(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::OnSet0(): Invalid VarID.", v, ExceptionType::OutOfRange);
  ZDDV tmp;
  tmp._zdd = _zdd.OnSet0(v);
  return tmp;
}

ZDDV ZDDV::Change(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::Change(): Invalid VarID.", v, ExceptionType::InvalidBDDValue);
  ZDDV tmp;
  tmp._zdd = _zdd.Change(v);
  return tmp;
}

ZDDV ZDDV::Swap(int v1, int v2) const
{
  if(BDD_LevOfVar(v1) > BDD_TopLev())
    BDDerr("ZDDV::Swap(): Invalid VarID.", v1, ExceptionType::InvalidBDDValue);
  /* this used to re-test v1 (a copy-paste slip), so a system variable passed
     as v2 was never rejected and the swap silently destroyed the vector's
     partition structure */
  if(BDD_LevOfVar(v2) > BDD_TopLev())
    BDDerr("ZDDV::Swap(): Invalid VarID.", v2, ExceptionType::InvalidBDDValue);
  ZDDV tmp;
  tmp._zdd = _zdd.Swap(v1, v2);
  return tmp;
}

int ZDDV::Top() const
{
  ZDDV fv1 = *this;
  if(fv1 == ZDDV(-1)) return 0;
  int top = 0;
  while(fv1 != ZDDV())
  {
    /* an OOM turns fv1 into the error vector, which never becomes the empty
       vector again: without this check (which operator<< and operator>>
       carry in the same loop) the loop never terminated */
    if(fv1 == ZDDV(-1)) return 0;
    int last = fv1.Last();
    int t = fv1.GetZDD(last).Top();
    if(BDD_LevOfVar(t) > BDD_LevOfVar(top)) top = t;
    fv1 -= fv1.Mask(last);
  }
  return top;
}

int ZDDV::Last() const
{
  int last = 0;
  ZDD f = _zdd;
  while(BDD_LevOfVar(f.Top()) > BDD_TopLev())
  {
    int t = f.Top();
    last += 1 << (t - 1);
    f = f.OnSet0(t);
    /* an OOM used to end the loop through Top() == 0 and hand back the
       partial sum as if it were the true index */
    if(f == -1)
      BDDerr("ZDDV::Last(): Operation failed.", ExceptionType::OutOfMemory);
  }
  return last;
}

ZDDV ZDDV::Mask(int start, int len) const
{
  if(start < 0 || start >= BDDV_MaxLen)
    BDDerr("ZDDV::Mask(): Illegal start index.", start, ExceptionType::OutOfRange);
  /* written as a subtraction so that a len near INT_MAX cannot overflow
     start+len (undefined behaviour that used to skip this check and return
     an empty vector instead of the error) */
  if(len <= 0 || len > BDDV_MaxLen - start)
    BDDerr("ZDDV::Mask(): Illegal len.", len, ExceptionType::OutOfRange);
  ZDDV tmp;
  for(int i=start; i<start+len; i++)
  	tmp += ZDDV(this -> GetZDD(i), i);
  return tmp;
}

ZDD ZDDV::GetZDD(int index) const
{
  if(index < 0 || index >= BDDV_MaxLen)
    BDDerr("ZDDV::GetZDD(): Illegal index.", index, ExceptionType::OutOfRange);
  int level = 0;
  for(int i=1; i<=index; i<<=1) level++;

  ZDD f = _zdd;
  while(BDD_LevOfVar(f.Top()) > BDD_TopLev() + level)
    f = f.OffSet(f.Top());
  while(level > 0)
  {
    if(f == 0) return f;
    if((index & (1<<(level-1))) != 0) f = f.OnSet0(level);
    else f = f.OffSet(level);
    level--;
  }
  return f;
}

bddword ZDDV::Size() const
{
  /* bddvsize() stops at the first bddnull in the array, so a component that
     failed with -1 used to truncate the count silently; and a plain new
     would throw std::bad_alloc past every BDDException handler. */
  if(_zdd == -1)
    BDDerr("ZDDV::Size(): Error vector.", ExceptionType::InvalidBDDValue);
  int len = this -> Last() + 1;
  std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[len]);
  if(!bddv)
    BDDerr("ZDDV::Size(): Memory allocation failed.", ExceptionType::OutOfMemory);
  for(int i=0; i<len; i++)
  {
    ZDD f = GetZDD(i);
    if(f == -1)
      BDDerr("ZDDV::Size(): Operation failed.", ExceptionType::OutOfMemory);
    bddv[i] = f.GetID();
  }
  return bddvsize(bddv.get(), len);
}

void ZDDV::Print() const
{
  /* the error vector used to be printed as its raw meta ID with all counts
     0, indistinguishable from a real vector without knowing the number */
  if(_zdd == -1)
  {
    cout << "[ null (error ZDDV) ]\n";
    cout.flush();
    return;
  }
  int len = this -> Last() + 1;
  for(int i=0; i<len; i++)
  {
    cout << "f" << i << ": ";
    GetZDD(i).Print();
  }
  cout << "Size= " << Size() << "\n\n";
  cout.flush();
}

void ZDDV::Export(FILE *strm) const
{
  /* as Size(): a -1 component used to make bddexport() write a file with
     silently missing components */
  if(_zdd == -1)
    BDDerr("ZDDV::Export(): Error vector.", ExceptionType::InvalidBDDValue);
  int len = this -> Last() + 1;
  std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[len]);
  if(!bddv)
    BDDerr("ZDDV::Export(): Memory allocation failed.", ExceptionType::OutOfMemory);
  for(int i=0; i<len; i++)
  {
    ZDD f = GetZDD(i);
    if(f == -1)
      BDDerr("ZDDV::Export(): Operation failed.", ExceptionType::OutOfMemory);
    bddv[i] = f.GetID();
  }
  bddexport(strm, bddv.get(), len);
}

/* The cube being printed and the number of outputs used to live in file scope
   statics, which made PrintPla() non-reentrant and unusable from more than one
   thread.  They are state of a single printing run, so they travel as
   arguments instead. */
static int ZDDV_PLA(const ZDDV& fv, int tlev, int len, char* cube)
{
  if(fv == ZDDV(-1)) return 1;
  if(fv == ZDDV()) return 0;
  if(tlev == 0)
  {
    cout << cube << " ";
    for(int i=0; i<len; i++)
    {
      /* a component that failed with -1 used to fall into the else branch
         and be printed as "1", turning the error into PLA content */
      ZDD z = fv.GetZDD(i);
      if(z == -1) return 1;
      if(z == 0) cout << "~";
      else cout << "1";
    }
    cout << "\n";
    cout.flush();
    return 0;
  }
  BDD_RECUR_INC;
  cube[tlev-1] = '1';
  if(ZDDV_PLA(fv.OnSet0(BDD_VarOfLev(tlev)), tlev-1, len, cube) == 1)
  {
    BDD_RECUR_DEC;
    return 1;
  }
  cube[tlev-1] = '0';
  int err = ZDDV_PLA(fv.OffSet(BDD_VarOfLev(tlev)), tlev-1, len, cube);
  BDD_RECUR_DEC;
  return err;
}

int ZDDV::PrintPla() const
{
  if(*this == ZDDV(-1)) return 1;
  int tlev = BDD_LevOfVar(Top());
  int len = Last() + 1;
  cout << ".i " << tlev << "\n";
  cout << ".o " << len << "\n";
  if(tlev == 0)
  {
    /* the same output symbols as ZDDV_PLA() prints for the rows of a
       non-constant vector: "~" for an output that is 0, "1" otherwise */
    for(int i=0; i<len; i++)
    if(GetZDD(i) == 0) cout << "~";
    else cout << "1";
    cout << "\n";
  }
  else
  {
    /* unique_ptr so that the buffer is released even when the recursion below
       throws, which BDD_RECUR_INC does on a stack overflow.  The nothrow form
       keeps a failed allocation inside the library's own error contract, as
       ZDDV::Size()/Export()/XPrint() do; the throwing new reported it as
       std::bad_alloc, the only place in this class that did. */
    std::unique_ptr<char[]> cube(new(std::nothrow) char[tlev + 1]);
    if(!cube)
      BDDerr("ZDDV::PrintPla(): Memory allocation failed.", ExceptionType::OutOfMemory);
    cube[tlev] = 0;
    int err = ZDDV_PLA(*this, tlev, len, cube.get());
    if(err == 1) return 1;
  }
  cout << ".e\n";
  cout.flush();
  return 0;
}

#define IMPORTHASH(x) ((((x)>>1)^((x)<<8)^((x)<<16)) & (hashsize-1))

/* Bounds accepted for the counts in the header of an imported file.  Anything
   above them describes a corrupt file rather than a huge one: no more levels
   than the manager can hold variables, no vector longer than BDDV_MaxLen (the
   bound ZDDV::ZDDV() enforces on a location), and no more nodes than the node
   table can address.  Rejecting them early is also what keeps the n_nd<<1
   below from wrapping around. */
static const unsigned long long ImportMaxLev =
  ((unsigned long long)bddvarmax < (unsigned long long)INT_MAX)?
    (unsigned long long)bddvarmax: (unsigned long long)INT_MAX;
static const unsigned long long ImportMaxLen = (unsigned long long)BDDV_MaxLen;
static const unsigned long long ImportMaxNode = (unsigned long long)BDD_MaxNode;
/* Node IDs stay below B_VAL_MASK, the empty-slot mark of the hash table
   below, see BDDV_Import(). */
static const unsigned long long ImportMaxId = (unsigned long long)B_VAL_MASK - 1ULL;

ZDDV ZDDV_Import(FILE *strm)
{
  int inv, e;
  bddword hashsize;
  ZDD f, f0, f1;
  std::string s;
  unsigned long long uval;
  std::unique_ptr<bddword[]> hash1;
  std::unique_ptr<ZDD[]> hash2;

  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading _i tag", ExceptionType::FileFormat);
  if(s != "_i") BDDerr("ZDDV_Import(): File format error, expected _i tag", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading variable count", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxLev, uval))
    BDDerr("ZDDV_Import(): Invalid number of levels", ExceptionType::FileFormat);
  int n = (int)uval;
  while(n > BDD_TopLev()) BDD_NewVar();

  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading _o tag", ExceptionType::FileFormat);
  if(s != "_o") BDDerr("ZDDV_Import(): File format error, expected _o tag", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading output count", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxLen, uval))
    BDDerr("ZDDV_Import(): Invalid vector length", ExceptionType::FileFormat);
  int m = (int)uval;

  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading _n tag", ExceptionType::FileFormat);
  if(s != "_n") BDDerr("ZDDV_Import(): File format error, expected _n tag", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) BDDerr("ZDDV_Import(): Unexpected end of file reading node count", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxNode, uval))
    BDDerr("ZDDV_Import(): Invalid number of nodes", ExceptionType::FileFormat);
  bddword n_nd = (bddword)uval;

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  /* A plain new would throw std::bad_alloc, which is not a BDDException and
     would escape every handler this library asks its users to write; nothrow
     keeps the failure on the library's own error path.  Holding the tables in
     unique_ptr also releases them when one of the format errors below throws,
     which the explicit delete[] calls could not do. */
  hash1.reset(new(std::nothrow) bddword[hashsize]);
  if(!hash1) BDDerr("ZDDV_Import(): Memory allocation failed for hash1", ExceptionType::OutOfMemory);
  hash2.reset(new(std::nothrow) ZDD[hashsize]);
  if(!hash2) BDDerr("ZDDV_Import(): Memory allocation failed for hash2", ExceptionType::OutOfMemory);
  for(bddword ix=0; ix<hashsize; ix++)
  {
    hash1[ix] = B_VAL_MASK;
    hash2[ix] = 0;
  }

  e = 0;
  for(bddword ix=0; ix<n_nd; ix++)
  {
    /* The node IDs are read with the same validation as the header counts:
       strtoll() turned a corrupt token into 0, and node ID 0 is a real node
       (the table starts at index 0), so a damaged file could resolve the
       junk into a silent reference to that node instead of being refused.
       An odd definition ID is refused as in BDDV_Import(). */
    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(ReadDecimal(s, ImportMaxId, uval) || (uval & 1)) { e = 1; break; }
    bddword nd = (bddword)uval;
    
    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    /* A level the file made up would make bddvaroflev() throw an out-of-range
       error of its own; report it as the file format error that it is. */
    if(ReadDecimal(s, (unsigned long long)BDD_TopLev(), uval) || uval == 0)
      { e = 1; break; }
    int var = bddvaroflev((bddvar)uval);

    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(s == "F") f0 = 0;
    else if(s == "T") f0 = 1;
    else
    {
      /* a 0-edge is never inverted */
      if(ReadDecimal(s, ImportMaxId, uval) || (uval & 1)) { e = 1; break; }
      bddword nd0 = (bddword)uval;

      bddword ixx = IMPORTHASH(nd0);
      while(hash1[ixx] != nd0)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f0 = hash2[ixx];
    }

    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(s == "F") f1 = 0;
    else if(s == "T") f1 = 1;
    else
    {
      if(ReadDecimal(s, ImportMaxId, uval)) { e = 1; break; }
      bddword nd1 = (bddword)uval;
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd1);
      while(hash1[ixx] != nd1)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f1 = (inv)? (hash2[ixx] + 1): hash2[ixx];
    }

    f = f1.Change(var) + f0;
    /* a -1 here is a memory overflow, not a defect of the library or of the
       file; reporting it as InternalError misled the caller's recovery */
    if(f == -1) BDDerr("ZDDV_Import(): Memory overflow", ExceptionType::OutOfMemory);

    bddword ixx = IMPORTHASH(nd);
    while(hash1[ixx] != B_VAL_MASK)
    {
      if(hash1[ixx] == nd)
        BDDerr("ZDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  if(e)
    BDDerr("ZDDV_Import(): File format error while reading nodes", ExceptionType::FileFormat);

  ZDDV v = ZDDV();
  for(int i=0; i<m; i++)
  {
    if(ReadToken(strm, s) == EOF)
      BDDerr("ZDDV_Import(): Unexpected end of file reading output values", ExceptionType::FileFormat);
    if(s == "F") v += ZDDV(0, i);
    else if(s == "T") v += ZDDV(1, i);
    else
    {
      if(ReadDecimal(s, ImportMaxId, uval))
        BDDerr("ZDDV_Import(): Invalid node ID", ExceptionType::FileFormat);
      bddword nd = (bddword)uval;
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd);
      while(hash1[ixx] != nd)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      v += ZDDV((inv? (hash2[ixx] + 1): hash2[ixx]), i);
    }
  }

  /* the vector concatenations above return the error vector on an OOM
     without an exception; every other failure of this function throws, so
     this one must not slip out as a normal return value */
  if(v == ZDDV(-1))
    BDDerr("ZDDV_Import(): Memory overflow", ExceptionType::OutOfMemory);
  return v;
}

#define ZLevNum(n) \
  (n-((n&2)?(n&1)? (n<512)?(n<64)?(n<16)?4:8:(n<128)?32:(n<256)?64:128:(n<4096)?(n<1024)?256:(n<2048)?512:1024:(n<8192)?2048:(n<32768)?4096:8192 \
  : (n<512)?(n<64)?4:(n<256)?16:32:(n<4096)?(n<1024)?64:128:(n<32768)?512:1024 \
  :(n&1)? (n<512)?(n<16)?4:8:(n<2048)?(n<1024)?16:32:(n<32768)?64:128 \
  : (n<1024)?4:(n<32768)?8:16 \
  ))

ZDD ZDD::ZLev(int lev, int last) const
{
  if(lev <= 0) return *this & 1;
  ZDD f = *this;
  ZDD u = *this & 1;
  int ftop = Top();
  int flev = BDD_LevOfVar(ftop);
  while(flev > lev)
  {
    if(flev - lev >= 5)
    {
      int n = ZLevNum(flev);
      if(flev >= 66)
      {
        if(n < lev || ((flev & 3) < 3 && ZLevNum((flev-3)) >= lev))
	  n = flev - 1;
      }
      else if(flev >= 18)
      {
        if(n < lev || ((flev & 1) < 1 && ZLevNum((flev-1)) >= lev))
	  n = flev - 1;
      }
      else if(n < lev) n = flev - 1;

      if(n < flev - 1)
      {
        bddword fx = f.GetID();
        ZDD g = BDD_CacheZDD(BC_ZDD_ZSkip, fx, fx);
        if(g != -1)
        {
          int gtop = g.Top();
          int glev = BDD_LevOfVar(gtop);
	  if(glev >= lev)
	  {
            f = g;
	    ftop = gtop;
	    flev = glev;
	    continue;
	  }
        }
      }
    }
    u = f;
    f = f.OffSet(ftop);
    ftop = f.Top();
    flev = BDD_LevOfVar(ftop);
  }
  /* an OOM inside the loop leaves f == -1 (whose level 0 ends the loop);
     with last != 0 the old return handed back u, the pre-failure
     intermediate, as if it were the answer -- and SetZSkip() would then
     store that wrong node in the ZSkip cache */
  if(f == -1) return -1;
  return (last == 0 || lev == flev)? f: u;
}

void ZDD::SetZSkip() const
{
  int t = Top();
  int lev = BDD_LevOfVar(t);
  if(lev <= 4) return;
  bddword fx = GetID();
  ZDD g = BDD_CacheZDD(BC_ZDD_ZSkip, fx, fx);
  if(g != -1) return;
  BDD_RECUR_INC;
  ZDD f0 = OffSet(t);
  f0.SetZSkip();
  g = ZLev(ZLevNum(lev), 1);
  if(g == *this) g = f0;
  bddword gx = g.GetID();
  BDD_CacheEnt(BC_ZDD_ZSkip, fx, fx, gx);
  OnSet0(t).SetZSkip();
  BDD_RECUR_DEC;
}

ZDD ZDD::Intersec(const ZDD& g) const
{
  /* check the error values first: ZDD(-1).Intersec(ZDD(0)) used to take the
     g == 0 shortcut and turn the failure into a normal empty answer */
  if(*this == -1 || g == -1) return -1;
  if(g == 0) return 0;
  if(g == 1) return *this & 1;
  int ftop = Top();
  if(ftop == 0) return *this & g;
  int gtop = g.Top();

  bddword fx = GetID();
  bddword gx = g.GetID();
  if(fx < gx) { fx = g.GetID(); gx = GetID(); }
  ZDD_CACHE_CHK_RETURN(BC_ZDD_INTERSEC, fx, gx);

  int flev = BDD_LevOfVar(ftop);
  int glev = BDD_LevOfVar(gtop);
  ZDD h;
  if(flev > glev) h = ZLev(glev).Intersec(g);
  else if(flev < glev) h = Intersec(g.OffSet(gtop));
  else
  {
    h = OnSet0(ftop).Intersec(g.OnSet0(ftop)).Change(ftop)
      + OffSet(ftop).Intersec(g.OffSet(ftop));
  }

  ZDD_CACHE_ENT_RETURN(BC_ZDD_INTERSEC, fx, gx, h);
}

} // namespace sapporobdd

