/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Node table for drawing)             *
 ****************************************/

#include "bddxc_internal.h"

namespace sapporobdd {

static train table;
train BDDIOpacks;
int   BDDIOfunctionlevel;

static int samenode(const void *a, const void *b)
{
  return ((const pack *)a)->node == *(const bddp *)b;
}


void ClearTable()
{
  TrainReset(&table, (int)sizeof(bddp));
  TrainReset(&BDDIOpacks, (int)sizeof(train));
}


void FreeTable()
{
  int i;

  for(i = 0; i < TrainBound(&BDDIOpacks); i++)
  {
    TrainFree((train *)TrainIndex(&BDDIOpacks, i));
  }
  TrainFree(&table);
  TrainFree(&BDDIOpacks);
}


static void Reserve(bddp node)
{
  TrainLoad(&table, &node);
}


static int IsReserved(bddp node)
{
  if(TrainCheck(&table, &node) == EMPTY) return FALSE;
  return TRUE;
}


void Traverse(bddp node)
{
  bddp  left, right, nudeleft, nuderight;
  short level;
  pack  tmp;
  train tr;

  node = Strip(node);
  if(!IsReserved(node))
  {
    Reserve(node);
    level = GetLevelOf(node);
    tmp.node = node;
    if(level > 0)
    {
      left = GetLeftPtrOf(node);
      nudeleft = Strip(left);
      right = GetRightPtrOf(node);
      nuderight = Strip(right);
      Traverse(left);
      Traverse(right);
      tmp.lattrib = AttributeOfEdge(left);
      tmp.left = nudeleft;
      tmp.llevel = GetLevelOf(nudeleft);
      tmp.lnumber = TrainComp((train *)TrainIndex(&BDDIOpacks, tmp.llevel),
                              &nudeleft, samenode);
      tmp.rattrib = AttributeOfEdge(right);
      tmp.right = nuderight;
      tmp.rlevel = GetLevelOf(nuderight);
      tmp.rnumber = TrainComp((train *)TrainIndex(&BDDIOpacks, tmp.rlevel),
                              &nuderight, samenode);
    }
    while(TrainBound(&BDDIOpacks) <= level)
    {
      TrainReset(&tr, (int)sizeof(pack));
      TrainLoad(&BDDIOpacks, &tr);
    }
    TrainLoad((train *)TrainIndex(&BDDIOpacks, level), &tmp);
  }
}


void TraverseFunctions(int number, bddp nodes[])
{
  int   i;
  train tr;
  pack  id;

  for(i = 0; i < number; i++)
  {
    Traverse(nodes[i]);
  }
  TrainReset(&tr, (int)sizeof(pack));
  for(i = 0; i < number; i++)
  {
    id.node = Strip(nodes[i]);
    id.llevel = id.rlevel = GetLevelOf(nodes[i]);
    id.lattrib = id.rattrib = AttributeOfEdge(nodes[i]);
    id.lnumber = id.rnumber = TrainComp((train *)TrainIndex(&BDDIOpacks,
                                                            id.llevel),
                                        &id.node, samenode);
    TrainLoad(&tr, &id);
  }
  TrainLoad(&BDDIOpacks, &tr);
  BDDIOfunctionlevel = TrainBound(&BDDIOpacks) - 1;
}


int TableMaximumBound()
{
  int i;
  int t, max = 0;

  for(i = 0; i < TrainBound(&BDDIOpacks); i++)
  {
    t = TrainBound((train *)TrainIndex(&BDDIOpacks, i));
    if(t > max)
    {
      max = t;
    }
  }
  return max;
}

} // namespace sapporobdd
