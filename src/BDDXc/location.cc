/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Layout of the nodes)                *
 ****************************************/

#include "bddxc_internal.h"

namespace sapporobdd {

static unsigned int width, height;

static int top, span;
static int xinterval, yinterval;


void LocationResetLocation(unsigned int w, unsigned int h)
{
  int r, rx, ry;

  width = w;
  height = h;
  xinterval = (int)width / (span + 1);
  yinterval = (int)height / (top + 2);
  rx = xinterval / 3;
  ry = yinterval / 3;
  if(rx > ry)
  {
    r = ry;
  }
  else
  {
    r = rx;
  }
  SetRadix(r);
}


void LocationSetMaximumSize(int x, int y)
{
  span = x;
  top = y;
  LocationResetLocation(width, height);
}


void Center(short level, int number, int *x, int *y)
{
  int size;

  size = TrainBound((train *)TrainIndex(&BDDIOpacks, level));
  *x = (
        (int)width
        + (2 * number - (size - 1))
          * (int)width / (size + 1)
       ) / 2;
  *y = yinterval * (top - level + 1);
}


void Head(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx;
  *y = cy - GetRadix();
}


void Top(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx;
  *y = cy - 2 * GetRadix();
}


void LeftHand(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx - GetRadix();
  *y = cy - 2 * GetRadix();
}


void RightHand(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx + GetRadix();
  *y = cy - 2 * GetRadix();
}


void LeftLeg(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx - GetRadix() / 4 * 3;
  *y = cy + GetRadix() / 4 * 3;
}


void RightLeg(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx + GetRadix() / 4 * 3;
  *y = cy + GetRadix() / 4 * 3;
}


void LeftFoot(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx - GetRadix() * 2;
  *y = cy + GetRadix() * 2;
}


void RightFoot(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx + GetRadix() * 2;
  *y = cy + GetRadix() * 2;
}


void Hip(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx;
  *y = cy + GetRadix();
}


void RightShoulder(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx + GetRadix() * 3 / 4;
  if(level != 0)
    *y = cy - GetRadix() * 3 / 4;
  else
    *y = cy - GetRadix();
}


void LeftShoulder(short level, int number, int *x, int *y)
{
  int cx, cy;

  Center(level, number, &cx, &cy);
  *x = cx - GetRadix() * 3 / 4;
  if(level != 0)
    *y = cy - GetRadix() * 3 / 4;
  else
    *y = cy - GetRadix();
}

} // namespace sapporobdd
