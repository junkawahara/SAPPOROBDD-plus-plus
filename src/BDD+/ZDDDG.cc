/**********************************************
 * ZDDDG - Decomposition Graph (SAPPORO-1.87)*
 * (C) Shin-ichi MINATO (May 14, 2021)        *
 **********************************************/

#include "ZDDDG.h"

using std::cout;
using std::cerr;

namespace sapporobdd {


ZDDDG_Tag::ZDDDG_Tag()
{
  _dg = 0;
  _ndx = ZDDDG_NIL;
}

int ZDDDG_Tag::Set(ZDDDG* dg, bddword ndx)
{
  _dg = dg;
  _ndx = ndx;
  if(_ndx >= _dg->_nodeUsed) return 1;
  _lkx = _dg->_nodeA[_ndx]._lkx;
  return 0;
}

bddword ZDDDG_Tag::TopNdx()
{
  _lkx = _dg->_nodeA[_ndx]._lkx;
  if(_lkx == ZDDDG_NIL) return ZDDDG_NIL;
  return _dg->_linkA[_lkx]._ndx;
}

bddword ZDDDG_Tag::NextNdx()
{
  _lkx = _dg->_linkA[_lkx]._nxt;
  if(_lkx == ZDDDG_NIL) return ZDDDG_NIL;
  return _dg->_linkA[_lkx]._ndx;
}

char ZDDDG_Tag::Type()
{
  return _dg->_nodeA[_ndx]._type;
}

ZDD ZDDDG_Tag::Func()
{
  return _dg->_nodeA[_ndx]._f;
}

ZDDDG::ZDDDG()
{
  _nodeA = 0;
  _linkA = 0;
  _hashWheel = 0;
  Clear();
}

ZDDDG::~ZDDDG()
{
  delete[] _hashWheel;
  delete[] _linkA;
  delete[] _nodeA;
}

void ZDDDG::Clear()
{
  delete[] _hashWheel;
  delete[] _linkA;
  delete[] _nodeA;
   
  _nodeSize = ZDDDG_InitSize;
  _nodeA = new Node[_nodeSize];
  _nodeUsed = 0;
  _linkSize = ZDDDG_InitSize;
  _linkA = new NodeLink[_linkSize];
  _linkUsed = 0;
  bddword hashSize = _nodeSize << 1;
  _hashWheel = new bddword[hashSize];
  for(bddword i=0; i<hashSize; i++) _hashWheel[i] = ZDDDG_NIL;
  _c0 = NewNdx(0, ZDDDG_C0);
  _c1 = NewNdx(1, ZDDDG_P1);
  LinkNodes(_c1, _c0);
}

bddword ZDDDG::HashIndex(ZDD key)
{
  bddword id = key.GetID();
  bddword hashSize = _nodeSize << 1;
  bddword hash = (id+(id>>10)+(id>>20)) & (hashSize - 1);
  bddword i = hash;
  while(_hashWheel[i] != ZDDDG_NIL)
  {
    ZDD f = _nodeA[_hashWheel[i]]._f;
    if(key == f) return i;
    i++;
    i &= (hashSize -1);
  }
  return i;
}

bddword ZDDDG::NewNdx(ZDD f, char type)
{
  if(_nodeUsed == _nodeSize)
    if(EnlargeNode()) return ZDDDG_NIL;
  bddword ndx = _nodeUsed++;
  _nodeA[ndx]._f = f;
  _nodeA[ndx]._type = type;
  bddword i = HashIndex(f);
  if(_hashWheel[i] != ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::NewNdx(): Duplicate node\n";
    exit(1);
  }
  _hashWheel[i] = ndx;
  return ndx;
}

int ZDDDG::EnlargeNode()
{
  bddword oldHS = _nodeSize << 1;
  Node* oldArray = _nodeA;
  bddword* oldWheel = _hashWheel;

  _nodeSize <<= 2;
  _nodeA = new Node[_nodeSize];
  bddword hashSize = _nodeSize << 1;
  _hashWheel = new bddword[hashSize];
  if(_nodeA == 0 || _hashWheel == 0)
  {
    cerr << "<ERROR> ZDDDG::EnlargeNode(): Memory overflow (";
    cerr << _nodeSize << ")\n";
    return 1;
  }
  for(bddword i=0; i<_nodeUsed; i++)
  {
    _nodeA[i]._lkx = oldArray[i]._lkx;
    _nodeA[i]._f = oldArray[i]._f;
    _nodeA[i]._type = oldArray[i]._type;
    _nodeA[i]._mark = oldArray[i]._mark;
    _nodeA[i]._ndxP = oldArray[i]._ndxP;
  }
  for(bddword i=0; i<hashSize; i++) _hashWheel[i] = ZDDDG_NIL;
  for(bddword i=0; i<oldHS; i++)
  {
    bddword ndx = oldWheel[i];
    if(ndx != ZDDDG_NIL)
    {
      ZDD f = _nodeA[ndx]._f;
      _hashWheel[HashIndex(f)] = ndx;
    }
  }
  delete[] oldArray;
  delete[] oldWheel;
  return 0;
}

bddword ZDDDG::NewLkx(bddword ndx)
{
  if(_linkUsed == _linkSize)
    if(EnlargeLink()) return ZDDDG_NIL;
  bddword lkx = _linkUsed++;
  _linkA[lkx]._ndx = ndx;
  return lkx;
}

int ZDDDG::EnlargeLink()
{
  NodeLink* oldArray = _linkA;

  _linkSize <<= 2;
  _linkA = new NodeLink[_linkSize];
  if(_linkA == 0)
  {
    cerr << "<ERROR> ZDDDG::EnlargeLink(): Memory overflow (";
    cerr << _linkSize << ")\n";
    return 1;
  }
  for(bddword i=0; i<_linkUsed; i++)
  {
    _linkA[i]._ndx = oldArray[i]._ndx;
    _linkA[i]._nxt = oldArray[i]._nxt;
  }
  delete[] oldArray;
  return 0;
}

bddword ZDDDG::ReferNdx(ZDD key)
{
  return _hashWheel[HashIndex(key)];
}

bddword ZDDDG::NodeUsed() { return _nodeUsed; }

ZDDDG::Node::Node()
{
  _lkx = ZDDDG_NIL;
  _f = ZDD(1);
  _type = ZDDDG_C0;
  _mark = 0;
  _ndxP = ZDDDG_NIL;
}

ZDDDG::Node::Node(ZDD f, char type)
{
  _lkx = ZDDDG_NIL;
  _f = f;
  _type = type;
  _mark = 0;
  _ndxP = ZDDDG_NIL;
}

int ZDDDG::PhaseSweep(bddword ndx)
{
  int fin = 0;
  bddword lkx = _nodeA[ndx]._lkx;

  switch(_nodeA[ndx]._type)
  {
  case ZDDDG_P1:
    /* Assertion check*/
    while(lkx != ZDDDG_NIL)
    {
      if(_nodeA[_linkA[lkx]._ndx]._type == ZDDDG_P1)
      {
        cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad structure (P1)\n";
        exit(1);
      }
      lkx = _linkA[lkx]._nxt;
      fin++;
    }
    if(fin != 1)
    {
      cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad fan-in (P1)\n";
      exit(1);
    }
    break;
  case ZDDDG_AND:
    /* Assertion check*/
    while(lkx != ZDDDG_NIL)
    {
      if(_nodeA[_linkA[lkx]._ndx]._type == ZDDDG_AND)
      {
        cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad structure (AND)\n";
        exit(1);
      }
      lkx = _linkA[lkx]._nxt;
      fin++;
    }
    if(fin < 2)
    {
      cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad fan-in (AND)\n";
      exit(1);
    }
    break;

  case ZDDDG_OR:
    /* Assertion check*/
    while(lkx != ZDDDG_NIL)
    {
      if(_nodeA[_linkA[lkx]._ndx]._type == ZDDDG_OR ||
         _nodeA[_linkA[lkx]._ndx]._type == ZDDDG_P1)
      {
        cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad structure (OR)\n";
        exit(1);
      }
      lkx = _linkA[lkx]._nxt;
      fin++;
    }
    if(fin < 2)
    {
      cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad fan-in (OR)\n";
//      exit(1);
    }
    if((_nodeA[ndx]._f & 1) == 1)
    {
      lkx = _nodeA[ndx]._lkx;
      int chk = 0;
      while(lkx != ZDDDG_NIL)
      {
	ZDD f1 = _nodeA[_linkA[lkx]._ndx]._f;
        if((f1 & 1) == 1)
	{
          chk = 1;
	  break;
	}
        lkx = _linkA[lkx]._nxt;
      }
      if(chk == 0)
      {
	ZDD f0 = _nodeA[ndx]._f - 1;
        bddword ndx0 = ReferNdx(f0);
	if(ndx0 == ZDDDG_NIL)
	{
          ndx0 = NewNdx(_nodeA[ndx]._f - 1, ZDDDG_OR);
	  if(ndx0 == ZDDDG_NIL) return 1;
	  _nodeA[ndx0]._lkx = _nodeA[ndx]._lkx;
	}
	_nodeA[ndx]._lkx = ZDDDG_NIL;
	_nodeA[ndx]._type = ZDDDG_P1;
	if(LinkNodes(ndx, ndx0)) return 1;
      }
    }
    break;

  case ZDDDG_OTHER:
    /* Assertion check*/
    while(lkx != ZDDDG_NIL)
    {
      if(_nodeA[_linkA[lkx]._ndx]._type == ZDDDG_P1)
      {
        cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad structure (OTHER)\n";
        exit(1);
      }
      lkx = _linkA[lkx]._nxt;
      fin++;
    }
    if(fin < 2)
    {
      cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad fan-in (OTHER)\n";
      exit(1);
    }

    if((_nodeA[ndx]._f & 1) == 1)
    {
      lkx = _nodeA[ndx]._lkx;
      int chk = 0;
      while(lkx != ZDDDG_NIL)
      {
	ZDD f1 = _nodeA[_linkA[lkx]._ndx]._f;
        if((f1 & 1) == 1 && (Func1(_nodeA[ndx]._f, f1) & 1) == 1)
	{
          chk = 1;
	  break;
	}
        lkx = _linkA[lkx]._nxt;
      }
      if(chk == 0)
      {
	ZDD f0 = _nodeA[ndx]._f - 1;
        bddword ndx0 = ReferNdx(f0);
	if(ndx0 == ZDDDG_NIL)
	{
          ndx0 = NewNdx(_nodeA[ndx]._f - 1, ZDDDG_OTHER);
	  if(ndx0 == ZDDDG_NIL) return 1;
	  _nodeA[ndx0]._lkx = _nodeA[ndx]._lkx;
	}
	_nodeA[ndx]._lkx = ZDDDG_NIL;
	_nodeA[ndx]._type = ZDDDG_P1;
	if(LinkNodes(ndx, ndx0)) return 1;
      }
    }

    break;
  default:
    cerr << "<ERROR> ZDDDG::PhaseSweep(): Bad node type\n";
    exit(1);
  }
  return 0;
}

int ZDDDG::LinkNodes(bddword ndx, bddword ndx2)
{
  if(ndx == ZDDDG_NIL || ndx2 == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::LinkNodes(): Null node\n";
    exit(1);
  }
  bddword lkx = NewLkx(ndx2);
  if(lkx == ZDDDG_NIL) return 1;
  _linkA[lkx]._nxt = _nodeA[ndx]._lkx;
  _nodeA[ndx]._lkx = lkx;

  bddword lkx2 = _linkA[lkx]._nxt;
  while(lkx2 != ZDDDG_NIL)
  {
    bddword ndx3 = _linkA[lkx]._ndx;
    bddword ndx4 = _linkA[lkx2]._ndx;
    ZDD f = _nodeA[ndx3]._f;
    ZDD f2 = _nodeA[ndx4]._f;
    if(f.Top() == f2.Top())
    {
      cerr << "<ERROR> ZDDDG::LinkNodes(): Same VarIndex(";
      cerr << f.Top() << ")\n";
      exit(1);
    }

    if(f.Top() < f2.Top()) break;

    _linkA[lkx]._ndx = ndx4;
    _linkA[lkx2]._ndx = ndx3;

    lkx = lkx2;
    lkx2 = _linkA[lkx2]._nxt;
  }
  return 0;
}

bddword ZDDDG::Decomp(ZDD f)
{
  if(f == 0) return _c0;
  if(f == 1) return _c1;

  bddword ndx = ReferNdx(f);
  if(ndx != ZDDDG_NIL) return ndx;

  int top = f.Top();
  ZDD f0 = f.OffSet(top);
  ZDD f1 = f.OnSet0(top);
  bddword ndx0 = Decomp(f0);
  if(ndx0 == ZDDDG_NIL) return ZDDDG_NIL;
  bddword ndx1 = Decomp(f1);
  if(ndx1 == ZDDDG_NIL) return ZDDDG_NIL;
//PrintDecomp(f0);
//PrintDecomp(f1);
  ndx = Merge(f, ndx0, ndx1);
//PrintDecomp(f); cout << "\n";

  return ndx;
}

void ZDDDG::MarkSweep(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::MarkSweep(): Bad ndx";
    exit(1);
  }
  _nodeA[ndx]._mark = 0;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    _nodeA[_linkA[lkx]._ndx]._mark = 0;
    lkx = _linkA[lkx]._nxt;
  }
}

void ZDDDG::MarkSweepR(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::MarkSweepR(): Bad ndx";
    exit(1);
  }
  _nodeA[ndx]._mark = 0;
  _nodeA[ndx]._ndxP = ZDDDG_NIL;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    MarkSweepR(_linkA[lkx]._ndx);
    lkx = _linkA[lkx]._nxt;
  }
}

int ZDDDG::Mark1(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark1(): Bad ndx";
    exit(1);
  }
  int fin = 0;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    bddword ndx1 = _linkA[lkx]._ndx;
    _nodeA[ndx1]._mark = 1;
    fin++;
    lkx = _linkA[lkx]._nxt;
  }
  return fin;
}

void ZDDDG::Mark2R(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark2R(): Bad ndx";
    exit(1);
  }
  _nodeA[ndx]._mark++;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    Mark2R(_linkA[lkx]._ndx);
    lkx = _linkA[lkx]._nxt;
  }
}

int ZDDDG::MarkChkR(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::MarkChkR(): Bad ndx";
    exit(1);
  }
  if(_nodeA[ndx]._mark != 0) return 1;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    if(MarkChkR(_linkA[lkx]._ndx)) return 1;
    lkx = _linkA[lkx]._nxt;
  }
  return 0;
}

void ZDDDG::Mark3R(bddword ndx)
{
  if(ndx == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark3R(): Bad ndx";
    exit(1);
  }
  if(_nodeA[ndx]._mark == 2) return;

  int cnt1 = 0;  // not decided.
  int cnt2 = 0;  // shared node.
  int cnt3 = 0;  // non-shared node.
  int cnt4 = 0;  // (possibly) partly shared node.

  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    bddword ndt = _linkA[lkx]._ndx;
    Mark3R(ndt);
    switch(_nodeA[ndt]._mark)
    {
    case 1:
      if(_nodeA[ndt]._type != ZDDDG_P1) cnt1++;
      else
      {
        if(_nodeA[_linkA[_nodeA[ndt]._lkx]._ndx]._mark == 2)
	  cnt2++;
	else cnt1++;
      }
      break;
    case 2:
      cnt2++;
      break;
    case 3:
      cnt3++;
      break;
    case 4:
      cnt4++;
      break;
    default:
      break;
    }
    lkx = _linkA[lkx]._nxt;
  }
  if(_nodeA[ndx]._type == ZDDDG_AND || _nodeA[ndx]._type == ZDDDG_OR)
  {
    if(cnt2 >= 1)
    {
      lkx = _nodeA[ndx]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
        _nodeA[ndt]._ndxP = ndx;
        lkx = _linkA[lkx]._nxt;
      }
      _nodeA[ndx]._mark = 4;
      return;
    }
  }
  
  if(cnt1 + cnt2 + cnt4 == 0 && _nodeA[ndx]._mark == 1)
    _nodeA[ndx]._mark = 3;
}

bddword ZDDDG::Mark4R(bddword ndx0, bddword ndx1, bddword ndy0)
{
  if(ndy0 == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark4R(): Bad ndx";
    exit(1);
  }
  if(_nodeA[ndy0]._mark == 1) return ZDDDG_NIL;
  if(ndx0 != ndy0 && MarkChkR(ndy0) == 0)
  {
    if(Func1(_nodeA[ndx0]._f, _nodeA[ndy0]._f) == _nodeA[ndx1]._f)
    {
      _nodeA[ndy0]._mark = 3; // hit at this node.
      return ndy0;
    }
  }
  bddword lkx = _nodeA[ndy0]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    bddword ndt = _linkA[lkx]._ndx;
    bddword ndh = Mark4R(ndx0, ndx1, ndt);
    if(ndh != ZDDDG_NIL)
    {
      _nodeA[ndy0]._mark = 2; // hit at a sub-node.
      return ndh;
    }
    lkx = _linkA[lkx]._nxt;
  }
  return ZDDDG_NIL;
}

bddword ZDDDG::Mark5R(bddword ndx0, bddword ndx1, bddword ndy0)
{
  if(ndy0 == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark5R(): Bad ndx";
    exit(1);
  }
  if(ndx0 != ndy0)
  {
    Mark6R(ndx0, ndy0);
    int fin1 = 0;
    int fin2 = 0;
    bddword lkx = _nodeA[ndx1]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(MarkChkR(ndt) != 0) _nodeA[ndt]._mark = 1;
      else fin2++;
      fin1++;
      lkx = _linkA[lkx]._nxt;
    }
    if(fin2 > 0 && fin1 - fin2 > 0)
    {
      ZDD f1 = 1;
      lkx = _nodeA[ndx1]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
        if(_nodeA[ndt]._mark == 1) f1 *= _nodeA[ndt]._f;
        lkx = _linkA[lkx]._nxt;
      }
      if(Func1(_nodeA[ndx0]._f, _nodeA[ndy0]._f) == f1)
      {
        _nodeA[ndy0]._mark = 3; // hit at this node.
        return ndy0;
      }
    }
    MarkSweepR(ndx0);
    MarkSweep(ndx1);
  }
  bddword lkx = _nodeA[ndy0]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    bddword ndt = _linkA[lkx]._ndx;
    bddword ndh = Mark5R(ndx0, ndx1, ndt);
    if(ndh != ZDDDG_NIL)
    {
      _nodeA[ndy0]._mark = 2; // hit at a sub-node.
      return ndh;
    }
    lkx = _linkA[lkx]._nxt;
  }
  return ZDDDG_NIL;
}

void ZDDDG::Mark6R(bddword ndx, bddword ndy)
{
  if(ndx == ZDDDG_NIL || ndy == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Mark6R(): Bad ndx";
    exit(1);
  }
  if(ndx == ndy) return;
  _nodeA[ndx]._mark++;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    Mark6R(_linkA[lkx]._ndx, ndy);
    lkx = _linkA[lkx]._nxt;
  }
}

bddword ZDDDG::Merge(ZDD f, bddword ndx0, bddword ndx1)
{
  if(ndx0 == ZDDDG_NIL || ndx1 == ZDDDG_NIL)
  {
    cerr << "<ERROR> ZDDDG::Merge(): Null node\n";
    exit(1);
  }

//cout << "[Merge]\n";
  int top = f.Top(); // (top > 0)

  // [LIT] ?
  if(ndx0 == _c0 && ndx1 == _c1)
  {
//cout << "[LIT]\n";
    bddword ndy = ReferNdx(f);
    if(ndy == ZDDDG_NIL)
      ndy = NewNdx(f, ZDDDG_LIT);
    return ndy;
  }

  // [AND] P0==1, P1==1 ?
  if(ndx0 == ndx1 && ndx0 != _c1)
  {
//cout << "[AND] P0==1, P1==1\n";
    bddword ndx = Decomp(ZDD(1).Change(top) + 1);
    if(ndx == ZDDDG_NIL) return ZDDDG_NIL;
    bddword ndy = NewNdx(f, ZDDDG_AND);
    if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
    if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    if(_nodeA[ndx1]._type != ZDDDG_AND)
    {
      if(LinkNodes(ndy, ndx1)) return ZDDDG_NIL;
    }
    else
    {
      bddword lkx = _nodeA[ndx1]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        if(LinkNodes(ndy, _linkA[lkx]._ndx)) return ZDDDG_NIL;
	lkx = _linkA[lkx]._nxt;
      }
    }
    if(PhaseSweep(ndy)) return ZDDDG_NIL;
    return ndy;
  }

  // [AND] P0==0, P1==1 ?
  if(ndx0 == _c0)
  {
//cout << "[AND] P0==0, P1==1\n";
    bddword ndx = Decomp(ZDD(1).Change(top));
    if(ndx == ZDDDG_NIL) return ZDDDG_NIL;
    bddword ndy = NewNdx(f, ZDDDG_AND);
    if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
    if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    if(_nodeA[ndx1]._type != ZDDDG_AND)
    {
      if(LinkNodes(ndy, ndx1)) return ZDDDG_NIL;
    }
    else
    {
      bddword lkx = _nodeA[ndx1]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        if(LinkNodes(ndy, _linkA[lkx]._ndx)) return ZDDDG_NIL;
	lkx = _linkA[lkx]._nxt;
      }
    }
    if(PhaseSweep(ndy)) return ZDDDG_NIL;
    return ndy;
  }

  // [OR] P0==0, P1==1 ?
  if(ndx1 == _c1 && _nodeA[ndx0]._type != ZDDDG_P1)
  {
//cout << "[OR] P0==0, P1==1\n";
    bddword ndx = Decomp(ZDD(1).Change(top));
    if(ndx == ZDDDG_NIL) return ZDDDG_NIL;
    bddword ndy = NewNdx(f, ZDDDG_OR);
    if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
    if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    if(_nodeA[ndx0]._type != ZDDDG_OR)
    {
      if(LinkNodes(ndy, ndx0)) return ZDDDG_NIL;
    }
    else
    {
      bddword lkx = _nodeA[ndx0]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        if(LinkNodes(ndy, _linkA[lkx]._ndx)) return ZDDDG_NIL;
	lkx = _linkA[lkx]._nxt;
      }
    }
    if(PhaseSweep(ndy)) return ZDDDG_NIL;
    return ndy;
  }

  // [AND] General or P0==1 or P1==1 ?
  if(_nodeA[ndx0]._type == ZDDDG_AND &&
     _nodeA[ndx1]._type == ZDDDG_AND)
  {
//cout << "[AND] general\n";
    int fin0 = 0;
    int fin1 = 0;
    int fin2 = 0;
  
    fin0 = Mark1(ndx0);
    bddword lkx = _nodeA[ndx1]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(_nodeA[ndt]._mark == 1)
      {
	fin2++; 
        _nodeA[ndt]._mark = 3;
      }
      lkx = _linkA[lkx]._nxt;
      fin1++;
    }

    if(fin2 > 0)
    {
      bddword ndy0 = bddnull; 
      if(fin0 - fin2 > 1)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx0]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark != 3) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy0 = ReferNdx(g);
        if(ndy0 == ZDDDG_NIL)
        {
          ndy0 = NewNdx(g, ZDDDG_AND);
          if(ndy0 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx0]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark != 3)
	      if(LinkNodes(ndy0, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else if(fin0 - fin2 == 1)
      {
        lkx = _nodeA[ndx0]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark != 3)
	    { ndy0 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }
      else ndy0 = _c1;
  
      bddword ndy1 = bddnull; 
      if(fin1 - fin2 > 1)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark != 3) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy1 = ReferNdx(g);
        if(ndy1 == ZDDDG_NIL)
        {
          ndy1 = NewNdx(g, ZDDDG_AND);
          if(ndy1 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx1]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark != 3)
	      if(LinkNodes(ndy1, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else if(fin1 - fin2 == 1)
      {
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark != 3)
	    { ndy1 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }
      else ndy1 = _c1;
  
      bddword ndy = NewNdx(f, ZDDDG_AND);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      lkx = _nodeA[ndx0]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
	if(_nodeA[ndt]._mark == 3)
          if(LinkNodes(ndy, ndt)) return ZDDDG_NIL;
        lkx = _linkA[lkx]._nxt;
      }
  
      MarkSweep(ndx0);
      if(Merge3(ndy, ndy0, ndy1)) return ZDDDG_NIL;;
      return ndy;
    }
    MarkSweep(ndx0);
  }

  // [AND] P1==1 (special) ? 
  if(_nodeA[ndx0]._type == ZDDDG_AND)
  {
//cout << "[AND] P1==1(special)\n";
    int fin0 = 0;
    int fin2 = 0;
    _nodeA[ndx1]._mark = 1;
    bddword lkx = _nodeA[ndx0]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(_nodeA[ndt]._mark == 1) fin2++;
      fin0++;
      lkx = _linkA[lkx]._nxt;
    }

    if(fin2 > 0)
    {
      bddword ndy0 = bddnull;
      if(fin0 > 2)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx0]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
          if(_nodeA[ndt]._mark == 0) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy0 = ReferNdx(g);
        if(ndy0 == ZDDDG_NIL)
        {
          ndy0 = NewNdx(g, ZDDDG_AND);
          if(ndy0 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx0]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
            if(_nodeA[ndt]._mark == 0)
	      if(LinkNodes(ndy0, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else
      {
        lkx = _nodeA[ndx0]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
          if(_nodeA[ndt]._mark == 0)
	    { ndy0 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }

      bddword ndy = NewNdx(f, ZDDDG_AND);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      if(LinkNodes(ndy, ndx1)) return ZDDDG_NIL;
  
      _nodeA[ndx1]._mark = 0;
      if(Merge3(ndy, ndy0, _c1)) return ZDDDG_NIL;;
      return ndy;
    }
    _nodeA[ndx1]._mark = 0;
  }

  // [AND] P0==1 (special) ? 
  if(_nodeA[ndx1]._type == ZDDDG_AND)
  {
//cout << "[AND] P0==1(special)\n";
    int fin1 = 0;
    int fin2 = 0;
    _nodeA[ndx0]._mark = 1;
    bddword lkx = _nodeA[ndx1]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(_nodeA[ndt]._mark == 1) fin2++;
      fin1++;
      lkx = _linkA[lkx]._nxt;
    }

    if(fin2 > 0)
    {
      bddword ndy1 = bddnull;
      if(fin1 > 2)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
          if(_nodeA[ndt]._mark == 0) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy1 = ReferNdx(g);
        if(ndy1 == ZDDDG_NIL)
        {
          ndy1 = NewNdx(g, ZDDDG_AND);
          if(ndy1 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx1]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
            if(_nodeA[ndt]._mark == 0)
	      if(LinkNodes(ndy1, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else
      {
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
          if(_nodeA[ndt]._mark == 0)
	    { ndy1 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }

      bddword ndy = NewNdx(f, ZDDDG_AND);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      if(LinkNodes(ndy, ndx0)) return ZDDDG_NIL;
  
      _nodeA[ndx0]._mark = 0;
      if(Merge3(ndy, _c1, ndy1)) return ZDDDG_NIL;;
      return ndy;
    }
    _nodeA[ndx0]._mark = 0;
  }

  // [OR] general or P0==0 ?
  bddword ndx00 = ndx0;
  if(_nodeA[ndx00]._type == ZDDDG_P1)
    ndx00 = _linkA[_nodeA[ndx00]._lkx]._ndx;

  if(_nodeA[ndx00]._type == ZDDDG_OR)
  {
//cout << "[OR] (general)\n";
    int fin0 = 0;
    int fin2 = 0;
    Mark2R(ndx1);
    bddword lkx = _nodeA[ndx00]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(MarkChkR(ndt) != 0) _nodeA[ndt]._mark = 1;
      else fin2++;
      fin0++;
      lkx = _linkA[lkx]._nxt;
    }
    if(fin2 > 0)
    {
      bddword ndy0 = bddnull; 
      if(fin0 - fin2 > 1)
      {
        ZDD g = 0;
        lkx = _nodeA[ndx00]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 1) g += _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy0 = ReferNdx(g);
        if(ndy0 == ZDDDG_NIL)
        {
          ndy0 = NewNdx(g, ZDDDG_OR);
          if(ndy0 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx00]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 1)
	      if(LinkNodes(ndy0, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else if(fin0 - fin2 == 1)
      {
        lkx = _nodeA[ndx00]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 1)
	    { ndy0 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }
      else ndy0 = _c0;

      if((f & 1) == 1)
      {
	ZDD g0 = _nodeA[ndy0]._f + 1;
        bddword ndy00 = ReferNdx(g0);
        if(ndy00 == ZDDDG_NIL)
        {
          ndy00 = NewNdx(g0, ZDDDG_P1);
          if(ndy00 == ZDDDG_NIL) return ZDDDG_NIL;
	  if(LinkNodes(ndy00, ndy0)) return ZDDDG_NIL;
        }
	ndy0 = ndy00;
      }

      bddword ndy = NewNdx(f, ZDDDG_OR);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      lkx = _nodeA[ndx00]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
	if(_nodeA[ndt]._mark == 0)
          if(LinkNodes(ndy, ndt)) return ZDDDG_NIL;
        lkx = _linkA[lkx]._nxt;
      }
  
      MarkSweepR(ndx1);
      MarkSweep(ndx00);
      if(Merge3(ndy, ndy0, ndx1)) return ZDDDG_NIL;
      return ndy;
    }

    MarkSweepR(ndx1);
    MarkSweep(ndx00);
  }

  // [OR] P0==0 (special) ?
  if(_nodeA[ndx00]._type != ZDDDG_OR)
  {
//cout << "[OR] P0==0 (special)\n";
    Mark2R(ndx1);
    if(MarkChkR(ndx00) == 0)
    {
      bddword ndy = NewNdx(f, ZDDDG_OR);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      if(LinkNodes(ndy, ndx00)) return ZDDDG_NIL;
  
      MarkSweepR(ndx1);
      if(Merge3(ndy, _c0, ndx1)) return ZDDDG_NIL;;
      return ndy;
    }
    MarkSweepR(ndx1);
  }

  // [OTHER] P1==1 ?
  if(_nodeA[ndx00]._type == ZDDDG_OTHER)
  {
//cout << "[OTHER] P1==1\n";
    Mark2R(ndx1);
    bddword ndh = Mark4R(ndx0, ndx1, ndx0);
    if(ndh != ZDDDG_NIL)
    {
      bddword ndy = AppendR(ndx0, top, ndh, _c1);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      if(PhaseSweep(ndy)) return ZDDDG_NIL;
      MarkSweepR(ndx0);
      MarkSweepR(ndx1);
      return ndy;
    }
    MarkSweepR(ndx0);
    MarkSweepR(ndx1);
  }
      
  // [OTHER] general ?
  if(_nodeA[ndx00]._type == ZDDDG_OTHER &&
     _nodeA[ndx1]._type == ZDDDG_AND)
  {
//cout << "[OTHER] general\n";
    bddword ndh = Mark5R(ndx0, ndx1, ndx0);
    if(ndh != ZDDDG_NIL)
    {
      bddword ndy1 = _c1;
      int fin1 = 0;
      int fin2 = 0;
      ZDD g = 1;
      bddword lkx = _nodeA[ndx1]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
	fin1++;
	if(_nodeA[ndt]._mark == 0)
	{
	  fin2++;
	  g *= _nodeA[ndt]._f;
	  ndy1 = ndt;
	}
        lkx = _linkA[lkx]._nxt;
      }
      if(fin2 >= 2)
      {
        ndy1 = ReferNdx(g);
        if(ndy1 == ZDDDG_NIL)
        {
          ndy1 = NewNdx(g, ZDDDG_AND);
          if(ndy1 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx1]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 0)
	      if(LinkNodes(ndy1, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }

      ZDD f0 = _nodeA[ndx0]._f;
      if(Func1(f0, ndh) - f0 == 0)
      {
        ZDD g0 = _nodeA[ndh]._f + 1;
        bddword ndh0 = ReferNdx(g0);
        if(ndh0 == ZDDDG_NIL)
        {
          ndh0 = NewNdx(g0, ZDDDG_P1);
          if(ndh0 == ZDDDG_NIL) return ZDDDG_NIL;
          if(LinkNodes(ndh0, ndh)) return ZDDDG_NIL;
        }
        ndh = ndh0;
      }

      bddword ndy = AppendR(ndx0, top, ndh, ndy1);
      MarkSweepR(ndx0);
      MarkSweep(ndx1);
      return ndy;
    }
    MarkSweepR(ndx0);
    MarkSweep(ndx1);
  }

  // [P1] ?
  if(_nodeA[ndx0]._type == ZDDDG_P1)
  {
//cout << "[P1]\n";
    bddword ndy = NewNdx(f, ZDDDG_P1);
    if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
    bddword ndy0 = ReferNdx(f - 1);
    if(ndy0 == ZDDDG_NIL)
      ndy0 = Merge(f - 1, _linkA[_nodeA[ndx0]._lkx]._ndx, ndx1);
    if(LinkNodes(ndy, ndy0)) return ZDDDG_NIL;
    return ndy;
  }

  // [OTHER] P0==0 ? 
  if(_nodeA[ndx0]._type == ZDDDG_OTHER
    && _nodeA[ndx1]._type == ZDDDG_AND)
  {
//cout << "[OTHER] P0==0\n";
    Mark2R(ndx0);

    int fin1 = 0;
    int fin2 = 0;
    bddword lkx = _nodeA[ndx1]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      bddword ndt = _linkA[lkx]._ndx;
      if(MarkChkR(ndt) != 0) _nodeA[ndt]._mark = 1;
      else fin2++;
      fin1++;
      lkx = _linkA[lkx]._nxt;
    }
    if(fin2 > 0 && fin1 - fin2 > 0)
    {
      bddword ndy1 = bddnull;
      if(fin2 >= 2)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 0) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy1 = ReferNdx(g);
        if(ndy1 == ZDDDG_NIL)
        {
          ndy1 = NewNdx(g, ZDDDG_AND);
          if(ndy1 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx1]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 0)
	      if(LinkNodes(ndy1, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else // fin2 == 1
      {
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 0)
	    { ndy1 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }

      bddword ndy2 = bddnull;
      if(fin1 - fin2 >= 2)
      {
        ZDD g = 1;
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 1) g *= _nodeA[ndt]._f;
          lkx = _linkA[lkx]._nxt;
        }
        ndy2 = ReferNdx(g);
        if(ndy2 == ZDDDG_NIL)
        {
          ndy2 = NewNdx(g, ZDDDG_AND);
          if(ndy2 == ZDDDG_NIL) return ZDDDG_NIL;
          lkx = _nodeA[ndx1]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 1)
	      if(LinkNodes(ndy2, ndt)) return ZDDDG_NIL;
            lkx = _linkA[lkx]._nxt;
	  }
        }
      }
      else // fin1 - fin2 == 1
      {
        lkx = _nodeA[ndx1]._lkx;
        while(lkx != ZDDDG_NIL)
        {
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 1)
	    { ndy2 = ndt; break; }
          lkx = _linkA[lkx]._nxt;
        }
      }

      MarkSweepR(ndx0);
      MarkSweepR(ndy2);
      Mark2R(ndx0);
      Mark2R(ndy2);
      Mark3R(ndx0);
      Mark3R(ndy2);

      bddword ndy = NewNdx(f, ZDDDG_OTHER);
      if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
      if(LinkNodesC3(ndy, ndx0)) return ZDDDG_NIL;
      if(LinkNodesC3(ndy, ndy2)) return ZDDDG_NIL;
      if(PhaseSweep(ndy)) return ZDDDG_NIL;
      MarkSweepR(ndx0);
      MarkSweepR(ndy2);
      if(Merge3(ndy, _c0, ndy1)) return ZDDDG_NIL;;
      return ndy;
    }
    MarkSweepR(ndx0);
  }
      
  // [OTHER] P1==1, P0==0 ?
//cout << "[OTHER] (default)\n";
  MarkSweepR(ndx0);
  MarkSweepR(ndx1);
  Mark2R(ndx0);
  Mark2R(ndx1);
  Mark3R(ndx0);
  Mark3R(ndx1);
  bddword ndx = Decomp(ZDD(1).Change(top));
  if(ndx == ZDDDG_NIL) return ZDDDG_NIL;

  bddword ndy = NewNdx(f, ZDDDG_OTHER);
  if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
  if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
  if(LinkNodesC3(ndy, ndx0)) return ZDDDG_NIL;
  if(LinkNodesC3(ndy, ndx1)) return ZDDDG_NIL;
  if(PhaseSweep(ndy)) return ZDDDG_NIL;
  MarkSweepR(ndx0);
  MarkSweepR(ndx1);
  return ndy;
}

bddword ZDDDG::AppendR(bddword ndx, int top, bddword ndy0, bddword ndy1)
{
  ZDD f = _nodeA[ndx]._f;
  ZDD h = _nodeA[ndy0]._f;
  ZDD g = f + Func1(f, h) * _nodeA[ndy1]._f.Change(top);
  bddword ndy = ReferNdx(g);
  if(ndy != ZDDDG_NIL) return ndy;
  ndy = NewNdx(g, _nodeA[ndx]._type);
  if(ndy == ZDDDG_NIL) return ZDDDG_NIL;
  int hit = 0;
  bddword lkx = _nodeA[ndx]._lkx;
  while(lkx != ZDDDG_NIL)
  {
    bddword ndt = _linkA[lkx]._ndx;
    if(_nodeA[ndt]._mark <= 1)
    {
      if(LinkNodes(ndy, ndt)) return ZDDDG_NIL;
    }
    else if(_nodeA[ndt]._mark == 2)
    {
      bddword ndr = AppendR(ndt, top, ndy0, ndy1);
      if(ndr == ZDDDG_NIL) return ZDDDG_NIL;
      if(LinkNodes(ndy, ndr)) return ZDDDG_NIL;
    }
    else hit = 1;
    lkx = _linkA[lkx]._nxt;
  }
  if(hit)
  {
    if(Merge3(ndy, ndy0, ndy1)) return ZDDDG_NIL;
  }
  if(PhaseSweep(ndy)) return ZDDDG_NIL;
  return ndy;
}

int ZDDDG::LinkNodesC3(bddword ndy, bddword ndx)
{
  bddword lkx;
  switch(_nodeA[ndx]._mark)
  {
  case 1:
    lkx = _nodeA[ndx]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      if(LinkNodesC3(ndy, _linkA[lkx]._ndx)) return 1;
      lkx = _linkA[lkx]._nxt;
    }
    break;
  case 2:
    if(_nodeA[ndx]._type == ZDDDG_P1)
    {
      if(LinkNodesC3(ndy, _linkA[_nodeA[ndx]._lkx]._ndx)) return 1;
      break;
    }
    if(LinkNodes(ndy, ndx)) return 1;
    _nodeA[ndx]._mark = 9;
    break;
  case 3:
    if(_nodeA[ndx]._type == ZDDDG_P1)
    {
      if(LinkNodesC3(ndy, _linkA[_nodeA[ndx]._lkx]._ndx)) return 1;
      break;
    }
    if(LinkNodes(ndy, ndx)) return 1;
    break;
  case 4:
    {
      lkx = _nodeA[ndx]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        bddword ndt = _linkA[lkx]._ndx;
	bddword ndxP = _nodeA[ndt]._ndxP;
	if(ndxP != ZDDDG_NIL && ndxP != ndx)
	{
	  if(_nodeA[ndxP]._type == ZDDDG_AND &&
	     _nodeA[ndx]._type == ZDDDG_AND )
	  {
	    ZDD g = 1;
	    int fin = 0;
	    bddword lkx2 = lkx;
            while(lkx2 != ZDDDG_NIL)
	    {
              bddword ndt2 = _linkA[lkx2]._ndx;
	      if(ndxP == _nodeA[ndt2]._ndxP)
	      {
	        g *= _nodeA[ndt2]._f;
		fin++;
	      }
              lkx2 = _linkA[lkx2]._nxt;
	    }
	    if(fin >= 2)
	    {
              bddword ndy0 = ReferNdx(g);
              if(ndy0 == ZDDDG_NIL)
	      {
                ndy0 = NewNdx(g, ZDDDG_AND);
                if(ndy0 == ZDDDG_NIL) return 1;
	        lkx2 = lkx;
                while(lkx2 != ZDDDG_NIL)
	        {
                  bddword ndt2 = _linkA[lkx2]._ndx;
	          if(ndxP == _nodeA[ndt2]._ndxP)
		  {
		    if(LinkNodes(ndy0, ndt2)) return 1;
		  }
                  lkx2 = _linkA[lkx2]._nxt;
	        }
	      }
              if(LinkNodes(ndy, ndy0)) return 1;

	      lkx2 = lkx;
              while(lkx2 != ZDDDG_NIL)
	      {
                bddword ndt2 = _linkA[lkx2]._ndx;
	        if(ndxP == _nodeA[ndt2]._ndxP)
	          _nodeA[ndt2]._mark = 9;
                lkx2 = _linkA[lkx2]._nxt;
	      }
	    }
	    lkx2 = lkx;
            while(lkx2 != ZDDDG_NIL)
	    {
              bddword ndt2 = _linkA[lkx2]._ndx;
	      if(ndxP == _nodeA[ndt2]._ndxP)
	        _nodeA[ndt2]._ndxP = ZDDDG_NIL;
              lkx2 = _linkA[lkx2]._nxt;
	    }
	  }

	  if(_nodeA[ndxP]._type == ZDDDG_OR &&
	     _nodeA[ndx]._type == ZDDDG_OR )
	  {
	    ZDD g = 0;
	    int fin = 0;
	    bddword lkx2 = lkx;
            while(lkx2 != ZDDDG_NIL)
	    {
              bddword ndt2 = _linkA[lkx2]._ndx;
	      if(ndxP == _nodeA[ndt2]._ndxP)
	      {
	        g += _nodeA[ndt2]._f;
		fin++;
	      }
              lkx2 = _linkA[lkx2]._nxt;
	    }
	    if(fin >= 2)
	    {
              bddword ndy0 = ReferNdx(g);
              if(ndy0 == ZDDDG_NIL)
	      {
                ndy0 = NewNdx(g, ZDDDG_OR);
                if(ndy0 == ZDDDG_NIL) return 1;
	        lkx2 = lkx;
                while(lkx2 != ZDDDG_NIL)
	        {
                  bddword ndt2 = _linkA[lkx2]._ndx;
	          if(ndxP == _nodeA[ndt2]._ndxP)
		    if(LinkNodes(ndy0, ndt2)) return 1;
                  lkx2 = _linkA[lkx2]._nxt;
	        }
	      }
              if(LinkNodes(ndy, ndy0)) return 1;

	      lkx2 = lkx;
              while(lkx2 != ZDDDG_NIL)
	      {
                bddword ndt2 = _linkA[lkx2]._ndx;
	        if(ndxP == _nodeA[ndt2]._ndxP)
	          _nodeA[ndt2]._mark = 9;
                lkx2 = _linkA[lkx2]._nxt;
	      }
	    }
	    lkx2 = lkx;
            while(lkx2 != ZDDDG_NIL)
	    {
              bddword ndt2 = _linkA[lkx2]._ndx;
	      if(ndxP == _nodeA[ndt2]._ndxP)
	        _nodeA[ndt2]._ndxP = ZDDDG_NIL;
              lkx2 = _linkA[lkx2]._nxt;
	    }
	  }
	}
        lkx = _linkA[lkx]._nxt;
      }

      if(_nodeA[ndx]._type == ZDDDG_AND)
      {
	ZDD g = 1;
	int fin = 0;
        lkx = _nodeA[ndx]._lkx;
        while(lkx != ZDDDG_NIL)
	{
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 3)
	  {
	    g *= _nodeA[ndt]._f;
            fin++;
	  }
          lkx = _linkA[lkx]._nxt;
        }
	if(fin >= 2)
	{
          bddword ndy0 = ReferNdx(g);
          if(ndy0 == ZDDDG_NIL)
	  {
            ndy0 = NewNdx(g, ZDDDG_AND);
            if(ndy0 == ZDDDG_NIL) return 1;
            lkx = _nodeA[ndx]._lkx;
            while(lkx != ZDDDG_NIL)
	    {
              bddword ndt = _linkA[lkx]._ndx;
	      if(_nodeA[ndt]._mark == 3)
	        if(LinkNodes(ndy0, ndt)) return 1;
              lkx = _linkA[lkx]._nxt;
	    }
	  }
          if(LinkNodes(ndy, ndy0)) return 1;
          lkx = _nodeA[ndx]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 3) _nodeA[ndt]._mark = 9;
            lkx = _linkA[lkx]._nxt;
	  }
	}
      }


      if(_nodeA[ndx]._type == ZDDDG_OR)
      {
	ZDD g = 0;
	int fin = 0;
        lkx = _nodeA[ndx]._lkx;
        while(lkx != ZDDDG_NIL)
	{
          bddword ndt = _linkA[lkx]._ndx;
	  if(_nodeA[ndt]._mark == 3)
	  {
	    g += _nodeA[ndt]._f;
            fin++;
	  }
          lkx = _linkA[lkx]._nxt;
        }
	if(fin >= 2)
	{
          bddword ndy0 = ReferNdx(g);
          if(ndy0 == ZDDDG_NIL)
	  {
            ndy0 = NewNdx(g, ZDDDG_OR);
            if(ndy0 == ZDDDG_NIL) return 1;
            lkx = _nodeA[ndx]._lkx;
            while(lkx != ZDDDG_NIL)
	    {
              bddword ndt = _linkA[lkx]._ndx;
	      if(_nodeA[ndt]._mark == 3)
	        if(LinkNodes(ndy0, ndt)) return 1;
              lkx = _linkA[lkx]._nxt;
	    }
	  }
          if(LinkNodes(ndy, ndy0)) return 1;
          lkx = _nodeA[ndx]._lkx;
          while(lkx != ZDDDG_NIL)
	  {
            bddword ndt = _linkA[lkx]._ndx;
	    if(_nodeA[ndt]._mark == 3) _nodeA[ndt]._mark = 9;
            lkx = _linkA[lkx]._nxt;
	  }
	}
      }

    }

    lkx = _nodeA[ndx]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      if(LinkNodesC3(ndy, _linkA[lkx]._ndx)) return 1;
      lkx = _linkA[lkx]._nxt;
    }
    break;
  case 9:
    break;
  default:
    cerr << "<ERROR> ZDDDG::LinkNodesC3(): wrong case (";
    cerr << _nodeA[ndx]._mark << ")\n";
    exit(1);
  }
  return 0;
}

ZDD ZDDDG::Func0(ZDD f, ZDD g)
{
  if(g == 1)
  {
    cerr << "<ERROR> ZDDDG::Func0: g == 1";
    exit(1);
  }
  ZDD h = f;
  while(g != 0)
  {
    int top = g.Top();
    ZDD g0 = g.OffSet(top);
    if(g0 != 1)
    {
      g = g0;
      h = h.OffSet(top);
    }
    else
    {
      g = g.OnSet0(top);
      h = h.OnSet0(top);
    }
  }
  return h;
}

ZDD ZDDDG::Func1(ZDD f, ZDD g)
{
  if(g == 0)
  {
    cerr << "<ERROR> ZDDDG::Func1: g == 0";
    exit(1);
  }
  ZDD h = f;
  while(g != 1)
  {
    int top = g.Top();
    g = g.OnSet0(top);
    h = h.OnSet0(top);
  }
  return h;
}

int ZDDDG::Merge3(bddword ndy, bddword ndy0, bddword ndy1)
{
//cout << "[Merge3]\n";
  int top = _nodeA[ndy]._f.Top();
  ZDD h0 = _nodeA[ndy0]._f;
  ZDD h1 = _nodeA[ndy1]._f;
  ZDD h = h0 + h1.Change(top);
  bddword ndx = ReferNdx(h);
  if(ndx == ZDDDG_NIL) ndx = Merge(h, ndy0, ndy1);
  if(ndx == ZDDDG_NIL) return 1;
  switch(_nodeA[ndy]._type)
  {
  case ZDDDG_AND:
    if(_nodeA[ndx]._type == ZDDDG_AND)
    {
      bddword lkx = _nodeA[ndx]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        if(LinkNodes(ndy, _linkA[lkx]._ndx)) return ZDDDG_NIL;
        lkx = _linkA[lkx]._nxt;
      }
    }
    else if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    break;
  case ZDDDG_OR:
    if(_nodeA[ndx]._type == ZDDDG_P1)
      ndx = _linkA[_nodeA[ndx]._lkx]._ndx;
    if(_nodeA[ndx]._type == ZDDDG_OR)
    {
      bddword lkx = _nodeA[ndx]._lkx;
      while(lkx != ZDDDG_NIL)
      {
        if(LinkNodes(ndy, _linkA[lkx]._ndx)) return ZDDDG_NIL;
        lkx = _linkA[lkx]._nxt;
      }
    }
    else if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    break;
  case ZDDDG_OTHER:
    if(_nodeA[ndx]._type == ZDDDG_P1)
      ndx = _linkA[_nodeA[ndx]._lkx]._ndx;
    if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    break;
  default:
    if(LinkNodes(ndy, ndx)) return ZDDDG_NIL;
    break;
  }
  if(PhaseSweep(ndy)) return ZDDDG_NIL;
  return 0;
}

int ZDDDG::PrintDecomp(ZDD f)
{
  bddword ndx = Decomp(f);
  if(ndx == ZDDDG_NIL) return 1;
  Print0(ndx);
  cout << "\n";
  return 0;
}

void ZDDDG::Print0(bddword ndx)
{
  bddword lkx, lkx2;
  bddword ndx2;
  switch(_nodeA[ndx]._type)
  {
  case ZDDDG_C0:
    cout << "0 ";
    break;
  case ZDDDG_P1:
    cout << "OR( ";
    lkx = _nodeA[ndx]._lkx;
    ndx2 = _linkA[lkx]._ndx;
    if(_nodeA[ndx2]._type != ZDDDG_OR)
      Print0(ndx2);
    else
    {
      lkx2 = _nodeA[ndx2]._lkx;
      while(lkx2 != ZDDDG_NIL)
      {
        Print0(_linkA[lkx2]._ndx);
        lkx2 = _linkA[lkx2]._nxt;
      }
    }
    cout << "1 ) ";
    break;
  case ZDDDG_LIT:
    cout << "x" << _nodeA[ndx]._f.Top() << " ";
    break;
  case ZDDDG_AND:
    cout << "AND( ";
    lkx = _nodeA[ndx]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      Print0(_linkA[lkx]._ndx);
      lkx = _linkA[lkx]._nxt;
    }
    cout << ") ";
    break;
  case ZDDDG_OR:
    cout << "OR( ";
    lkx = _nodeA[ndx]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      Print0(_linkA[lkx]._ndx);
      lkx = _linkA[lkx]._nxt;
    }
    cout << ") ";
    break;
  case ZDDDG_OTHER:
    cout << "[ ";
    lkx = _nodeA[ndx]._lkx;
    while(lkx != ZDDDG_NIL)
    {
      Print0(_linkA[lkx]._ndx);
      lkx = _linkA[lkx]._nxt;
    }
    cout << "] ";
    break;
  default:
    cerr << "<ERROR> ZDDDG::Print0: wrong case (";
    cerr << (int)_nodeA[ndx]._type << ")\n";
    exit(1);
  }
}

} // namespace sapporobdd

