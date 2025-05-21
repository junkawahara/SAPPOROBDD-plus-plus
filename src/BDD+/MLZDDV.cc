/*****************************************
 * Multi-Level ZDDV class (SAPPORO-1.91)*
 * (Main part)                           *
 * (C) Shin-ichi MINATO (Sep. 3, 2021)   *
 *****************************************/

#include "MLZDDV.h"

using std::cout;
using std::cerr;

namespace sapporobdd {


//-------------- Class methods of MLZDDV -----------------

MLZDDV::MLZDDV()
{
  _pin = 0;
  _out = 0;
  _sin = 0;
  _zddv = ZDDV();
}

MLZDDV::~MLZDDV() { }

int MLZDDV::N_pin() { return _pin; }
int MLZDDV::N_out() { return _out; }
int MLZDDV::N_sin() { return _sin; }

MLZDDV& MLZDDV::operator=(const MLZDDV& v)
{
  _pin = v._pin;
  _out = v._out;
  _sin = v._sin;
  _zddv = v._zddv;
  return *this;
}

MLZDDV::MLZDDV(ZDDV& zddv)
{
  int pin = BDD_LevOfVar(zddv.Top());
  int out = zddv.Last()+1;
  MLZDDV v = MLZDDV(zddv, pin, out);
  _pin = v._pin;
  _out = v._out;
  _sin = v._sin;
  _zddv = v._zddv;
}

MLZDDV::MLZDDV(ZDDV& zddv, int pin, int out)
{
  if(zddv == ZDDV(-1))
  {
    _pin = 0;
    _out = 0;
    _sin = 0;
    _zddv = zddv;
    return;
  }

  _pin = pin;
  _out = out;
  _sin = 0;
  _zddv = zddv;

  /* check each output as a divisor */
  for(int i=0; i<_out; i++)
  {
    _sin++;
    int plev = _pin + _sin;
    if(plev > BDD_TopLev()) BDD_NewVar();
    ZDD p = _zddv.GetZDD(i);
    int pt = BDD_LevOfVar(p.Top());
    if(p != 0)
    {
      for(int j=0; j<_out; j++)
      {
        if(i != j)
        {
	  ZDD f = _zddv.GetZDD(j);
          int ft = BDD_LevOfVar(f.Top());
	  if(ft >= pt)
	  {
	    ZDD q = f / p;
	    if(q != 0)
	    {
	      int v = BDD_VarOfLev(plev);
	      _zddv -= ZDDV(f, j);
	      f = q.Change(v) + (f % p);
	      if(f == -1) { cerr << "overflow.\n"; exit(1);}
	      _zddv += ZDDV(f, j);
	    }
	  }
        }
      }
    }
  }

  /* extract 0-level kernels */
  for(int i=0; i<_out; i++)
  {
    ZDD f = _zddv.GetZDD(i);
    while(1)
    {
      ZDD p = f.Divisor();
      int pt = BDD_LevOfVar(p.Top());
      if(p.Top() == 0) break;
      if(p == f) break;
      _sin++;
      cout << _sin << " "; cout.flush();
      int plev = _pin + _sin;
      if(plev > BDD_TopLev()) BDD_NewVar();
      _zddv += ZDDV(p, _sin-1);
      int v = BDD_VarOfLev(plev);
      ZDD q = f / p;
      _zddv -= ZDDV(f, i);
      f = q.Change(v) + (f % p);
      if(f == -1) { cerr << "overflow.\n"; exit(1);}
      _zddv += ZDDV(f, i);
      for(int j=0; j<_out; j++)
      {
        if(i != j)
        {
	  f = _zddv.GetZDD(j);
          int ft = BDD_LevOfVar(f.Top());
	  if(ft >= pt)
	  {
	    q = f / p;
	    if(q != 0)
	    {
	      _zddv -= ZDDV(f, j);
	      f = q.Change(v) + (f % p);
              if(f == -1) { cerr << "overflow.\n"; exit(1);}
	      _zddv += ZDDV(f, j);
	    }
	  }
        }
      }
    }
  }
}

void MLZDDV::Print()
{
  cout << "pin:" << _pin << "\n";
  cout << "out:" << _out << "\n";
  cout << "sin:" << _sin << "\n";
  _zddv.Print();
}

ZDDV MLZDDV::GetZDDV()
{
  return _zddv;
}

} // namespace sapporobdd

