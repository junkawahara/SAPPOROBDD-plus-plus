/****************************************
 * BDD+ Manipulator (SAPPORO-1.55)      *
 * (Graphic methods)                    *
 * (C) Shin-ichi MINATO (Dec. 11, 2012) *
 ****************************************/

#include <memory>
#include <new>

#include "BDD.h"

namespace sapporobdd {

/* as BDDV::Size()/Export(): collect the component IDs without letting a
   failed (-1) component truncate the array silently, and without a plain
   new[] whose std::bad_alloc would bypass every BDDException handler */
static std::unique_ptr<bddword[]> CollectIDs(const BDDV& fv, const char* who)
{
  int len = fv.Len();
  std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[len]);
  if(!bddv && len > 0)
    BDDerr(who, ExceptionType::OutOfMemory);
  for(int i=0; i<len; i++)
  {
    BDD f = fv.GetBDD(i);
    if(f == -1)
      BDDerr(who, ExceptionType::OutOfMemory);
    bddv[i] = f.GetID();
  }
  return bddv;
}

void BDD::XPrint0() const
{
	bddgraph0(_bdd);
}

void BDDV::XPrint0() const
{
	std::unique_ptr<bddword[]> bddv = CollectIDs(*this, "BDDV::XPrint0: Operation failed.");
	bddvgraph0(bddv.get(), _len);
}

void BDD::XPrint() const
{
	bddgraph(_bdd);
}

void BDDV::XPrint() const
{
	std::unique_ptr<bddword[]> bddv = CollectIDs(*this, "BDDV::XPrint: Operation failed.");
	bddvgraph(bddv.get(), _len);
}

} // namespace sapporobdd

