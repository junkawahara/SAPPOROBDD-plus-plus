/****************************************
 * BDD Cost Table class - Body v1.97    *
 * (C) Shin-ichi MINATO (Jan. 2, 2023)  *
 ****************************************/

#include "BDDCT.h"
#include "bddplus_internal.h"
using namespace std;

namespace sapporobdd {


/* bddcost is a plain int, so the sums taken along a path can leave its range;
   every one of them used to overflow silently.  The helpers below report the
   overflow instead.  They also keep the smallest int out of the class, as it
   is the one value whose negation does not exist and the cache keys are
   negated costs. */
static const bddcost CostMin = -bddcost_null;
static const bddcost CostMax = bddcost_null - 1;

static bddcost NegCost(const bddcost a)
{
  if(a < CostMin)
    BDDerr("BDDCT: cost out of range", ExceptionType::OutOfRange);
  return -a;
}

static bddcost AddCost(const bddcost a, const bddcost b)
{
  if(a < CostMin || b < CostMin)
    BDDerr("BDDCT: cost out of range", ExceptionType::OutOfRange);
  /* both operands are in [CostMin, bddcost_null] now, so the bounds compared
     against here cannot overflow themselves.  The result is held to CostMax
     so that a computed cost never comes out as the bddcost_null mark, which
     the callers read as "no value". */
  if((b > 0)? (a > CostMax - b): (a < CostMin - b))
    BDDerr("BDDCT: cost overflow", ExceptionType::OutOfRange);
  return a + b;
}

static bddcost SubCost(const bddcost a, const bddcost b)
{
  return AddCost(a, NegCost(b));
}


BDDCT::BDDCT()
{
  _n = 0;
  _cost = 0;
  _label = 0;

  _casize = 0;
  _caent = 0;
  _ca = 0;

  _ca0size = 0;
  _ca0ent = 0;
  _ca0 = 0;

  _call = 0;
}

BDDCT::~BDDCT()
{
  if(_cost) delete[] _cost; 
  if(_label)
  {
    for(int i=0; i<_n; i++) if(_label[i]) delete[] _label[i];
    delete[] _label; _label = 0;
  }
  if(_ca) delete[] _ca; 
  if(_ca0) delete[] _ca0; 
}

bddcost BDDCT::Cost(const int ix) const
{
  return (ix >= _n)? bddcost_null: (ix < 0)? 1: _cost[ix];
}

char* BDDCT::Label(const int ix) const
{
  return (ix >= _n || ix < 0)? 0: _label[ix];
}

/* bddcost_null is the "no value" mark: Cost() returns it for an index past
   the table and the recursions read it as "this branch holds no set", so it
   cannot double as a cost.  CostMin is left out as well, as it is the one
   value the negated cache keys have no room for. */
static int CostChk(const bddcost cost)
{ return (cost > CostMax || cost <= CostMin)? 1: 0; }

int BDDCT::SetCost(const int ix, const bddcost cost)
{
  if(ix < 0 || ix >= _n) return 1;
  if(CostChk(cost)) return 1;
  _cost[ix] = cost;
  if(_caent > 0) if(CacheClear()) return 1;
  if(_ca0ent > 0) if(Cache0Clear()) return 1;
  return 0;
}

int BDDCT::SetLabel(const int ix, const char* label)
{
  if(ix < 0 || ix >= _n) return 1;
  int j;
  for(j=0; j<CT_STRLEN; j++)
  {
    _label[ix][j] = label[j];
    if(!label[j]) break;
  }
  if(j == CT_STRLEN) _label[ix][j] = 0;
  return 0;
}

int BDDCT::Alloc(const int n, const bddcost cost)
{
  if(CostChk(cost)) return 1;
  if(_cost) { delete[] _cost; _cost = 0; }
  if(_label)
  {
    for(int i=0; i<_n; i++) if(_label[i]) delete[] _label[i];
    delete[] _label; _label = 0;
  }

  _n = (n < 0)? 0: n;

  if(_n > 0)
  {
    if(!(_cost = new bddcost[_n])) { Alloc(0); return 1; }
    if(!(_label = new char*[_n])) { Alloc(0); return 1; }
    for(int i=0; i<_n; i++)
    {
      _cost[i] = cost;
      _label[i] = 0;
    }
    for(int i=0; i<_n; i++)
      if((_label[i] = new char[CT_STRLEN + 1])) _label[i][0] = 0;
      else { Alloc(0); return 1; }
  }

  if(CacheClear()) return 1;
  if(Cache0Clear()) return 1;
  return 0;
}

int BDDCT::Import(FILE *fp)
{
  /* every failure leaves the empty table: the early returns used to keep
     either the old table or a half-imported one, depending on where the
     input broke off */
  std::string s;
  do if(ReadToken(fp, s) == EOF) { Alloc(0); return 1; }
  while(s[0] == '#'); // go next word
  int n = strtol(s.c_str(), NULL, 10);
  if(Alloc(n)) return 1;
  /* an empty table has nothing after its size, so its own Export() output
     used to be rejected at the EOF here */
  if(_n == 0) return 0;

  do if(ReadToken(fp, s) == EOF) { Alloc(0); return 1; }
  while(s[0] == '#'); // go next word
  int e = 0;
  for(int ix=0; ix<_n; ix++)
  {
    if((e = SetCost(ix, strtol(s.c_str(), NULL, 10)))) break;
    if(ReadToken(fp, s) == EOF) { if(ix<_n-1) e = 1; break; }
    if(s[0] == '#') 
    {
      if((e = SetLabel(ix, s.c_str()+1))) break;
      do if(ReadToken(fp, s) == EOF) { if(ix<_n-1) e = 1; break; }
      while(s[0] == '#'); // go next word
    }
  }
  if(e) { Alloc(0); return 1; }
  return 0;
}

int BDDCT::AllocRand(const int n, const bddcost min, const bddcost max)
{
  Alloc(n);
  /* the width of the widest valid range does not fit in bddcost, and
     max - min + 1 used to overflow it; min > max keeps its historical
     meaning of "the min everywhere" */
  unsigned long long m = (min <= max)?
    (unsigned long long)((long long)max - (long long)min) + 1ULL: 1ULL;
  for(int ix=0; ix<_n; ix++)
  {
    long long r = (long long)(((double)rand()/((double)RAND_MAX+1)) * (double)m);
    if(SetCost(ix, (bddcost)(r + (long long)min)))
    {
      Alloc(0);
      return 1;
    }
  }
  return 0;
}

void BDDCT::Export() const
{
  cout << "#n " << _n << "\n";
  for(int i=0; i<_n; i++)
  {
    cout << _cost[i];
    if(_label[i] && _label[i][0])
      cout << " #" << _label[i];
    cout << "\n";
  }
}

int BDDCT::CacheClear()
{
  if(_ca) { delete[] _ca; _ca = 0; }
  _casize = 1 << 4;
  _caent = 0;
  if(!(_ca = new CacheEntry[_casize])) return 1;
  return 0;
}

#define Hash(id) ((id)*1234567)

int BDDCT::CacheEnlarge()
{
  bddword newsize = _casize << 2;
  //cout << "enlarge: " << newsize << "\n";
  CacheEntry* newca = 0;
  if(!(newca = new CacheEntry[newsize])) return 1;
  for(bddword i=0; i<_casize; i++)
  {
    if(_ca[i]._zmap)
    {
      bddword k = Hash(_ca[i]._key.GetID()) & (newsize - 1);
      while(1)
      {
        if(!newca[k]._zmap) break;
	k++;
	k &= newsize - 1;
      }
      newca[k]._key = _ca[i]._key;
      newca[k]._zmap = _ca[i]._zmap;
      _ca[i]._zmap = 0;
    }
  }
  delete[] _ca;
  _ca = newca;
  _casize = newsize;
  return 0;
}

ZDD BDDCT::CacheRef(const ZDD& f, const bddcost bound,
                      bddcost& acc_worst, bddcost& rej_best)
{
  if(!_casize) return -1;
  bddword id = f.GetID();
  bddword k = Hash(id) & (_casize - 1);
  while(1)
  {
    if(!_ca[k]._zmap) return -1; 
    if(_ca[k]._key.GetID() == id)
    {
      Zmap* zm = _ca[k]._zmap;
      Zmap::iterator itr = zm->lower_bound(NegCost(bound));
      if(itr == zm->end())
      {
        --itr;
	if(itr->second != 0) return -1;
	acc_worst = bddcost_null;
	--itr;
	rej_best = -(itr->first);
	return 0;
      }
      ZDD h = itr->second;
      if(h == -1) return -1;
      acc_worst = -(itr->first);
      if(acc_worst == -bddcost_null) acc_worst = bddcost_null;
      if(itr == zm->begin()) rej_best = bddcost_null;
      else
      {
        --itr;
        rej_best = -(itr->first);
      }
      return h;
    }
    k++;
    k &= _casize - 1;
  }
}

int BDDCT::CacheEnt(const ZDD& f, const ZDD& h,
                     const bddcost acc_worst, const bddcost rej_best)
{
  /* The entries are keyed by the negated cost and the key bddcost_null is
     reserved for "every set was rejected", so a cost negating onto it cannot
     be stored. */
  if(acc_worst == -bddcost_null || rej_best == -bddcost_null)
    BDDerr("BDDCT::CacheEnt: cost out of range", ExceptionType::OutOfRange);
  if(!_casize) return 1;
  if(_caent >= (_casize >> 1) && CacheEnlarge()) return 1;
  bddword id = f.GetID();
  bddword k = Hash(id) & (_casize - 1);
  while(1)
  {
    if(!_ca[k]._zmap)
    {
      _caent++;
      if(!(_ca[k]._zmap = new Zmap)) return 1;
      _ca[k]._key = f;
      break;
    }
    if(_ca[k]._key.GetID() == id) break;
    k++;
    k &= _casize - 1;
  }
  Zmap* zm = _ca[k]._zmap;
  if(acc_worst != bddcost_null) (*zm)[NegCost(acc_worst)] = h;
  else if(h == 0) (*zm)[bddcost_null] = 0;
  if(rej_best != bddcost_null)
     if(zm->find(NegCost(rej_best)) == zm->end()) (*zm)[NegCost(rej_best)] = -1;
  return 0;
}

int BDDCT::Cache0Clear()
{
  if(_ca0) { delete[] _ca0; _ca0 = 0; }
  _ca0size = 1 << 4;
  _ca0ent = 0;
  if(!(_ca0 = new Cache0Entry[_ca0size])) return 1;
  return 0;
}

#define Hash0(op, id) ((id)*1234567+(op))

int BDDCT::Cache0Enlarge()
{
  bddword newsize = _ca0size << 2;
  //cout << "enlarge: " << newsize << "\n";
  Cache0Entry* newca0 = 0;
  if(!(newca0 = new Cache0Entry[newsize])) return 1;
  for(bddword i=0; i<_ca0size; i++)
  {
    if(_ca0[i]._b != bddcost_null)
    {
      unsigned char op = _ca0[i]._op;
      bddword k = Hash0(op, _ca0[i]._key.GetID()) & (newsize - 1);
      while(1)
      {
        if(newca0[k]._b == bddcost_null) break;
	k++;
	k &= newsize - 1;
      }
      newca0[k]._op = op;
      newca0[k]._key = _ca0[i]._key;
      newca0[k]._b = _ca0[i]._b;
    }
  }
  delete[] _ca0;
  _ca0 = newca0;
  _ca0size = newsize;
  return 0;
}

bddcost BDDCT::Cache0Ref(const unsigned char op, const ZDD& f) const
{
  if(!_ca0size) return bddcost_null;
  bddword id = f.GetID();
  bddword k = Hash0(op, id) & (_ca0size - 1);
  while(1)
  {
    if(_ca0[k]._b == bddcost_null) return bddcost_null;
    if(_ca0[k]._op == op && _ca0[k]._key.GetID() == id) return _ca0[k]._b;
    k++;
    k &= _ca0size - 1;
  }
}

int BDDCT::Cache0Ent(const unsigned char op, const ZDD& f, const bddcost b)
{
  /* An entry holding bddcost_null is the mark for an empty slot, so storing
     one would make Cache0Ref miss this entry, hide the entries behind it on
     the same probe chain, and have Cache0Enlarge drop them all. */
  if(b == bddcost_null) return 1;
  if(!_ca0size) return 1;
  if(_ca0ent >= (_ca0size >> 1) && Cache0Enlarge()) return 1;
  bddword id = f.GetID();
  bddword k = Hash0(op, id) & (_ca0size - 1);
  while(1)
  {
    if(_ca0[k]._b == bddcost_null) { _ca0ent++; break; }
    if(_ca0[k]._op == op && _ca0[k]._key.GetID() == id) break;
    k++;
    k &= _ca0size - 1;
  }
  _ca0[k]._op = op;
  _ca0[k]._key = f;
  _ca0[k]._b = b;
  return 0;
}

static BDDCT* CT;
static ZDD CLE(const ZDD &, const bddcost, bddcost &, bddcost &);
ZDD CLE(const ZDD& f, const bddcost bound,
          bddcost& acc_worst, bddcost& rej_best)
{
  CT->_call++;
  if(f == 0)
  {
    acc_worst = bddcost_null;
    rej_best = bddcost_null;
    return 0;
  }
  if(f == 1)
  {
    if(bound >= 0)
    {
      acc_worst = 0;
      rej_best = bddcost_null;
      return 1;
    }
    else
    {
      acc_worst = bddcost_null;
      rej_best = 0;
      return 0;
    }
  }
  ZDD h;
  h =  CT->CacheRef(f, bound, acc_worst, rej_best);
  if(h != -1) return h;
  int top = f.Top();
  bddcost cost = CT->CostOfLev(BDD_LevOfVar(top));
  bddcost aw0, aw1, rb0, rb1;
  ZDD f1 = f.OnSet0(top);
  ZDD f0 = f.OffSet(top);
  if(f1 == -1 || f0 == -1)
    BDDerr("BDDCT::ZDD_CostLE(): memory overflow", ExceptionType::OutOfMemory);
  h = CLE(f1, SubCost(bound, cost), aw1, rb1).Change(top)
    + CLE(f0, bound, aw0, rb0);
  /* the error value must never reach the cache: it is what CacheRef returns
     for a miss and what CacheEnt stores for a rejected bound */
  if(h == -1)
    BDDerr("BDDCT::ZDD_CostLE(): memory overflow", ExceptionType::OutOfMemory);
  /*
  h = CLE(f.OffSet(top), bound, aw0, rb0)
    + CLE(f.OnSet0(top), bound - cost, aw1, rb1).Change(top);
  */
  if(aw1 == bddcost_null) acc_worst = aw0;
  else
  {
    aw1 = AddCost(aw1, cost);
    acc_worst = (aw0 == bddcost_null)? aw1: (aw0 > aw1)? aw0: aw1;
  }
  if(rb1 == bddcost_null) rej_best = rb0;
  else
  {
    rb1 = AddCost(rb1, cost);
    rej_best = (rb0 == bddcost_null)? rb1: (rb0 < rb1)? rb0: rb1;
  }
  CT->CacheEnt(f, h, acc_worst, rej_best);
  /*
  if(h == 0) CT->CacheEnt(f, h, bddcost_null, bound+1);
  else if(h == f) CT->CacheEnt(f, h, bound, bddcost_null);
  else CT->CacheEnt(f, h, bound, bound+1);
  */
  //CT->CacheEnt(f, h, bound, bound+1);
  return h;
}

ZDD BDDCT::ZDD_CostLE(const ZDD& f, const bddcost bound,
                         bddcost& acc_worst, bddcost& rej_best)
{
  if(f == -1)
    BDDerr("BDDCT::ZDD_CostLE(): invalid ZDD", ExceptionType::InvalidBDDValue);
  CT = this;
  _call = 0;
  ZDD h = CLE(f, bound, acc_worst, rej_best);
  return h;
}

static bddcost MinC(const ZDD&);
bddcost MinC(const ZDD& f)
{
  if(f == 0) return bddcost_null;
  if(f == 1) return 0;
  bddcost min = CT->Cache0Ref(4, f);
  if(min != bddcost_null) return min;
  int top = f.Top();
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::MinCost(): memory overflow", ExceptionType::OutOfMemory);
  min = MinC(f0);
  bddcost min1 = MinC(f1);
  if(min1 != bddcost_null)
    min1 = AddCost(min1, CT->CostOfLev(BDD_LevOfVar(top)));
  min = (min != bddcost_null && min < min1)? min: min1;
  CT->Cache0Ent(4, f, min);
  return min;
}

bddcost BDDCT::MinCost(const ZDD& f)
{
  if(f == -1)
    BDDerr("BDDCT::MinCost(): invalid ZDD", ExceptionType::InvalidBDDValue);
  CT = this;
  return MinC(f);
}

static bddcost MaxC(const ZDD&);
bddcost MaxC(const ZDD& f)
{
  if(f == 0) return bddcost_null;
  if(f == 1) return 0;
  bddcost max = CT->Cache0Ref(5, f);
  if(max != bddcost_null) return max;
  int top = f.Top();
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::MaxCost(): memory overflow", ExceptionType::OutOfMemory);
  max = MaxC(f0);
  bddcost max1 = MaxC(f1);
  if(max1 != bddcost_null)
    max1 = AddCost(max1, CT->CostOfLev(BDD_LevOfVar(top)));
  max = (max != bddcost_null && max > max1)? max: max1;
  CT->Cache0Ent(5, f, max);
  return max;
}

bddcost BDDCT::MaxCost(const ZDD& f)
{
  if(f == -1)
    BDDerr("BDDCT::MaxCost(): invalid ZDD", ExceptionType::InvalidBDDValue);
  CT = this;
  return MaxC(f);
}

static bddcost B;
static bddcost RetMin;
static bddcost RetMax;
static ZDD CLE0(const ZDD &, const bddcost);
ZDD CLE0(const ZDD& f, const bddcost spent)
{
  if(f == 0)
  {
    RetMin = bddcost_null; RetMax = bddcost_null;
    return 0;
  }
  if(f == 1)
  {
    RetMin = 0; RetMax = 0;
    return (B >= spent)? 1: 0;
  }
  bddcost min = CT->Cache0Ref(4, f);
  bddcost max = CT->Cache0Ref(5, f);
  // Pruning returns without recurring, so RetMin and RetMax have to be known
  // beforehand: the caller reads them as the minimum and the maximum cost of
  // this very sub-ZDD, and cannot tell a missing cache entry (bddcost_null)
  // from the same value's other meaning, "this branch holds no set at all".
  // With one of the two missing we fall through to the recursion below, which
  // computes and caches it; only the first visit of the node pays for that.
  if(min != bddcost_null && max != bddcost_null)
  {
    RetMin = min; RetMax = max;
    if(B < AddCost(min, spent)) return 0;
    if(B >= AddCost(max, spent)) return f;
  }
  int top = f.Top();
  int tlev = BDD_LevOfVar(top);
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  ZDD h = CLE0(f0, spent);
  bddcost min0 = RetMin;
  bddcost max0 = RetMax;
  bddcost cost = CT->CostOfLev(tlev);
  ZDD h1 = CLE0(f1, AddCost(spent, cost)).Change(top);
  if(h1 == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  h += h1;
  if(h == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  if(min == bddcost_null)
  {
    min = AddCost(RetMin, cost);
    if(min0 != bddcost_null) min = (min0 <= min)? min0: min;
    CT->Cache0Ent(4, f, min);
  }
  if(max == bddcost_null)
  {
    max = AddCost(RetMax, cost);
    if(max0 != bddcost_null) max = (max0 >= max)? max0: max;
    CT->Cache0Ent(5, f, max);
  }
  RetMin = min; RetMax = max;
  return h;
}

ZDD BDDCT::ZDD_CostLE0(const ZDD& f, const bddcost bound)
{
  if(f == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): invalid ZDD", ExceptionType::InvalidBDDValue);
  CT = this;
  B = bound;
  ZDD h = CLE0(f, 0);
  return h;
}

} // namespace sapporobdd

