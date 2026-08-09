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

typedef std::map<bddcost, ZDD> Zmap;

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
  inline bddcost CostOfLev(const int lev) const 
  { return Cost(_n-lev); }
  /* The label is read through a pointer into the table's own buffer, which
     holds at most CT_STRLEN characters and is released by the next Alloc(),
     Import() or AllocRand() and by the destructor: a caller that wants to
     keep a label past that has to copy it.  The pointer used to be a plain
     char*, so a caller could also write through it and overrun the buffer. */
  const char* Label(const int) const;
  inline const char* LabelOfLev(const int lev) const 
  { return Label(_n-lev); }

  /* 1 for an index outside the table or a cost outside its range, and for
     nothing else: the invalidation of the caches this does cannot fail */
  int SetCost(const int, const bddcost);
  inline int SetCostOfLev(const int lev, const bddcost cost) 
  { return SetCost(_n-lev, cost); }
  /* a label longer than CT_STRLEN characters is refused, not shortened */
  int SetLabel(const int, const char *);
  inline int SetLabelOfLev(const int lev, const char* label)
  { return SetLabel(_n-lev, label); }

  int Alloc(const int n, const bddcost cost = 1);
  int Import(FILE* fp = stdin);
  /* costs drawn from the closed range [min, max] with the C library's
     rand(), which a program that never calls srand() draws the same way on
     every run; an empty or invalid range is refused */
  int AllocRand(const int n, const bddcost min, const bddcost max);
  void Export(void) const;

  int CacheClear(void);
  int CacheEnlarge(void);
  ZDD CacheRef(const ZDD &, const bddcost, bddcost &, bddcost &);
  int CacheEnt(const ZDD &, const ZDD &, const bddcost, const bddcost);

  int Cache0Clear(void);
  int Cache0Enlarge(void);
  bddcost Cache0Ref(const unsigned char, const ZDD &) const;
  int Cache0Ent(const unsigned char, const ZDD &, const bddcost);

  ZDD ZDD_CostLE(const ZDD& f, const bddcost bound)
  { bddcost aw, rb; return ZDD_CostLE(f, bound, aw, rb); }
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
     CacheClear() / Cache0Clear(), which SetCost() and Alloc() also do. */
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
    bddcost _b;
    unsigned char _op;
    Cache0Entry(void)
    {
      _b = bddcost_null;
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
  
  bddword _call;

  /* The recursions behind the four entry points above.  They used to be
     file-static functions reaching the table through a file-static BDDCT*,
     with the bound and the two cost results of ZDD_CostLE0() handed around
     through three more file-static variables; as members they take their
     context as arguments instead. */
  int CacheAlloc(void);
  int Cache0Alloc(void);
  bddcost TopCost(const int, const char *) const;
  ZDD CLE(const ZDD &, const bddcost, bddcost &, bddcost &);
  bddcost MinC(const ZDD &);
  bddcost MaxC(const ZDD &);
  ZDD CLE0(const ZDD &, const bddcost, const bddcost, bddcost &, bddcost &);
};

} // namespace sapporobdd

#endif // BDDCT_H
