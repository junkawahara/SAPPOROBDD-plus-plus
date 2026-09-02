/********************************************
 * BDD+ Manipulator (SAPPORO-1.93) - Header *
 * (C) Shin-ichi MINATO  (Dec. 6, 2021)     *
 ********************************************/

/* The include guard used to be _BDD_, and an identifier that starts with an
   underscore followed by a capital is reserved to the implementation. */
#ifndef SAPPOROBDD_BDD_H
#define SAPPOROBDD_BDD_H

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>

#define BDD_CPP
#include "bddc.h"
#include "BDDException.h"

namespace sapporobdd {

class BDD;
class BDDV;

//--------- Definition of "bddword" type --------
#ifdef B_32
  typedef unsigned int bddword;
#else
  typedef unsigned long long bddword;
#endif

//--------- External data for BDD ---------
extern const bddword BDD_MaxNode;
extern const int BDD_MaxVar;

//----- External constant data for BDDV ---------
extern int BDDV_Active;
extern const int BDDV_SysVarTop;
extern const int BDDV_MaxLen;
/* Unused: BDDV_Import() bounds the vector length it accepts by BDDV_MaxLen,
   not by this constant.  Kept only so that code referring to it still
   compiles. */
extern const int BDDV_MaxLenImport;

//--------- Stack overflow limiter ---------
/* The BDD+ copy of the two macros of src/BDDc/bddc_internal.h (which this
   header cannot include): the same counter and limit, reported through
   BDDerr() instead of the core's err().  The two copies have to be changed
   together. */
#define BDD_RECUR_INC \
  do { if(++BDD_RecurCount >= BDD_RecurLimit) \
    BDDerr("BDD_RECUR_INC:Stack overflow ", (bddword) BDD_RecurCount, \
           ExceptionType::InternalError); } while(0)
#define BDD_RECUR_DEC BDD_RecurCount--

class BDD
{
  bddword _bdd;

public:
  BDD(void) { _bdd = bddfalse; }
  BDD(int a) { _bdd = (a==0)? bddfalse:(a>0)? bddtrue:bddnull; }
  BDD(const BDD& f) { _bdd = bddcopy(f._bdd); }

  /* bddfree() throws for an invalid bddp, which this can hold when
     BDD_Init() was called again while the object was alive:
     re-initialization invalidates every earlier bddp.  A destructor is
     noexcept, so letting that exception out would terminate the process for
     an object that merely goes out of scope; there is nothing left to
     release then, so swallow it. */
  ~BDD(void) { try { bddfree(_bdd); } catch(...) { } }

  BDD& operator=(const BDD& f) { 
    if(_bdd != f._bdd) {
      /* copy before free: bddcopy() throws when f holds an invalid bddp,
         and freeing first would leave _bdd already released for the
         destructor to release again */
      bddword t = bddcopy(f._bdd);
      bddfree(_bdd);
      _bdd = t;
    }
    return *this; 
  }

  BDD& operator&=(const BDD& f)
    { BDD h; h._bdd = bddand(_bdd, f._bdd); return *this = h; }
  BDD& operator|=(const BDD& f)
    { BDD h; h._bdd = bddor(_bdd, f._bdd); return *this = h; }
  BDD& operator^=(const BDD& f)
    { BDD h; h._bdd = bddxor(_bdd, f._bdd); return *this = h; }
  BDD& operator<<=(const int s)
    { BDD h; h._bdd = bddlshift(_bdd, s); return *this = h; }
  BDD& operator>>=(const int s)
    { BDD h; h._bdd = bddrshift(_bdd, s); return *this = h; }

  BDD operator~(void) const { BDD h; h._bdd = bddnot(_bdd); return h; }
  BDD operator<<(int s) const
    { BDD h; h._bdd = bddlshift(_bdd, s); return h; }
  BDD operator>>(int s) const
    { BDD h; h._bdd = bddrshift(_bdd, s); return h; }

  int Top(void) const { return bddtop(_bdd); }
  BDD At0(int v) const { BDD h; h._bdd = bddat0(_bdd, v); return h; }
  BDD At1(int v) const { BDD h; h._bdd = bddat1(_bdd, v); return h; }
  BDD Cofact(const BDD& f) const
    { BDD h; h._bdd = bddcofactor(_bdd, f._bdd); return h; }
  BDD Univ(const BDD& f) const
    { BDD h; h._bdd = bdduniv(_bdd, f._bdd); return h; }
  BDD Exist(const BDD& f) const
    { BDD h; h._bdd = bddexist(_bdd, f._bdd); return h; }
  BDD Support(void) const
    { BDD h; h._bdd = bddsupport(_bdd); return h; }

  bddword GetID(void) const {return _bdd; }
   
  bddword Size(void) const;
  void Export(FILE *strm = stdout) const;
  void Print(void) const;
  void XPrint0(void) const;
  void XPrint(void) const;

  BDD Swap(const int&, const int&) const;
  BDD Smooth(const int&) const;
  BDD Spread(const int&) const;

  friend BDD BDD_ID(bddword);
};

//--------- External functions for BDD ---------
extern int     BDD_Init(bddword init=256, bddword limit=BDD_MaxNode, double cacheRatio=0.5);
extern int     BDD_NewVarOfLev(int);
extern int     BDD_VarUsed(void);
extern bddword BDD_Used(void);
extern void    BDD_GC(void);
extern void    BDD_SetCacheRatio(double ratio);
extern double  BDD_GetCacheRatio(void);
extern void    BDD_SetGCThreshold(bddword threshold);
extern bddword BDD_GetGCThreshold(void);
extern BDD BDD_Import(FILE *strm = stdin);
/* Draws a random function of the top "level" variables by deciding every row
   of its truth table independently (true with probability density%), so it
   costs 2^level calls to std::rand() and takes time exponential in its first
   argument.  Sampling the whole truth table is what the result is defined to
   be, so keep the level small. */
extern BDD BDD_Random(int, int density = 50);
extern void BDDerr(const char *, ExceptionType);
extern void BDDerr(const char *, bddword, ExceptionType);
extern void BDDerr(const char *, const char *, ExceptionType);

//--------- Automatic initialization of the manager ---------
/* BDD_Init() has to have run before the first BDD operation, including one
   issued from the constructor of a static object.  The order in which static
   objects of different translation units are constructed is unspecified, so a
   single manager object defined in BDD.cc would not be guaranteed to come
   first.  Instead every translation unit that includes this header defines its
   own guard below, and the one constructed first initializes the manager (the
   counter is zero-initialized before any constructor runs, so the count is
   reliable).  Within a translation unit the guard is constructed before every
   static object declared after this #include, which is the ordering the
   language does guarantee.  Calling BDD_Init() explicitly afterwards still
   re-initializes the manager as before. */
class BDD_InitGuard
{
public:
  BDD_InitGuard(void);
};
static BDD_InitGuard BDD_InitGuardInstance;

//--------- Inline functions for BDD ---------
inline int BDD_TopLev(void)
  { return BDDV_Active? bddvarused() - BDDV_SysVarTop: bddvarused(); }

inline int BDD_NewVar(void)
  { return bddnewvaroflev(BDD_TopLev() + 1); }

inline int BDD_LevOfVar(int v) { return bddlevofvar(v); }
inline int BDD_VarOfLev(int lev) { return bddvaroflev(lev); }

/* Wraps a raw ID handed over by the C core WITHOUT taking a new reference:
   the returned BDD assumes ownership of the reference the caller holds.
   Passing an ID that another owner keeps - e.g. BDD_ID(f.GetID()) - makes
   two owners of one reference and corrupts the reference count when both
   are destroyed.  To share a BDD, copy the object; use BDD_ID() only for
   IDs returned by C-layer functions that hand over their reference. */
inline BDD BDD_ID(bddword bdd)
  { BDD h; h._bdd = bdd; return h; }

inline bddword BDD_CacheInt(unsigned char op, bddword fx, bddword gx)
  { return bddrcache(op, fx, gx); }

inline BDD BDD_CacheBDD(unsigned char op, bddword fx, bddword gx)
  { return BDD_ID(bddcopy(bddrcache(op, fx, gx))); }

inline void BDD_CacheEnt(unsigned char op, bddword fx, bddword gx, bddword hx)
  { bddwcache(op, fx, gx, hx); }

inline BDD BDDvar(int v) { return BDD_ID(bddprime(v)); }

inline BDD operator&(const BDD& f, const BDD& g) 
  { return BDD_ID(bddand(f.GetID(), g.GetID())); }

inline BDD operator|(const BDD& f, const BDD& g) 
  { return BDD_ID(bddor(f.GetID(), g.GetID())); }

inline BDD operator^(const BDD& f, const BDD& g) 
  { return BDD_ID(bddxor(f.GetID(), g.GetID())); }

/* ID comparison.  Note that two error values compare equal: when both
   operands hold the -1 of two failed operations, f == g answers 1, so
   check the operands against -1 before comparing computed results. */
inline int operator==(const BDD& f, const BDD& g) 
  { return f.GetID() == g.GetID(); }

inline int operator!=(const BDD& f, const BDD& g) 
  { return f.GetID() != g.GetID(); }

inline int BDD_Imply(const BDD& f, const BDD& g) 
{
  /* an int has no error value, so the -1 of a failed operation would
     silently decay into "does not imply"; bddimply() refuses a null operand
     as well, but this check names the function the user called */
  if(f.GetID() == bddnull || g.GetID() == bddnull)
    BDDerr("BDD_Imply: null operand.", ExceptionType::InvalidBDDValue);
  return bddimply(f.GetID(), g.GetID());
}

class BDDV
{
  BDD _bdd;
  int _len;
  int _lev;

  int GetLev(int len) const {
    int lev = 0;
    for(len--; len>0; len>>=1) lev++;
    return lev;
  }

public:
  BDDV(void) { _bdd = 0; _len = 0; _lev = 0; }

  BDDV(const BDDV& fv)
    { _bdd = fv._bdd; _len = fv._len; _lev = fv._lev; } 

  BDDV(const BDD& f) {
    int t = f.Top();
    if(t > 0 && BDD_LevOfVar(t) > BDD_TopLev())
      BDDerr("BDDV::BDDV: Invalid top var.", t, ExceptionType::InvalidBDDValue);
    _bdd = f;
    _len = 1;
    _lev = 0;
  }

  BDDV(const BDD&, int len);

  ~BDDV(void) { }

  BDDV& operator=(const BDDV& fv)
    { _bdd = fv._bdd; _len = fv._len; _lev = fv._lev; return *this; } 

  BDDV& operator&=(const BDDV&);
  BDDV& operator|=(const BDDV&);
  BDDV& operator^=(const BDDV&);
  BDDV& operator<<=(int);
  BDDV& operator>>=(int);

  BDDV operator~(void) const
    { BDDV h; h._bdd = ~_bdd; h._len = _len; h._lev = _lev; return h; } 
  BDDV operator<<(int) const;
  BDDV operator>>(int) const;

  BDDV At0(int v) const {
    if(v > 0 && BDD_LevOfVar(v) > BDD_TopLev())
      BDDerr("BDDV::At0: Invalid var.", v, ExceptionType::OutOfRange);
    BDDV hv;
    if((hv._bdd = _bdd.At0(v)) == -1) return BDDV(-1);
    hv._len = _len;
    hv._lev = _lev;
    return hv;
  }

  BDDV At1(int v) const {
    if(v > 0 && BDD_LevOfVar(v) > BDD_TopLev())
      BDDerr("BDDV::At1: Invalid var.", v, ExceptionType::OutOfRange);
    BDDV hv;
    if((hv._bdd = _bdd.At1(v)) == -1) return BDDV(-1);
    hv._len = _len;
    hv._lev = _lev;
    return hv;
  }

  BDDV Cofact(const BDDV&) const;
  BDDV Swap(int, int) const;
  BDDV Spread(int) const;

  int Top(void) const;

  bddword Size() const;
  void Export(FILE *strm = stdout) const;

  BDDV Former(void) const {
    /* keep the error mark: the _len <= 1 path below would turn the error
       vector (whose length is 1) into an empty vector and lose the failure,
       while Latter() keeps it -- BDDV::Part() guards the same way */
    if(_bdd == -1) return *this;
    BDDV hv;
    if(_len <= 1) return hv;
    if((hv._bdd = _bdd.At0(_lev)) == -1) return BDDV(-1);
    hv._len = 1 << (_lev - 1);
    hv._lev = _lev - 1;
    return hv;
  }

  BDDV Latter(void) const {
    BDDV hv;
    if(_len == 0) return hv;
    if(_len == 1) return *this;
    if((hv._bdd = _bdd.At1(_lev)) == -1) return BDDV(-1);
    hv._len = _len - (1 << (_lev - 1));
    hv._lev = GetLev(hv._len);
    return hv;
  }

  BDDV Part(int, int) const;
  BDD GetBDD(int) const;

  BDD GetMetaBDD(void) const { return _bdd; }
  int Uniform(void) const
    { return BDD_LevOfVar(_bdd.Top()) <= BDD_TopLev(); }
  int Len(void) const { return _len; }

  void Print() const;
  void XPrint0() const;
  void XPrint() const;

  friend BDDV operator&(const BDDV&, const BDDV&);
  friend BDDV operator|(const BDDV&, const BDDV&);
  friend BDDV operator^(const BDDV&, const BDDV&);
  friend BDDV operator||(const BDDV&, const BDDV&);
};

//----- External functions for BDDV ---------
extern int     BDDV_Init(bddword init=256, bddword limit=BDD_MaxNode);
extern BDDV operator||(const BDDV&, const BDDV&);
extern BDDV BDDV_Mask1(int, int);
extern BDDV BDDV_Mask2(int, int);
extern BDDV BDDV_Import(FILE *strm = stdin);
extern BDDV BDDV_ImportPla(FILE *strm = stdin, int sopf = 0);

//----- Inline functions for BDDV ---------
inline int BDDV_UserTopLev(void) { return BDD_TopLev(); }
inline int BDDV_NewVar(void) { return BDD_NewVar(); }
inline int BDDV_NewVarOfLev(int lev) {return BDD_NewVarOfLev(lev); }

inline BDDV operator&(const BDDV& fv, const BDDV& gv) {
  BDDV hv;
  if((hv._bdd = fv._bdd & gv._bdd) == -1) return BDDV(-1);
  if(fv._len != gv._len) BDDerr("BDDV::operator&: Length mismatch", ExceptionType::OutOfRange);
  hv._len = fv._len;
  hv._lev = fv._lev;
  return hv;
}

inline BDDV operator|(const BDDV& fv, const BDDV& gv) {
  BDDV hv;
  if((hv._bdd = fv._bdd | gv._bdd) == -1) return BDDV(-1);
  if(fv._len != gv._len) BDDerr("BDDV::operator|: Length mismatch", ExceptionType::OutOfRange);
  hv._len = fv._len;
  hv._lev = fv._lev;
  return hv;
}

inline BDDV operator^(const BDDV& fv, const BDDV& gv) {
  BDDV hv;
  if((hv._bdd = fv._bdd ^ gv._bdd) == -1) return BDDV(-1);
  if(fv._len != gv._len) BDDerr("BDDV::operator^: Length mismatch", ExceptionType::OutOfRange);
  hv._len = fv._len;
  hv._lev = fv._lev;
  return hv;
}

inline int operator==(const BDDV& fv, const BDDV& gv)

  { return fv.GetMetaBDD() == gv.GetMetaBDD() && fv.Len() == gv.Len(); }

inline int operator!=(const BDDV& fv, const BDDV& gv)
  { return !(fv == gv); }

inline int BDDV_Imply(const BDDV& fv, const BDDV& gv) {
  return fv.Len() == gv.Len() 
    && BDD_Imply(fv.GetMetaBDD(), gv.GetMetaBDD());
}

inline BDDV& BDDV::operator&=(const BDDV& fv) { return *this = *this & fv; }
inline BDDV& BDDV::operator|=(const BDDV& fv) { return *this = *this | fv; }
inline BDDV& BDDV::operator^=(const BDDV& fv) { return *this = *this ^ fv; }
inline BDDV& BDDV::operator<<=(int s) { return *this = *this << s; }
inline BDDV& BDDV::operator>>=(int s) { return *this = *this >> s; }


/* A hash table mapping BDD keys to void* values, used as a scratch table by
   applications.  Enter(key, p) inserts or updates; Enter(key, 0) deletes the
   key (a null value cannot be stored, since Refer() answers 0 for "not
   found").  Refer(key) returns the stored pointer or 0; Amount() the number
   of live entries.  Clear() releases every entry at once.  An entry keeps a
   reference to its key BDD for as long as it is in the table, so bddgc()
   cannot collect the key's nodes until the entry is deleted or cleared. */
class BDD_Hash
{
  struct BDD_Entry
  {
    enum State { Empty = 0, Occupied = 1, Deleted = 2 };
    BDD _key;
    void* _ptr;
    char _state;
    BDD_Entry(void){ _ptr = 0; _state = Empty; }
  };

  bddword _amount;
  bddword _tombstone;
  bddword _hashSize;
  BDD_Entry* _wheel;

  BDD_Entry* GetEntry(const BDD&) const;
  void Rehash(bddword newSize);
public:
  BDD_Hash(void);
  ~BDD_Hash(void);

  /* _wheel is owned through a raw pointer, so the implicitly generated copy
     would make two owners of it and double-free.  Copies are therefore not
     part of the contract. */
#if __cplusplus >= 201103L
  BDD_Hash(const BDD_Hash&) = delete;
  BDD_Hash& operator=(const BDD_Hash&) = delete;
#else
private:
  BDD_Hash(const BDD_Hash&);
  BDD_Hash& operator=(const BDD_Hash&);
public:
#endif

  void Clear(void);
  void Enter(const BDD&, void *);
  void* Refer(const BDD&) const;
  bddword Amount(void) const;
};

} // namespace sapporobdd

#endif // SAPPOROBDD_BDD_H 
