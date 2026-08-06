/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Interface to the BDD package)       *
 ****************************************/

#ifndef BDDXC_INTERFACE_H
#define BDDXC_INTERFACE_H

#include "bddc.h"

namespace sapporobdd {

bddp  Strip(bddp n_node);
int   AttributeOfEdge(bddp node);
int   SameNode(bddp n1, bddp n2);
short GetLevelOf(bddp node);
bddp  GetLeftPtrOf(bddp node);
bddp  GetRightPtrOf(bddp node);
int   NameOfLeaf(bddp leaf);

} // namespace sapporobdd

#endif /* BDDXC_INTERFACE_H */
