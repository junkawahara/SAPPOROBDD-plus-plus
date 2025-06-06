/****************************************
 * SeqBDD Class (SAPPORO-1.87)          *
 * (Main part)                          *
 * (C) Shin-ichi MINATO (May 14, 2021)  *
 ****************************************/

#include "SeqBDD.h"

using std::cout;

namespace sapporobdd {

//------------ Internal constant data for SeqBDD ----------
static const char BC_SeqBDD_MULT = 70;
static const char BC_SeqBDD_DIV = 71;

//----------- Macros for operation cache -----------
#define SeqBDD_CACHE_CHK_RETURN(op, fx, gx) \
  { ZDD z = BDD_CacheZDD(op, fx, gx); \
    if(z != -1) return SeqBDD(z); \
    BDD_RECUR_INC; }

#define SeqBDD_CACHE_ENT_RETURN(op, fx, gx, h) \
  { BDD_RECUR_DEC; \
    if(h != -1) BDD_CacheEnt(op, fx, gx, h.GetZDD().GetID()); \
    return h; }

// class SeqBDD ---------------------------------------------

SeqBDD SeqBDD::OffSet(int v) const
{
  if(_zdd.Top() == v) return SeqBDD(_zdd.OffSet(v));
  else return *this - OnSet0(v).Push(v);
}

SeqBDD SeqBDD::OnSet0(int v) const
{
  ZDD f = _zdd;
  int top = f.Top();
  while(BDD_LevOfVar(v) < BDD_LevOfVar(top))
  {
    f = f.OffSet(top);
    top = f.Top();
  }
  return SeqBDD(f.OnSet0(v));
}

bddword SeqBDD::Size() const { return _zdd.Size(); }
bddword SeqBDD::Card() const { return _zdd.Card(); }
bddword SeqBDD::Lit() const { return _zdd.Lit(); }
bddword SeqBDD::Len() const { return _zdd.Len(); }

void SeqBDD::Export(FILE *strm) const { _zdd.Export(strm); }

void SeqBDD::Print() const { GetZDD().Print(); }

SeqBDD operator*(const SeqBDD& f, const SeqBDD& g)
{
  if(f == -1) return -1;
  if(g == -1) return -1;
  if(f == 0) return 0;
  if(g == 0) return 0;
  if(f == 1) return g;
  if(g == 1) return f;

  int ftop = f.Top();
  bddword fx = f.GetZDD().GetID();
  bddword gx = g.GetZDD().GetID();

  SeqBDD_CACHE_CHK_RETURN(BC_SeqBDD_MULT, fx, gx);

  SeqBDD f1 = f.OnSet0(ftop);
  SeqBDD f0 = f.OffSet(ftop);
  SeqBDD h = f1 * g;
  h = h.Push(ftop) + (f0 * g);

  SeqBDD_CACHE_ENT_RETURN(BC_SeqBDD_MULT, fx, gx, h);
}

SeqBDD operator/(const SeqBDD& f, const SeqBDD& p)
{
  if(f == -1) return -1;
  if(p == -1) return -1;
  if(p == 1) return f;
  if(f == p) return 1;
  if(p == 0) BDDerr("operator /(): Divided by zero.", ExceptionType::InvalidBDDValue);
  int top = p.Top();
  if(BDD_LevOfVar(f.Top()) < BDD_LevOfVar(top)) return 0;

  bddword fx = f.GetZDD().GetID();
  bddword px = p.GetZDD().GetID();
  SeqBDD_CACHE_CHK_RETURN(BC_SeqBDD_DIV, fx, px);

  SeqBDD q = f.OnSet0(top) / p.OnSet0(top);
  if(q != 0)
  {
    SeqBDD p0 = p.OffSet(top);
    if(p0 != 0) q &= f.OffSet(top) / p0;
  }
  SeqBDD_CACHE_ENT_RETURN(BC_SeqBDD_DIV, fx, px, q);
}

static int* Seq;
static bddword Index;
static int Flag;

static void SeqBDD_PrintSeq(SeqBDD);
static void SeqBDD_PrintSeq(SeqBDD f)
{
  if((f & 1)== 1)
  {
    if(Flag > 0) cout << "+ ";
    if(Index == 0) cout << "e ";
    else for(bddword i=0; i<Index; i++) cout << Seq[i] << " ";
    Flag = 1;
    cout.flush();
    f -= 1;
  }
  if(f == 0) return;
  int top = f.Top();
  SeqBDD f1 = f.OnSet0(top);
  Seq[Index] = BDD_LevOfVar(top);
  Index++;
  SeqBDD_PrintSeq(f1);
  Index--;
  SeqBDD f0 = f.OffSet(top);
  SeqBDD_PrintSeq(f0);
}

void SeqBDD::PrintSeq() const
{
  if(*this == -1)
  {
    cout << "(undefined)\n";
    cout.flush();
    return;
  }
  if(*this == 0)
  {
    cout << "(empty)\n";
    cout.flush();
    return;
  }
  bddword len = Len();
  Seq = new int[len];
  Index = 0;
  Flag = 0;
  SeqBDD_PrintSeq(*this);
  delete[] Seq;
  cout << "\n";
  cout.flush();
}

} // namespace sapporobdd

