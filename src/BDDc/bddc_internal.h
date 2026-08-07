/*****************************************
*  BDD Package (SAPPORO-1.94)   - Internal Header *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)          *
*  Split from bddc.cc for modularity              *
******************************************/

#ifndef BDDC_INTERNAL_H
#define BDDC_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include "bddc.h"
#include "BDDException.h"

namespace sapporobdd {

/* ----------------- MACRO Definitions ---------------- */
/* Operation IDs in Cache */
#define BC_NULL        0
#define BC_AND         1
#define BC_XOR         2
#define BC_AT0         3
#define BC_AT1         4
#define BC_LSHIFT      5
#define BC_RSHIFT      6
#define BC_COFACTOR    7
#define BC_UNIV        8
#define BC_SUPPORT     9
#define BC_INTERSEC   10
#define BC_UNION      11
#define BC_SUBTRACT   12
#define BC_OFFSET     13
#define BC_ONSET      14
#define BC_CHANGE     15
#define BC_CARD       16
#define BC_LIT        17
#define BC_LEN        18
#define BC_CARD2      19

/* Macros for malloc, realloc */
#define B_MALLOC(type, size) \
  (type *)malloc(sizeof(type) * size)
#define B_REALLOC(ptr, type, size) \
  (type *)realloc(ptr, sizeof(type) * size)

/* Printf format of bddp */
#ifdef B_32
#  define B_BDDP_FD "%d"
#  define B_BDDP_FX "0x%X"
#elif defined(B_EXTEND)
#  define B_BDDP_FD "%lld"
#  define B_BDDP_FX "0x%llX"
#else
#  define B_BDDP_FD "%lld"
#  define B_BDDP_FX "0x%llX"
#endif

/* strtol or strtoll */
#ifdef B_32
#  define B_STRTOI strtol
#elif defined(B_EXTEND)
#  define B_STRTOI strtoll
#else
#  define B_STRTOI strtoll
#endif

/* Table spaces */
#define B_NODE_MAX (B_VAL_MASK>>1U) /* Max number of BDD nodes */
#define B_NODE_SPC0 256 /* Default initial node size */
#define B_VAR_SPC0   16 /* Initial var table size */
#define B_HASH_SPC0   4 /* Initial hash size */
#define B_RFCT_SPC0   4 /* Initial RFCT size */

/* Negative edge manipulation */
#define B_NEG(f)  ((f) & B_INV_MASK)
#define B_NOT(f)  ((f) ^ B_INV_MASK)
#define B_ABS(f)  ((f) & ~B_INV_MASK)

/* Constant node manipulation */
#define B_CST(f)  ((f) & B_CST_MASK)
#define B_VAL(f)  ((f) & B_VAL_MASK)

/* Conversion of bddp and node index/pointer  */
#define B_NP(f)       (Node+(B_ABS(f)>>1U))
#define B_NDX(f)      (B_ABS(f)>>1U)
#define B_BDDP_NP(p)  ((bddp)((p)-Node) << 1U)

/* Read & Write of bddp field in the tables */
#ifdef B_EXTEND
#  define B_SET_NXP(p, f, i) (p ## _64 = f ## _64 + i)
#  define B_GET_BDDP(f) (f ## _64)
#  define B_SET_BDDP(f, g) (f ## _64 = g)
#  define B_CPY_BDDP(f, g) (f ## _64 = g ## _64)
#elif defined(B_32)
#  define B_SET_NXP(p, f, i) (p ## _32 = f ## _32 + i)
#  define B_GET_BDDP(f) (f ## _32)
#  define B_SET_BDDP(f, g) (f ## _32 = g)
#  define B_CPY_BDDP(f, g) (f ## _32 = g ## _32)
#else
#  define B_LOW32(f) ((bddp_32)((f)&((1ULL<<32U)-1U)))
#  define B_HIGH8(f) ((bddp_h8)((f)>>32U))
#  define B_SET_NXP(p, f, i) \
    (p ## _h8 = f ## _h8 + i, p ## _32 = f ## _32 + i)
#  define B_GET_BDDP(f) \
    ((bddp) f ## _32 | ((bddp) f ## _h8 << 32U))
#  define B_SET_BDDP(f, g) \
    (f ## _h8 = B_HIGH8(g), f ## _32 = B_LOW32(g))
#  define B_CPY_BDDP(f, g) \
    (f ## _h8 = g ## _h8, f ## _32 = g ## _32)
#endif /* B_32 */

/* var & rfc manipulation */
#ifdef B_EXTEND
#define B_VAR_NP(p)    ((bddvar)((p)->varrfc & 0xFFFFFFFFULL))
#define B_RFC_MASK     0xFFFFFFFF00000000ULL
#define B_RFC_UNIT     0x100000000ULL
#define B_RFC_NP(p)    ((p)->varrfc >> 32U)
#define B_RFC_ZERO_NP(p) ((p)->varrfc < B_RFC_UNIT)
#define B_RFC_ONE_NP(p) (((p)->varrfc & B_RFC_MASK) == B_RFC_UNIT)
#define B_RFC_INC_NP(p) \
  (((p)->varrfc < B_RFC_MASK - B_RFC_UNIT)? \
   ((p)->varrfc += B_RFC_UNIT, 0) : rfc_inc_ovf(p))
#define B_RFC_DEC_NP(p) \
  (((p)->varrfc >= B_RFC_MASK)? rfc_dec_ovf(p): \
   (B_RFC_ZERO_NP(p))? \
    err("B_RFC_DEC_NP: rfc under flow", p-Node, ExceptionType::InternalError): \
    ((p)->varrfc -= B_RFC_UNIT, 0))
#else
#define B_VAR_NP(p)    ((p)->varrfc & B_VAR_MASK)
#define B_RFC_MASK  (~B_VAR_MASK)
#define B_RFC_UNIT  (1U << B_VAR_WIDTH)
#define B_RFC_NP(p)    ((p)->varrfc >> B_VAR_WIDTH)
#define B_RFC_ZERO_NP(p) ((p)->varrfc < B_RFC_UNIT)
#define B_RFC_ONE_NP(p) (((p)->varrfc & B_RFC_MASK) == B_RFC_UNIT)
#define B_RFC_INC_NP(p) \
  (((p)->varrfc < B_RFC_MASK - B_RFC_UNIT)? \
   ((p)->varrfc += B_RFC_UNIT, 0) : rfc_inc_ovf(p))
#define B_RFC_DEC_NP(p) \
  (((p)->varrfc >= B_RFC_MASK)? rfc_dec_ovf(p): \
   (B_RFC_ZERO_NP(p))? \
    err("B_RFC_DEC_NP: rfc under flow", p-Node, ExceptionType::InternalError): \
    ((p)->varrfc -= B_RFC_UNIT, 0))
#endif /* B_EXTEND */

/* ----------- Stack overflow limitter ------------ */
extern const int BDD_RecurLimit;
extern int BDD_RecurCount;

#define BDD_RECUR_INC \
  {if(++BDD_RecurCount >= BDD_RecurLimit) \
    err("BDD_RECUR_INC: Recursion Limit", BDD_RecurCount, ExceptionType::InternalError);}
#define BDD_RECUR_DEC BDD_RecurCount--

/* Conversion of ZDD node flag */
#ifdef B_EXTEND
#define B_Z_NP(p) ((p)->f0_64 & (bddp_64)B_INV_MASK)
#elif defined(B_32)
#define B_Z_NP(p) ((p)->f0_32 & (bddp_32)B_INV_MASK)
#else
#define B_Z_NP(p) ((p)->f0_32 & (bddp_32)B_INV_MASK)
#endif

/* Hash Functions */
#define B_HASHKEY(f0, f1, hashSpc) \
  (((B_CST(f0)? (f0): ((f0)+2U)) \
   ^(B_NEG(f0)? ~((f0)>>1U): ((f0)>>1U))\
   ^((B_CST(f1)? (f1): ((f1)+2U))) \
   ^((B_NEG(f1)? ~((f1)>>1U):((f1)>>1U))<<4U))\
  & (hashSpc-1U))
#define B_CACHEKEY(op, f, g) \
  ((((bddp)(op)<<4U)\
   ^((B_CST(f)? (f):((f)+2U)))\
   ^((B_NEG(f)? ~((f)>>1U): ((f)>>1U))) \
   ^((B_CST(g)? (g):((g)+2U))) \
   ^((B_NEG(g)? ~((g)>>1U):((g)>>1U))*4369U) )\
   & (CacheSpc-1U))

/* Multi-Precision Count */
#define B_MP_LWID 4U
#define B_MP_LPOS (B_MSB_POS - B_MP_LWID)
#define B_MP_LMAX (1U<<B_MP_LWID)
#define B_MP_NULL (B_CST_MASK + B_VAL_MASK)
#define B_MP_LEN(f) (B_CST(f)? (B_VAL(f)>>B_MP_LPOS)+1: 0)
#define B_MP_VAL(f) ((f) & (B_VAL_MASK>>B_MP_LWID))

#define CACHE_RATIO_MAX  1024

/* ------- Declaration of internal data types ------- */
/* typedef of bddp field in the tables */
typedef unsigned int bddp_32;
#ifdef B_EXTEND
  typedef unsigned long long bddp_64;
#elif !defined(B_32)
  typedef unsigned char bddp_h8;
#endif

/* Declaration of Node table */
struct B_NodeTable
{
#ifdef B_EXTEND
  bddp_64      f0_64;    /* 0-edge (64bit) */
  bddp_64      f1_64;    /* 1-edge (64bit) */
  bddp_64      nx_64;    /* Node index (64bit) */
  unsigned long long varrfc; /* VarID & Reference counter (64bit) */
#elif defined(B_32)
  bddp_32      f0_32;    /* 0-edge */
  bddp_32      f1_32;    /* 1-edge */
  bddp_32      nx_32;    /* Node index */
  unsigned int varrfc;   /* VarID & Reference counter */
#else
  bddp_32      f0_32;    /* 0-edge */
  bddp_32      f1_32;    /* 1-edge */
  bddp_32      nx_32;    /* Node index */
  unsigned int varrfc;   /* VarID & Reference counter */
  bddp_h8      f0_h8;    /* Extension of 0-edge */
  bddp_h8      f1_h8;    /* Extension of 1-edge */
  bddp_h8      nx_h8;    /* Extension of node index */
#endif
};

/* Declaration of Hash-table per Var */
struct B_VarTable
{
  bddp    hashSpc;  /* Current hash-table size */
  bddp    hashUsed;  /* Current used entries */
  bddvar  lev;      /* Level of the variable */
#ifdef B_EXTEND
  bddp_64 *hash_64; /* Hash-table (64bit) */
#elif defined(B_32)
  bddp_32 *hash_32; /* Hash-table */
#else
  bddp_32 *hash_32; /* Hash-table */
  bddp_h8 *hash_h8; /* Extension of hash-table */
#endif
};

/* Declaration of Operation Cache */
struct B_CacheTable
{
#ifdef B_EXTEND
  bddp_64       f_64; /* an operand BDD (64bit) */
  bddp_64       g_64; /* an operand BDD (64bit) */
  bddp_64       h_64; /* Result BDD (64bit) */
#elif defined(B_32)
  bddp_32       f_32; /* an operand BDD */
  bddp_32       g_32; /* an operand BDD */
  bddp_32       h_32; /* Result BDD */
#else
  bddp_32       f_32; /* an operand BDD */
  bddp_32       g_32; /* an operand BDD */
  bddp_32       h_32; /* Result BDD */
  bddp_h8       f_h8; /* Extention of an operand BDD */
  bddp_h8       g_h8; /* Extention of an operand BDD */
  bddp_h8       h_h8; /* Extention of result BDD */
#endif
  unsigned char op;   /* Operation code */
};

/* Declaration of RFC-table */
struct B_RFC_Table
{
#ifdef B_EXTEND
  bddp_64 nx_64;   /* Node index (64bit) */
  bddp_64 rfc_64;  /* RFC (64bit) */
#elif defined(B_32)
  bddp_32 nx_32;   /* Node index */
  bddp_32 rfc_32;  /* RFC */
#else
  bddp_32 nx_32;   /* Node index */
  bddp_32 rfc_32;  /* RFC */
  bddp_h8 nx_h8;   /* Extension of Node index */
  bddp_h8 rfc_h8;  /* Extension of RFC */
#endif
};

/* Declaration of MP-Count */
struct B_MPTable
{
  bddp size;  /* Table size */
  bddp used;  /* Used entries */
  bddp* word; /* Table head */
};

struct B_MP
{
  int len;
  bddp word[B_MP_LMAX];
};

/* ------- Declaration of global variables (extern) ------- */
extern struct B_NodeTable *Node;  /* Node Table */
extern bddp NodeLimit;            /* Final limit size */
extern bddp NodeUsed;             /* Number of used node */
extern bddp Avail;                /* Head of available node */
extern bddp NodeSpc;              /* Current Node-Table size */

extern struct B_VarTable *Var;    /* Var-tables */
extern bddvar *VarID;             /* VarID reverse table */
extern bddvar VarUsed;            /* Number of used Var */
extern bddvar VarSpc;             /* Current Var-table size */

extern struct B_CacheTable *Cache; /* Opeartion cache */
extern bddp CacheSpc;              /* Current cache size */
extern double CacheRatio;          /* Cache size ratio to node table size */
extern bddp GCThreshold;           /* GC threshold - minimum freed nodes for successful GC */

extern struct B_RFC_Table *RFCT;   /* RFC-Table */
extern bddp RFCT_Spc;              /* Current RFC-table size */
extern bddp RFCT_Used;             /* Current RFC-table used entries */

extern struct B_MPTable mptable[B_MP_LMAX]; /* MP-Count Table */
/* Byte size of the multi-precision table allocation that failed during
   the current bddcardmp16() call, or 0 if no allocation failed.  It lets
   the caller tell a genuine out-of-memory condition apart from the
   B_MP_NULL that merely reports an exhausted table index space. */
extern bddp MPAllocFailSize;

/* ----- Declaration of internal functions ------ */
/* Error handling.  err() never returns: it always throws the BDDException
   subclass that corresponds to exType.  The int return type is kept only so
   that err() can be used inside the conditional expressions of the
   B_RFC_DEC_NP / BDD_RECUR_INC macros. */
[[noreturn]] int err(const char *msg, bddp num, ExceptionType exType);

/* Reference count overflow handling */
int  rfc_inc_ovf(struct B_NodeTable *np);
int  rfc_dec_ovf(struct B_NodeTable *np);

/* Table management */
void var_enlarge(void);
int  node_enlarge(void);
int  hash_enlarge(bddvar v);

/* Node creation */
bddp getnode(bddvar v, bddp f0, bddp f1);
bddp getbddp(bddvar v, bddp f0, bddp f1);
bddp getzddp(bddvar v, bddp f0, bddp f1);

/* Core apply algorithm */
bddp apply(bddp f, bddp g, unsigned char op, unsigned char skip);

/* Garbage collection helper */
void gc1(struct B_NodeTable *np);

/* Graph traversal */
bddp count(bddp f);
void dump(bddp f);
void reset(bddp f);

/* Import/Export helpers */
void export_static(FILE *strm, bddp f);
int import(FILE *strm, bddp *p, int lim, int z);

/* Misc */
int andfalse(bddp f, bddp g);
int mp_add(struct B_MP *p, bddp ix);

/* Cache management */
void setcacheratiovalue(double cacheRatio);
bool allocatecache();
void fprintf_check(FILE *strm, const char *format, ...);

} // namespace sapporobdd

#endif /* BDDC_INTERNAL_H */
