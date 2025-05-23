/****************************************
 * BDD+ Manipulator (SAPPORO-1.21)      *
 * (LCM interface)                      *
 * (C) Shin-ichi MINATO (Feb. 18, 2009) *
 ****************************************/

#include "ZDD.h"

#define BDD_CPP
#include "bddc.h"

namespace sapporobdd {


extern "C"
{
  /***************** Init. and config. ****************/
  extern void   bddlcm1 B_ARG((char *fname, int th, int param));
  extern bddp   bddlcm2 B_ARG(());
  extern void   bddfree B_ARG((bddp f));
}

extern int LCM_Eend;

ZDD ZDD_LCM_A(char *fname, int th)
{
  bddlcm1(fname, th, 0);
  while(LCM_Eend > BDDV_UserTopLev()) BDD_NewVar();
  bddword z = bddlcm2();
  ZDD h = ZDD_ID(z);
  bddfree(z);
  return h;
}

ZDD ZDD_LCM_C(char *fname, int th)
{
  bddlcm1(fname, th, 1);
  while(LCM_Eend > BDDV_UserTopLev()) BDD_NewVar();
  bddword z = bddlcm2();
  ZDD h = ZDD_ID(z);
  bddfree(z);
  return h;
}

ZDD ZDD_LCM_M(char *fname, int th)
{
  bddlcm1(fname, th, 2);
  while(LCM_Eend > BDDV_UserTopLev()) BDD_NewVar();
  bddword z = bddlcm2();
  ZDD h = ZDD_ID(z);
  bddfree(z);
  return h;
}

} // namespace sapporobdd

