/****************************************
 * ZDD+ Manipulator (SAPPORO-1.87)     *
 * (Main part)                          *
 * (C) Shin-ichi MINATO (May 14, 2021)  *
 ****************************************/

#include "ZDD.h"

#define BDD_CPP
#include "bddc.h"

using std::cout;

namespace sapporobdd {

static const char BC_ZDD_MULT = 20;
static const char BC_ZDD_DIV = 21;
static const char BC_ZDD_RSTR = 22;
static const char BC_ZDD_PERMIT = 23;
static const char BC_ZDD_PERMITSYM = 24;
static const char BC_ZDD_SYMCHK = 25;
static const char BC_ZDD_ALWAYS = 26;
static const char BC_ZDD_SYMSET = 27;
static const char BC_ZDD_COIMPSET = 28;
static const char BC_ZDD_MEET = 29;

static const char BC_ZDD_ZSkip = 65;
static const char BC_ZDD_INTERSEC = 66;

extern "C"
{
	int rand();
};

// class ZDD ---------------------------------------------

void ZDD::Export(FILE *strm) const
{
  bddword p = _zdd;
  bddexport(strm, &p, 1);
}

void ZDD::Print() const
{
  cout << "[ " << GetID();
  cout << " Var:" << Top() << "(" << BDD_LevOfVar(Top()) << ")";
  cout << " Size:" << Size() << " Card:";
  cout << Card() << " Lit:" << Lit() << " Len:" << Len() << " ]\n";
  cout.flush();
}

void ZDD::PrintPla() const { ZDDV(*this).PrintPla(); }

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
  if(*this == 1) return 1;
  if(n < 1) return *this & 1;

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
  if(v1 <= 0) BDDerr("ZDD::SymChk(): invalid v1.", v1);
  if(v2 <= 0) BDDerr("ZDD::SymChk(): invalid v2.", v2);
  if(*this == 0 || *this == 1) return 1;
  if(v1 == v2) return 1;
  if(v1 < v2) { int tmp = v1; v1 = v2; v2 = tmp; }

  ZDD S = ZDD(1).Change(v1) + ZDD(1).Change(v2);
  bddword fx = GetID();
  bddword gx = S.GetID();
  int Y = BDD_CacheInt(BC_ZDD_SYMCHK, fx, gx);
  if(Y != -1) return Y;
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
      Y = (f0.OnSet0(v2) == f1.OffSet(v2));
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
  ZDD h = 0;
  ZDD g = Support();
  while(g != 0)
  {
    int t = g.Top();
    ZDD hh = ZDD(1).Change(t);
    g = g.OffSet(t);

    ZDD g2 = g;
    while(g2 != 0)
    {
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
    if(hh.OnSet0(t) != 1) h += hh;
  }
  return h;
}

ZDD ZDD::SymGrpNaive() const
{
  ZDD h = 0;
  ZDD g = Support();
  while(g != 0)
  {
    int t = g.Top();
    ZDD hh = ZDD(1).Change(t);
    g = g.OffSet(t);
    ZDD f0 = OffSet(t);
    ZDD f1 = OnSet0(t);

    ZDD g2 = g;
    while(g2 != 0)
    {
      int t2 = g2.Top();
      g2 = g2.OffSet(t2);
      if(f0.OnSet0(t2) == f1.OffSet(t2))
      {
	hh = hh.Change(t2);
	g = g.OffSet(t2);
      }
    }
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
  
  ZDD h;
  if(f11 == 0) h = ZDD_SymSet(f00, f10) - f01.Support();
  else if(f10 == 0) h = ZDD_SymSet(f01, f11) - f00.Support();
  else
  {
    h = ZDD_SymSet(f01, f11);
    if(h != 0) h &= ZDD_SymSet(f00, f10);
  }
  if(f10 == f01) h += ZDD(1).Change(t);

  ZDD_CACHE_ENT_RETURN(BC_ZDD_SYMSET, fx, gx, h);
}

ZDD ZDD::SymSet(int v) const
{
  if(*this == -1) return -1;
  if(v <= 0) BDDerr("ZDD::SymSet(): invalid v.", v);
  ZDD f0 = OffSet(v);
  ZDD f1 = OnSet0(v);
  return ZDD_SymSet(f0, f1);
}

int ZDD::ImplyChk(int v1, int v2) const
{
  if(*this == -1) return -1;
  if(v1 <= 0) BDDerr("ZDD::IndImplyChk(): invalid v1.", v1);
  if(v2 <= 0) BDDerr("ZDD::IndImplyChk(): invalid v2.", v2);
  if(v1 == v2) return 1;
  if(*this == 0 || *this == 1) return 1;

  ZDD f10 = OnSet0(v1).OffSet(v2);
  if(f10 == -1) return -1;
  return (f10 == 0);
}

ZDD ZDD::ImplySet(int v) const
{
  if(*this == -1) return -1;
  if(v <= 0) BDDerr("ZDD::ImplySet(): invalid v.", v);
  ZDD f1 = OnSet0(v);
  if(f1 == 0) return Support();
  return f1.Always();
}

int ZDD::CoImplyChk(int v1, int v2) const
{
  if(*this == -1) return -1;
  if(v1 <= 0) BDDerr("ZDD::IndImplyChk(): invalid v1.", v1);
  if(v2 <= 0) BDDerr("ZDD::IndImplyChk(): invalid v2.", v2);
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
  
  ZDD h;
  if(f11 == 0) h = ZDD_CoImplySet(f00, f10);
  else if(f10 == 0) h = ZDD_CoImplySet(f01, f11);
  else
  {
    h = ZDD_CoImplySet(f01, f11);
    if(h != 0) h &= ZDD_CoImplySet(f00, f10);
  }
  if(f10 - f01 == 0) h += ZDD(1).Change(t);

  ZDD_CACHE_ENT_RETURN(BC_ZDD_COIMPSET, fx, gx, h);
}

ZDD ZDD::CoImplySet(int v) const
{
  if(*this == -1) return -1;
  if(v <= 0) BDDerr("ZDD::CoImplySet(): invalid v.", v);
  ZDD f0 = OffSet(v);
  ZDD f1 = OnSet0(v);
  if(f1 == 0) return Support();
  return ZDD_CoImplySet(f0, f1);
}

int ZDD::IsPoly() const
{
  int top = Top();
  if(top == 0) return 0;
  ZDD f1 = OnSet0(top);
  ZDD f0 = OffSet(top);
  if(f0 != 0) return 1;
  return f1.IsPoly();
}

ZDD ZDD::Divisor() const
{
  if(*this == -1) return -1;
  if(*this == 0) return 0;
  if(! IsPoly()) return 1;
  ZDD f = *this;
  ZDD g = Support();
  int t;
  while(g != 0)
  {
    t = g.Top();
    g = g.OffSet(t);
    ZDD f1 = f.OnSet0(t);
    if(f1.IsPoly()) f = f1;
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
  if(f == p) return 1;
  if(p == 0) BDDerr("operator /(): Divided by zero.");
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
  if(lev < 0) BDDerr("ZDD_Random(): lev < 0.", lev);
  if(lev == 0) return ((rand()%100) < density)? 1: 0;
  return ZDD_Random(lev-1, density) +
         ZDD_Random(lev-1, density).Change(BDD_VarOfLev(lev));
}

ZDD ZDD_Import(FILE *strm)
{
  bddword zdd;
  if(bddimportz(strm, &zdd, 1)) return -1;
  return ZDD_ID(zdd);
}


// class ZDDV ---------------------------------------------

ZDDV::ZDDV(const ZDD& f, int location)
{
  if(location < 0) BDDerr("ZDDV::ZDDV(): location < 0.", location);
  if(location >= BDDV_MaxLen)
    BDDerr("ZDDV::ZDDV(): Too large location.", location);
  if(BDD_LevOfVar(f.Top()) > BDD_TopLev())
    BDDerr("ZDDV::ZDDV(): Invalid top var.", f.Top());
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
    BDDerr("ZDDV::OffSet(): Invalid VarID.", v);
  ZDDV tmp;
  tmp._zdd = _zdd.OffSet(v);
  return tmp;
}

ZDDV ZDDV::OnSet(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::OnSet(): Invalid VarID.", v);
  ZDDV tmp;
  tmp._zdd = _zdd.OnSet(v);
  return tmp;
}

ZDDV ZDDV::OnSet0(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::OnSet0(): Invalid VarID.", v);
  ZDDV tmp;
  tmp._zdd = _zdd.OnSet0(v);
  return tmp;
}

ZDDV ZDDV::Change(int v) const
{
  if(BDD_LevOfVar(v) > BDD_TopLev())
    BDDerr("ZDDV::Change(): Invalid VarID.", v);
  ZDDV tmp;
  tmp._zdd = _zdd.Change(v);
  return tmp;
}

ZDDV ZDDV::Swap(int v1, int v2) const
{
  if(BDD_LevOfVar(v1) > BDD_TopLev())
    BDDerr("ZDDV::Swap(): Invalid VarID.", v1);
  if(BDD_LevOfVar(v1) > BDD_TopLev())
    BDDerr("ZDDV::Swap(): Invalid VarID.", v2);
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
  }
  return last;
}

ZDDV ZDDV::Mask(int start, int len) const
{
  if(start < 0 || start >= BDDV_MaxLen)
    BDDerr("ZDDV::Mask(): Illegal start index.", start);
  if(len <= 0 || start+len > BDDV_MaxLen)
    BDDerr("ZDDV::Mask(): Illegal len.", len);
  ZDDV tmp;
  for(int i=start; i<start+len; i++)
  	tmp += ZDDV(this -> GetZDD(i), i);
  return tmp;
}

ZDD ZDDV::GetZDD(int index) const
{
  if(index < 0 || index >= BDDV_MaxLen)
    BDDerr("ZDDV::GetZDD(): Illegal index.",index);
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
  int len = this -> Last() + 1;
  bddword* bddv = new bddword[len];
  for(int i=0; i<len; i++) bddv[i] = GetZDD(i).GetID(); 
  bddword s = bddvsize(bddv, len);
  delete[] bddv;
  return s;
}

void ZDDV::Print() const
{
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
  int len = this -> Last() + 1;
  bddword* bddv = new bddword[len];
  for(int i=0; i<len; i++) bddv[i] = GetZDD(i).GetID(); 
  bddexport(strm, bddv, len);
  delete[] bddv;
}

static int Len;
static char* Cube;
static int ZDDV_PLA(const ZDDV&, int);
static int ZDDV_PLA(const ZDDV& fv, int tlev)
{
  if(fv == ZDDV(-1)) return 1;
  if(fv == ZDDV()) return 0;
  if(tlev == 0)
  {
    cout << Cube << " ";
    for(int i=0; i<Len; i++)
      if(fv.GetZDD(i) == 0) cout << "~";
      else cout << "1";
    cout << "\n";
    cout.flush();
    return 0;
  }
  Cube[tlev-1] = '1';
  if(ZDDV_PLA(fv.OnSet0(BDD_VarOfLev(tlev)), tlev-1) == 1)
    return 1;
  Cube[tlev-1] = '0';
  return ZDDV_PLA(fv.OffSet(BDD_VarOfLev(tlev)), tlev-1);
}

int ZDDV::PrintPla() const
{
  if(*this == ZDDV(-1)) return 1;
  int tlev = BDD_LevOfVar(Top());
  Len = Last() + 1;
  cout << ".i " << tlev << "\n";
  cout << ".o " << Len << "\n";
  if(tlev == 0)
  {
    for(int i=0; i<Len; i++)
    if(GetZDD(i) == 0) cout << "0";
    else cout << "1";
    cout << "\n";
  }
  else
  {
    Cube = new char[tlev + 1];
    Cube[tlev] = 0;
    int err = ZDDV_PLA(*this, tlev);
    delete[] Cube;
    if(err == 1) return 1;
  }
  cout << ".e\n";
  cout.flush();
  return 0;
}

#define IMPORTHASH(x) ((((x)>>1)^((x)<<8)^((x)<<16)) & (hashsize-1))

#ifdef B_32
#  define B_STRTOI strtol
#else
#  define B_STRTOI strtoll
#endif

ZDDV ZDDV_Import(FILE *strm)
{
  int inv, e;
  bddword hashsize;
  ZDD f, f0, f1;
  char s[256];
  bddword *hash1 = 0;
  ZDD *hash2 = 0;

  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  if(strcmp(s, "_i") != 0) return ZDDV(-1);
  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  int n = strtol(s, NULL, 10);
  while(n > BDD_TopLev()) BDD_NewVar();

  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  if(strcmp(s, "_o") != 0) return ZDDV(-1);
  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  int m = strtol(s, NULL, 10);

  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  if(strcmp(s, "_n") != 0) return ZDDV(-1);
  if(fscanf(strm, "%s", s) == EOF) return ZDDV(-1);
  bddword n_nd = B_STRTOI(s, NULL, 10);

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  hash1 = new bddword[hashsize];
  if(hash1 == 0) return ZDDV(-1);
  hash2 = new ZDD[hashsize];
  if(hash2 == 0) { delete[] hash1; return ZDDV(-1); }
  for(bddword ix=0; ix<hashsize; ix++)
  {
    hash1[ix] = B_VAL_MASK;
    hash2[ix] = 0;
  }

  e = 0;
  for(bddword ix=0; ix<n_nd; ix++)
  {
    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    bddword nd = B_STRTOI(s, NULL, 10);
    
    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    int lev = strtol(s, NULL, 10);
    int var = bddvaroflev(lev);

    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f0 = 0;
    else if(strcmp(s, "T") == 0) f0 = 1;
    else
    {
      bddword nd0 = B_STRTOI(s, NULL, 10);

      bddword ixx = IMPORTHASH(nd0);
      while(hash1[ixx] != nd0)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx);
        ixx++;
        ixx &= (hashsize-1);
      }
      f0 = hash2[ixx];
    }

    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f1 = 0;
    else if(strcmp(s, "T") == 0) f1 = 1;
    else
    {
      bddword nd1 = B_STRTOI(s, NULL, 10);
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd1);
      while(hash1[ixx] != nd1)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx);
        ixx++;
        ixx &= (hashsize-1);
      }
      f1 = (inv)? (hash2[ixx] + 1): hash2[ixx];
    }

    f = f1.Change(var) + f0;
    if(f == -1) { e = 1; break; }

    bddword ixx = IMPORTHASH(nd);
    while(hash1[ixx] != B_VAL_MASK)
    {
      if(hash1[ixx] == nd)
        BDDerr("ZDDV_Import(): internal error", ixx);
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  if(e)
  {
    delete[] hash2;
    delete[] hash1;
    return ZDDV(-1);
  }

  ZDDV v = ZDDV();
  for(int i=0; i<m; i++)
  {
    if(fscanf(strm, "%s", s) == EOF)
    {
      delete[] hash2;
      delete[] hash1;
      return ZDDV(-1);
    }
    bddword nd = B_STRTOI(s, NULL, 10);
    if(strcmp(s, "F") == 0) v += ZDDV(0, i);
    else if(strcmp(s, "T") == 0) v += ZDDV(1, i);
    else
    {
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd);
      while(hash1[ixx] != nd)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("ZDDV_Import(): internal error", ixx);
        ixx++;
        ixx &= (hashsize-1);
      }
      v += ZDDV((inv? (hash2[ixx] + 1): hash2[ixx]), i);
    }
  }

  delete[] hash2;
  delete[] hash1;
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
  ZDD f0 = OffSet(t);
  f0.SetZSkip();
  g = ZLev(ZLevNum(lev), 1);
  if(g == *this) g = f0;
  bddword gx = g.GetID();
  BDD_CacheEnt(BC_ZDD_ZSkip, fx, fx, gx);
  OnSet0(t).SetZSkip();
}

ZDD ZDD::Intersec(const ZDD& g) const
{
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

