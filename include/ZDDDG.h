/*****************************************
 * ZZDDDG - Decomposition Graph         *
 * (SAPPORO-1.87) - Header               *
 * (C) Shin-ichi MINATO  (May 14, 2021)  *
 *****************************************/

#ifndef _ZDDDG_
#define _ZDDDG_

#include "ZDD.h"

namespace sapporobdd {


#define ZDDDG_InitSize 4

#define ZDDDG_NIL   BDD_MaxNode

#define ZDDDG_C0    1
#define ZDDDG_P1    2
#define ZDDDG_LIT   3
#define ZDDDG_AND   4
#define ZDDDG_OR    5
#define ZDDDG_OTHER 6

class ZDDDG
{
  struct Node;
  struct NodeLink
  {
    bddword _ndx;
    bddword _nxt;
    NodeLink(void){ _ndx = ZDDDG_NIL; _nxt = ZDDDG_NIL; }
  };

  bddword _nodeSize;
  bddword _nodeUsed;
  bddword _linkSize;
  bddword _linkUsed;
  bddword* _hashWheel;
  Node* _nodeA;
  NodeLink* _linkA;
  bddword _c0;
  bddword _c1;

  bddword HashIndex(ZDD);
  bddword NewNdx(ZDD, char);
  int EnlargeNode(void);
  bddword NewLkx(bddword);
  int EnlargeLink(void);
  int LinkNodes(bddword, bddword);
  int Merge3(bddword, bddword, bddword);
  ZDD Func0(ZDD, ZDD);
  ZDD Func1(ZDD, ZDD);
  int LinkNodesC3(bddword, bddword);
  void Print0(bddword);
  bddword Merge(ZDD, bddword, bddword);
  bddword ReferNdx(ZDD);
  int PhaseSweep(bddword);
  void MarkSweep(bddword);
  void MarkSweepR(bddword);
  int Mark1(bddword);
  void Mark2R(bddword);
  int MarkChkR(bddword);
  void Mark3R(bddword);
  bddword Mark4R(bddword, bddword, bddword);
  bddword Mark5R(bddword, bddword, bddword);
  void Mark6R(bddword, bddword);
  bddword AppendR(bddword, int, bddword, bddword);

  struct Node
  {
    bddword _lkx;
    ZDD _f;
    bddword _ndxP;
    char _type;  // NULL, C1, P1, LIT, OR, XOR, OTHER
    char _mark;
    Node(void);
    Node(ZDD, char);
  };

public:
  ZDDDG(void);
  ~ZDDDG(void);
  void Clear(void);
  bddword NodeUsed(void);
  int PrintDecomp(ZDD);
  bddword Decomp(ZDD);

  friend class ZDDDG_Tag;
};

typedef ZDDDG ZBDDDG; // for backward compatibility

class ZDDDG_Tag
{
  ZDDDG* _dg;
  bddword _ndx;
  bddword _lkx;
public:
  ZDDDG_Tag(void);
  int Set(ZDDDG *, bddword);
  bddword TopNdx(void);
  bddword NextNdx(void);
  char Type(void);
  ZDD Func(void);
};

typedef ZDDDG_Tag ZBDDDG_Tag; // for backward compatibility

} // namespace sapporobdd

#endif // _ZDDDG_
