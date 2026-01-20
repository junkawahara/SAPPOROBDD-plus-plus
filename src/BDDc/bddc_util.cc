/*****************************************
*  BDD Package (SAPPORO-1.94)   - Util  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"
#include <string>

namespace sapporobdd {

bddp bddused() { return NodeUsed; }

bddp bddsize(bddp f)
/* Returns 0 for bddnull */
{
  bddp num;
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0; /* Constant */
  if((fp=B_NP(f))>=Node+NodeSpc || fp->varrfc == 0)
    err("bddsize: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  num = count(f);
  reset(f);
  return num;
}

bddp bddvsize(bddp *p, int lim)
/* Returns 0 for bddnull */
{
  bddp num;
  struct B_NodeTable *fp;
  int n, i;

  /* Check operand */
  n = lim;
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull)
    {
      n = i;
      break;
    }
    if(!B_CST(p[i])&&
       ((fp=B_NP(p[i]))>=Node+NodeSpc || fp->varrfc==0))
      err("bddvsize: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
  }
  num = 0;
  for(i=0; i<n; i++) num += count(p[i]);
  for(i=0; i<n; i++) reset(p[i]);
  return num;
}

bddp count(bddp f)
{
  bddp nx;
  bddp c;
  struct B_NodeTable *fp;

  /* Check consistensy
  if(f == bddnull)
    err("count: bddnull found", bddnull, ExceptionType::InternalError);
  */

  if(B_CST(f)) return 0; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK) return 0;

  /* Check consistensy
  flev = Var[B_VAR_NP(fp)].lev;
  g = B_GET_BDDP(fp->f0);
  if(!B_CST(g))
  {
    gp = B_NP(g); glev = Var[B_VAR_NP(gp)].lev;
    if(flev <= glev)
        err("count: inconsistensy found at f0", fp-Node, ExceptionType::InternalError);
  }
  g = B_GET_BDDP(fp->f1);
  if(!B_CST(g))
  {
    gp = B_NP(g); glev = Var[B_VAR_NP(gp)].lev;
    if(flev <= glev)
        err("count: inconsistensy found at f1", fp-Node, ExceptionType::InternalError);
  }
  */

  BDD_RECUR_INC;
  c = count(B_GET_BDDP(fp->f0)) + count(B_GET_BDDP(fp->f1)) + 1U ;
  BDD_RECUR_DEC;

  /* Set visit flag */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  return c;
}

void dump(bddp f)
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
  dump(f0);
  dump(f1);
  BDD_RECUR_DEC;

  /* Dump this node */
  printf("N");
  printf(B_BDDP_FD, B_NDX(f));
  printf(" = [V%d(%d), ", v, Var[v].lev);
  if(B_CST(f0)) printf(B_BDDP_FD, B_VAL(f0));
  else { printf("N"); printf(B_BDDP_FD, B_NDX(f0)); }
  printf(", ");
  if(B_NEG(f1)) putchar('~');
  if(B_CST(f1)) printf(B_BDDP_FD, B_ABS(B_VAL(f1)));
  else { printf("N"); printf(B_BDDP_FD, B_NDX(f1)); }
  printf("]");
  if(B_Z_NP(fp)) printf(" #Z");
  printf("\n");
}

void reset(bddp f)
{
  bddp nx;
  struct B_NodeTable *fp;

  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK)
  {
    /* Reset visit flag */
    B_SET_BDDP(fp->nx, nx & ~B_CST_MASK);
    BDD_RECUR_INC;
    reset(B_GET_BDDP(fp->f0));
    reset(B_GET_BDDP(fp->f1));
    BDD_RECUR_DEC;
  }
}

int mp_add(struct B_MP *p, bddp ix)
{
  int len, i;
  bddp c, *wp;

  if(ix == B_MP_NULL) return 1;
  len = B_MP_LEN(ix);
  if(len) wp = mptable[len-1].word+(B_MP_VAL(ix)*len);
  else { wp = &ix; len = 1; }
  while(p->len < len) p->word[p->len++] = 0;

  c = 0;
  for(i=0; i<p->len; i++)
  {
    p->word[i] += c;
    c = (p->word[i] >= c)? 0: 1;
    if(i < len)
    {
      p->word[i] += wp[i];
      c = (p->word[i] >= wp[i])? c: 1;
    }
  }
  if(c)
  {
    if(p->len == B_MP_LMAX)
    {
      for(i=0; i<p->len; i++) p->word[i] = ~((bddp)0);
      return 1;
    }
    p->word[p->len++] = c;
  }
  return 0;
}

int err(const char *msg, bddp num, ExceptionType exType)
{
  const int msg_buf_size = 1024;
  char msg_buf[msg_buf_size];
  snprintf(msg_buf, msg_buf_size, "***** ERROR  %s ( ", msg);
  snprintf(msg_buf, msg_buf_size, B_BDDP_FX, num);
  snprintf(msg_buf, msg_buf_size, " ) *****\n");
  snprintf(msg_buf, msg_buf_size, " NodeLimit : ");
  snprintf(msg_buf, msg_buf_size, B_BDDP_FD, NodeLimit);
  snprintf(msg_buf, msg_buf_size, "\t NodeSpc : ");
  snprintf(msg_buf, msg_buf_size, B_BDDP_FD, NodeSpc);
  snprintf(msg_buf, msg_buf_size, "\t VarSpc : %d", VarSpc);
  snprintf(msg_buf, msg_buf_size, "\n CacheSpc : ");
  snprintf(msg_buf, msg_buf_size, B_BDDP_FD, CacheSpc);
  snprintf(msg_buf, msg_buf_size, "\t NodeUsed : ");
  snprintf(msg_buf, msg_buf_size, B_BDDP_FD, NodeUsed);
  snprintf(msg_buf, msg_buf_size, "\t VarUsed : %d\n", VarUsed);

  std::string errorMsg(msg_buf);

  // Throw appropriate exception based on exType
  switch (exType) {
    case ExceptionType::InvalidBDDValue:
      throw BDDInvalidBDDValueException(errorMsg, num);
    case ExceptionType::OutOfRange:
      throw BDDOutOfRangeException(errorMsg, num);
    case ExceptionType::OutOfMemory:
      throw BDDOutOfMemoryException(errorMsg, num);
    case ExceptionType::FileFormat:
      throw BDDFileFormatException(errorMsg, num);
    case ExceptionType::InternalError:
    default:
      throw BDDInternalErrorException(errorMsg, num);
  }

  return 1; // This line will never be reached but kept for compatibility
}

} // namespace sapporobdd
