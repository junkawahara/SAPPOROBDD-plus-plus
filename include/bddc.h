/*****************************************
*  BDD Package (SAPPORO-1.94)   - Header *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)  *
******************************************/

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
#define B_VAR_WIDTH 16U  /* Width of variable index */
#define B_VAR_MASK       ((1U << B_VAR_WIDTH) - 1U)

/***************** Internal macro for bddp *****************/

#ifdef B_32
#  define B_MSB_POS   31U
#  define B_LSB_MASK  1U
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
#define bddvarmax B_VAR_MASK /* Max value of variable index */
#define bddnull   B_VAL_MASK /* Special value for null pointer */
#define bddfalse  B_CST_MASK /* bddp of constant false (0) */
#define bddtrue   (bddfalse ^ B_INV_MASK)
                    /* bddp of constant true (1) */
#define bddempty  bddfalse /* bddp of empty ZDD (0) */
#define bddsingle bddtrue  /* bddp of single unit ZDD (1) */
#define bddconst(c) (((c) & B_VAL_MASK) | B_CST_MASK)
                    /* bddp of a constant valued node */
#define bddvalmax B_VAL_MASK  /* Max constant value */

#define CACHE_OP_USER_START   100   /* Start of user-defined cache operations */

/***************** For stack overflow limit *****************/
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
extern bddp   bddprime(bddvar v);
extern bddvar bddtop(bddp f);
extern bddp   bddcopy(bddp f);
extern bddp   bddnot(bddp f);
extern bddp   bddand(bddp f, bddp g);
extern bddp   bddor(bddp f, bddp g);
extern bddp   bddxor(bddp f, bddp g);
extern bddp   bddnand(bddp f, bddp g);
extern bddp   bddnor(bddp f, bddp g);
extern bddp   bddxnor(bddp f, bddp g);
extern bddp   bddat0(bddp f, bddvar v);
extern bddp   bddat1(bddp f, bddvar v);

/********** Memory management and observation ***********/
extern void   bddfree(bddp f);
extern bddp   bddused(void);
extern int    bddgc(void);
extern bddp   bddsize(bddp f);
extern bddp   bddvsize(bddp *p, int lim);
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
extern int    bddisbdd(bddp f);
extern int    bddiszdd(bddp f);
extern int    bddiszbdd(bddp f); // for compatibility

/************** SeqBDD operations *************/
extern bddp   bddpush(bddp f, bddvar v);

/************** SAPPOROBDD++ additions *************/
extern void   bddsetcacheratio(double ratio);
extern double bddgetcacheratio(void);
extern void   bddsetgcthreshold(bddp threshold);
extern bddp   bddgetgcthreshold(void);

} // namespace sapporobdd


#endif /* bddc_h */
