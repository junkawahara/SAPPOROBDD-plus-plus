/***************************************************
 * Multi-Level ZDDV class (SAPPORO-1.00) - Header *
 * (C) Shin-ichi MINATO  (Aug 6, 2008 )            *
 ***************************************************/

#ifndef _MLZDDV_
#define _MLZDDV_

#include "ZDD.h"

namespace sapporobdd {

class MLZDDV;

class MLZDDV
{
  int _pin;
  int _out;
  int _sin;
  ZDDV _zddv;

public:
  MLZDDV(void);

  MLZDDV(ZDDV& zddv);
  MLZDDV(ZDDV& zddv, int pin, int out);

  ~MLZDDV(void);

  MLZDDV& operator=(const MLZDDV&);

  int N_pin(void);
  int N_out(void);
  int N_sin(void);
  ZDDV GetZDDV(void);
  
  void Print(void);
  
};

typedef MLZDDV MLZBDDV; // for backward compatibility

} // namespace sapporobdd

#endif // _MLZDDV_

