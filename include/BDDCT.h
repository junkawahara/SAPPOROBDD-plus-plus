/*****************************************
 *  BDD Cost Table class - Header v1.97  *
 *  (C) Shin-ichi MINATO (Jan. 2, 2023)  *
 *****************************************/

#ifndef _BDDCT_
#define _BDDCT_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include "ZDD.h"

namespace sapporobdd {

typedef int bddcost;
#define bddcost_null 0x7FFFFFFF
#define CT_STRLEN 15

typedef std::map<bddcost, ZDD> Zmap;

class BDDCT;

class BDDCT
{
public:
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

  bddcost Cost(const int ix) const;
  inline bddcost CostOfLev(const int lev) const 
  { return Cost(_n-lev); }
  char* Label(const int) const;
  inline char* LabelOfLev(const int lev) const 
  { return Label(_n-lev); }

  int SetCost(const int, const bddcost);
  inline int SetCostOfLev(const int lev, const bddcost cost) 
  { return SetCost(_n-lev, cost); }
  int SetLabel(const int, const char *);
  inline int SetLabelOfLev(const int lev, const char* label)
  { return SetLabel(_n-lev, label); }

  int Alloc(const int n, const bddcost cost = 1);
  int Import(FILE* fp = stdin);
  int AllocRand(const int, const bddcost, const bddcost);
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

private:
  /* The recursions behind the four entry points above.  They used to be
     file-static functions reaching the table through a file-static BDDCT*,
     with the bound and the two cost results of ZDD_CostLE0() handed around
     through three more file-static variables; as members they take their
     context as arguments instead. */
  ZDD CLE(const ZDD &, const bddcost, bddcost &, bddcost &);
  bddcost MinC(const ZDD &);
  bddcost MaxC(const ZDD &);
  ZDD CLE0(const ZDD &, const bddcost, const bddcost, bddcost &, bddcost &);
};

} // namespace sapporobdd

#endif // _BDDCT_
