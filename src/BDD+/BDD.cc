 /***************************************
 * BDD+ Manipulator (SAPPORO-1.87)      *
 * (Basic methods)                      *
 * (C) Shin-ichi MINATO (May 14, 2021)  *
 ****************************************/

#include <cstdlib>
#include <memory>
#include <new>

#include "BDDException.h"
#include "BDD.h"
#include "bddplus_internal.h"

using std::cout;
using std::cerr;

namespace sapporobdd {

static const unsigned char BC_Smooth = 60;
static const unsigned char BC_Spread = 61;

//----- External constant data for BDD -------

const bddword BDD_MaxNode = B_VAL_MASK >> 1U;
/* The C++ interface passes variable IDs as int - BDD_NewVar(), BDDvar(),
   ZDD::Change() and the rest - so INT_MAX is the largest ID this layer can
   name.  In B_EXTEND the C core's bddvarmax is close to 2^32, and the plain
   "= bddvarmax" was an out-of-range conversion that made the advertised
   maximum come out as -2: every "v <= BDD_MaxVar" check then rejected all
   real variables. */
const int BDD_MaxVar =
  ((unsigned long long)bddvarmax > (unsigned long long)INT_MAX)?
    INT_MAX: (int)bddvarmax;

//--- Automatic initialization of the manager ----

int BDDV_Active = 0;

/* Zero-initialized before any constructor runs, so the first BDD_InitGuard to
   be constructed - in whichever translation unit that happens to be - is the
   one that initializes the manager.  See the comment in BDD.h. */
static int BDD_InitGuardCount = 0;

BDD_InitGuard::BDD_InitGuard(void)
{
  if(BDD_InitGuardCount++ == 0) BDD_Init();
}

//-------------- class BDD --------------------

bddword BDD::Size() const { return bddsize(_bdd); }

void BDD::Export(FILE *strm) const 
{
  /* an error BDD used to be written out as an empty file, which re-imported
     as the constant 0: the error became a normal value for good */
  if(_bdd == bddnull)
    BDDerr("BDD::Export: Cannot export the error BDD.", ExceptionType::InvalidBDDValue);
  bddword p = _bdd;
  bddexport(strm, &p, 1);
}

void BDD::Print() const
{
  cout << "[ " << GetID();
  cout << " Var:" << Top() << "(" << BDD_LevOfVar(Top()) << ")";
  cout << " Size:" << Size() << " ]\n";
  cout.flush();
}

BDD BDD::Swap(const int& v1, const int& v2) const
{
  /* validate before the early returns, so that v1 == v2 or a constant
     operand does not silently skip the check */
  if(v1 < 1 || v1 > BDD_VarUsed())
    BDDerr("BDD::Swap: Invalid VarID.", (bddword)v1, ExceptionType::OutOfRange);
  if(v2 < 1 || v2 > BDD_VarUsed())
    BDDerr("BDD::Swap: Invalid VarID.", (bddword)v2, ExceptionType::OutOfRange);
  if(v1 == v2) return *this;
  BDD x = BDDvar(v1);
  BDD y = BDDvar(v2);
  BDD fx0 = At0(v1);
  BDD fx1 = At1(v1);
  return (x & ((~y & fx0.At1(v2) )|(y & fx1.At1(v2) )))|
        (~x & ((~y & fx0.At0(v2) )|(y & fx1.At0(v2) )));
}

#define BDD_CACHE_CHK_RETURN(op, fx, gx) \
  { BDD h = BDD_CacheBDD(op, fx, gx); \
    if(h != -1) return h; \
    BDD_RECUR_INC; }

#define BDD_CACHE_ENT_RETURN(op, fx, gx, h) \
  { BDD_RECUR_DEC; \
    if(h != -1) BDD_CacheEnt(op, fx, gx, h.GetID()); \
    return h; }

BDD BDD::Smooth(const int& v) const
{
  /* validate before the early returns, so that a constant or error operand
     does not silently skip the check (the error used to be reported later,
     under bddprime's name, and only for non-constant operands) */
  if(v < 1 || v > BDD_VarUsed())
    BDDerr("BDD::Smooth: Invalid VarID.", (bddword)v, ExceptionType::OutOfRange);
  int t = Top();
  if(t == 0) return *this;
  if(BDD_LevOfVar(t) <= BDD_LevOfVar(v)) return 1;
  bddword fx = GetID();
  bddword gx = BDDvar(v).GetID();
  BDD_CACHE_CHK_RETURN(BC_Smooth, fx, gx);
  BDD x = BDDvar(t);
  BDD h = (~x & At0(t).Smooth(v))|(x & At1(t).Smooth(v));
  BDD_CACHE_ENT_RETURN(BC_Smooth, fx, gx, h);
}

BDD BDD::Spread(const int& k) const
{
  /* Validate before the early returns, so that a constant or error operand
     does not silently skip the checks.  k doubles as a variable ID that keys
     the operation cache, so a k above the number of variables in use used to
     be reported by bddprime() under its own name instead of this
     function's. */
  if(k < 0) BDDerr("BDD::Spread: k < 0.", k, ExceptionType::OutOfRange);
  if(k > BDD_TopLev())
    BDDerr("BDD::Spread: k > BDD_TopLev().", k, ExceptionType::OutOfRange);
  int t = Top();
  if(t == 0) return *this;
  if(k == 0) return *this;
  bddword fx = GetID();
  bddword gx = BDDvar(k).GetID();
  BDD_CACHE_CHK_RETURN(BC_Spread, fx, gx);
  BDD x = BDDvar(t);
  BDD f0 = At0(t);
  BDD f1 = At1(t);
  BDD h = (~x & f0.Spread(k)) | (x & f1.Spread(k))
    | (~x & f1.Spread(k-1)) | (x & f0.Spread(k-1));
  BDD_CACHE_ENT_RETURN(BC_Spread, fx, gx, h);
}

//----- External functions for BDD -------

int BDD_Init(bddword init, bddword limit, double cacheRatio)
{
  /* bddinit() reports failure by exception (BDDOutOfMemoryException), so the
     old "if(bddinit(...)) return 1;" branch was unreachable and callers that
     tested the return value could never see a failure.  The return type is
     kept for source compatibility; the answer is always 0. */
  bddinit(init, limit, cacheRatio);
  BDDV_Active = 0;
  return 0;
}
	
int BDD_NewVarOfLev(int lev)
{
  /* a negative lev used to fall through to the C layer, where it turned into
     a huge unsigned bddvar; the level check there caught it, but its
     ++VarUsed side effect survived the throw and left a ghost variable */
  if(lev < 1 || lev > BDD_TopLev() + 1)
    BDDerr("BDD_NewVarOfLev:Invalid lev ", (bddword)lev, ExceptionType::OutOfRange);
  return bddnewvaroflev(lev);
}

int BDD_VarUsed(void) { return bddvarused(); }

bddword BDD_Used(void) { return bddused(); }

void BDD_GC() { bddgc(); }

void BDD_SetCacheRatio(double ratio) { bddsetcacheratio(ratio); }

double BDD_GetCacheRatio(void) { return bddgetcacheratio(); }

void BDD_SetGCThreshold(bddword threshold) { bddsetgcthreshold(threshold); }

bddword BDD_GetGCThreshold(void) { return bddgetgcthreshold(); }

BDD BDD_Import(FILE *strm)
{
  bddword bdd;
  // bddimport throws an exception on failure
  if (bddimport(strm, &bdd, 1)) {
    throw BDDFileFormatException("BDD_Import: Failed to import BDD from file.", 0);
  }
  /* a file declaring "_o 0" makes import() succeed without writing an
     output; the bddnull it leaves would escape as an error BDD returned
     without an exception, unlike every other failure of this function */
  if (bdd == bddnull) {
    throw BDDFileFormatException("BDD_Import: No output in file.", 0);
  }
  return BDD_ID(bdd);
}

BDD BDD_Random(int level, int density)
{
  if(level < 0)
    BDDerr("BDD_Random: level < 0.", level, ExceptionType::OutOfRange);
  /* in a BDDV environment the levels above BDD_TopLev() belong to the
     partitioning system variables; a level up there used to slip past
     bddvaroflev's check and return a random function over system variables */
  if(level > BDD_TopLev())
    BDDerr("BDD_Random: level > BDD_TopLev().", level, ExceptionType::OutOfRange);
  if(density < 0 || density > 100)
    BDDerr("BDD_Random: Invalid density.", density, ExceptionType::OutOfRange);
  if(level == 0) return ((std::rand()%100) < density)? 1: 0;
  /* As ZDD_Random(): the recursion reaches level 0 before it produces
     anything, so the machine stack goes "level" frames deep whatever the
     density is, and only the limitter keeps a large level from crashing the
     process instead of reporting the limit. */
  BDD_RECUR_INC;
  BDD h = (BDDvar(BDD_VarOfLev(level))
         & BDD_Random(level-1, density)) |
          (~BDDvar(BDD_VarOfLev(level))
         & BDD_Random(level-1, density));
  BDD_RECUR_DEC;
  return h;
}

void BDDerr(const char* msg, ExceptionType exType)
{
  /* As in the C core's err(): the exception unwinds every BDD+ recursion
     frame without passing their BDD_RECUR_DEC, and by the time it reaches
     the user no library frame is left, so the correct depth is 0. */
  BDD_RecurCount = 0;
  switch (exType) {
    case ExceptionType::InvalidBDDValue:
      throw BDDInvalidBDDValueException(msg, 0);
    case ExceptionType::OutOfRange:
      throw BDDOutOfRangeException(msg, 0);
    case ExceptionType::OutOfMemory:
      throw BDDOutOfMemoryException(msg, 0);
    case ExceptionType::FileFormat:
      throw BDDFileFormatException(msg, 0);
    case ExceptionType::InternalError:
    default:
      throw BDDInternalErrorException(msg);
  }
}

void BDDerr(const char* msg, bddword key, ExceptionType exType)
{
  BDD_RecurCount = 0;
  switch (exType) {
    case ExceptionType::InvalidBDDValue:
      throw BDDInvalidBDDValueException(msg, key);
    case ExceptionType::OutOfRange:
      throw BDDOutOfRangeException(msg, key);
    case ExceptionType::OutOfMemory:
      throw BDDOutOfMemoryException(msg, key);
    case ExceptionType::FileFormat:
      throw BDDFileFormatException(msg, key);
    case ExceptionType::InternalError:
    default:
      throw BDDInternalErrorException(msg, key);
  }
}

void BDDerr(const char* msg, const char* name, ExceptionType exType)
{
  std::string message(msg);
  message += " (";
  message += name;
  message += ")";

  BDDerr(message.c_str(), static_cast<bddword>(0), exType);
}


//----- External constant data for BDDV -------

const int BDDV_SysVarTop = 20;
const int BDDV_MaxLen = 1 << BDDV_SysVarTop;
const int BDDV_MaxLenImport = 1000;


//--------------- class BDDV ------------------------

BDDV::BDDV(const BDD& f, int len)
{
  if(len < 0) BDDerr("BDDV::BDDV: len < 0.", len, ExceptionType::OutOfRange);
  if(len > BDDV_MaxLen) BDDerr("BDDV::BDDV: Too large len.", len, ExceptionType::OutOfRange);
  /* a vector longer than 1 is indexed through the partitioning system
     variables that only BDDV_Init() creates; without them the vector
     operations would consume the user's variables 1, 2, ... as partition
     bits and silently corrupt every component */
  if(len > 1 && !BDDV_Active)
    BDDerr("BDDV::BDDV: BDDV_Init() has not been run.", len, ExceptionType::InternalError);
  if(f == -1)
  {
    /* An error vector carries no length of its own, so normalize it to what
       BDDV(-1) builds.  Forcing _len to 1 while leaving _lev = GetLev(len)
       from the requested length broke the invariant that _len <= 1<<_lev with
       _lev minimal, and for len == 0 the assignment below even dropped the
       error and stored the constant 0 instead. */
    _bdd = f;
    _len = 1;
    _lev = 0;
    return;
  }
  int t = f.Top();
  if(t > 0 && BDD_LevOfVar(t) > BDD_TopLev())
    BDDerr("BDDV::BDDV: Invalid Top Var.", t, ExceptionType::InvalidBDDValue);
  _bdd = (len == 0)? 0: f;
  _len = len;
  _lev = GetLev(len);
}

BDDV BDDV::operator<<(int shift) const
{
  if(!Uniform()) return (Former() << shift) || (Latter() << shift);
  BDDV hv;
  if((hv._bdd = _bdd << shift) == -1) 
    BDDerr("BDDV::operator<<: Operation failed.", ExceptionType::OutOfMemory);
  hv._len = _len;
  hv._lev = _lev;
  return hv;
}

BDDV BDDV::operator>>(int shift) const
{
  if(!Uniform()) return (Former() >> shift) || (Latter() >> shift);
  BDDV hv;
  if((hv._bdd = _bdd >> shift) == -1) 
    BDDerr("BDDV::operator>>: Operation failed.", ExceptionType::OutOfMemory);
  hv._len = _len;
  hv._lev = _lev;
  return hv;
}

BDDV BDDV::Cofact(const BDDV& fv) const
{
  if(_lev > 0)
    return Former().Cofact(fv.Former()) || Latter().Cofact(fv.Latter());
  BDDV hv;
  if((hv._bdd = _bdd.Cofact(fv._bdd)) == -1) 
    BDDerr("BDDV::Cofact: Operation failed.", ExceptionType::OutOfMemory);
  if(_len != fv._len) BDDerr("BDDV::Cofact: Length mismatch.", ExceptionType::OutOfRange);
  hv._len = _len;
  // hv._lev = _lev; (always zero)
  return hv;
}

BDDV BDDV::Swap(int v1, int v2) const
{
  if(BDD_LevOfVar(v1) > BDD_TopLev())
    BDDerr("BDDV::Swap: Invalid VarID.", v1, ExceptionType::OutOfRange);
  if(BDD_LevOfVar(v2) > BDD_TopLev())
    BDDerr("BDDV::Swap: Invalid VarID.", v2, ExceptionType::OutOfRange);
  BDDV hv;
  if((hv._bdd = _bdd.Swap(v1, v2)) == -1) 
    BDDerr("BDDV::Swap: Operation failed.", ExceptionType::OutOfMemory);
  hv._len = _len;
  hv._lev = _lev;
  return hv;
}

int BDDV::Top() const
{
  if(Uniform()) return _bdd.Top();
  int t0 = Former().Top();
  int t1 = Latter().Top();
  if(BDD_LevOfVar(t0) > BDD_LevOfVar(t1)) return t0;
  else return t1;
}

bddword BDDV::Size() const
{
  /* bddvsize() stops at the first bddnull in the array, so a component that
     failed with -1 used to truncate the count silently; and a plain new
     would throw std::bad_alloc past every BDDException handler. */
  if(_bdd == -1)
    BDDerr("BDDV::Size: Error vector.", ExceptionType::InvalidBDDValue);
  std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[_len]);
  if(!bddv && _len > 0)
    BDDerr("BDDV::Size: Memory allocation failed.", ExceptionType::OutOfMemory);
  for(int i=0; i<_len; i++)
  {
    BDD f = GetBDD(i);
    if(f == -1)
      BDDerr("BDDV::Size: Operation failed.", ExceptionType::OutOfMemory);
    bddv[i] = f.GetID();
  }
  return bddvsize(bddv.get(), _len);
}

void BDDV::Export(FILE *strm) const
{
  /* as Size(): a -1 component used to make bddexport() write a file with
     silently missing components */
  if(_bdd == -1)
    BDDerr("BDDV::Export: Error vector.", ExceptionType::InvalidBDDValue);
  std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[_len]);
  if(!bddv && _len > 0)
    BDDerr("BDDV::Export: Memory allocation failed.", ExceptionType::OutOfMemory);
  for(int i=0; i<_len; i++)
  {
    BDD f = GetBDD(i);
    if(f == -1)
      BDDerr("BDDV::Export: Operation failed.", ExceptionType::OutOfMemory);
    bddv[i] = f.GetID();
  }
  bddexport(strm, bddv.get(), _len);
}

BDDV BDDV::Spread(int k) const
{
  /* The uniform result has to be rebuilt with the original length: returning
     the BDD alone went through BDDV(const BDD&), which fixes the length at 1,
     so a uniform vector of length 4 came back as length 1 and the callers that
     concatenate the result silently lost the rest of the vector.  operator<<
     and operator>> keep _len over the same branch. */
  if(Uniform()) return BDDV(_bdd.Spread(k), _len);
  return Former().Spread(k) || Latter().Spread(k);
}

BDDV BDDV::Part(int start, int len) const
{
  if(_bdd == -1) return *this;
  if(len == 0) return BDDV();

  if(start < 0 || start + len  > _len)
    BDDerr("BDDV::Part: Illegal index.", ExceptionType::OutOfRange);
  
  if(start == 0 && len == _len) return *this;
  
  int half = 1 << (_lev-1);
  
  if(start + len <= half) return Former().Part(start, len);
  if(start >= half) return Latter().Part(start - half, len);
  return Former().Part(start, half - start)
      || Latter().Part(0, start + len - half);
}

BDD BDDV::GetBDD(int index) const
{
  if(index < 0 || index >= _len)
    BDDerr("BDDV::GetBDD: Illegal index.", index, ExceptionType::OutOfRange);
  if(_len == 1) return _bdd;
  BDD f = _bdd;
  for(int i=_lev-1; i>=0; i--)
    if((index & (1<<i)) == 0) f = f.At0(i + 1);
    else f = f.At1(i + 1);
  return f;
}

void BDDV::Print() const
{
  for(int i=0; i<_len; i++)
  {
    cout << "f" << i << ": ";
    GetBDD(i).Print();
  }
  cout << "Size= " << Size() << "\n\n";
  cout.flush();
}

//----- External functions for BDD Vector -------

int BDDV_Init(bddword init, bddword limit)
{
  /* as BDD_Init(): bddinit() reports failure by exception, never by value */
  bddinit(init, limit);
  for(int i=0; i<BDDV_SysVarTop; i++) bddnewvar();
  BDDV_Active = 1;
  return 0;
}
	
BDDV operator||(const BDDV& fv, const BDDV& gv)
{
  if(fv._len == 0) return gv;
  if(gv._len == 0) return fv;
  if(fv._len != (1 << fv._lev))
    return BDDV(fv).Former() || (BDDV(fv).Latter() || gv);
  if(fv._len < gv._len)
    return (fv || BDDV(gv).Former()) || BDDV(gv).Latter();
  /* the concatenation is encoded on a partitioning system variable, which
     only exists after BDDV_Init(); without it BDDvar(fv._lev + 1) would
     consume a user variable and silently corrupt the vector */
  if(!BDDV_Active)
    BDDerr("BDDV::operator||: BDDV_Init() has not been run.", ExceptionType::InternalError);
  BDDV hv;
  BDD x = BDDvar(fv._lev + 1);
  if((hv._bdd = (~x & fv._bdd)|(x & gv._bdd)) == -1) 
    BDDerr("BDDV::operator||: Operation failed.", ExceptionType::OutOfMemory);
  if((hv._len = fv._len + gv._len) > BDDV_MaxLen)
    BDDerr("BDDV::operator||: Too large len.", hv._len, ExceptionType::OutOfRange);
  hv._lev = fv._lev + 1;
  return hv;
}

BDDV BDDV_Mask1(int index, int len)
{
  if(len < 0) BDDerr("BDDV_Mask1: len < 0.", len, ExceptionType::OutOfRange);
  if(index < 0 || index >= len)
    BDDerr("BDDV_Mask1: Illegal index.", index, ExceptionType::OutOfRange);
  return BDDV(0,index)||BDDV(1,1)||BDDV(0,len-index-1);
}

BDDV BDDV_Mask2(int index, int len)
{
  if(len < 0) BDDerr("BDDV_Mask2: len < 0.", len, ExceptionType::OutOfRange);
  if(index < 0 || index > len)
    BDDerr("BDDV_Mask2: Illegal index.", index, ExceptionType::OutOfRange);
  return BDDV(0,index)||BDDV(1,len-index);
}

#define IMPORTHASH(x) ((((x)>>1)^((x)<<8)^((x)<<16)) & (hashsize-1))

/* Bounds accepted for the counts in the header of an imported file.  Anything
   above them describes a corrupt file rather than a huge one: no more levels
   than the manager can hold variables, no vector longer than BDDV_MaxLen, and
   no more nodes than the node table can address.  Rejecting them early is also
   what keeps the n_nd<<1 below from wrapping around. */
static const unsigned long long ImportMaxLev =
  ((unsigned long long)bddvarmax < (unsigned long long)INT_MAX)?
    (unsigned long long)bddvarmax: (unsigned long long)INT_MAX;
static const unsigned long long ImportMaxLen = (unsigned long long)BDDV_MaxLen;
static const unsigned long long ImportMaxNode = (unsigned long long)BDD_MaxNode;

#ifdef B_32
#  define B_STRTOI strtol
#else
#  define B_STRTOI strtoll
#endif

BDDV BDDV_Import(FILE *strm)
{
  int inv, e;
  bddword hashsize;
  BDD f, f0, f1;
  std::string s;
  unsigned long long uval;
  std::unique_ptr<bddword[]> hash1;
  std::unique_ptr<BDD[]> hash2;

  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(s != "_i") 
    BDDerr("BDDV_Import: Invalid format, expected '_i'.", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxLev, uval))
    BDDerr("BDDV_Import: Invalid number of levels.", ExceptionType::FileFormat);
  int n = (int)uval;
  while(n > BDD_TopLev()) BDD_NewVar();

  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(s != "_o") 
    BDDerr("BDDV_Import: Invalid format, expected '_o'.", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxLen, uval))
    BDDerr("BDDV_Import: Invalid vector length.", ExceptionType::FileFormat);
  int m = (int)uval;

  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(s != "_n") 
    BDDerr("BDDV_Import: Invalid format, expected '_n'.", ExceptionType::FileFormat);
  if(ReadToken(strm, s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(ReadDecimal(s, ImportMaxNode, uval))
    BDDerr("BDDV_Import: Invalid number of nodes.", ExceptionType::FileFormat);
  bddword n_nd = (bddword)uval;

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  /* A plain new would throw std::bad_alloc, which is not a BDDException and
     would escape every handler this library asks its users to write; nothrow
     keeps the failure on the library's own error path.  Holding the tables in
     unique_ptr also releases them when one of the format errors below throws,
     which the explicit delete[] calls could not do. */
  hash1.reset(new(std::nothrow) bddword[hashsize]);
  if(!hash1)
    BDDerr("BDDV_Import: Failed to allocate memory for hash1.", ExceptionType::OutOfMemory);
  hash2.reset(new(std::nothrow) BDD[hashsize]);
  if(!hash2)
    BDDerr("BDDV_Import: Failed to allocate memory for hash2.", ExceptionType::OutOfMemory);
  for(bddword ix=0; ix<hashsize; ix++)
  {
    hash1[ix] = B_VAL_MASK;
    hash2[ix] = 0;
  }

  e = 0;
  for(bddword ix=0; ix<n_nd; ix++)
  {
    /* The node IDs are read with the same validation as the header counts:
       B_STRTOI turned a corrupt token into 0, and node ID 0 is a real node
       (the table starts at index 0), so a damaged file could resolve the
       junk into a silent reference to that node instead of being refused. */
    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(ReadDecimal(s, (unsigned long long)B_VAL_MASK, uval)) { e = 1; break; }
    bddword nd = (bddword)uval;
    
    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    /* A level the file made up would make bddvaroflev() throw an out-of-range
       error of its own; report it as the file format error that it is. */
    if(ReadDecimal(s, (unsigned long long)BDD_TopLev(), uval) || uval == 0)
      { e = 1; break; }
    int var = bddvaroflev((bddvar)uval);

    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(s == "F") f0 = 0;
    else if(s == "T") f0 = 1;
    else
    {
      if(ReadDecimal(s, (unsigned long long)B_VAL_MASK, uval)) { e = 1; break; }
      bddword nd0 = (bddword)uval;

      bddword ixx = IMPORTHASH(nd0);
      while(hash1[ixx] != nd0)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("BDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f0 = hash2[ixx];
    }

    if(ReadToken(strm, s) == EOF) { e = 1; break; }
    if(s == "F") f1 = 0;
    else if(s == "T") f1 = 1;
    else
    {
      if(ReadDecimal(s, (unsigned long long)B_VAL_MASK, uval)) { e = 1; break; }
      bddword nd1 = (bddword)uval;
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd1);
      while(hash1[ixx] != nd1)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("BDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f1 = (inv)? ~hash2[ixx]: hash2[ixx];
    }

    BDD x = BDDvar(var);
    f = (x & f1) | (~x & f0);
    /* e = 2: memory overflow, not a defect of the file */
    if(f == -1) { 
      e = 2; 
      break; 
    }

    bddword ixx = IMPORTHASH(nd);
    while(hash1[ixx] != B_VAL_MASK)
    {
      if(hash1[ixx] == nd)
        BDDerr("BDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  if(e == 2)
    BDDerr("BDDV_Import: Memory overflow.", ExceptionType::OutOfMemory);
  if(e)
    BDDerr("BDDV_Import: Error during node processing.", ExceptionType::FileFormat);

  BDDV v = BDDV();
  for(int i=0; i<m; i++)
  {
    if(ReadToken(strm, s) == EOF)
      BDDerr("BDDV_Import: Unexpected end of file during vector processing.", ExceptionType::FileFormat);
    if(s == "F") v = v || BDD(0);
    else if(s == "T") v = v || BDD(1);
    else
    {
      if(ReadDecimal(s, (unsigned long long)B_VAL_MASK, uval))
        BDDerr("BDDV_Import: Invalid node ID.", ExceptionType::FileFormat);
      bddword nd = (bddword)uval;
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;
  
      bddword ixx = IMPORTHASH(nd);
      while(hash1[ixx] != nd)
      {
        if(hash1[ixx] == B_VAL_MASK)
          BDDerr("BDDV_Import(): internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      v = v || (inv? ~hash2[ixx]: hash2[ixx]);
    }
  }

  return v;
}

BDDV BDDV_ImportPla(FILE *strm, int sopf)
{
  std::string s;
  unsigned long long uval;
  int n = 0;
  int m = 0;
  int mode = 1; // 0:f 1:fd 2:fr 3:fdr

  /* A '#' starts a comment that runs to the end of its line.  Skipping just
     the '#' token, as this used to, made every free-text comment
     ("# comments like this") derail the parse at its second word. */
  do
  {
    if(ReadToken(strm, s) == EOF) 
      BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
    if(s[0] == '#') SkipLine(strm);
  } while(s[0] == '#');

  // declaration part 
  while(s[0] == '.')
  {
    if(s == ".i")
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      /* Levels up to 2*n are used below, so half the level bound is the
         largest input count that can be honoured. */
      if(ReadDecimal(s, ImportMaxLev / 2ULL, uval))
        BDDerr("BDDV_ImportPla: Error in input size.", ExceptionType::FileFormat);
      n = (int)uval;
    }
    else if(s == ".o")
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(ReadDecimal(s, ImportMaxLen, uval))
        BDDerr("BDDV_ImportPla: Error in output size.", ExceptionType::FileFormat);
      m = (int)uval;
    }
    else if(s == ".type")
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(s == "f") mode = 0;
      else if(s == "fd") mode = 1;
      else if(s == "fr") mode = 2;
      else if(s == "fdr") mode = 3;
      /* a mistyped type used to be processed silently as the default fd */
      else BDDerr("BDDV_ImportPla: Unknown .type value.", s.c_str(), ExceptionType::FileFormat);
    }
    else 
    {
      /* an unknown directive such as .ilb or .ob takes an arbitrary number
         of arguments; reading exactly one, as this used to, left the rest to
         be misparsed as product terms */
      SkipLine(strm);
    }
    do
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(s[0] == '#') SkipLine(strm);
    } while(s[0] == '#');
  }
  
  if(m <= 0) 
    BDDerr("BDDV_ImportPla: Error in output size.", ExceptionType::FileFormat);
  /* Levels 1..2n are laid out whether or not sopf is set, so that the same
     file read with either flag sees the same variable arrangement; with
     sopf == 0 only levels 1..n are used and the rest stay as spares. */
  while(BDD_TopLev() < n*2) BDD_NewVar();
  BDDV onset = BDDV(0, m);
  BDDV offset = BDDV(0, m);
  BDDV dcset = BDDV(0, m);
  BDD term;

  // logic description part
  while(s[0] != '.')
  {
    if((int)s.size() != n)
      BDDerr("BDDV_ImportPla: Error at product term.", ExceptionType::FileFormat);
    term = 1;
    for(int i=0; i<n; i++)
    {
      switch(s[i])
      {
      case '0':
        term &= ~BDDvar(BDD_VarOfLev(sopf? 2*i+2: i+1));
	break;
      case '1':
        term &= BDDvar(BDD_VarOfLev(sopf? 2*i+2: i+1));
	break;
      case '-':
	break;
      default:
        BDDerr("BDDV_ImportPla: Error at product term.", ExceptionType::FileFormat);
      }
    }
    do
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(s[0] == '#') SkipLine(strm);
    } while(s[0] == '#');
    if((int)s.size() != m) 
      BDDerr("BDDV_ImportPla: Error at output symbol.", ExceptionType::FileFormat);
    for(int i=0; i<m; i++)
    {
      BDDV tv = BDDV(term, m) & BDDV_Mask1(i, m);
      switch(s[i])
      {
      case '0':
        offset |= tv;
        break;
      case '1':
        onset |= tv;
	break;
      case '-':
        dcset |= tv;
	break;
      case '~':
	break;
      default:
        BDDerr("BDDV_ImportPla: Error at output symbol.", ExceptionType::FileFormat);
      }
    }
    do
    {
      if(ReadToken(strm, s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(s[0] == '#') SkipLine(strm);
    } while(s[0] == '#');
  }

  // final part
  switch(mode)
  {
  case 0:
    offset = ~onset;
    dcset = BDDV(0, m);
    break;
  case 1:
    onset &= ~dcset;
    offset = ~(onset | dcset);
    break;
  case 2:
    if((onset & offset) != BDDV(0, m)) 
      BDDerr("BDDV_ImportPla: Overlapping onset & offset.", ExceptionType::FileFormat);
    dcset = ~(onset | offset);
    break;
  case 3:
    if((onset & offset) != BDDV(0, m)) 
      BDDerr("BDDV_ImportPla: Overlapping onset & offset.", ExceptionType::FileFormat);
    if((onset & dcset) != BDDV(0, m))
      BDDerr("BDDV_ImportPla: Overlapping onset & dcset.", ExceptionType::FileFormat);
    if((offset & dcset) != BDDV(0, m))
      BDDerr("BDDV_ImportPla: Overlapping offset & dcset.", ExceptionType::FileFormat);
    if((onset | offset | dcset) != BDDV(1, m))
      BDDerr("BDDV_ImportPla: Not covering function.", ExceptionType::FileFormat);
    break;
  }
  return (onset || dcset);
}

} // namespace sapporobdd
