/****************************************
 * ZDD+ Manipulator (SAPPORO-1.55)     *
 * (Graphic methods)                    *
 * (C) Shin-ichi MINATO (Dec. 11, 2012) *
 ****************************************/

#include <memory>
#include <new>

#include "ZDD.h"

namespace sapporobdd {

void ZDD::XPrint() const
{
	bddgraph(_zdd);
}

void ZDDV::XPrint() const
{
	/* as ZDDV::Size()/Export(): refuse the error vector and -1 components
	   instead of drawing a silently truncated graph, and allocate with
	   nothrow so the failure stays a BDDException */
	if(GetMetaZDD() == -1)
		BDDerr("ZDDV::XPrint(): Error vector.", ExceptionType::InvalidBDDValue);
	int len = Last() + 1;
	std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[len]);
	if(!bddv)
		BDDerr("ZDDV::XPrint(): Memory allocation failed.", ExceptionType::OutOfMemory);
	for(int i=0; i<len; i++)
	{
		ZDD f = GetZDD(i);
		if(f == -1)
			BDDerr("ZDDV::XPrint(): Operation failed.", ExceptionType::OutOfMemory);
		bddv[i] = f.GetID();
	}
	bddvgraph(bddv.get(), len);
}

/* The two methods below used to be commented out, although the manual listed
   ZDD::XPrint0() as part of the interface and BDD/BDDV/CtoI all have their
   own.  They are back, written like XPrint() above: const, refusing the error
   value, and allocating with the nothrow form so that a failure stays a
   BDDException. */
void ZDD::XPrint0() const
{
	bddgraph0(_zdd);
}

void ZDDV::XPrint0() const
{
	if(GetMetaZDD() == -1)
		BDDerr("ZDDV::XPrint0(): Error vector.", ExceptionType::InvalidBDDValue);
	int len = Last() + 1;
	std::unique_ptr<bddword[]> bddv(new(std::nothrow) bddword[len]);
	if(!bddv)
		BDDerr("ZDDV::XPrint0(): Memory allocation failed.", ExceptionType::OutOfMemory);
	for(int i=0; i<len; i++)
	{
		ZDD f = GetZDD(i);
		if(f == -1)
			BDDerr("ZDDV::XPrint0(): Operation failed.", ExceptionType::OutOfMemory);
		bddv[i] = f.GetID();
	}
	bddvgraph0(bddv.get(), len);
}
} // namespace sapporobdd

