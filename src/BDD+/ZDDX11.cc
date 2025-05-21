/****************************************
 * ZDD+ Manipulator (SAPPORO-1.55)     *
 * (Graphic methods)                    *
 * (C) Shin-ichi MINATO (Dec. 11, 2012) *
 ****************************************/

#include "ZDD.h"

namespace sapporobdd {

void ZDD::XPrint() const
{
	bddgraph(_zdd);
}

void ZDDV::XPrint() const
{
	int len = Last() + 1;
	bddword* bddv = new bddword[len];
	for(int i=0; i<len; i++) bddv[i] = GetZDD(i).GetID(); 
	bddvgraph(bddv, len);
	delete[] bddv;
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

