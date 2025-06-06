/*********************************************
 * ZDD+ Manipulator (SAPPORO-1.87) - Header *
 * (C) Shin-ichi MINATO  (May 14, 2021)      *
 *********************************************/

#ifndef _ZDD_
#define _ZDD_

#include "BDD.h"

namespace sapporobdd {

class SeqBDD;
class ZDD;
class ZDDV;

class ZDD
{
  bddword _zdd;

public:
  ZDD(void) { _zdd = bddempty; }
  ZDD(int v) { _zdd = (v==0)? bddempty:(v>0)? bddsingle:bddnull; }
  ZDD(const ZDD& f) { _zdd = bddcopy(f._zdd); }

  ~ZDD(void) { bddfree(_zdd); }

  ZDD& operator=(const ZDD& f) { 
    if(_zdd != f._zdd) { bddfree(_zdd); _zdd = bddcopy(f._zdd); } 
    return *this;
  }

  ZDD& operator&=(const ZDD& f)
    { ZDD h; h._zdd = bddintersec(_zdd, f._zdd); return *this = h; }

  ZDD& operator+=(const ZDD& f)
    { ZDD h; h._zdd = bddunion(_zdd, f._zdd); return *this = h; }

  ZDD& operator-=(const ZDD& f)
    { ZDD h; h._zdd = bddsubtract(_zdd, f._zdd); return *this = h; }

  ZDD& operator<<=(int s)
    { ZDD h; h._zdd = bddlshift(_zdd, s); return *this = h; }

  ZDD& operator>>=(int s)
    { ZDD h; h._zdd = bddrshift(_zdd, s); return *this = h; }

  ZDD& operator*=(const ZDD&);
  ZDD& operator/=(const ZDD&);
  ZDD& operator%=(const ZDD&);

  ZDD operator<<(int s) const
    { ZDD h; h._zdd = bddlshift(_zdd, s); return h; }

  ZDD operator>>(int s) const
    { ZDD h; h._zdd = bddrshift(_zdd, s); return h; }

  int Top(void) const { return bddtop(_zdd); }

  ZDD OffSet(int v) const
    { ZDD h; h._zdd = bddoffset(_zdd, v); return h; }

  ZDD OnSet(int v) const
    { ZDD h; h._zdd = bddonset(_zdd, v); return h; }

  ZDD OnSet0(int v) const
    { ZDD h; h._zdd = bddonset0(_zdd, v); return h; }

  ZDD Change(int v) const
    { ZDD h; h._zdd = bddchange(_zdd, v); return h; }

  bddword GetID(void) const { return _zdd; }
  bddword Size(void) const { return bddsize(_zdd); }
  bddword Card(void) const { return bddcard(_zdd); }
  bddword Lit(void) const { return bddlit(_zdd); }
  bddword Len(void) const { return bddlen(_zdd); }
  char* CardMP16(char* s) const { return bddcardmp16(_zdd, s); }

  void Export(FILE *strm = stdout) const;
  void XPrint(void) const;
  void Print(void) const;
  void PrintPla(void) const;

  ZDD Swap(int, int) const;
  ZDD Restrict(const ZDD&) const;
  ZDD Permit(const ZDD&) const;
  ZDD PermitSym(int) const;
  ZDD Support(void) const
    { ZDD h; h._zdd = bddsupport(_zdd); return h; }
  ZDD Always(void) const;

  int SymChk(int, int) const;
  ZDD SymGrp(void) const;
  ZDD SymGrpNaive(void) const;

  ZDD SymSet(int) const;
  int ImplyChk(int, int) const;
  int CoImplyChk(int, int) const;
  ZDD ImplySet(int) const;
  ZDD CoImplySet(int) const;

  int IsPoly(void) const;
  ZDD Divisor(void) const;

  ZDD ZLev(int lev, int last = 0) const;
  void SetZSkip(void) const;
  ZDD Intersec(const ZDD&) const;

  friend ZDD ZDD_ID(bddword);
  friend ZDD ZBDD_ID(bddword); // for backward compatibility

  //friend class SeqBDD;
};

typedef ZDD ZBDD; // for backward compatibility

extern ZDD operator*(const ZDD&, const ZDD&);
extern ZDD operator/(const ZDD&, const ZDD&);
extern ZDD ZDD_Meet(const ZDD&, const ZDD&);
extern ZDD ZDD_Random(int, int density = 50);
extern ZDD ZDD_Import(FILE *strm = stdin);

extern ZDD ZDD_LCM_A(char *, int);
extern ZDD ZDD_LCM_C(char *, int);
extern ZDD ZDD_LCM_M(char *, int);

// Aliases for backward compatibility
inline ZDD ZBDD_Meet(const ZDD& f, const ZDD& g) { return ZDD_Meet(f, g); }
inline ZDD ZBDD_Random(int n, int density = 50) { return ZDD_Random(n, density); }
inline ZDD ZBDD_Import(FILE *strm = stdin) { return ZDD_Import(strm); }
inline ZDD ZBDD_LCM_A(char *fname, int th) { return ZDD_LCM_A(fname, th); }
inline ZDD ZBDD_LCM_C(char *fname, int th) { return ZDD_LCM_C(fname, th); }
inline ZDD ZBDD_LCM_M(char *fname, int th) { return ZDD_LCM_M(fname, th); }

inline ZDD ZDD_ID(bddword zdd)
  { ZDD h; h._zdd = zdd; return h; }

inline ZDD ZBDD_ID(bddword zdd)
  { return ZDD_ID(zdd); } // for backward compatibility

inline ZDD BDD_CacheZDD(char op, bddword fx, bddword gx)
  { return ZDD_ID(bddcopy(bddrcache(op, fx, gx))); }

inline ZDD operator&(const ZDD& f, const ZDD& g)
  { return ZDD_ID(bddintersec(f.GetID(), g.GetID())); }

inline ZDD operator+(const ZDD& f, const ZDD& g)
  { return ZDD_ID(bddunion(f.GetID(), g.GetID())); }

inline ZDD operator-(const ZDD& f, const ZDD& g)
  { return ZDD_ID(bddsubtract(f.GetID(), g.GetID())); }

inline ZDD operator%(const ZDD& f, const ZDD& p)
  { return f - (f/p) * p; }

inline int operator==(const ZDD& f, const ZDD& g)
  { return f.GetID() == g.GetID(); }

inline int operator!=(const ZDD& f, const ZDD& g)
  { return !(f == g); }

inline bool operator<(const ZDD& f, const ZDD& g)
  { return f.GetID() < g.GetID(); }

inline ZDD& ZDD::operator*=(const ZDD& f)
  { return *this = *this * f; }

inline ZDD& ZDD::operator/=(const ZDD& f)
  { return *this = *this / f; }

inline ZDD& ZDD::operator%=(const ZDD& f)
  { return *this = *this % f; }


class ZDDV
{
  ZDD _zdd;

public:
  ZDDV(void) { _zdd = 0; }
  ZDDV(const ZDDV& fv) { _zdd = fv._zdd; }
  ZDDV(const ZDD& f, int location = 0);
  ~ZDDV(void) { }

  ZDDV& operator=(const ZDDV& fv) { _zdd = fv._zdd; return *this; }
  ZDDV& operator&=(const ZDDV& fv) { _zdd &= fv._zdd; return *this; }
  ZDDV& operator+=(const ZDDV& fv) { _zdd += fv._zdd; return *this; }
  ZDDV& operator-=(const ZDDV& fv) { _zdd -= fv._zdd; return *this; }
  ZDDV& operator<<=(int);
  ZDDV& operator>>=(int);

  ZDDV operator<<(int) const;
  ZDDV operator>>(int) const;

  ZDDV OffSet(int) const;
  ZDDV OnSet(int) const;
  ZDDV OnSet0(int) const;
  ZDDV Change(int) const;
  ZDDV Swap(int, int) const;
  
  int Top(void) const;
  int Last(void) const;
  ZDDV Mask(int start, int length = 1) const;
  ZDD GetZDD(int) const;

  ZDD GetMetaZDD(void) const { return _zdd; }
  bddword Size(void) const;
  void Print(void) const;
  void Export(FILE *strm = stdout) const;
  int PrintPla(void) const;
  void XPrint(void) const;
	
  friend ZDDV operator&(const ZDDV&, const ZDDV&);
  friend ZDDV operator+(const ZDDV&, const ZDDV&);
  friend ZDDV operator-(const ZDDV&, const ZDDV&);
};

typedef ZDDV ZBDDV; // for backward compatibility

extern ZDDV ZDDV_Import(FILE *strm = stdin);
inline ZDDV ZBDDV_Import(FILE *strm = stdin) { return ZDDV_Import(strm); } // for backward compatibility

inline ZDDV operator&(const ZDDV& fv, const ZDDV& gv)
  { ZDDV hv; hv._zdd = fv._zdd & gv._zdd; return hv; }
inline ZDDV operator+(const ZDDV& fv, const ZDDV& gv)
  { ZDDV hv; hv._zdd = fv._zdd + gv._zdd; return hv; }
inline ZDDV operator-(const ZDDV& fv, const ZDDV& gv)
  { ZDDV hv; hv._zdd = fv._zdd - gv._zdd; return hv; }
inline int operator==(const ZDDV& fv, const ZDDV& gv)
  {  return fv.GetMetaZDD() == gv.GetMetaZDD(); }
inline int operator!=(const ZDDV& fv, const ZDDV& gv)
  {  return !(fv == gv); }

inline ZDDV& ZDDV::operator<<=(int s)
  { return *this = *this << s; }

inline ZDDV& ZDDV::operator>>=(int s)
  { return *this = *this >> s; }

class ZDD_Hash;
class ZDD_Hash
{
  struct ZDD_Entry
  {
    ZDD _key;
    void* _ptr;
    ZDD_Entry(void){ _key = -1; }
  };

  bddword _amount;
  bddword _hashSize;
  ZDD_Entry* _wheel;
  
  ZDD_Entry* GetEntry(ZDD);
  void Enlarge(void);
public:
  ZDD_Hash(void);
  ~ZDD_Hash(void);
  void Clear(void);
  void Enter(ZDD, void *);
  void* Refer(ZDD);
  bddword Amount(void);
};

typedef ZDD_Hash ZBDD_Hash; // for backward compatibility

} // namespace sapporobdd

// Hash function specialization for ZDD (C++11 and later)
#if __cplusplus >= 201103L
#include <functional>
namespace std {
  template<>
  struct hash<sapporobdd::ZDD> {
    std::size_t operator()(const sapporobdd::ZDD& zdd) const {
      return std::hash<sapporobdd::bddword>{}(zdd.GetID());
    }
  };
}
#endif

#endif // _ZDD_
