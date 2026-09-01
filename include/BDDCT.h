/*****************************************
 *  BDD Cost Table class - Header v1.97  *
 *  (C) Shin-ichi MINATO (Jan. 2, 2023)  *
 *****************************************/

/* The include guard used to be _BDDCT_, and an identifier that starts with an
   underscore followed by a capital is reserved to the implementation. */
#ifndef BDDCT_H
#define BDDCT_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include "ZDD.h"

namespace sapporobdd {

typedef int bddcost;

/* These two were macros, so every translation unit that included this header
   carried them in its global macro space, where a short general name like
   CT_STRLEN collides with the code of whoever uses the library, and where the
   namespace around them means nothing.  As constants they follow the same
   scope rules as bddcost itself.

   bddcost_null is the "no value" mark of the class: Cost() answers with it
   for an index outside the table, MinCost() and MaxCost() for the empty
   family, and the two caches use it for an empty slot, so it is not a cost a
   table can hold.  CT_STRLEN is the longest label a table stores. */
const bddcost bddcost_null = 0x7FFFFFFF;
const int CT_STRLEN = 15;

/* The costs a table holds are bddcost values, but the numbers the four cost
   operations work with are not: the sum along a path, the bound that is left
   over after the cost of a variable has been taken off it, and the smallest
   and largest cost of a sub-ZDD are all intermediate values, and costs of
   different signs that cancel in the end can drive any of them outside the
   range of a bddcost on the way.  Every one of them used to overflow
   silently, and then every one of them was made to throw, which refused
   families whose own costs the table can express perfectly well.  They are
   held in bddcostsum instead, which is wide enough for every sum a table of
   bddcost values can produce, and the range is checked where a value is
   handed back to the caller as a bddcost.

   bddcostsum_null is the "no value" mark of that type, as bddcost_null is of
   bddcost. */
typedef long long bddcostsum;
const bddcostsum bddcostsum_null = 0x7FFFFFFFFFFFFFFFLL;

typedef std::map<bddcostsum, ZDD> Zmap;

class BDDCT;

class BDDCT
{
public:
  BDDCT(void);
  ~BDDCT(void);

  /* The table, the labels and both caches are owned through raw pointers,
     so the implicitly generated copy (and with it the implicit move) would
     make two owners of them and double-free.  Copies are therefore not
     part of the contract. */
#if __cplusplus >= 201103L
  BDDCT(const BDDCT&) = delete;
  BDDCT& operator=(const BDDCT&) = delete;
#else
private:
  BDDCT(const BDDCT&);
  BDDCT& operator=(const BDDCT&);
public:
#endif

  inline int Size(void) const { return _n; }

  /* Cost() and CostOfLev() answer with the bddcost_null mark for a variable
     the table has no entry for, on both sides of it; the four cost
     operations refuse such a variable rather than pricing it. */
  bddcost Cost(const int ix) const;
  /* the range is checked before _n - lev is formed: with lev == INT_MIN the
     subtraction itself overflowed (undefined behaviour) before the old
     Cost(_n-lev) could reject the index */
  inline bddcost CostOfLev(const int lev) const 
  { return (lev <= 0 || lev > _n)? bddcost_null: Cost(_n-lev); }
  /* The label is read through a pointer into the table's own buffer, which
     holds at most CT_STRLEN characters and is released by the next Alloc(),
     Import() or AllocRand() and by the destructor: a caller that wants to
     keep a label past that has to copy it.  The pointer used to be a plain
     char*, so a caller could also write through it and overrun the buffer. */
  const char* Label(const int) const;
  inline const char* LabelOfLev(const int lev) const 
  { return (lev <= 0 || lev > _n)? 0: Label(_n-lev); }

  /* 1 for an index outside the table or a cost outside its range, and for
     nothing else: the invalidation of the caches this does cannot fail */
  int SetCost(const int, const bddcost);
  inline int SetCostOfLev(const int lev, const bddcost cost) 
  { return (lev <= 0 || lev > _n)? 1: SetCost(_n-lev, cost); }
  /* a label longer than CT_STRLEN characters, or one containing whitespace
     (which the Export() format could not carry), is refused, not adjusted */
  int SetLabel(const int, const char *);
  inline int SetLabelOfLev(const int lev, const char* label)
  { return (lev <= 0 || lev > _n)? 1: SetLabel(_n-lev, label); }

  int Alloc(const int n, const bddcost cost = 1);
  int Import(FILE* fp = stdin);
  /* costs drawn from the closed range [min, max] with the C library's
     rand(), which a program that never calls srand() draws the same way on
     every run; an empty or invalid range is refused */
  int AllocRand(const int n, const bddcost min, const bddcost max);
  void Export(void) const;

  /* The cache of the four cost operations, open to the caller.  What is in
     it is answered as the result of a cost operation, and nothing here
     checks an entry against the cost table or against the ZDD it is keyed
     by, so an entry that does not describe the operation it is filed under
     is a wrong result returned without a word.  The preconditions of a
     hand-made entry are therefore:

       - the two opcodes the class itself uses are reserved: 4 is MinCost()
         and 5 is MaxCost(), which ZDD_CostLE0() reads as well.  A caller
         that files its own results under one of them replaces the answer of
         that operation for that ZDD;
       - an entry has to hold what the current cost table gives for the key
         ZDD: acc_worst the largest cost accepted under a bound, rej_best the
         smallest cost rejected by it, and h the family the bound leaves.

     Everything a cost operation of this class enters satisfies them.  The
     safe calls for a caller that only wants the memory back are
     CacheClear() and Cache0Clear(), which lose no information. */
  int CacheClear(void);
  int CacheEnlarge(void);
  ZDD CacheRef(const ZDD &, const bddcost, bddcost &, bddcost &);
  int CacheEnt(const ZDD &, const ZDD &, const bddcost, const bddcost);

  int Cache0Clear(void);
  int Cache0Enlarge(void);
  bddcost Cache0Ref(const unsigned char, const ZDD &) const;
  int Cache0Ent(const unsigned char, const ZDD &, const bddcost);

  /* The two-argument form reports no cost, so it answers a family whose
     costs leave the range of bddcost as well; the four-argument form has to
     express acc_worst and rej_best as bddcost values and reports a
     BDDOutOfRangeException when one of them does not fit. */
  ZDD ZDD_CostLE(const ZDD& f, const bddcost bound)
  { bddcostsum aw, rb; return CostLE(f, bound, aw, rb); }
  ZDD ZDD_CostLE(const ZDD &, const bddcost, bddcost &, bddcost &);

  // For backward compatibility
  ZDD ZBDD_CostLE(const ZDD& f, const bddcost bound)
  { return ZDD_CostLE(f, bound); }
  ZDD ZBDD_CostLE(const ZDD& f, const bddcost bound,
    bddcost& aw, bddcost& rb)
  { return ZDD_CostLE(f, bound, aw, rb); }

  ZDD ZDD_CostLE0(const ZDD &, const bddcost);
  // For backward compatibility
  ZDD ZBDD_CostLE0(const ZDD& f, const bddcost bound)
  { return ZDD_CostLE0(f, bound); }
  bddcost MinCost(const ZDD &);
  bddcost MaxCost(const ZDD &);

  /* How many recursive calls the cost operation that ran last needed.  Each
     of ZDD_CostLE(), ZDD_CostLE0(), MinCost() and MaxCost() resets the count
     when it starts, so it always describes that one call. */
  inline bddword CallCount(void) const { return _call; }

/* Everything below is the implementation of the class.  It all used to be
   public, which let any user of the class swap an array out, resize the
   table without its arrays, or edit a cache entry, and so break the
   invariants the code above relies on. */
private:
  /* Both caches are keyed by the node ID of the argument ZDD.  An ID is only
     unique while the node it names is alive: once the last reference to the
     node goes away, the garbage collector may reclaim it and hand the same ID
     to an unrelated node, and the stale entry would then be returned for that
     other function.  Each entry therefore holds the key ZDD itself, which
     keeps the key node alive for as long as the entry lives.  The cost is
     that a cached node is not collectable until the cache is released by
     CacheClear() / Cache0Clear(), which SetCost() and Alloc() also do.

     The entries are also only valid for one variable order: the recursions
     price a node through the level its variable has at the time of the call,
     and BDD_NewVarOfLev() below the top moves the levels of the variables
     above the insertion point.  _levsnap and _snapvars record, per table,
     which variable sat on each of the table's levels when the entries went
     in; CacheSync() compares that snapshot against the present order and
     drops both caches when a level the table covers has changed hands.  An
     insertion above the table's levels moves nothing the table prices, so
     the caches survive it. */
  struct CacheEntry
  {
    ZDD _key;
    Zmap* _zmap;
    CacheEntry(void)
    {
      _zmap = 0;
    }
    ~CacheEntry(void)
    {
      delete _zmap;
    }
  };

  struct Cache0Entry
  {
    ZDD _key;
    bddcostsum _b;
    unsigned char _op;
    Cache0Entry(void)
    {
      _b = bddcostsum_null;
      _op = 255;
    }
    ~Cache0Entry(void) { }
  };

  int _n;
  bddcost *_cost;
  char **_label;

  bddword _casize;
  bddword _caent;
  CacheEntry* _ca;

  bddword _ca0size;
  bddword _ca0ent;
  Cache0Entry* _ca0;

  /* the variable that sat on level lev when the caches were filled is
     _levsnap[lev-1], for the levels the table covers and that carried a
     variable at snapshot time; _snapvars is bddvarused() of that moment,
     so an unchanged bddvarused() certifies the whole snapshot in O(1) */
  bddvar* _levsnap;
  bddvar _snapvars;

  bddword _call;

  /* The recursions behind the four entry points above.  They used to be
     file-static functions reaching the table through a file-static BDDCT*,
     with the bound and the two cost results of ZDD_CostLE0() handed around
     through three more file-static variables; as members they take their
     context as arguments instead. */
  int CacheAlloc(void);
  int Cache0Alloc(void);
  void Snapshot(void);
  void CacheSync(void);
  bddcost TopCost(const int, const char *) const;
  /* The recursions and both caches work in bddcostsum.  The four public cache
     methods above are these four with the conversion to and from bddcost: an
     entry whose costs are not bddcost values is one the narrow form cannot
     describe, so it answers a miss for it and files nothing for it. */
  ZDD CacheRefSum(const ZDD &, const bddcostsum, bddcostsum &, bddcostsum &);
  int CacheEntSum(const ZDD &, const ZDD &, const bddcostsum, const bddcostsum);
  bddcostsum Cache0RefSum(const unsigned char, const ZDD &) const;
  int Cache0EntSum(const unsigned char, const ZDD &, const bddcostsum);
  /* the body both ZDD_CostLE() forms run, before either of them looks at
     whether the two costs it reports fit in a bddcost */
  ZDD CostLE(const ZDD &, const bddcostsum, bddcostsum &, bddcostsum &);
  ZDD CLE(const ZDD &, const bddcostsum, bddcostsum &, bddcostsum &);
  bddcostsum MinC(const ZDD &);
  bddcostsum MaxC(const ZDD &);
  ZDD CLE0(const ZDD &, const bddcostsum, const bddcostsum,
           bddcostsum &, bddcostsum &);
};

} // namespace sapporobdd

#endif // BDDCT_H
