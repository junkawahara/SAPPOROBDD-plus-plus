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

/*
void ZDD::XPrint0()
{
	bddgraph0(_zdd);
}

void ZDDV::XPrint0()
{
	int len = Last() + 1;
	bddword* bddv = new bddword[len];
	for(int i=0; i<len; i++) bddv[i] = GetZDD(i).GetID(); 
	bddvgraph0(bddv, len);
	delete[] bddv;
}
*/
} // namespace sapporobdd

