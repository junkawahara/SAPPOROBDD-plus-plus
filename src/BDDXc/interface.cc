/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Interface to the BDD package)       *
 ****************************************/

#include "bddxc_internal.h"

namespace sapporobdd {

int edgemode;

bddp Strip(bddp n_node)
{
  if(edgemode >= 1) n_node &= ~B_INV_MASK;
  return n_node;
}


int AttributeOfEdge(bddp node)
{
  int attr;

  attr = NORMAL;
  if(edgemode >= 1 && (node & B_INV_MASK)) attr |= NEGATIV;
  return attr;
}


int SameNode(bddp n1, bddp n2)
{
  return (n1 & ~B_INV_MASK) == (n2 & ~B_INV_MASK);
}


short GetLevelOf(bddp node)
{
  return (short)bddlevofvar(bddtop(node));
}


bddp GetLeftPtrOf(bddp node)
{
  bddp ret;

  ret = bddat0(node, bddvaroflev(GetLevelOf(node)));
  bddfree(ret);
  return ret;
}


bddp GetRightPtrOf(bddp node)
{
  bddp ret;

  ret = bddat1(node, bddvaroflev(GetLevelOf(node)));
  bddfree(ret);
  return ret;
}


int NameOfLeaf(bddp leaf)
{
  if(leaf == bddfalse)
  {
    return 0;
  }
  else if(leaf == bddtrue)
  {
    return 1;
  }
  else if(leaf == bddnull)
  {
    return -1;
  }
  else
  {
    fprintf(stderr, "Something is wrong. (%d)\n", (int)leaf);
    return -1;
  }
}

} // namespace sapporobdd
