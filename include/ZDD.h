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
  /* The count is at most 16 words, so s must hold 16 * sizeof(bddp) * 2 hex
     digits plus the terminating null: 257 bytes in a 64bit build (the
     default and B_EXTEND), 129 bytes with B_32.  There is no length
     argument, so a shorter buffer is overrun without being detected.
     s == 0 asks bddcardmp16() to allocate a buffer of the size it needs. */
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
/* Draws a random family over the top "lev" variables by deciding every one of
   the 2^lev subsets independently (included with probability density%), so it
   costs 2^lev calls to std::rand() and takes time exponential in its first
   argument.  Sampling the whole power set is what the result is defined to be,
   so keep the level small. */
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

inline ZDD BDD_CacheZDD(unsigned char op, bddword fx, bddword gx)
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

/* A hash table mapping ZDD keys to void* values, used as a scratch table by
   applications.  Enter(key, p) inserts or updates; Enter(key, 0) deletes the
   key (a null value cannot be stored, since Refer() answers 0 for "not
   found").  Refer(key) returns the stored pointer or 0; Amount() the number
   of live entries.  Clear() releases every entry at once.  An entry keeps a
   reference to its key ZDD for as long as it is in the table, so bddgc()
   cannot collect the key's nodes until the entry is deleted or cleared. */
class ZDD_Hash
{
  struct ZDD_Entry
  {
    enum State { Empty = 0, Occupied = 1, Deleted = 2 };
    ZDD _key;
    void* _ptr;
    char _state;
    ZDD_Entry(void){ _ptr = 0; _state = Empty; }
  };

  bddword _amount;
  bddword _tombstone;
  bddword _hashSize;
  ZDD_Entry* _wheel;

  ZDD_Entry* GetEntry(const ZDD&) const;
  void Rehash(bddword newSize);
public:
  ZDD_Hash(void);
  ~ZDD_Hash(void);

  /* _wheel is owned through a raw pointer, so the implicitly generated copy
     would make two owners of it and double-free.  Copies are therefore not
     part of the contract. */
#if __cplusplus >= 201103L
  ZDD_Hash(const ZDD_Hash&) = delete;
  ZDD_Hash& operator=(const ZDD_Hash&) = delete;
#else
private:
  ZDD_Hash(const ZDD_Hash&);
  ZDD_Hash& operator=(const ZDD_Hash&);
public:
#endif

  void Clear(void);
  void Enter(const ZDD&, void *);
  void* Refer(const ZDD&) const;
  bddword Amount(void) const;
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
