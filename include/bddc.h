/*****************************************
*  BDD Package (SAPPORO-1.94)   - Header *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)  *
******************************************/

/* Threading and lifetime model
 *
 * The package keeps the node table, the operation cache and the variable
 * table in global state, so a process has exactly one BDD manager and the
 * whole API is not thread safe: all calls have to come from one thread, or be
 * serialised by the caller.  bddinit() may be called again to start over, but
 * doing so discards those tables, which invalidates every bddp value obtained
 * before the call.  Both are by design.
 */

#ifndef bddc_h
#define bddc_h

#define SAPPOROBDD_PLUS_PLUS
#define SAPPOROBDDPP_MAJOR_VERSION 1
#define SAPPOROBDDPP_MINOR_VERSION 0
#define SAPPOROBDDPP_PATCH_VERSION 0
#define SAPPOROBDDPP_VERSION \
  (SAPPOROBDDPP_MAJOR_VERSION * 10000 + \
    SAPPOROBDDPP_MINOR_VERSION * 100 + \
    SAPPOROBDDPP_PATCH_VERSION)

namespace sapporobdd {

/***************** Internal macro for index *****************/
#ifdef B_EXTEND
#  define B_VAR_WIDTH 32U  /* Width of variable index */
#else
#  define B_VAR_WIDTH 20U  /* Width of variable index (supports 2^20 variables) */
#endif
#ifdef B_EXTEND
#  define B_VAR_MASK       ((1ULL << B_VAR_WIDTH) - 1ULL)
#else
#  define B_VAR_MASK       ((1U << B_VAR_WIDTH) - 1U)
#endif

/***************** Internal macro for bddp *****************/

#ifdef B_32
#  define B_MSB_POS   31U
#  define B_LSB_MASK  1U
#elif defined(B_EXTEND)
#  define B_MSB_POS   63ULL
#  define B_LSB_MASK  1ULL
#else
#  define B_MSB_POS   39ULL
#  define B_LSB_MASK  1ULL
#endif
#define B_MSB_MASK  (B_LSB_MASK << B_MSB_POS)
#define B_INV_MASK  B_LSB_MASK /* Mask of inverter-flag */
#define B_CST_MASK  B_MSB_MASK /* Mask of constant-flag */
#define B_VAL_MASK  (B_MSB_MASK - 1U)
                      /* Mask of value-field */

/***************** External typedef *****************/
typedef unsigned int bddvar;

#ifdef B_32
  typedef unsigned int bddp;
#else
  typedef unsigned long long bddp;
#endif

/***************** External Macro *****************/
#ifdef B_EXTEND
#define bddvarmax (B_VAR_MASK - 1) /* Max value of variable index. -1
                                      to avoid overflow when var_enlarge */
#else
#define bddvarmax B_VAR_MASK /* Max value of variable index */
#endif
#define bddnull   B_VAL_MASK /* Special value for null pointer */
#define bddfalse  B_CST_MASK /* bddp of constant false (0) */
#define bddtrue   (bddfalse ^ B_INV_MASK)
                    /* bddp of constant true (1) */
#define bddempty  bddfalse /* bddp of empty ZDD (0) */
#define bddsingle bddtrue  /* bddp of single unit ZDD (1) */
#define bddconst(c) (((c) & B_VAL_MASK) | B_CST_MASK)
                    /* bddp of a constant valued node */
#define bddvalmax B_VAL_MASK  /* Max constant value */

#define CACHE_OP_USER_START   100   /* Start of user-defined cache operations,
                                       see bddrcache() below */

/***************** For stack overflow limiter *****************/
/* Every recursion of the library, in the C core and in the BDD+ layer,
   counts its depth in BDD_RecurCount and throws BDDInternalErrorException
   when it reaches BDD_RecurLimit.  The core switches to its iterative
   implementations whenever the remaining budget does not cover the number
   of variables in use, so the limit is reached only by recursions that have
   no iterative form.  Every exception thrown by the library resets the
   counter to 0, since no library frame is left once it reaches the user. */
extern const int BDD_RecurLimit;
extern int BDD_RecurCount;

/***************** External operations *****************/

/***************** Init. and config. ****************/
extern int    bddinit (bddp initsize, bddp limitsize, double cacheRatio = 0.5);
extern bddvar bddnewvar (void);
extern bddvar bddnewvaroflev (bddvar lev);
extern bddvar bddlevofvar (bddvar v);
extern bddvar bddvaroflev (bddvar lev);
extern bddvar bddvarused(void);

/************** Basic logic operations *************/
/* Every operation validates its operands: an invalid or freed bddp, a
   constant other than bddfalse/bddtrue, or a node of the wrong kind (a ZDD
   node in a BDD operation and the reverse) throws
   BDDInvalidBDDValueException, and a VarID or level outside 1..bddvarused()
   throws BDDOutOfRangeException.  bddnull propagates: an operation whose
   operand is bddnull returns bddnull (the exceptions are the int-valued
   bddimply(), which refuses it, and the counts, which answer 0). */
extern bddp   bddprime(bddvar v);
extern bddvar bddtop(bddp f);
extern bddp   bddcopy(bddp f);
extern bddp   bddnot(bddp f);   /* BDD only: a ZDD has no complement */
extern bddp   bddand(bddp f, bddp g);
extern bddp   bddor(bddp f, bddp g);
extern bddp   bddxor(bddp f, bddp g);
extern bddp   bddnand(bddp f, bddp g);
extern bddp   bddnor(bddp f, bddp g);
extern bddp   bddxnor(bddp f, bddp g);
extern bddp   bddat0(bddp f, bddvar v);   /* BDD only; ZDDs use bddoffset() */
extern bddp   bddat1(bddp f, bddvar v);   /* BDD only; ZDDs use bddonset0() */

/********** Memory management and observation ***********/
extern void   bddfree(bddp f);
extern bddp   bddused(void);
extern int    bddgc(void);
extern bddp   bddsize(bddp f);
extern bddp   bddvsize(bddp *p, int lim);
/* bddexport() writes p[0..lim-1], up to the first bddnull.  The file does
   not record whether it holds BDDs or ZDDs: bddimport() reads it as BDDs and
   bddimportz() as ZDDs, and reading a file with the wrong one is not
   detected.  Both readers create the variables the header declares if they
   do not exist yet, and those stay after a failed import; a file declaring
   more outputs than lim yields the first lim of them.  A malformed file
   throws BDDFileFormatException, with nothing left allocated. */
extern void   bddexport(FILE *strm, bddp *p, int lim);
extern int    bddimport(FILE *strm, bddp *p, int lim);
extern void   bdddump(bddp f);
extern void   bddvdump(bddp *p, int lim);
extern void   bddgraph(bddp f);
extern void   bddgraph0(bddp f);
extern void   bddvgraph(bddp *p, int lim);
extern void   bddvgraph0(bddp *p, int lim);

/************** Advanced logic operations *************/
extern bddp   bddlshift(bddp f, bddvar shift);
extern bddp   bddrshift(bddp f, bddvar shift);
extern bddp   bddsupport(bddp f);
extern bddp   bdduniv(bddp f, bddp g);
extern bddp   bddexist(bddp f, bddp g);
extern bddp   bddcofactor(bddp f, bddp g);
extern int    bddimply(bddp f, bddp g);
/* User-defined operation cache.  op is at least 20 (the codes below that
   are the library's own and are refused); the codes from
   CACHE_OP_USER_START upward are guaranteed never to be taken by the
   library.  The cache stores h without taking a reference and hands it
   back without one: bddrcache() returns the stored value only while the
   caller still holds the diagram it registered, and a caller that keeps
   the result takes its own reference with bddcopy() (as BDD_CacheBDD()
   does).  A garbage collection drops every user entry. */
extern bddp   bddrcache(unsigned char op, bddp f, bddp g);
extern void   bddwcache
              (unsigned char op, bddp f, bddp g, bddp h);

/************** ZDD operations *************/
extern bddp   bddoffset(bddp f, bddvar v);
extern bddp   bddonset(bddp f, bddvar v);
extern bddp   bddonset0(bddp f, bddvar v);
extern bddp   bddchange(bddp f, bddvar v);
extern bddp   bddintersec(bddp f, bddp g);
extern bddp   bddunion(bddp f, bddp g);
extern bddp   bddsubtract(bddp f, bddp g);
extern bddp   bddcard(bddp f);
extern bddp   bddlit(bddp f);
extern bddp   bddlen(bddp f);
extern int    bddimportz(FILE* strm, bddp* p, int lim);
extern char  *bddcardmp16(bddp f, char *s);
       /* s must hold 16 * sizeof(bddp) * 2 hex digits and the terminating
          null (257 bytes in a 64bit build, 129 with B_32), or be 0 to have
          a buffer of the needed size allocated. */
extern int    bddisbdd(bddp f);
extern int    bddiszdd(bddp f);
extern int    bddiszbdd(bddp f); // for compatibility

/************** SeqBDD operations *************/
/* the node (v, 0, f) of a sequence BDD; the level of v is not compared with
   the top level of f, since a SeqBDD repeats variables along a path */
extern bddp   bddpush(bddp f, bddvar v);

/************** SAPPOROBDD++ additions *************/
extern void   bddsetcacheratio(double ratio);
extern double bddgetcacheratio(void);
extern void   bddsetgcthreshold(bddp threshold);
extern bddp   bddgetgcthreshold(void);

} // namespace sapporobdd


#endif /* bddc_h */
