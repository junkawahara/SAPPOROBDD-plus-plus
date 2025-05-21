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
  struct CacheEntry
  {
    bddword _id;
    Zmap* _zmap;
    CacheEntry(void)
    {
      _id = BDD(-1).GetID();
      _zmap = 0;
    }
    ~CacheEntry(void)
    {
      if(!_zmap) delete _zmap;
    }
  };

  struct Cache0Entry
  {
    bddword _id;
    bddcost _b;
    unsigned char _op;
    Cache0Entry(void)
    {
      _id = BDD(-1).GetID();
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
  bddcost Cache0Ref(const unsigned char, const bddword) const;
  int Cache0Ent(const unsigned char, const bddword, const bddcost);

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
};

} // namespace sapporobdd

#endif // _BDDCT_
