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
/* Every internal operation code is below this value.  bddrcache() and
   bddwcache() refuse smaller codes, so a user-defined operation can never
   read or overwrite an internal entry; the assertion keeps the two in step
   when a code is added.  CACHE_OP_USER_START (bddc.h) is the recommended
   first user code: the codes from BC_OP_LIMIT up to it are accepted for
   compatibility with programs written against the original SAPPOROBDD
   documentation, but they may be taken by internal operations one day. */
#define BC_OP_LIMIT   20
static_assert(BC_CARD2 < BC_OP_LIMIT, "an internal opcode reached BC_OP_LIMIT");

/* Allocation helpers behind B_MALLOC / B_REALLOC.  They return a null pointer
   when sizeof(type) * count would wrap around size_t, so an implausible
   element count cannot produce a buffer shorter than the caller asked for.
   The count is taken as unsigned long long and compared against SIZE_MAX
   before it is narrowed: the macros used to cast it to size_t first, so on a
   host where size_t is narrower than bddp (an ILP32 build of the 64bit
   variant) a count above 2^32 was truncated before the check ever saw it.
   A count of 0 is rounded up to 1: malloc(0) may return null, which every
   caller reads as an allocation failure, and realloc(p, 0) may free p. */
inline void *b_alloc_checked(size_t elemsize, unsigned long long count)
{
  if(count == 0) count = 1;
  if(count > (unsigned long long)((size_t)-1)) return 0;
  if(elemsize != 0 && (size_t)count > ((size_t)-1) / elemsize) return 0;
  return malloc(elemsize * (size_t)count);
}

inline void *b_realloc_checked(void *ptr, size_t elemsize,
                               unsigned long long count)
{
  if(count == 0) count = 1;
  if(count > (unsigned long long)((size_t)-1)) return 0;
  if(elemsize != 0 && (size_t)count > ((size_t)-1) / elemsize) return 0;
  return realloc(ptr, elemsize * (size_t)count);
}

/* Macros for malloc, realloc.  The element count has to be parenthesised:
   B_MALLOC(char, n*2+1) would otherwise expand to sizeof(char)*n*2+1, which
   only happens to be right because sizeof(char) is 1.  B_REALLOC keeps the
   old block when it fails, so the caller's state stays valid. */
#define B_MALLOC(type, size) \
  ((type *)b_alloc_checked(sizeof(type), (unsigned long long)(size)))
#define B_REALLOC(ptr, type, size) \
  ((type *)b_realloc_checked((void *)(ptr), sizeof(type), \
                             (unsigned long long)(size)))

/* Printf format of bddp.  bddp is an unsigned type, so the conversions have
   to be unsigned as well: a signed conversion would print node numbers close
   to the top of the range as negative values, which bddimport() could not
   read back.  (The B_EXTEND and the default build share the 64bit type.) */
#ifdef B_32
#  define B_BDDP_FD "%u"
#  define B_BDDP_FX "0x%X"
#else
#  define B_BDDP_FD "%llu"
#  define B_BDDP_FX "0x%llX"
#endif

/* Table spaces */
#define B_NODE_MAX (B_VAL_MASK>>1U) /* Max number of BDD nodes */
#define B_NODE_SPC0 256 /* Default initial node size */
/* allocatecache() derives every cache size by doubling B_NODE_SPC0, and
   B_CACHEKEY reduces a key modulo that size with a & (CacheSpc-1U) mask,
   which is only a modulo while the sizes are powers of 2. */
static_assert((B_NODE_SPC0 & (B_NODE_SPC0 - 1)) == 0,
              "B_NODE_SPC0 must be a power of 2");
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

/* Conversion of bddp and node index/pointer.  B_NP() is only defined for a
   non-constant bddp that names a slot of the node table: computing Node +
   index for an index past the table is undefined pointer arithmetic, and
   with B_32 the product can wrap around the address space back into the
   table, so a validity check has to compare indices, not pointers. */
#define B_NP(f)       (Node+(B_ABS(f)>>1U))
#define B_NDX(f)      (B_ABS(f)>>1U)
#define B_BDDP_NP(p)  ((bddp)((p)-Node) << 1U)
/* True when the non-constant bddp f does not name a live node: its index is
   outside the node table or the slot is free.  This is the check every API
   entry point runs on its operands before touching the node. */
#define B_BAD_NODE(f) (B_NDX(f) >= NodeSpc || Node[B_NDX(f)].varrfc == 0)

/* Read & Write of bddp field in the tables.  The arguments are evaluated
   more than once (the default build splits a bddp into a 32bit and an 8bit
   field), and B_SET_BDDP writes the first half before it reads g again, so
   the arguments must be plain variables or constants without side effects,
   and g must not be an expression over the field being assigned. */
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

/* ----------- Stack overflow limiter ------------ */
/* BDD_RecurLimit and BDD_RecurCount are declared in bddc.h.  BDD.h carries
   a copy of these two macros for the BDD+ layer (it cannot include this
   header, and it reports through BDDerr() instead of err()); the two copies
   have to be changed together.  The counter is shared by every recursion in
   the library, so what is left of the budget depends on the caller's depth;
   b_recursion_fits() below is how the dispatchers take that into account. */
#define BDD_RECUR_INC \
  do { if(++BDD_RecurCount >= BDD_RecurLimit) \
    err("BDD_RECUR_INC: Recursion Limit", BDD_RecurCount, \
        ExceptionType::InternalError); } while(0)
#define BDD_RECUR_DEC BDD_RecurCount--

/* Conversion of ZDD node flag: a ZDD node stores its 0-edge with the
   inverter bit set (the negation of a ZDD edge is carried by the parent's
   edge instead), a BDD node never does. */
#ifdef B_EXTEND
#define B_Z_NP(p) ((p)->f0_64 & (bddp_64)B_INV_MASK)
#else
#define B_Z_NP(p) ((p)->f0_32 & (bddp_32)B_INV_MASK)
#endif

/* Hash Functions.  The reduction "& (size-1U)" is a modulo only while the
   table size is a non-zero power of 2; the allocators keep every hash table
   and the operation cache that way (B_HASH_SPC0 doubled, B_NODE_SPC0
   doubled), and B_CACHEKEY must not be evaluated before the cache exists.
   The arguments are evaluated several times, so they have to be plain
   variables or constants. */
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

/* The widest count the rfc field of an RFC-table entry can hold: 32 bits
   with B_32, the 40 bits of the split field in the default build, 64 bits
   with B_EXTEND.  rfc_inc_ovf() refuses to go past it instead of wrapping
   the count to 0. */
#ifdef B_32
#  define B_RFCT_MAX 0xFFFFFFFFU
#elif defined(B_EXTEND)
#  define B_RFCT_MAX (~0ULL)
#else
#  define B_RFCT_MAX ((1ULL << 40U) - 1ULL)
#endif

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
  bddp_h8       f_h8; /* Extension of an operand BDD */
  bddp_h8       g_h8; /* Extension of an operand BDD */
  bddp_h8       h_h8; /* Extension of result BDD */
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

extern struct B_CacheTable *Cache; /* Operation cache */
extern bddp CacheSpc;              /* Current cache size */
extern double CacheRatio;          /* Cache size ratio to node table size */
extern bddp GCThreshold;           /* GC threshold - minimum freed nodes for successful GC */
/* Set when a shift operation may have left entries in the operation cache.
   Those are the only entries that depend on the variable order, so
   bddnewvaroflev() has to drop them; the flag keeps that sweep out of the
   way of programs that never shift.  See shift_cache_clear(). */
extern int ShiftCacheUsed;

extern struct B_RFC_Table *RFCT;   /* RFC-Table */
extern bddp RFCT_Spc;              /* Current RFC-table size */
extern bddp RFCT_Used;             /* Current RFC-table used entries */

extern struct B_MPTable mptable[B_MP_LMAX]; /* MP-Count Table */
/* Byte size of the multi-precision table allocation that failed during
   the current bddcardmp16() call, or 0 if no allocation failed.  It lets
   the caller tell a genuine out-of-memory condition apart from the
   B_MP_NULL that merely reports an exhausted table index space. */
extern bddp MPAllocFailSize;
/* Set when mp_add() had to saturate during the current bddcardmp16() call,
   i.e. the cardinality no longer fits in B_MP_LMAX words.  The saturated
   all-ones value must not be handed back as a result, so the count unwinds
   through B_MP_NULL and bddcardmp16() reports it at the API boundary. */
extern int MPCountOverflowed;

/* True when a recursion that descends at most one level per call and starts
   now fits into what is left of the recursion budget.  The recursive apply
   and traversal routines are used only when this holds; otherwise the
   iterative versions, which keep their stack on the heap, take over.  Testing
   VarUsed alone against the limit used to leave no headroom: an apply started
   from inside a BDD+ recursion, or the AND nested inside BC_UNIV, ran out of
   budget on inputs the iterative version handles.  Every threshold in the
   library is derived from BDD_RecurLimit through this one test. */
inline int b_recursion_fits(void)
{
  return (long long)BDD_RecurCount + (long long)VarUsed < (long long)BDD_RecurLimit;
}

/* ----- Declaration of internal functions ------ */
/* Error handling.  err() never returns: it always throws the BDDException
   subclass that corresponds to exType.  The int return type is kept only so
   that err() can be used inside the conditional expressions of the
   B_RFC_DEC_NP / BDD_RECUR_INC macros. */
[[noreturn]] int err(const char *msg, bddp num, ExceptionType exType);

/* Reference count overflow handling */
int  rfc_inc_ovf(struct B_NodeTable *np);
int  rfc_dec_ovf(struct B_NodeTable *np);

/* Table management.  var_enlarge() and hash_enlarge() throw
   BDDOutOfMemoryException when they cannot grow their table and leave it as
   it was; node_enlarge() returns 1 instead, because getnode() then falls
   back to a garbage collection. */
void var_enlarge(void);
int  node_enlarge(void);
void hash_enlarge(bddvar v);

/* Node creation.  v is a variable in use (1..VarUsed); f0 and f1 are valid
   bddp values, never bddnull, each carrying one reference that the call
   consumes (a shared node hands them back, a new node keeps them).  The
   functions never return bddnull: memory exhaustion is reported by
   BDDOutOfMemoryException, thrown before any reference is consumed, so the
   caller still owns f0 and f1 and releases them itself. */
bddp getnode(bddvar v, bddp f0, bddp f1);
bddp getbddp(bddvar v, bddp f0, bddp f1);
bddp getzddp(bddvar v, bddp f0, bddp f1);

/* Core apply algorithm */
bddp apply(bddp f, bddp g, unsigned char op, unsigned char skip);

/* Garbage collection helper */
void gc1(struct B_NodeTable *np);

/* Graph traversal.  traverse_postorder() calls visit(f, ctx) once for every
   node reachable from f, children before parents, and leaves the visit flags
   set; count() also leaves them set.  reset() clears them again. */
bddp count(bddp f);
void traverse_postorder(bddp f, void (*visit)(bddp, void *), void *ctx);
void dump(bddp f);
void reset(bddp f);
void reset_aborted(bddp *p, int n, int recur_count);

/* Import/Export helpers */
void export_static(FILE *strm, bddp f);
int import(FILE *strm, bddp *p, int lim, int z);

/* Misc */
int andfalse(bddp f, bddp g);
int mp_add(struct B_MP *p, bddp ix);

/* Cache management */
/* Validates a cache ratio and stores it in CacheRatio.  caller names the
   public function the diagnostics should blame, since bddinit() checks its
   cacheRatio argument through this same code. */
void setcacheratiovalue(double cacheRatio,
                        const char *caller = "bddsetcacheratio");
bool allocatecache();
/* fprintf() that throws on a write error.  The format attribute lets the
   compiler check the B_BDDP_FD conversions against their arguments, which
   change type with the build variant. */
void fprintf_check(FILE *strm, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((format(printf, 2, 3)))
#endif
  ;

} // namespace sapporobdd

#endif /* BDDC_INTERNAL_H */
