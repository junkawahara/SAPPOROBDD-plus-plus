/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Node table for drawing)             *
 ****************************************/

#ifndef BDDXC_REORDER_H
#define BDDXC_REORDER_H

#include "bddc.h"
#include "train.h"

namespace sapporobdd {

struct pack {
  bddp  node;
  bddp  left;
  int   lnumber;
  short llevel;
  int   lattrib;
  bddp  right;
  int   rnumber;
  short rlevel;
  int   rattrib;
};

/* Table of the traversed nodes; a train of trains of pack */
extern train BDDIOpacks;
extern int   BDDIOfunctionlevel;

void ClearTable();
void FreeTable();
void Traverse(bddp node);
void TraverseFunctions(int number, bddp nodes[]);
int  TableMaximumBound();

} // namespace sapporobdd

#endif /* BDDXC_REORDER_H */
