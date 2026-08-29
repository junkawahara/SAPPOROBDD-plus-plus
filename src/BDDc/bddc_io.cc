/*****************************************
*  BDD Package (SAPPORO-1.94)   - I/O   *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"
#include <limits.h>

namespace sapporobdd {

#define IMPORTHASH(x) ((((x)>>1)^((x)<<8)^((x)<<16)) & (hashsize-1))

void fprintf_check(FILE *strm, const char *format, ...)
{
  /* err() rather than a direct throw: a write error surfaces in the middle
     of the export_static() recursion, and err() resets the recursion depth
     counter that the unwound frames never decrement. */
  if (strm == NULL) {
    err("fprintf_check: Output stream is null", 0, ExceptionType::FileFormat);
  }
  if (format == NULL) {
    err("fprintf_check: Format string is null", 0, ExceptionType::FileFormat);
  }

  va_list args;
  va_start(args, format);
  int r = vfprintf(strm, format, args);
  va_end(args);
  if (r < 0) {
    err("fprintf_check: Error writing to file", 0, ExceptionType::FileFormat);
  }
}

void export_static(FILE *strm, bddp f)
{
  bddp nx, f0, f1;
  bddvar v;
  struct B_NodeTable *fp;

  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK) return;

  /* Set visit flag */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  /* Dump its subgraphs recursively */
  v = B_VAR_NP(fp);
  f0 = B_GET_BDDP(fp->f0);
  f0 = B_ABS(f0);
  f1 = B_GET_BDDP(fp->f1);
  BDD_RECUR_INC;
  export_static(strm, f0);
  export_static(strm, f1);
  BDD_RECUR_DEC;

  /* Dump this node */
  fprintf_check(strm, B_BDDP_FD, B_ABS(f));
  fprintf_check(strm, " %d ", Var[v].lev);
  if(f0 == bddfalse) fprintf_check(strm, "F");
  else if(f0 == bddtrue) fprintf_check(strm, "T");
  else fprintf_check(strm, B_BDDP_FD, f0);
  fprintf_check(strm, " ");
  if(f1 == bddfalse) fprintf_check(strm, "F");
  else if(f1 == bddtrue) fprintf_check(strm, "T");
  else fprintf_check(strm, B_BDDP_FD, f1);
  fprintf_check(strm, "\n");
}

void bddexport(FILE *strm, bddp *p, int lim)
{
  struct B_NodeTable *fp;
  int n, i, lev, lev0, recur_count;

  /* Check operands */
  n = lim;
  lev = 0;
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull)
    {
      n = i;
      break;
    }
    if(!B_CST(p[i])&&
       ((fp=B_NP(p[i]))>=Node+NodeSpc || fp->varrfc==0))
      err("bddexport: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
    lev0 = bddlevofvar(bddtop(p[i]));
    if(lev0 > lev) lev = lev0;
  }

  fprintf_check(strm, "_i %d\n_o %d\n_n ", lev, n);
  fprintf_check(strm, B_BDDP_FD, bddvsize(p, n));
  fprintf_check(strm, "\n");

  /* Put internal nodes.  A write error (or the recursion limit) makes
     export_static() throw in the middle of the traversal, so the visit flags
     it left in the nx fields have to be cleared before leaving. */
  recur_count = BDD_RecurCount;
  try { for(i=0; i<n; i++) export_static(strm, p[i]); }
  catch(...) { reset_aborted(p, n, recur_count); throw; }
  for(i=0; i<n; i++) reset(p[i]);

  /* Put external node */
  for(i=0; i<n; i++)
  {
    if(p[i] == bddfalse) fprintf_check(strm, "F");
    else if(p[i] == bddtrue) fprintf_check(strm, "T");
    else fprintf_check(strm, B_BDDP_FD, p[i]);
    fprintf_check(strm, "\n");
  }
}

void bdddump(bddp f)
{
  struct B_NodeTable *fp;
  int recur_count;

  /* Check indexes */
  if(f == bddnull) { printf("RT = NULL\n\n"); return; }
  if(!B_CST(f)&&
     ((fp=B_NP(f))>=Node+NodeSpc || fp->varrfc==0))
      err("bdddump: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  /* Dump nodes */
  recur_count = BDD_RecurCount;
  try { dump(f); }
  catch(...) { reset_aborted(&f, 1, recur_count); throw; }
  reset(f);

  /* Dump top node */
  printf("RT = ");
  if(B_NEG(f)) putchar('~');
  if(B_CST(f)) printf(B_BDDP_FD, B_ABS(B_VAL(f)));
  else { printf("N"); printf(B_BDDP_FD, B_NDX(f)); }
  printf("\n\n");
}

void bddvdump(bddp *p, int n)
{
  struct B_NodeTable *fp;
  int i, recur_count;

  /* Check operands.  A bddnull entry is skipped rather than abandoning the
     whole array: the loops below already handle it element by element and
     print it as "NULL". */
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull) continue;
    if(!B_CST(p[i])&&
       ((fp=B_NP(p[i]))>=Node+NodeSpc || fp->varrfc==0))
      err("bddvdump: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
  }

  /* Dump nodes */
  recur_count = BDD_RecurCount;
  try { for(i=0; i<n; i++) if(p[i] != bddnull) dump(p[i]); }
  catch(...) { reset_aborted(p, n, recur_count); throw; }
  for(i=0; i<n; i++) if(p[i] != bddnull) reset(p[i]);

  /* Dump top node */
  for(i=0; i<n; i++)
  {
    printf("RT%d = ", i);
    if(p[i] == bddnull) printf("NULL");
    else
    {
      if(B_NEG(p[i])) putchar('~');
      if(B_CST(p[i])) printf(B_BDDP_FD, B_ABS(B_VAL(p[i])));
      else { printf("N"); printf(B_BDDP_FD, B_NDX(p[i])); }
    }
    putchar('\n');
  }
  printf("\n");
}

bddp bddrcache(unsigned char op, bddp f, bddp g)
{
  struct B_CacheTable *cachep;

  cachep = Cache + B_CACHEKEY(op, f, g);
  if(op == cachep->op &&
     f == B_GET_BDDP(cachep->f) &&
     g == B_GET_BDDP(cachep->g))
    return B_GET_BDDP(cachep->h); /* Hit */
  return bddnull;
}

void bddwcache(unsigned char op, bddp f, bddp g, bddp h)
{
  struct B_CacheTable *cachep;

  if(op < 20) err("bddwcache: op < 20", op, ExceptionType::OutOfRange);
  if(h == bddnull) return;
  cachep = Cache + B_CACHEKEY(op, f, g);
  cachep->op = op;
  B_SET_BDDP(cachep->f, f);
  B_SET_BDDP(cachep->g, g);
  B_SET_BDDP(cachep->h, h);
}

int bddimport(FILE *strm, bddp *p, int lim)
{
  return import(strm, p, lim, 0);
}

int bddimportz(FILE *strm, bddp *p, int lim)
{
  return import(strm, p, lim, 1);
}

/* Parses one decimal token: returns 0 and stores the value when the whole
   token is a decimal number in [0, limit], and 1 otherwise (empty token, a
   sign, junk characters, or too large).  Mirrors ReadDecimal of the BDD+
   layer.  import() used to hand every count and node ID of the file straight
   to strtol/strtoull: a huge "_n" wrapped the "hashsize < n_nd<<1" loop
   below into an endless one, a huge "_i" created variables up to the
   manager's limit one by one before failing, and a corrupt node ID token
   quietly became 0, which is a real node the reference could resolve to. */
static int read_decimal_token(const char *s, unsigned long long limit,
                              unsigned long long *val)
{
  unsigned long long v = 0;
  int i;

  if(s[0] == 0) return 1;
  for(i=0; s[i]; i++)
  {
    unsigned long long d;
    if(s[i] < '0' || s[i] > '9') return 1;
    d = (unsigned long long)(s[i] - '0');
    if(v > (~0ULL - d) / 10ULL) return 1;
    v = v * 10ULL + d;
    if(v > limit) return 1;
  }
  *val = v;
  return 0;
}

/* Bounds for the header counts: no more levels than the manager can hold
   variables, no more outputs than an int can index, and no more nodes than
   the node table can address.  Anything above them describes a corrupt file
   rather than a huge one. */
#define IMPORT_MAX_LEV \
  (((unsigned long long)bddvarmax < (unsigned long long)INT_MAX)? \
    (unsigned long long)bddvarmax: (unsigned long long)INT_MAX)
#define IMPORT_MAX_OUT ((unsigned long long)INT_MAX)
#define IMPORT_MAX_NODE ((unsigned long long)B_NODE_MAX)

int import(FILE *strm, bddp *p, int lim, int z)
{
  int n, m, v, lev, var, inv, e;
  bddp n_nd, ix, f, nd, nd0, nd1, hashsize, ixx;
  /* Token buffer. The field width in the fscanf calls below ("%255s")
     must be kept equal to this size minus one. */
  char s[256];
  bddp *hash1;
  bddp *hash2;

  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  if(strcmp(s, "_i") != 0) throw BDDFileFormatException("Import error: Expected '_i' marker", 0);
  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  {
    unsigned long long uval;
    if(read_decimal_token(s, IMPORT_MAX_LEV, &uval))
      throw BDDFileFormatException("Import error: Invalid number of levels", 0);
    n = (int)uval;
  }
  while(n > (int)bddvarused()) bddnewvar();

  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  if(strcmp(s, "_o") != 0) throw BDDFileFormatException("Import error: Expected '_o' marker", 0);
  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  {
    unsigned long long uval;
    if(read_decimal_token(s, IMPORT_MAX_OUT, &uval))
      throw BDDFileFormatException("Import error: Invalid number of outputs", 0);
    m = (int)uval;
  }

  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  if(strcmp(s, "_n") != 0) throw BDDFileFormatException("Import error: Expected '_n' marker", 0);
  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  {
    unsigned long long uval;
    if(read_decimal_token(s, IMPORT_MAX_NODE, &uval))
      throw BDDFileFormatException("Import error: Invalid number of nodes", 0);
    n_nd = (bddp)uval;
  }

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  hash1 = 0;
  hash1 = B_MALLOC(bddp, hashsize);
  if(hash1 == 0) throw BDDOutOfMemoryException("Import error: Memory allocation failed", hashsize);
  hash2 = 0;
  hash2 = B_MALLOC(bddp, hashsize);
  if(hash2 == 0)
  {
    free(hash1);
    throw BDDOutOfMemoryException("Import error: Memory allocation failed", hashsize);
  }
  for(ix=0; ix<hashsize; ix++) hash1[ix] = bddnull;

  /* From here on, hash1/hash2, the BDDs registered in hash2 and the outputs
     already written to p[] have to be released on every exit path.  Callees
     such as bddvaroflev(), get{b,z}ddp() and err() can all throw, so the body
     is wrapped and the whole cleanup lives in a single handler.
     f0/f1 are owned here until get{b,z}ddp() succeeds and takes them over;
     i counts the outputs already written to p[]. */
  int i = 0;
  bddp f0 = bddnull;
  bddp f1 = bddnull;
  try
  {

  e = 0;
  for(ix=0; ix<n_nd; ix++)
  {
    unsigned long long uval;

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    if(read_decimal_token(s, (unsigned long long)B_VAL_MASK, &uval))
      { e = 1; break; }
    nd = (bddp)uval;

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    /* a level the file made up would make bddvaroflev() throw an
       out-of-range error of its own; report it as the format error it is */
    if(read_decimal_token(s, (unsigned long long)bddvarused(), &uval)
       || uval == 0)
      { e = 1; break; }
    lev = (int)uval;
    var = bddvaroflev(lev);

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f0 = bddfalse;
    else if(strcmp(s, "T") == 0) f0 = bddtrue;
    else
    {
      if(read_decimal_token(s, (unsigned long long)B_VAL_MASK, &uval))
        { e = 1; break; }
      nd0 = (bddp)uval;

      ixx = IMPORTHASH(nd0);
      while(hash1[ixx] != nd0)
      {
        if(hash1[ixx] == bddnull)
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f0 = bddcopy(hash2[ixx]);
    }

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f1 = bddfalse;
    else if(strcmp(s, "T") == 0) f1 = bddtrue;
    else
    {
      if(read_decimal_token(s, (unsigned long long)B_VAL_MASK, &uval))
        { e = 1; break; }
      nd1 = (bddp)uval;
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;

      ixx = IMPORTHASH(nd1);
      while(hash1[ixx] != nd1)
      {
        if(hash1[ixx] == bddnull)
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      f1 = (inv)? bddnot(hash2[ixx]): bddcopy(hash2[ixx]);
    }

    f = (z)? getzddp(var, f0, f1): getbddp(var, f0, f1);
    if(f == bddnull) { e = 1; break; }
    /* f0 and f1 are now owned by f */
    f0 = bddnull;
    f1 = bddnull;

    ixx = IMPORTHASH(nd);
    while(hash1[ixx] != bddnull)
    {
      if(hash1[ixx] == nd)
      {
        bddfree(f); /* not registered in hash2 yet */
        err("bddimport: internal error", ixx, ExceptionType::FileFormat);
      }
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  if(e)
    throw BDDFileFormatException("Import error: Unexpected end of file or format error", 0);

  for(i=0; i<m; i++)
  {
    if(i >= lim) break;
    v = fscanf(strm, "%255s", s);
    if(v == EOF)
      throw BDDFileFormatException("Import error: Unexpected end of file", 0);
    if(strcmp(s, "F") == 0) p[i] = bddfalse;
    else if(strcmp(s, "T") == 0) p[i] = bddtrue;
    else
    {
      unsigned long long uval;
      if(read_decimal_token(s, (unsigned long long)B_VAL_MASK, &uval))
        throw BDDFileFormatException("Import error: Invalid node ID", 0);
      nd = (bddp)uval;
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;

      ixx = IMPORTHASH(nd);
      while(hash1[ixx] != nd)
      {
        if(hash1[ixx] == bddnull)
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        ixx++;
        ixx &= (hashsize-1);
      }
      p[i] = (inv)? bddnot(hash2[ixx]): bddcopy(hash2[ixx]);
    }
  }
  if(i < lim) p[i] = bddnull;

  }
  catch(...)
  {
    bddfree(f1);
    bddfree(f0);
    for(int j=0; j<i; j++) bddfree(p[j]);
    for(ix=0; ix<hashsize; ix++)
      if(hash1[ix] != bddnull) bddfree(hash2[ix]);
    free(hash2);
    free(hash1);
    throw;
  }

  /* clear hash table */
  for(ix=0; ix<hashsize; ix++)
    if(hash1[ix] != bddnull) bddfree(hash2[ix]);
  free(hash2);
  free(hash1);

  return 0;
}

} // namespace sapporobdd
