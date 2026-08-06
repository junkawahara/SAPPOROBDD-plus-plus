/*****************************************
*  BDD Package (SAPPORO-1.94)   - I/O   *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"

namespace sapporobdd {

#define IMPORTHASH(x) ((((x)>>1)^((x)<<8)^((x)<<16)) & (hashsize-1))

void fprintf_check(FILE *strm, const char *format, ...)
{
  if (strm == NULL) {
    throw BDDFileFormatException("Output stream is null", 0);
  }
  if (format == NULL) {
    throw BDDFileFormatException("Format string is null", 0);
  }

  va_list args;
  va_start(args, format);
  if (vfprintf(strm, format, args) < 0) {
    throw BDDFileFormatException("Error writing to file", 0);
  }
  va_end(args);
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
  int n, i, lev, lev0;

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
      err("bddvexport: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
    lev0 = bddlevofvar(bddtop(p[i]));
    if(lev0 > lev) lev = lev0;
  }

  fprintf_check(strm, "_i %d\n_o %d\n_n ", lev, n);
  fprintf_check(strm, B_BDDP_FD, bddvsize(p, n));
  fprintf_check(strm, "\n");

  /* Put internal nodes */
  for(i=0; i<n; i++) export_static(strm, p[i]);
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

  /* Check indexes */
  if(f == bddnull) { printf("RT = NULL\n\n"); return; }
  if(!B_CST(f)&&
     ((fp=B_NP(f))>=Node+NodeSpc || fp->varrfc==0))
      err("bdddump: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  /* Dump nodes */
  dump(f);
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
  int i;

  /* Check operands */
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull) return;
    if(!B_CST(p[i])&&
       ((fp=B_NP(p[i]))>=Node+NodeSpc || fp->varrfc==0))
      err("bddvdump: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
  }

  /* Dump nodes */
  for(i=0; i<n; i++) if(p[i] != bddnull) dump(p[i]);
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

int import(FILE *strm, bddp *p, int lim, int z)
{
  int n, m, v, i, lev, var, inv, e;
  bddp n_nd, ix, f, f0, f1, nd, nd0, nd1, hashsize, ixx;
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
  n = strtol(s, 0, 10);
  while(n > (int)bddvarused()) bddnewvar();

  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  if(strcmp(s, "_o") != 0) throw BDDFileFormatException("Import error: Expected '_o' marker", 0);
  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  m = strtol(s, 0, 10);

  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  if(strcmp(s, "_n") != 0) throw BDDFileFormatException("Import error: Expected '_n' marker", 0);
  v = fscanf(strm, "%255s", s);
  if(v == EOF) throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  n_nd = B_STRTOI(s, 0, 10);

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

  e = 0;
  for(ix=0; ix<n_nd; ix++)
  {
    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    nd = B_STRTOI(s, 0, 10);

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    lev = strtol(s, 0, 10);
    var = bddvaroflev(lev);

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; break; }
    if(strcmp(s, "F") == 0) f0 = bddfalse;
    else if(strcmp(s, "T") == 0) f0 = bddtrue;
    else
    {
      nd0 = B_STRTOI(s, 0, 10);

      ixx = IMPORTHASH(nd0);
      while(hash1[ixx] != nd0)
      {
        if(hash1[ixx] == bddnull)
        {
          // Clean up before throwing
          for(bddp j=0; j<hashsize; j++)
            if(hash1[j] != bddnull) bddfree(hash2[j]);
          free(hash2);
          free(hash1);
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        }
        ixx++;
        ixx &= (hashsize-1);
      }
      f0 = bddcopy(hash2[ixx]);
    }

    v = fscanf(strm, "%255s", s);
    if(v == EOF) { e = 1; bddfree(f0); break; }
    if(strcmp(s, "F") == 0) f1 = bddfalse;
    else if(strcmp(s, "T") == 0) f1 = bddtrue;
    else
    {
      nd1 = B_STRTOI(s, 0, 10);
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;

      ixx = IMPORTHASH(nd1);
      while(hash1[ixx] != nd1)
      {
        if(hash1[ixx] == bddnull)
        {
          bddfree(f0);
          // Clean up before throwing
          for(bddp j=0; j<hashsize; j++)
            if(hash1[j] != bddnull) bddfree(hash2[j]);
          free(hash2);
          free(hash1);
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        }
        ixx++;
        ixx &= (hashsize-1);
      }
      f1 = (inv)? bddnot(hash2[ixx]): bddcopy(hash2[ixx]);
    }

    f = (z)? getzddp(var, f0, f1): getbddp(var, f0, f1);
    if(f == bddnull)
    {
      e = 1;
      bddfree(f1);
      bddfree(f0);
      break;
    }

    ixx = IMPORTHASH(nd);
    while(hash1[ixx] != bddnull)
    {
      if(hash1[ixx] == nd)
      {
        // Clean up before throwing
        bddfree(f);
        for(bddp j=0; j<hashsize; j++)
          if(hash1[j] != bddnull) bddfree(hash2[j]);
        free(hash2);
        free(hash1);
        err("bddimport: internal error", ixx, ExceptionType::FileFormat);
      }
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  if(e)
  {
    // Cleanup and throw exception for EOF or other errors
    for(ix=0; ix<hashsize; ix++)
      if(hash1[ix] != bddnull) bddfree(hash2[ix]);
    free(hash2);
    free(hash1);
    throw BDDFileFormatException("Import error: Unexpected end of file or format error", 0);
  }

  for(i=0; i<m; i++)
  {
    if(i >= lim) break;
    v = fscanf(strm, "%255s", s);
    if(v == EOF)
    {
      // Cleanup on error
      for(i--; i>=0; i--) bddfree(p[i]);
      for(ix=0; ix<hashsize; ix++)
        if(hash1[ix] != bddnull) bddfree(hash2[ix]);
      free(hash2);
      free(hash1);
      throw BDDFileFormatException("Import error: Unexpected end of file", 0);
    }
    if(strcmp(s, "F") == 0) p[i] = bddfalse;
    else if(strcmp(s, "T") == 0) p[i] = bddtrue;
    else
    {
      nd = B_STRTOI(s, 0, 10);
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;

      ixx = IMPORTHASH(nd);
      while(hash1[ixx] != nd)
      {
        if(hash1[ixx] == bddnull)
        {
          // Cleanup on error
          for(i--; i>=0; i--) bddfree(p[i]);
          for(ix=0; ix<hashsize; ix++)
            if(hash1[ix] != bddnull) bddfree(hash2[ix]);
          free(hash2);
          free(hash1);
          err("bddimport: internal error", ixx, ExceptionType::FileFormat);
        }
        ixx++;
        ixx &= (hashsize-1);
      }
      p[i] = (inv)? bddnot(hash2[ixx]): bddcopy(hash2[ixx]);
    }
  }
  if(i < lim) p[i] = bddnull;

  /* clear hash table */
  for(ix=0; ix<hashsize; ix++)
    if(hash1[ix] != bddnull) bddfree(hash2[ix]);
  free(hash2);
  free(hash1);

  return 0;
}

} // namespace sapporobdd
