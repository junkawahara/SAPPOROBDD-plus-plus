/****************************************
 * BDD Cost Table class - Body v1.97    *
 * (C) Shin-ichi MINATO (Jan. 2, 2023)  *
 ****************************************/

#include <new>

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

/* Cost() answers with the bddcost_null mark for every index outside the
   table, on both sides.  A negative index, that is a variable at a level
   above the table, used to answer 1 instead, and none of the recursions
   looked at what came back: a diagram over more variables than the table
   describes was then filtered, minimised and maximised as if each of the
   variables the table knows nothing about cost 1, without a word.  The
   recursions now refuse such a variable through TopCost(). */
bddcost BDDCT::Cost(const int ix) const
{
  return (ix < 0 || ix >= _n)? bddcost_null: _cost[ix];
}

/* the cost of the variable a node tests, refusing the ones the table has no
   entry for; what names the operation in the error message */
bddcost BDDCT::TopCost(const int top, const char* what) const
{
  bddcost cost = CostOfLev(BDD_LevOfVar(top));
  if(cost == bddcost_null) BDDerr(what, ExceptionType::OutOfRange);
  return cost;
}

const char* BDDCT::Label(const int ix) const
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
  /* the costs behind every cached result just changed; both clears raise the
     out-of-memory error themselves, so the 1 returned above is the index or
     the cost being wrong and nothing else */
  if(_caent > 0) CacheClear();
  if(_ca0ent > 0) Cache0Clear();
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
    /* The null checks below used to guard a plain new, which never returns
       null: it throws std::bad_alloc, which is not a BDDException and would
       escape every handler this library asks its users to write, so the
       checks were dead and the failure left the class's own error path.
       With nothrow the checks are real, and the failure is reported as the
       library's out-of-memory error, on the empty table Alloc(0) leaves. */
    _cost = new(std::nothrow) bddcost[_n];
    if(!_cost)
    {
      Alloc(0);
      BDDerr("BDDCT::Alloc(): memory overflow", ExceptionType::OutOfMemory);
    }
    _label = new(std::nothrow) char*[_n];
    if(!_label)
    {
      Alloc(0);
      BDDerr("BDDCT::Alloc(): memory overflow", ExceptionType::OutOfMemory);
    }
    for(int i=0; i<_n; i++)
    {
      _cost[i] = cost;
      _label[i] = 0;
    }
    for(int i=0; i<_n; i++)
    {
      _label[i] = new(std::nothrow) char[CT_STRLEN + 1];
      if(!_label[i])
      {
        Alloc(0);
        BDDerr("BDDCT::Alloc(): memory overflow", ExceptionType::OutOfMemory);
      }
      _label[i][0] = 0;
    }
  }

  /* both raise the out-of-memory error themselves if they cannot allocate */
  CacheClear();
  Cache0Clear();
  return 0;
}

/* Parses one decimal token, with an optional sign, into a bddcost.  Returns
   0 and stores the value when the whole token is a number whose magnitude is
   at most bddcost_null, and 1 otherwise; strtol() used to accept junk as 0
   and to truncate values past the int range silently.  SetCost() then still
   rejects the two magnitude-bddcost_null values themselves. */
static int ParseCost(const std::string& s, bddcost& val)
{
  std::string::size_type i = 0;
  int neg = 0;
  if(i < s.size() && (s[i] == '+' || s[i] == '-'))
  {
    neg = (s[i] == '-');
    i++;
  }
  if(i >= s.size()) return 1;
  long long v = 0;
  for(; i<s.size(); i++)
  {
    if(!isdigit((unsigned char)s[i])) return 1;
    v = v * 10 + (s[i] - '0');
    if(v > (long long)bddcost_null) return 1;
  }
  val = (bddcost)(neg? -v: v);
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
  /* strtol() used to turn junk into a size 0 and quietly truncate numbers
     past the int range; both are format errors */
  unsigned long long n;
  if(ReadDecimal(s, (unsigned long long)INT_MAX, n)) { Alloc(0); return 1; }
  if(Alloc((int)n)) return 1;
  /* an empty table has nothing after its size, so its own Export() output
     used to be rejected at the EOF here */
  if(_n == 0) return 0;

  do if(ReadToken(fp, s) == EOF) { Alloc(0); return 1; }
  while(s[0] == '#'); // go next word
  int e = 0;
  int eof = 0;
  for(int ix=0; ix<_n && !e && !eof; ix++)
  {
    bddcost cost;
    if((e = ParseCost(s, cost))) break;
    if((e = SetCost(ix, cost))) break;
    if(ReadToken(fp, s) == EOF) { eof = 1; if(ix<_n-1) e = 1; break; }
    if(s[0] == '#') 
    {
      if((e = SetLabel(ix, s.c_str()+1))) break;
      /* an EOF inside this skip has to end the outer loop as well: it used
         to only leave the do-while, and the next round then reused the
         label token as a cost and overwrote the error flag */
      do if(ReadToken(fp, s) == EOF) { eof = 1; if(ix<_n-1) e = 1; break; }
      while(!eof && s[0] == '#'); // go next word
    }
  }
  if(e) { Alloc(0); return 1; }
  return 0;
}

/* Fills a fresh table of n variables with costs drawn uniformly from the
   closed range [min, max].  The numbers come from the C library's rand(), so
   a program that never calls srand() gets the same table on every run. */
int BDDCT::AllocRand(const int n, const bddcost min, const bddcost max)
{
  /* An invalid or empty range is refused before the table it would replace is
     dropped.  min > max used to mean "the min everywhere", which answers a
     mistake -- swapped arguments -- with a table rather than with an error. */
  if(CostChk(min) || CostChk(max) || min > max) return 1;
  /* the result of Alloc() used to be dropped, and a failure then left the
     empty table behind and reported success */
  if(Alloc(n)) return 1;
  /* the width of the widest valid range does not fit in bddcost, and
     max - min + 1 used to overflow it */
  unsigned long long m =
    (unsigned long long)((long long)max - (long long)min) + 1ULL;
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

/* Releases the cache and starts a new empty one.  The return value is kept
   at 0: a failure to allocate the new table raises the out-of-memory error
   instead, as the null check on the plain new never fired. */
int BDDCT::CacheClear()
{
  if(_ca) { delete[] _ca; _ca = 0; }
  _casize = 0;
  _caent = 0;
  CacheEntry* ca = new(std::nothrow) CacheEntry[1 << 4];
  /* the empty cache left behind is consistent: CacheRef() misses and
     CacheEnt() declines while _casize is 0 */
  if(!ca) BDDerr("BDDCT::CacheClear(): memory overflow",
                 ExceptionType::OutOfMemory);
  _ca = ca;
  _casize = 1 << 4;
  return 0;
}

#define Hash(id) ((id)*1234567)

int BDDCT::CacheEnlarge()
{
  bddword newsize = _casize << 2;
  //cout << "enlarge: " << newsize << "\n";
  /* growing the cache is optional: the failure is reported to the caller,
     which goes on with the cache it has */
  CacheEntry* newca = new(std::nothrow) CacheEntry[newsize];
  if(!newca) return 1;
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
      /* Every key is a negated cost, so landing past the end means the entry
         knows of nothing at or below this bound, which is a miss.

         The branch that used to be here walked back from end() looking for
         the "every set was rejected" mark.  That mark is stored under the key
         bddcost_null, the largest key there is, so a lower_bound() never runs
         past it and the walk could not reach it: the mark is served by the
         ordinary path below.  On an entry whose map is empty -- which the
         public CacheEnt() leaves behind when both of its bounds are the
         bddcost_null mark and its result is not the empty family -- the same
         walk stepped off the front of the map, which is undefined. */
      if(itr == zm->end()) return -1;
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
      /* the count used to go up before the allocation, so a failure left the
         table counting one entry more than it holds */
      Zmap* zm = new(std::nothrow) Zmap;
      if(!zm) return 1;
      _ca[k]._zmap = zm;
      _ca[k]._key = f;
      _caent++;
      break;
    }
    if(_ca[k]._key.GetID() == id) break;
    k++;
    k &= _casize - 1;
  }
  Zmap* zm = _ca[k]._zmap;
  /* the map allocates a node per cost; caching is an optimisation, so a
     failure here costs the entry and not the computation */
  try
  {
    if(acc_worst != bddcost_null) (*zm)[NegCost(acc_worst)] = h;
    else if(h == 0) (*zm)[bddcost_null] = 0;
    if(rej_best != bddcost_null)
       if(zm->find(NegCost(rej_best)) == zm->end()) (*zm)[NegCost(rej_best)] = -1;
  }
  catch(const std::bad_alloc&) { return 1; }
  return 0;
}

/* as CacheClear(), for the cost cache */
int BDDCT::Cache0Clear()
{
  if(_ca0) { delete[] _ca0; _ca0 = 0; }
  _ca0size = 0;
  _ca0ent = 0;
  Cache0Entry* ca0 = new(std::nothrow) Cache0Entry[1 << 4];
  if(!ca0) BDDerr("BDDCT::Cache0Clear(): memory overflow",
                  ExceptionType::OutOfMemory);
  _ca0 = ca0;
  _ca0size = 1 << 4;
  return 0;
}

#define Hash0(op, id) ((id)*1234567+(op))

int BDDCT::Cache0Enlarge()
{
  bddword newsize = _ca0size << 2;
  //cout << "enlarge: " << newsize << "\n";
  /* as CacheEnlarge(): the caller goes on with the cache it has */
  Cache0Entry* newca0 = new(std::nothrow) Cache0Entry[newsize];
  if(!newca0) return 1;
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


/* Every one of the four recursions below counts its own calls in _call, and
   every entry point resets the counter before it recurses, so CallCount()
   always reports the operation that ran last.  The counter used to be kept by
   ZDD_CostLE() alone, and MinCost(), MaxCost() and ZDD_CostLE0() left whatever
   the last ZDD_CostLE() had counted in place.

   The four recursions below are members of the class.  They used to be
   file-static functions that reached their table through a file-static
   BDDCT* and handed the bound and the two cost results of ZDD_CostLE0() to
   each other through three more file-static variables.  That made every one
   of these operations non-reentrant and unusable from more than one thread
   even on tables of their own, and it left the pointer to the last table
   used behind after that table was destroyed.  Passing the context in the
   ordinary way costs nothing and removes all of it. */

ZDD BDDCT::CLE(const ZDD& f, const bddcost bound,
               bddcost& acc_worst, bddcost& rej_best)
{
  _call++;
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
  h =  CacheRef(f, bound, acc_worst, rej_best);
  if(h != -1) return h;
  BDD_RECUR_INC;
  int top = f.Top();
  bddcost cost = TopCost(top,
    "BDDCT::ZDD_CostLE(): variable outside the cost table");
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
  CacheEnt(f, h, acc_worst, rej_best);
  BDD_RECUR_DEC;
  return h;
}

ZDD BDDCT::ZDD_CostLE(const ZDD& f, const bddcost bound,
                         bddcost& acc_worst, bddcost& rej_best)
{
  if(f == -1)
    BDDerr("BDDCT::ZDD_CostLE(): invalid ZDD", ExceptionType::InvalidBDDValue);
  _call = 0;
  ZDD h = CLE(f, bound, acc_worst, rej_best);
  return h;
}

bddcost BDDCT::MinC(const ZDD& f)
{
  _call++;
  if(f == 0) return bddcost_null;
  if(f == 1) return 0;
  bddcost min = Cache0Ref(4, f);
  if(min != bddcost_null) return min;
  BDD_RECUR_INC;
  int top = f.Top();
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::MinCost(): memory overflow", ExceptionType::OutOfMemory);
  bddcost cost = TopCost(top,
    "BDDCT::MinCost(): variable outside the cost table");
  min = MinC(f0);
  bddcost min1 = MinC(f1);
  if(min1 != bddcost_null) min1 = AddCost(min1, cost);
  min = (min != bddcost_null && min < min1)? min: min1;
  Cache0Ent(4, f, min);
  BDD_RECUR_DEC;
  return min;
}

bddcost BDDCT::MinCost(const ZDD& f)
{
  if(f == -1)
    BDDerr("BDDCT::MinCost(): invalid ZDD", ExceptionType::InvalidBDDValue);
  _call = 0;
  return MinC(f);
}

bddcost BDDCT::MaxC(const ZDD& f)
{
  _call++;
  if(f == 0) return bddcost_null;
  if(f == 1) return 0;
  bddcost max = Cache0Ref(5, f);
  if(max != bddcost_null) return max;
  BDD_RECUR_INC;
  int top = f.Top();
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::MaxCost(): memory overflow", ExceptionType::OutOfMemory);
  bddcost cost = TopCost(top,
    "BDDCT::MaxCost(): variable outside the cost table");
  max = MaxC(f0);
  bddcost max1 = MaxC(f1);
  if(max1 != bddcost_null) max1 = AddCost(max1, cost);
  max = (max != bddcost_null && max > max1)? max: max1;
  Cache0Ent(5, f, max);
  BDD_RECUR_DEC;
  return max;
}

bddcost BDDCT::MaxCost(const ZDD& f)
{
  if(f == -1)
    BDDerr("BDDCT::MaxCost(): invalid ZDD", ExceptionType::InvalidBDDValue);
  _call = 0;
  return MaxC(f);
}

/* retmin and retmax report the minimum and the maximum cost of f, which the
   caller needs on top of the filtered family itself; they used to be two
   file-static variables. */
ZDD BDDCT::CLE0(const ZDD& f, const bddcost bound, const bddcost spent,
                bddcost& retmin, bddcost& retmax)
{
  _call++;
  if(f == 0)
  {
    retmin = bddcost_null; retmax = bddcost_null;
    return 0;
  }
  if(f == 1)
  {
    retmin = 0; retmax = 0;
    return (bound >= spent)? 1: 0;
  }
  bddcost min = Cache0Ref(4, f);
  bddcost max = Cache0Ref(5, f);
  // Pruning returns without recurring, so retmin and retmax have to be known
  // beforehand: the caller reads them as the minimum and the maximum cost of
  // this very sub-ZDD, and cannot tell a missing cache entry (bddcost_null)
  // from the same value's other meaning, "this branch holds no set at all".
  // With one of the two missing we fall through to the recursion below, which
  // computes and caches it; only the first visit of the node pays for that.
  if(min != bddcost_null && max != bddcost_null)
  {
    retmin = min; retmax = max;
    if(bound < AddCost(min, spent)) return 0;
    if(bound >= AddCost(max, spent)) return f;
  }
  BDD_RECUR_INC;
  int top = f.Top();
  bddcost cost = TopCost(top,
    "BDDCT::ZDD_CostLE0(): variable outside the cost table");
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  if(f0 == -1 || f1 == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  bddcost min0, max0, min1, max1;
  ZDD h = CLE0(f0, bound, spent, min0, max0);
  ZDD h1 = CLE0(f1, bound, AddCost(spent, cost), min1, max1).Change(top);
  if(h1 == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  h += h1;
  if(h == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): memory overflow", ExceptionType::OutOfMemory);
  if(min == bddcost_null)
  {
    min = AddCost(min1, cost);
    if(min0 != bddcost_null) min = (min0 <= min)? min0: min;
    Cache0Ent(4, f, min);
  }
  if(max == bddcost_null)
  {
    max = AddCost(max1, cost);
    if(max0 != bddcost_null) max = (max0 >= max)? max0: max;
    Cache0Ent(5, f, max);
  }
  retmin = min; retmax = max;
  BDD_RECUR_DEC;
  return h;
}

ZDD BDDCT::ZDD_CostLE0(const ZDD& f, const bddcost bound)
{
  if(f == -1)
    BDDerr("BDDCT::ZDD_CostLE0(): invalid ZDD", ExceptionType::InvalidBDDValue);
  _call = 0;
  bddcost retmin, retmax;
  ZDD h = CLE0(f, bound, 0, retmin, retmax);
  return h;
}

} // namespace sapporobdd
