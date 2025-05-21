/*********************************************
 * SeqBDD+ Class (SAPPORO-1.87) - Header     *
 * (C) Shin-ichi MINATO  (May 14, 2021)      *
 *********************************************/

#ifndef _SeqBDD_
#define _SeqBDD_

#include "ZDD.h"

namespace sapporobdd {

class SeqBDD;
class SeqBDDV;

extern SeqBDD operator*(const SeqBDD&, const SeqBDD&);
extern SeqBDD operator/(const SeqBDD&, const SeqBDD&);
extern SeqBDD operator%(const SeqBDD&, const SeqBDD&);

//extern SeqBDD SeqBDD_Import(FILE *strm = stdin);
//extern SeqBDD SeqBDD_ID(bddword);

class SeqBDD
{
  ZDD _zdd;
public:
  SeqBDD(void){ _zdd = ZDD(); }
  SeqBDD(int val) { _zdd = ZDD(val); }
  SeqBDD(const SeqBDD& f){ _zdd = f._zdd; }
  SeqBDD(const ZDD& zdd){ _zdd = zdd; }
  ~SeqBDD(void){ }

  SeqBDD& operator=(const SeqBDD& f) { _zdd = f._zdd; return *this; }
  SeqBDD operator&=(const SeqBDD& f)
    { _zdd = _zdd & f._zdd; return *this; }

  SeqBDD operator+=(const SeqBDD& f)
    { _zdd = _zdd + f._zdd; return *this; }
  SeqBDD operator-=(const SeqBDD& f)
    { _zdd = _zdd - f._zdd; return *this; }

  SeqBDD operator*=(const SeqBDD&); // inline
  SeqBDD operator/=(const SeqBDD&); // inline
  SeqBDD operator%=(const SeqBDD&); // inline

  SeqBDD OffSet(int v) const;
  SeqBDD OnSet0(int) const;
  SeqBDD OnSet(int v) const
    { return OnSet0(v).Push(v); }
  SeqBDD Push(int v) const
    { return SeqBDD(ZDD_ID(bddpush(_zdd.GetID(), v))); }

  int Top(void) const { return _zdd.Top(); }
  ZDD GetZDD(void) const { return _zdd; }

  bddword Size(void) const;
  bddword Card(void) const;
  bddword Lit(void) const;
  bddword Len(void) const;
  void PrintSeq(void) const;
  void Export(FILE *strm = stdout) const;
  void Print(void) const;
  //void XPrint(void) const;

  friend SeqBDD operator&(const SeqBDD&, const SeqBDD&);
  friend SeqBDD operator+(const SeqBDD&, const SeqBDD&);
  friend SeqBDD operator-(const SeqBDD&, const SeqBDD&);
  friend SeqBDD operator*(const SeqBDD&, const SeqBDD&);
  friend SeqBDD operator/(const SeqBDD&, const SeqBDD&);
  friend SeqBDD operator%(const SeqBDD&, const SeqBDD&);
  friend int operator==(const SeqBDD&, const SeqBDD&);
  friend SeqBDD SeqBDD_ID(bddword);
};

inline SeqBDD operator&(const SeqBDD& f, const SeqBDD& g)
  { return SeqBDD(f._zdd & g._zdd); }

inline SeqBDD operator+(const SeqBDD& f, const SeqBDD& g)
  { return SeqBDD(f._zdd + g._zdd); }

inline SeqBDD operator-(const SeqBDD& f, const SeqBDD& g)
  { return SeqBDD(f._zdd - g._zdd); } 

inline SeqBDD operator%(const SeqBDD& f, const SeqBDD& p)
  { return f - p * (f/p); }

inline int operator==(const SeqBDD& f, const SeqBDD& g)
  { return f._zdd == g._zdd; }

inline int operator!=(const SeqBDD& f, const SeqBDD& g)
  { return !(f == g); }

inline SeqBDD SeqBDD::operator*=(const SeqBDD& f)
  { return *this = *this * f; }

inline SeqBDD SeqBDD::operator/=(const SeqBDD& f)
  { return *this = *this / f; }

inline SeqBDD SeqBDD::operator%=(const SeqBDD& f)
  { return *this = *this % f; }

} // namespace sapporobdd

#endif // _SeqBDD_
