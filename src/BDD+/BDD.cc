 /***************************************
 * BDD+ Manipulator (SAPPORO-1.87)      *
 * (Basic methods)                      *
 * (C) Shin-ichi MINATO (May 14, 2021)  *
 ****************************************/

#include "BDDException.h"
#include "BDD.h"

using std::cout;
using std::cerr;

namespace sapporobdd {

static const char BC_Smooth = 60;
static const char BC_Spread = 61;

extern "C"
{
  int rand();
};

//----- External constant data for BDD -------

const bddword BDD_MaxNode = B_VAL_MASK >> 1U;
const int BDD_MaxVar = bddvarmax;

//--- SBDD class for default initialization ----

int BDDV_Active = 0;

class SBDD
{
public:
  //SBDD(bddword init, bddword limit) { bddinit(init, limit); }
  SBDD(void) { BDD_Init(); }
};
static SBDD BDD_Manager;

//-------------- class BDD --------------------

bddword BDD::Size() const { return bddsize(_bdd); }

void BDD::Export(FILE *strm) const 
{
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
  int t = Top();
  if(t == 0) return *this;
  if(k == 0) return *this;
  if(k < 0) BDDerr("BDD::Spread: k < 0.", k, ExceptionType::OutOfRange);
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
  if(bddinit(init, limit, cacheRatio)) return 1;
  BDDV_Active = 0;
  return 0;
}
	
int BDD_NewVarOfLev(int lev)
{
  if(lev > BDD_TopLev() + 1)
    BDDerr("BDD_NewVarOfLev:Invald lev ", (bddword)lev, ExceptionType::OutOfRange);
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
  return BDD_ID(bdd);
}

BDD BDD_Random(int level, int density)
{
  if(level < 0)
    BDDerr("BDD_Random: level < 0.", level, ExceptionType::OutOfRange);
  if(level == 0) return ((rand()%100) < density)? 1: 0;
  return (BDDvar(BDD_VarOfLev(level))
        & BDD_Random(level-1, density)) |
         (~BDDvar(BDD_VarOfLev(level))
        & BDD_Random(level-1, density));
}

void BDDerr(const char* msg, ExceptionType exType)
{
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
  int t = f.Top();
  if(t > 0 && BDD_LevOfVar(t) > BDD_TopLev())
    BDDerr("BDDV::BDDV: Invalid Top Var.", t, ExceptionType::InvalidBDDValue);
  _bdd = (len == 0)? 0: f;
  _len = (f == -1)? 1: len;
  _lev = GetLev(len);
}

BDDV BDDV::operator<<(int shift) const
{
  if(!Uniform()) return (Former() << shift) || (Latter() << shift);
  BDDV hv;
  if((hv._bdd = _bdd << shift) == -1) 
    BDDerr("BDDV::operator<<: Operation failed.", ExceptionType::InternalError);
  hv._len = _len;
  hv._lev = _lev;
  return hv;
}

BDDV BDDV::operator>>(int shift) const
{
  if(!Uniform()) return (Former() >> shift) || (Latter() >> shift);
  BDDV hv;
  if((hv._bdd = _bdd >> shift) == -1) 
    BDDerr("BDDV::operator>>: Operation failed.", ExceptionType::InternalError);
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
    BDDerr("BDDV::Cofact: Operation failed.", ExceptionType::InternalError);
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
  bddword* bddv = new bddword[_len];
  for(int i=0; i<_len; i++) bddv[i] = GetBDD(i).GetID(); 
  bddword s = bddvsize(bddv, _len);
  delete[] bddv;
  return s;
}

void BDDV::Export(FILE *strm) const
{
  bddword* bddv = new bddword[_len];
  for(int i=0; i<_len; i++) bddv[i] = GetBDD(i).GetID(); 
  bddexport(strm, bddv, _len);
  delete[] bddv;
}

BDDV BDDV::Spread(int k) const
{
  if(Uniform()) return _bdd.Spread(k);
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
  if(bddinit(init, limit)) return 1;
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
  BDDV hv;
  BDD x = BDDvar(fv._lev + 1);
  if((hv._bdd = (~x & fv._bdd)|(x & gv._bdd)) == -1) 
    BDDerr("BDDV::operator||: Operation failed.", ExceptionType::InternalError);
  if((hv._len = fv._len + gv._len) > BDDV_MaxLen)
    BDDerr("BDDV::operatop||: Too large len.", hv._len, ExceptionType::OutOfRange);
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
  char s[256];
  bddword *hash1 = 0;
  BDD *hash2 = 0;

  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(strcmp(s, "_i") != 0) 
    BDDerr("BDDV_Import: Invalid format, expected '_i'.", ExceptionType::FileFormat);
  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  int n = strtol(s, NULL, 10);
  while(n > BDD_TopLev()) BDD_NewVar();

  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(strcmp(s, "_o") != 0) 
    BDDerr("BDDV_Import: Invalid format, expected '_o'.", ExceptionType::FileFormat);
  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  int m = strtol(s, NULL, 10);

  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  if(strcmp(s, "_n") != 0) 
    BDDerr("BDDV_Import: Invalid format, expected '_n'.", ExceptionType::FileFormat);
  if(fscanf(strm, "%s", s) == EOF) 
    BDDerr("BDDV_Import: Unexpected end of file.", ExceptionType::FileFormat);
  bddword n_nd = B_STRTOI(s, NULL, 10);

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  hash1 = new bddword[hashsize];
  if(hash1 == 0) 
    BDDerr("BDDV_Import: Failed to allocate memory for hash1.", ExceptionType::OutOfMemory);
  hash2 = new BDD[hashsize];
  if(hash2 == 0) { 
    delete[] hash1; 
    BDDerr("BDDV_Import: Failed to allocate memory for hash2.", ExceptionType::OutOfMemory);
  }
  for(bddword ix=0; ix<hashsize; ix++)
  {
    hash1[ix] = B_VAL_MASK;
    hash2[ix] = 0;
  }

  e = 0;
  for(bddword ix=0; ix<n_nd; ix++)
  {
    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    bddword nd = B_STRTOI(s, NULL, 10);
    
    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    int lev = strtol(s, NULL, 10);
    int var = bddvaroflev(lev);

    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f0 = 0;
    else if(strcmp(s, "T") == 0) f0 = 1;
    else
    {
      bddword nd0 = B_STRTOI(s, NULL, 10);

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

    if(fscanf(strm, "%s", s) == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f1 = 0;
    else if(strcmp(s, "T") == 0) f1 = 1;
    else
    {
      bddword nd1 = B_STRTOI(s, NULL, 10);
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
    if(f == -1) { 
      e = 1; 
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

  if(e)
  {
    delete[] hash2;
    delete[] hash1;
    BDDerr("BDDV_Import: Error during node processing.", ExceptionType::FileFormat);
  }

  BDDV v = BDDV();
  for(int i=0; i<m; i++)
  {
    if(fscanf(strm, "%s", s) == EOF)
    {
      delete[] hash2;
      delete[] hash1;
      BDDerr("BDDV_Import: Unexpected end of file during vector processing.", ExceptionType::FileFormat);
    }
    bddword nd = B_STRTOI(s, NULL, 10);
    if(strcmp(s, "F") == 0) v = v || BDD(0);
    else if(strcmp(s, "T") == 0) v = v || BDD(1);
    else
    {
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

  delete[] hash2;
  delete[] hash1;
  return v;
}

BDDV BDDV_ImportPla(FILE *strm, int sopf)
{
  char s[256];
  int n = 0;
  int m = 0;
  int mode = 1; // 0:f 1:fd 2:fr 3:fdr

  do if(fscanf(strm, "%s", s) == EOF) 
      BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
  while(s[0] == '#');

  // declaration part 
  while(s[0] == '.')
  {
    if(strcmp(s, ".i") == 0)
    {
      if(fscanf(strm, "%s", s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      n = strtol(s, NULL, 10);
    }
    else if(strcmp(s, ".o") == 0)
    {
      if(fscanf(strm, "%s", s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      m = strtol(s, NULL, 10);
    }
    else if(strcmp(s, ".type") == 0)
    {
      if(fscanf(strm, "%s", s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
      if(strcmp(s, "f") == 0) mode = 0;
      else if(strcmp(s, "fd") == 0) mode = 1;
      else if(strcmp(s, "fr") == 0) mode = 2;
      else if(strcmp(s, "fdr") == 0) mode = 3;
      else { } // nop
    }
    else 
    {
      if(fscanf(strm, "%s", s) == EOF)
        BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
    }
    if(fscanf(strm, "%s", s) == EOF)
      BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
  }
  
  if(n < 0) 
    BDDerr("BDDV_ImportPla: Error in input size.", ExceptionType::FileFormat);
  if(m <= 0) 
    BDDerr("BDDV_ImportPla: Error in output size.", ExceptionType::FileFormat);
  while(BDD_TopLev() < n*2) BDD_NewVar();
  BDDV onset = BDDV(0, m);
  BDDV offset = BDDV(0, m);
  BDDV dcset = BDDV(0, m);
  BDD term;

  // logic description part
  while(s[0] != '.')
  {
    if((int)strlen(s) != n)
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
    if(fscanf(strm, "%s", s) == EOF)
      BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
    if((int)strlen(s) != m) 
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
    if(fscanf(strm, "%s", s) == EOF)
      BDDerr("BDDV_ImportPla: Unexpected end of file.", ExceptionType::FileFormat);
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
