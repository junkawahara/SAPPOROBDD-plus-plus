/*****************************************
*  BDD Package (SAPPORO-1.94)   - I/O   *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"
#include <limits.h>
#include <ctype.h>

namespace sapporobdd {

void fprintf_check(FILE *strm, const char *format, ...)
{
  /* err() rather than a direct throw: a write error surfaces in the middle
     of the export traversal, and err() resets the recursion depth counter
     that the unwound frames never decrement.  A null stream or format is
     an invalid argument of the caller, not a defect of a file, so it is
     not reported as a file format error. */
  if (strm == NULL) {
    err("fprintf_check: Output stream is null", 0, ExceptionType::InvalidBDDValue);
  }
  if (format == NULL) {
    err("fprintf_check: Format string is null", 0, ExceptionType::InvalidBDDValue);
  }

  va_list args;
  va_start(args, format);
  int r = vfprintf(strm, format, args);
  va_end(args);
  if (r < 0) {
    err("fprintf_check: Error writing to file", 0, ExceptionType::FileFormat);
  }
}

/* Writes one node in the export format: its ID (the even bddp of the node),
   its level, and its two edges, "F"/"T" for the constants.  The 0-edge is
   written without its inverter bit, which is the ZDD flag of the node and
   not part of the edge; bddimportz() puts it back. */
static void export_node(bddp f, void *ctx)
{
  FILE *strm = (FILE *)ctx;
  struct B_NodeTable *fp = B_NP(f);
  bddvar v = B_VAR_NP(fp);
  bddp f0 = B_ABS(B_GET_BDDP(fp->f0));
  bddp f1 = B_GET_BDDP(fp->f1);

  fprintf_check(strm, B_BDDP_FD, B_ABS(f));
  fprintf_check(strm, " %u ", Var[v].lev);
  if(f0 == bddfalse) fprintf_check(strm, "F");
  else if(f0 == bddtrue) fprintf_check(strm, "T");
  else fprintf_check(strm, B_BDDP_FD, f0);
  fprintf_check(strm, " ");
  if(f1 == bddfalse) fprintf_check(strm, "F");
  else if(f1 == bddtrue) fprintf_check(strm, "T");
  else fprintf_check(strm, B_BDDP_FD, f1);
  fprintf_check(strm, "\n");
}

void export_static(FILE *strm, bddp f)
/* Writes every node reachable from f, children first, through the shared
   traversal: an iterative one when the recursion budget does not cover the
   levels, so a graph that can be counted and dumped can be exported too. */
{
  traverse_postorder(f, export_node, strm);
}

void bddexport(FILE *strm, bddp *p, int lim)
/* Writes p[0..lim-1] (up to the first bddnull) to strm in the format that
   bddimport() reads.  The file does not record whether the diagrams are
   BDDs or ZDDs; the reader has to know. */
{
  int n, i, recur_count;
  bddvar lev, lev0;
  bddp size;

  /* Check operands.  A negative lim used to write "_o -5", a header the
     importer refuses; a null array with a positive lim was dereferenced. */
  if(strm == NULL)
    err("bddexport: Output stream is null", 0, ExceptionType::InvalidBDDValue);
  if(lim < 0) err("bddexport: Invalid lim", 0, ExceptionType::OutOfRange);
  if(p == 0 && lim > 0)
    err("bddexport: Null array", 0, ExceptionType::InvalidBDDValue);
  n = lim;
  lev = 0;
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull)
    {
      n = i;
      break;
    }
    if(!B_CST(p[i]) && B_BAD_NODE(p[i]))
      err("bddexport: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
    lev0 = bddlevofvar(bddtop(p[i]));
    if(lev0 > lev) lev = lev0;
  }

  /* The node count is taken before the header is written: bddvsize() can
     throw (its iterative traversal allocates), and a header cut off after
     "_n " used to be left in the stream. */
  size = bddvsize(p, n);
  fprintf_check(strm, "_i %u\n_o %d\n_n ", lev, n);
  fprintf_check(strm, B_BDDP_FD, size);
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

  /* A small export stays in the stream buffer, where a write failure only
     shows up at the caller's fclose(); flushing here makes fprintf_check()'s
     promise hold for those as well.  The stream stays the caller's. */
  if(fflush(strm) != 0 || ferror(strm))
    err("bddexport: Error writing to file", 0, ExceptionType::FileFormat);
}

/* The dumps print with unchecked printf() calls; a write failure is picked
   up here once the dump is complete. */
static void dump_check_stdout(const char *who)
{
  if(ferror(stdout))
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: Error writing to stdout", who);
    err(msg, 0, ExceptionType::FileFormat);
  }
}

void bdddump(bddp f)
{
  int recur_count;

  /* Check indexes */
  if(f == bddnull) { printf("RT = NULL\n\n"); return; }
  if(!B_CST(f) && B_BAD_NODE(f))
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
  dump_check_stdout("bdddump");
}

void bddvdump(bddp *p, int n)
{
  int i, recur_count;

  /* Check operands.  A bddnull entry is skipped rather than abandoning the
     whole array: the loops below already handle it element by element and
     print it as "NULL". */
  if(n < 0) err("bddvdump: Invalid lim", 0, ExceptionType::OutOfRange);
  if(p == 0 && n > 0)
    err("bddvdump: Null array", 0, ExceptionType::InvalidBDDValue);
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull) continue;
    if(!B_CST(p[i]) && B_BAD_NODE(p[i]))
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
  dump_check_stdout("bddvdump");
}

/* The user-defined part of the operation cache.  An op below BC_OP_LIMIT
   names an internal operation; the reader used to accept it, and the entry
   of a counting operation then came back as a number that looks like a
   bddp.  Both functions need the cache to exist: B_CACHEKEY over a missing
   cache (before bddinit()) reduces modulo 0 and indexes a null pointer. */
bddp bddrcache(unsigned char op, bddp f, bddp g)
/* Returns the h stored by bddwcache(op, f, g, h), or bddnull if there is no
   such entry.  The value is returned as it is stored: it carries no
   reference of its own (bddwcache() takes none), so it is valid only while
   the caller still holds the diagram it registered, and a caller that wants
   to keep it takes its own reference with bddcopy(). */
{
  struct B_CacheTable *cachep;

  if(op < BC_OP_LIMIT)
    err("bddrcache: op below the user range", op, ExceptionType::OutOfRange);
  if(Cache == 0)
    err("bddrcache: bddinit() has not been called", 0, ExceptionType::InternalError);
  cachep = Cache + B_CACHEKEY(op, f, g);
  if(op == cachep->op &&
     f == B_GET_BDDP(cachep->f) &&
     g == B_GET_BDDP(cachep->g))
    return B_GET_BDDP(cachep->h); /* Hit */
  return bddnull;
}

void bddwcache(unsigned char op, bddp f, bddp g, bddp h)
/* Registers h as the result of the user operation op on (f, g).  No
   reference is taken; bddgc() drops every user entry. */
{
  struct B_CacheTable *cachep;

  if(op < BC_OP_LIMIT)
    err("bddwcache: op below the user range", op, ExceptionType::OutOfRange);
  if(Cache == 0)
    err("bddwcache: bddinit() has not been called", 0, ExceptionType::InternalError);
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

/* ------- import ------- */

/* Token reader.  Reads one whitespace-delimited token of at most
   IMPORT_TOKEN_MAX characters into s and returns 0; returns 1 at the end of
   the file, 2 on a read error, and 3 for a token that is longer than the
   buffer.  fscanf("%255s") used to serve here: it reports a read error as
   EOF, and it splits an over-long token into two, so that a damaged file
   was diagnosed at the wrong place. */
#define IMPORT_TOKEN_MAX 255

static int read_token(FILE *strm, char *s)
{
  int c, n = 0;

  do c = getc(strm); while(c != EOF && isspace(c));
  if(c == EOF) return ferror(strm)? 2: 1;
  while(c != EOF && !isspace(c))
  {
    if(n == IMPORT_TOKEN_MAX) return 3;
    s[n++] = (char)c;
    c = getc(strm);
  }
  if(c == EOF && ferror(strm)) return 2;
  s[n] = 0;
  return 0;
}

[[noreturn]] static void token_error(int r)
{
  switch(r)
  {
  case 1: throw BDDFileFormatException("Import error: Unexpected end of file", 0);
  case 2: throw BDDFileFormatException("Import error: Read error", 0);
  default: throw BDDFileFormatException("Import error: Token too long", 0);
  }
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
   the node table may grow to (a file with more distinct nodes than the node
   limit cannot be imported, so the hash table need not be sized for it).
   Anything above them describes a corrupt file rather than a huge one.
   Node IDs stay below bddnull, which is the empty-slot mark of the hash
   table: a reference equal to it used to stop the search at the first empty
   slot and read a child out of an uninitialized entry, and a definition
   equal to it was invisible to the search. */
#define IMPORT_MAX_LEV \
  (((unsigned long long)bddvarmax < (unsigned long long)INT_MAX)? \
    (unsigned long long)bddvarmax: (unsigned long long)INT_MAX)
#define IMPORT_MAX_OUT ((unsigned long long)INT_MAX)
#define IMPORT_MAX_ID ((unsigned long long)bddnull - 1ULL)

#define IMPORTHASH(x, hashsize) \
  ((((x)>>1)^((x)<<8)^((x)<<16)) & ((hashsize)-1))

/* Looks a node ID up in the import hash table; throws when it is unknown. */
static bddp import_lookup(bddp *hash1, bddp *hash2, bddp hashsize, bddp nd)
{
  bddp ixx = IMPORTHASH(nd, hashsize);
  while(hash1[ixx] != nd)
  {
    if(hash1[ixx] == bddnull)
      err("bddimport: reference to an undefined node", nd, ExceptionType::FileFormat);
    ixx++;
    ixx &= (hashsize-1);
  }
  return hash2[ixx];
}

/* Releases the hash tables and the references they hold. */
static void import_cleanup(bddp *hash1, bddp *hash2, bddp hashsize)
{
  bddp ix;
  for(ix=0; ix<hashsize; ix++)
    if(hash1[ix] != bddnull) bddfree(hash2[ix]);
  free(hash2);
  free(hash1);
}

int import(FILE *strm, bddp *p, int lim, int z)
/* Reads the file written by bddexport() and stores the outputs it declares
   in p[0..lim-1] (as BDDs, or as ZDDs when z is set; the file does not say
   which it holds).  A file with more outputs than lim yields the first lim
   of them; the remaining ones are read over, so the stream is left behind
   the record.  When fewer than lim outputs were read, the entry after the
   last one is set to bddnull.  Returns 0; every failure throws, with the
   references taken so far released.  The levels the header declares are
   created as variables, and they stay. */
{
  int n, m, var, inv, r;
  bddvar lev, levc;
  bddp n_nd, ix, f, nd, nd0, nd1, hashsize, ixx;
  /* Token buffer, one longer than the longest token read_token() accepts. */
  char s[IMPORT_TOKEN_MAX + 1];
  bddp *hash1;
  bddp *hash2;
  unsigned long long uval;

  /* A null stream used to reach fscanf(); bddexport() refuses one. */
  if(strm == NULL)
    throw BDDInvalidBDDValueException("Import error: Input stream is null", 0);
  if(p == 0 && lim > 0)
    throw BDDInvalidBDDValueException("Import error: Null array", 0);

  if((r = read_token(strm, s))) token_error(r);
  if(strcmp(s, "_i") != 0) throw BDDFileFormatException("Import error: Expected '_i' marker", 0);
  if((r = read_token(strm, s))) token_error(r);
  if(read_decimal_token(s, IMPORT_MAX_LEV, &uval))
    throw BDDFileFormatException("Import error: Invalid number of levels", 0);
  n = (int)uval;
  while((bddvar)n > bddvarused()) bddnewvar();

  if((r = read_token(strm, s))) token_error(r);
  if(strcmp(s, "_o") != 0) throw BDDFileFormatException("Import error: Expected '_o' marker", 0);
  if((r = read_token(strm, s))) token_error(r);
  if(read_decimal_token(s, IMPORT_MAX_OUT, &uval))
    throw BDDFileFormatException("Import error: Invalid number of outputs", 0);
  m = (int)uval;

  if((r = read_token(strm, s))) token_error(r);
  if(strcmp(s, "_n") != 0) throw BDDFileFormatException("Import error: Expected '_n' marker", 0);
  if((r = read_token(strm, s))) token_error(r);
  if(read_decimal_token(s, (unsigned long long)NodeLimit, &uval))
    throw BDDFileFormatException("Import error: Invalid number of nodes", 0);
  n_nd = (bddp)uval;

  for(hashsize = 1; hashsize < (n_nd<<1); hashsize <<= 1)
    ; /* empty */
  hash1 = B_MALLOC(bddp, hashsize);
  if(hash1 == 0) throw BDDOutOfMemoryException("Import error: Memory allocation failed", hashsize);
  hash2 = B_MALLOC(bddp, hashsize);
  if(hash2 == 0)
  {
    free(hash1);
    throw BDDOutOfMemoryException("Import error: Memory allocation failed", hashsize);
  }
  for(ix=0; ix<hashsize; ix++) { hash1[ix] = bddnull; hash2[ix] = bddnull; }

  /* From here on, hash1/hash2, the BDDs registered in hash2 and the outputs
     already written to p[] have to be released on every exit path.  Callees
     such as bddvaroflev(), get{b,z}ddp() and err() can all throw, so the body
     is wrapped and the whole cleanup lives in a single handler.
     f0/f1 are owned here until get{b,z}ddp() takes them over (it takes the
     reference to a shared node before it releases them, so they are still
     owned here when it throws); i counts the outputs written to p[]. */
  int i = 0;
  bddp f0 = bddnull;
  bddp f1 = bddnull;
  try
  {

  for(ix=0; ix<n_nd; ix++)
  {
    /* node ID: the even bddp the exporter wrote.  An odd one is refused;
       it used to be registered as it was, where an odd reference (which
       means "inverted") could never find it. */
    if((r = read_token(strm, s))) token_error(r);
    if(read_decimal_token(s, IMPORT_MAX_ID, &uval) || (uval & 1))
      throw BDDFileFormatException("Import error: Invalid node ID", 0);
    nd = (bddp)uval;

    /* level: a level the file made up would make bddvaroflev() throw an
       out-of-range error of its own; report it as the format error it is */
    if((r = read_token(strm, s))) token_error(r);
    if(read_decimal_token(s, (unsigned long long)bddvarused(), &uval)
       || uval == 0)
      throw BDDFileFormatException("Import error: Invalid level", 0);
    lev = (bddvar)uval;
    var = bddvaroflev(lev);

    /* 0-edge: never inverted (an inverted 0-edge is not a BDD edge, and
       the exporter writes the ZDD flag out) */
    if((r = read_token(strm, s))) token_error(r);
    if(strcmp(s, "F") == 0) f0 = bddfalse;
    else if(strcmp(s, "T") == 0) f0 = bddtrue;
    else
    {
      if(read_decimal_token(s, IMPORT_MAX_ID, &uval) || (uval & 1))
        throw BDDFileFormatException("Import error: Invalid 0-edge", 0);
      nd0 = (bddp)uval;
      f0 = bddcopy(import_lookup(hash1, hash2, hashsize, nd0));
    }

    /* 1-edge, with the inverter bit */
    if((r = read_token(strm, s))) token_error(r);
    if(strcmp(s, "F") == 0) f1 = bddfalse;
    else if(strcmp(s, "T") == 0) f1 = bddtrue;
    else
    {
      if(read_decimal_token(s, IMPORT_MAX_ID, &uval))
        throw BDDFileFormatException("Import error: Invalid 1-edge", 0);
      nd1 = (bddp)uval;
      if(nd1 & 1) { inv = 1; nd1 ^= 1; }
      else inv = 0;
      f1 = bddcopy(import_lookup(hash1, hash2, hashsize, nd1));
      if(inv) f1 = B_NOT(f1);
    }

    /* The children have to sit below the node: every operation descends
       by level, and a node whose child is at the same or a higher level
       breaks the canonical form -- two diagrams of the same function then
       fail to compare equal, and bddimply() answers 0 both ways.  Nothing
       used to check this, and getnode() cannot (bddpush() builds such
       nodes on purpose). */
    if(!B_CST(f0))
    {
      levc = Var[B_VAR_NP(B_NP(f0))].lev;
      if(levc >= lev)
        throw BDDFileFormatException("Import error: 0-edge is not below the node", nd);
    }
    if(!B_CST(f1))
    {
      levc = Var[B_VAR_NP(B_NP(f1))].lev;
      if(levc >= lev)
        throw BDDFileFormatException("Import error: 1-edge is not below the node", nd);
    }

    f = (z)? getzddp(var, f0, f1): getbddp(var, f0, f1);
    /* f0 and f1 are now owned by f */
    f0 = bddnull;
    f1 = bddnull;

    ixx = IMPORTHASH(nd, hashsize);
    while(hash1[ixx] != bddnull)
    {
      if(hash1[ixx] == nd)
      {
        bddfree(f); /* not registered in hash2 yet */
        throw BDDFileFormatException("Import error: Node defined twice", nd);
      }
      ixx++;
      ixx &= (hashsize-1);
    }
    hash1[ixx] = nd;
    hash2[ixx] = f;
  }

  for(i=0; i<m; i++)
  {
    if((r = read_token(strm, s))) token_error(r);
    if(i >= lim) continue; /* read over, see the function comment */
    if(strcmp(s, "F") == 0) p[i] = bddfalse;
    else if(strcmp(s, "T") == 0) p[i] = bddtrue;
    else
    {
      if(read_decimal_token(s, IMPORT_MAX_ID, &uval))
        throw BDDFileFormatException("Import error: Invalid node ID", 0);
      nd = (bddp)uval;
      if(nd & 1) { inv = 1; nd ^= 1; }
      else inv = 0;
      p[i] = bddcopy(import_lookup(hash1, hash2, hashsize, nd));
      if(inv) p[i] = B_NOT(p[i]);
    }
  }
  if(i > lim) i = lim;
  if(i < lim) p[i] = bddnull;

  }
  catch(...)
  {
    bddfree(f1);
    bddfree(f0);
    if(i > lim) i = lim;
    for(int j=0; j<i; j++) bddfree(p[j]);
    import_cleanup(hash1, hash2, hashsize);
    throw;
  }

  import_cleanup(hash1, hash2, hashsize);
  return 0;
}

} // namespace sapporobdd
