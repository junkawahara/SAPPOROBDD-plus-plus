/*****************************************
*  BDD Package (SAPPORO-1.94)   - Operations  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

/* ------- Operand validation shared by the entry points below ------- */

/* Every public operation validates its operands here, under its own name:
   the derived operations (bddor(), bddexist(), ...) used to leave the check
   to the primitive they call, and an invalid operand was then reported as an
   error of bddand() or bdduniv().  The three checks differ in the kind of
   node they accept.  A constant is accepted only as bddfalse or bddtrue:
   the value-carrying constants (bddconst(), the counts returned by bddcard()
   and the multi-precision table references) are not diagrams, and the
   single-operand entry points used to pass them straight through -- into a
   node in the case of bddchange(). */

static void check_const(const char *who, bddp f)
{
  if(B_ABS(f) != bddfalse)
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: Invalid bddp", who);
    err(msg, f, ExceptionType::InvalidBDDValue);
  }
}

[[noreturn]] static void bad_operand(const char *who, const char *what, bddp f)
{
  char msg[64];
  snprintf(msg, sizeof(msg), "%s: %s", who, what);
  err(msg, f, ExceptionType::InvalidBDDValue);
}

/* a live node of either kind, or bddfalse/bddtrue */
static void check_node(const char *who, bddp f)
{
  if(B_CST(f)) { check_const(who, f); return; }
  if(B_BAD_NODE(f)) bad_operand(who, "Invalid bddp", f);
}

/* a BDD: a live non-ZDD node, or bddfalse/bddtrue */
static void check_bdd(const char *who, bddp f)
{
  check_node(who, f);
  if(!B_CST(f) && B_Z_NP(B_NP(f))) bad_operand(who, "applying ZDD node", f);
}

/* a ZDD: a live ZDD node, or bddfalse/bddtrue */
static void check_zdd(const char *who, bddp f)
{
  check_node(who, f);
  if(!B_CST(f) && !B_Z_NP(B_NP(f))) bad_operand(who, "applying non-ZDD node", f);
}

/* a variable in use.  Every entry point reports a VarID outside 1..VarUsed
   as BDDOutOfRangeException; bddprime(), bddat0() and bddat1() used to
   report the same condition as BDDInvalidBDDValueException. */
static void check_var(const char *who, bddvar v)
{
  if(v == 0 || v > VarUsed)
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: Invalid VarID", who);
    err(msg, v, ExceptionType::OutOfRange);
  }
}

/* ------- Reference management ------- */

bddp bddcopy(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return bddnull;
  if(B_CST(f)) return f; /* Constant */
  if(B_BAD_NODE(f)) bad_operand("bddcopy", "Invalid bddp", f);
  fp = B_NP(f);
  B_RFC_INC_NP(fp);
  return f;
}

void bddfree(bddp f)
{
  struct B_NodeTable *fp;

  if(f == bddnull) return;
  if(B_CST(f)) return; /* Constant */
  if(B_BAD_NODE(f)) bad_operand("bddfree", "Invalid bddp", f);
  fp = B_NP(f);
  B_RFC_DEC_NP(fp);
}

bddp bddnot(bddp f)
/* Logical negation of a BDD.  A ZDD node is refused: flipping the edge of a
   ZDD toggles the membership of the empty set, which is not a complement,
   and the result would still be accepted by every ZDD operation. */
{
  if(f == bddnull) return bddnull;
  check_bdd("bddnot", f);
  return B_NOT(bddcopy(f));
}

/* ------- Variables ------- */

bddvar bddlevofvar(bddvar v)
/* v = 0 is the pseudo variable of the constants, whose level is 0; bddtop()
   returns it for a constant, and bddlevofvar(bddtop(f)) has to work for
   every f. */
{
  if(v > VarUsed)
    err("bddlevofvar: Invalid VarID", v, ExceptionType::OutOfRange);
  return Var[v].lev;
}

bddvar bddvaroflev(bddvar lev)
/* lev = 0 is the level of the constants and answers VarID 0, see
   bddlevofvar(). */
{
  if(lev > VarUsed)
    err("bddvaroflev: Invalid level", lev, ExceptionType::OutOfRange);
  return VarID[lev];
}

bddvar bddvarused()
{
  return VarUsed;
}

/* The variable table is grown before the new variable is counted.  With the
   count incremented first, a var_enlarge() that threw (index range full, or
   no memory) left VarUsed == VarSpc behind: a variable that had no table
   entry, and a count that never triggered the enlargement again. */
static void var_reserve(const char *who)
{
  if(VarUsed >= bddvarmax)
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: var index range full", who);
    err(msg, VarUsed, ExceptionType::OutOfRange);
  }
  if(VarUsed + 1U >= VarSpc) var_enlarge();
}

bddvar bddnewvar()
{
  var_reserve("bddnewvar");
  return ++VarUsed;
}

static void shift_cache_clear(void)
/* Drops the cached results of the shift operations.  They are the only
   cached operations whose result depends on the variable ORDER rather than
   on the diagram alone: bddlshift()/bddrshift() rewrite a node's variable
   into the one that now sits "shift" levels away, so a cache entry keyed by
   (op, f, shift) answers for the level assignment that produced it.  Every
   other operation only ever compares levels, and an insertion leaves the
   relative order of the existing variables intact. */
{
  struct B_CacheTable *cachep;

  /* Nothing to sweep unless a shift has run since the last sweep; the cache
     can be very large, and a program that never shifts must not pay for a
     full scan on every variable it inserts. */
  if(!ShiftCacheUsed) return;
  for(cachep=Cache; cachep<Cache+CacheSpc; cachep++)
    if(cachep->op == BC_LSHIFT || cachep->op == BC_RSHIFT)
      cachep->op = BC_NULL;
  ShiftCacheUsed = 0;
}

bddvar bddnewvaroflev(bddvar lev)
{
  bddvar i;
  int moved;

  /* the ++VarUsed used to live inside this condition, so the error path for
     an invalid level had already created a ghost variable when it threw.
     The range check comes after the capacity check: with B_EXTEND and every
     variable in use, VarUsed + 1 wraps to 0 and would refuse every level
     under the wrong name. */
  var_reserve("bddnewvaroflev");
  if(lev == 0 || lev > VarUsed + 1U)
    err("bddnewvaroflev: Invalid level", lev, ExceptionType::OutOfRange);
  moved = (lev <= VarUsed);
  ++VarUsed;
  for(i=VarUsed; i>lev; i--) Var[ VarID[i] = VarID[i-1U] ].lev = i;
  Var[ VarID[lev] = VarUsed ].lev = lev;
  /* The levels of the variables at lev and above have just moved, so the
     shift results cached under the old assignment are wrong now.  An
     insertion at the very top (which is what bddnewvar() and BDD_NewVar()
     do) moves nothing and keeps them valid. */
  if(moved) shift_cache_clear();
  return VarUsed;
}

bddvar bddtop(bddp f)
{
  if(f == bddnull) return 0;
  if(B_CST(f)) return 0; /* Constant */
  if(B_BAD_NODE(f)) bad_operand("bddtop", "Invalid bddp", f);
  return B_VAR_NP(B_NP(f));
}

/* ------- BDD operations ------- */

bddp bddprime(bddvar v)
{
  check_var("bddprime", v);
  return getbddp(v, bddfalse, bddtrue);
}

/* The operations below propagate bddnull (the error value of the BDD+
   layer) and otherwise validate their operands before applying; memory
   exhaustion inside apply() is reported by BDDOutOfMemoryException. */

bddp bddand(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddand", f);
  check_bdd("bddand", g);
  return apply(f, g, BC_AND, 0);
}

bddp bddor(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddor", f);
  check_bdd("bddor", g);
  return B_NOT(apply(B_NOT(f), B_NOT(g), BC_AND, 0));
}

bddp bddxor(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddxor", f);
  check_bdd("bddxor", g);
  return apply(f, g, BC_XOR, 0);
}

bddp bddnand(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddnand", f);
  check_bdd("bddnand", g);
  return B_NOT(apply(f, g, BC_AND, 0));
}

bddp bddnor(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddnor", f);
  check_bdd("bddnor", g);
  return apply(B_NOT(f), B_NOT(g), BC_AND, 0);
}

bddp bddxnor(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddxnor", f);
  check_bdd("bddxnor", g);
  return apply(f, B_NOT(g), BC_XOR, 0);
}

bddp bddcofactor(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddcofactor", f);
  check_bdd("bddcofactor", g);
  return apply(f, g, BC_COFACTOR, 0);
}

/* True when g has the shape bdduniv()/bddexist() quantify over: the
   disjunction of a set of variables, which is what bddsupport() returns and
   what x | y | ... builds.  That BDD is a positive chain in which every node
   has the constant 1 on its 1-edge and the next variable on its 0-edge,
   ending in the constant 0.  The quantification only ever descends the
   0-edges of g, so a g of any other shape -- a cube x & y & ..., say -- was
   quantified over its top variable alone, without a word.  The constants
   stand for the empty set. */
static int is_varset(bddp g)
{
  struct B_NodeTable *gp;

  if(B_CST(g)) return 1;
  if(B_NEG(g)) return 0;
  while(!B_CST(g))
  {
    gp = B_NP(g);
    if(B_GET_BDDP(gp->f1) != bddtrue) return 0;
    g = B_GET_BDDP(gp->f0); /* the stored 0-edge of a BDD node is positive */
  }
  return g == bddfalse;
}

bddp bdduniv(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bdduniv", f);
  check_bdd("bdduniv", g);
  if(!is_varset(g))
    bad_operand("bdduniv", "g is not a set of variables (x | y | ...)", g);
  return apply(f, g, BC_UNIV, 0);
}

bddp bddexist(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_bdd("bddexist", f);
  check_bdd("bddexist", g);
  if(!is_varset(g))
    bad_operand("bddexist", "g is not a set of variables (x | y | ...)", g);
  return B_NOT(apply(B_NOT(f), g, BC_UNIV, 0));
}

int bddimply(bddp f, bddp g)
/* 1 if f implies g, 0 if it does not.  bddnull is refused: an int has no
   error value, and answering 0 for it used to be indistinguishable from
   "does not imply". */
{
  if(f == bddnull) bad_operand("bddimply", "Invalid bddp", f);
  if(g == bddnull) bad_operand("bddimply", "Invalid bddp", g);
  check_bdd("bddimply", f);
  check_bdd("bddimply", g);
  return ! andfalse(f, B_NOT(g));
}

bddp bddsupport(bddp f)
{
  if(f == bddnull) return bddnull;
  check_node("bddsupport", f);
  if(B_CST(f)) return bddfalse;
  return apply(f, bddfalse, BC_SUPPORT, 0);
}

/* bddat0() and bddat1() are BDD operations: the negative-edge rule the
   cofactor applies is the BDD one, and on a ZDD bddat1() used to answer with
   a set that bddonset0() disagrees with.  ZDD nodes are refused, as the
   other BDD operations do; bddoffset() and bddonset0() are the ZDD
   counterparts. */

bddp bddat0(bddp f, bddvar v)
{
  check_var("bddat0", v);
  if(f == bddnull) return bddnull;
  check_bdd("bddat0", f);
  if(B_CST(f)) return f;
  return apply(f, (bddp)v, BC_AT0, 0);
}

bddp bddat1(bddp f, bddvar v)
{
  check_var("bddat1", v);
  if(f == bddnull) return bddnull;
  check_bdd("bddat1", f);
  if(B_CST(f)) return f;
  return apply(f, (bddp)v, BC_AT1, 0);
}

bddp bddlshift(bddp f, bddvar shift)
{
  /* Check operands.  A shift of 0 is the identity whatever the variable
     count; testing it after the range check used to make "f << 0" fail
     when no variable existed yet (shift >= VarUsed with VarUsed == 0). */
  if(f == bddnull) return bddnull;
  if(shift == 0) return bddcopy(f);
  if(shift >= VarUsed)
    err("bddlshift: Invalid shift", shift, ExceptionType::OutOfRange);
  check_node("bddlshift", f);
  if(B_CST(f)) return f;

  /* tells bddnewvaroflev() that there may be order-dependent cache entries */
  ShiftCacheUsed = 1;
  return apply(f, (bddp)shift, BC_LSHIFT, 0);
}

bddp bddrshift(bddp f, bddvar shift)
{
  /* Check operands.  A shift of 0 is the identity whatever the variable
     count; testing it after the range check used to make "f >> 0" fail
     when no variable existed yet (shift >= VarUsed with VarUsed == 0). */
  if(f == bddnull) return bddnull;
  if(shift == 0) return bddcopy(f);
  if(shift >= VarUsed)
    err("bddrshift: Invalid shift", shift, ExceptionType::OutOfRange);
  check_node("bddrshift", f);
  if(B_CST(f)) return f;

  /* tells bddnewvaroflev() that there may be order-dependent cache entries */
  ShiftCacheUsed = 1;
  return apply(f, (bddp)shift, BC_RSHIFT, 0);
}

/* ------- ZDD operations ------- */

bddp    bddoffset(bddp f, bddvar v)
{
  check_var("bddoffset", v);
  if(f == bddnull) return bddnull;
  check_zdd("bddoffset", f);
  if(B_CST(f)) return f;
  return apply(f, (bddp)v, BC_OFFSET, 0);
}

bddp    bddonset0(bddp f, bddvar v)
{
  check_var("bddonset0", v);
  if(f == bddnull) return bddnull;
  check_zdd("bddonset0", f);
  if(B_CST(f)) return bddfalse;
  return apply(f, (bddp)v, BC_ONSET, 0);
}

bddp    bddonset(bddp f, bddvar v)
{
  bddp g, h;

  g = bddonset0(f, v);
  /* bddchange() throws when it runs out of memory, and the intermediate g
     would then keep its node referenced for the rest of the run: bddgc()
     could not collect it, which is the opposite of what a caller that
     catches BDDOutOfMemoryException and retries needs. */
  try { h = bddchange(g, v); }
  catch(...) { bddfree(g); throw; }
  bddfree(g);
  return h;
}

bddp    bddchange(bddp f, bddvar v)
{
  check_var("bddchange", v);
  if(f == bddnull) return bddnull;
  check_zdd("bddchange", f);
  return apply(f, (bddp)v, BC_CHANGE, 0);
}

bddp bddintersec(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_zdd("bddintersec", f);
  check_zdd("bddintersec", g);
  return apply(f, g, BC_INTERSEC, 0);
}

bddp bddunion(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_zdd("bddunion", f);
  check_zdd("bddunion", g);
  return apply(f, g, BC_UNION, 0);
}

bddp bddsubtract(bddp f, bddp g)
{
  if(f == bddnull || g == bddnull) return bddnull;
  check_zdd("bddsubtract", f);
  check_zdd("bddsubtract", g);
  return apply(f, g, BC_SUBTRACT, 0);
}

bddp bddcard(bddp f)
{
  if(f == bddnull) return 0;
  check_zdd("bddcard", f);
  if(B_CST(f)) return (f == bddfalse)? 0: 1;
  return apply(f, bddfalse, BC_CARD, 0);
}

bddp bddlit(bddp f)
{
  if(f == bddnull) return 0;
  check_zdd("bddlit", f);
  if(B_CST(f)) return 0;
  return apply(f, bddfalse, BC_LIT, 0);
}

bddp bddlen(bddp f)
{
  if(f == bddnull) return 0;
  check_zdd("bddlen", f);
  if(B_CST(f)) return 0;
  return apply(f, bddfalse, BC_LEN, 0);
}

char *bddcardmp16(bddp f, char *s)
{
  int i, j, k, nz;
  struct B_MP mp;
  bddp h, d;

  mp.len = 1;
  if(f == bddnull) mp.word[0] = 0;
  else
  {
    check_zdd("bddcardmp16", f);
    if(B_CST(f)) mp.word[0] = (f == bddtrue)? 1: 0;
    else
    {
      MPAllocFailSize = 0;
      MPCountOverflowed = 0;
      h = apply(B_ABS(f), bddfalse, BC_CARD2, 0);
      if(h == B_MP_NULL)
      {
        /* Out of memory while counting: the documented contract is to raise
           BDDOutOfMemoryException.  A B_MP_NULL without a recorded allocation
           failure means the multi-precision table index space is exhausted,
           which keeps its historical behaviour of storing an empty string. */
        if(MPAllocFailSize != 0)
        {
          bddp failsize = MPAllocFailSize;
          MPAllocFailSize = 0;
          err("bddcardmp16: not enough memory for mp table", failsize,
              ExceptionType::OutOfMemory);
        }
        if(MPCountOverflowed)
        {
          MPCountOverflowed = 0;
          err("bddcardmp16: cardinality does not fit in B_MP_LMAX words",
              B_MP_LMAX, ExceptionType::OutOfRange);
        }
        mp.len = 0;
      }
      else
      {
        mp.word[0] = B_NEG(f)? 1: 0;
        if(mp_add(&mp, h))
          err("bddcardmp16: cardinality does not fit in B_MP_LMAX words",
              B_MP_LMAX, ExceptionType::OutOfRange);
      }
    }
  }
  if(!s) s = B_MALLOC(char, mp.len*sizeof(bddp)*2+1);
  if(!s)
    err("bddcardmp16: memory allocation failed", mp.len*sizeof(bddp)*2+1,
        ExceptionType::OutOfMemory);
  k = 0;
  nz = 0;
  for(i=mp.len-1; i>=0; i--)
    for(j=sizeof(bddp)*2-1; j>=0; j--)
    {
      d = (mp.word[i] >> (j*4) ) & 15;
      if(d) nz = 1;
      if(nz) s[k++] = "0123456789ABCDEF"[d];
    }
  if(!nz && mp.len) s[k++] = '0';
  s[k++] = 0;

#ifdef DEBUG
  for(i=0; i<B_MP_LMAX; i++)
  {
    printf("%d: ", i);
    printf(B_BDDP_FD, mptable[i].size);
    printf("\n");
  }
#endif

  return s;
}

int bddisbdd(bddp f)
{
  if(f == bddnull) return 0;
  if(B_CST(f)) return 1;
  if(B_BAD_NODE(f)) bad_operand("bddisbdd", "Invalid bddp", f);
  return (B_Z_NP(B_NP(f)) ? 0 : 1);
}

int bddiszdd(bddp f)
{
  if(f == bddnull) return 0;
  if(B_CST(f)) return 1;
  if(B_BAD_NODE(f)) bad_operand("bddiszdd", "Invalid bddp", f);
  return (B_Z_NP(B_NP(f)) ? 1 : 0);
}

// for compatibility
int bddiszbdd(bddp f)
{
  return bddiszdd(f);
}

/* ------- SeqBDD operations ------- */

bddp    bddpush(bddp f, bddvar v)
/* The node (v, 0, f) of a sequence BDD.  The level of v is deliberately not
   compared with the top level of f: a SeqBDD repeats variables along a
   path, in any order.  f itself is validated like every other operand --
   this used to be the one entry point that touched its operand unchecked,
   and a BDD node under a ZDD node makes a diagram no operation can read. */
{
  struct B_NodeTable *fp;
  bddp h;

  /* Check operands */
  check_var("bddpush", v);
  if(f == bddnull) return bddnull;
  check_zdd("bddpush", f);

  if(!B_CST(f)) { fp = B_NP(f); B_RFC_INC_NP(fp); }
  /* getnode() reports memory exhaustion by exception; release the reference
     taken above instead of leaking it */
  try { h = getzddp(v, bddfalse, f); }
  catch(...) { bddfree(f); throw; }
  return h;
}

/* ------- Configuration ------- */

void bddsetcacheratio(double cacheRatio)
/* Set cache size ratio (must be power of 2: ..., 0.25, 0.5, 1, 2, 4, ...) */
{
  double oldRatio = CacheRatio;

  /* throw an exception if cacheRatio is illegal */
  setcacheratiovalue(cacheRatio);
  if (!allocatecache()) {
    /* the cache is still the old one, so the ratio it reports has to be the
       old one as well */
    CacheRatio = oldRatio;
    err("bddsetcacheratio: memory allocation failed", 0, ExceptionType::OutOfMemory);
  }
}

double bddgetcacheratio(void)
/* Get current cache size ratio */
{
  return CacheRatio;
}

void bddsetgcthreshold(bddp threshold)
/* Set GC threshold - minimum nodes that must be freed for successful GC */
{
  GCThreshold = threshold;
}

bddp bddgetgcthreshold(void)
/* Get current GC threshold */
{
  return GCThreshold;
}

} // namespace sapporobdd
