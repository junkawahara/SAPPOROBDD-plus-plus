/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Drawing of the diagram)             *
 ****************************************/

#include "bddxc_internal.h"

#define DEFAULTRADIX 20
#define SPOTRADIX    3
#define CHECKCOUNT   20

namespace sapporobdd {

static int radix = DEFAULTRADIX;

static int top, toe;
static int fore, back;


void SetRadix(int r)
{
  if(r > DEFAULTRADIX)
  {
    radix = DEFAULTRADIX;
  }
  else
  {
    radix = r;
  }
}


int GetRadix()
{
  return radix;
}


void PutNode(short level, int number)
{
  int x, y;

  Center(level, number, &x, &y);
  if(level == BDDIOfunctionlevel)
  {
    Circle(x, y, (unsigned int)radix);
    DrawBoldInt(number + 1, x, y);
  }
  else if(level > 0)
  {
    Circle(x, y, (unsigned int)radix);
    if(radix > MeanFontWidth())
    {
      DrawInt(level, x, y);
    }
  }
  else
  {
    Square(x, y, (unsigned int)radix);
    DrawInt(NameOfLeaf(*(bddp *)TrainIndex((train *)TrainIndex(&BDDIOpacks,
                                                               level),
                                           number)),
            x, y);
  }
}


void LeftEdge(short flevel, int fnumber, short tlevel, int tnumber)
{
  int x0, y0, xf, yf, xp, yp, xt, yt;

  if(flevel > 0)
  {
    Center(flevel, fnumber, &x0, &y0);
    LeftLeg(flevel, fnumber, &xf, &yf);
    LeftFoot(flevel, fnumber, &xp, &yp);
    Head(tlevel, tnumber, &xt, &yt);
    if(xf > xt)
    {
      RightShoulder(tlevel, tnumber, &xt, &yt);
    }
    else if(xf < xt)
    {
      LeftShoulder(tlevel, tnumber, &xt, &yt);
    }
    Curve(x0, y0, xf, yf, xp, yp, xt, yt, (flevel - tlevel != 1));
  }
}


void RightEdge(short flevel, int fnumber, short tlevel, int tnumber)
{
  int x0, y0, xf, yf, xp, yp, xt, yt;

  if(flevel > 0)
  {
    Center(flevel, fnumber, &x0, &y0);
    RightLeg(flevel, fnumber, &xf, &yf);
    RightFoot(flevel, fnumber, &xp, &yp);
    Head(tlevel, tnumber, &xt, &yt);
    if(xf > xt)
    {
      RightShoulder(tlevel, tnumber, &xt, &yt);
    }
    else if(xf < xt)
    {
      LeftShoulder(tlevel, tnumber, &xt, &yt);
    }
    Curve(x0, y0, xf, yf, xp, yp, xt, yt, (flevel - tlevel != 1));
  }
}


void TailEdge(short flevel, int fnumber, short tlevel, int tnumber)
{
  int x1, x2, y1, y2;

  if(flevel > 0)
  {
    Hip(flevel, fnumber, &x1, &y1);
    Head(tlevel, tnumber, &x2, &y2);
    if(x1 > x2)
    {
      RightShoulder(tlevel, tnumber, &x2, &y2);
    }
    else if(x1 < x2)
    {
      LeftShoulder(tlevel, tnumber, &x2, &y2);
    }
    Line(x1, y1, x2, y2);
  }
}


static void NegativPochi(short level, int number)
{
  int x, y;

  if(level != BDDIOfunctionlevel)
  {
    RightLeg(level, number, &x, &y);
  }
  else
  {
    Hip(level, number, &x, &y);
  }
/*  Circle( x, y, SPOTRADIX / 2 );*/
  Circle(x, y, SPOTRADIX);
}


static void LeftNegativPochi(short level, int number)
{
  int x, y;

  if(level != BDDIOfunctionlevel)
  {
    LeftLeg(level, number, &x, &y);
  }
  else
  {
    Hip(level, number, &x, &y);
  }
/*  Circle( x, y, SPOTRADIX / 2 );*/
  Circle(x, y, SPOTRADIX);
}


void ExPochi(short level, int number)
{
  int x, y;

  if(level != BDDIOfunctionlevel)
  {
    RightLeg(level, number, &x, &y);
  }
  else
  {
    Hip(level, number, &x, &y);
  }
  Cross(x, y, SPOTRADIX * 2);
}


void LeftExPochi(short level, int number)
{
  int x, y;

  if(level != BDDIOfunctionlevel)
  {
    LeftLeg(level, number, &x, &y);
  }
  else
  {
    Hip(level, number, &x, &y);
  }
  Cross(x, y, SPOTRADIX * 2);
}


static int HaveTheAttribute(int atr, int name)
{
  return (atr & name) != 0;
}


static void DrawNodes()
{
  int i, j;
  int cnt;
  train *p;
  pack  *pk;

  cnt = 0;
  for(i = toe; i <= top; i++)
  {
    p = (train *)TrainIndex(&BDDIOpacks, i);
    for(j = 0; j < TrainBound(p); j++)
    {
      pk = (pack *)TrainIndex(p, j);
      if(cnt == 0)
      {
        cnt = CHECKCOUNT;
      }
      else
      {
        cnt--;
      }
      PutNode((short)i, j);
      if(i == BDDIOfunctionlevel)
      {
        TailEdge((short)i, j, pk->llevel, pk->lnumber);
        if(HaveTheAttribute(pk->lattrib, NEGATIV)) NegativPochi((short)i, j);
        if(HaveTheAttribute(pk->lattrib, EXCHANGE)) ExPochi((short)i, j);
      }
      else if(i > 0)
      {
        if((AttributeOfEdge(pk->node) & EXCHANGE) != 0)
        {
          RightEdge((short)i, j, pk->llevel, pk->lnumber);
          if(HaveTheAttribute(pk->lattrib, NEGATIV)) NegativPochi((short)i, j);
          if(HaveTheAttribute(pk->lattrib, EXCHANGE)) ExPochi((short)i, j);
          LeftEdge((short)i, j, pk->rlevel, pk->rnumber);
          if(HaveTheAttribute(pk->rattrib, NEGATIV))
            LeftNegativPochi((short)i, j);
          if(HaveTheAttribute(pk->rattrib, EXCHANGE)) LeftExPochi((short)i, j);
        }
        else
        {
          LeftEdge((short)i, j, pk->llevel, pk->lnumber);
          if(HaveTheAttribute(pk->lattrib, NEGATIV))
            LeftNegativPochi((short)i, j);
          if(HaveTheAttribute(pk->lattrib, EXCHANGE)) LeftExPochi((short)i, j);
          RightEdge((short)i, j, pk->rlevel, pk->rnumber);
          if(HaveTheAttribute(pk->rattrib, NEGATIV)) NegativPochi((short)i, j);
          if(HaveTheAttribute(pk->rattrib, EXCHANGE)) ExPochi((short)i, j);
        }
      }
    }
  }
}


void Show()
{
  ResetWindowSize();
  DrawNodes();
}


void Draw(const char *server, const char *fontname)
{
  top = TrainBound(&BDDIOpacks) - 1;
  toe = 0;
  LocationSetMaximumSize(TableMaximumBound(), top);
  if(WindowOpen(server, fontname) != FALSE)
  {
    QueryColor(&fore, &back);
    Wait();
    WindowClose();
  }
}


void BDDgraph0(const char *server, const char *fontname,
               int number, bddp bddps[])
{
  edgemode = 0;
  ClearTable();
  TraverseFunctions(number, bddps);
  Draw(server, fontname);
  FreeTable();
}


void BDDgraph1(const char *server, const char *fontname,
               int number, bddp bddps[])
{
  edgemode = 1;
  ClearTable();
  TraverseFunctions(number, bddps);
  Draw(server, fontname);
  FreeTable();
}


void BDDgraph2(const char *server, const char *fontname,
               int number, bddp bddps[])
{
  edgemode = 2;
  ClearTable();
  TraverseFunctions(number, bddps);
  Draw(server, fontname);
  FreeTable();
}


void SetWindowSize(int x, int y)
{
  BDDIOwidth = (unsigned int)x;
  BDDIOheight = (unsigned int)y;
}


void bddgraph0(bddp f)
{
  if(f != bddnull) BDDgraph0(0, 0, 1, &f);
}


void bddvgraph0(bddp *ptr, int lim)
{
  int n;

  n = 0;
  while(n < lim)
  {
    if(ptr[n] == bddnull) break;
    n++;
  }
  BDDgraph0(0, 0, n, ptr);
}


void bddgraph(bddp f)
{
  if(f != bddnull) BDDgraph1(0, 0, 1, &f);
}


void bddvgraph(bddp *ptr, int lim)
{
  int n;

  n = 0;
  while(n < lim)
  {
    if(ptr[n] == bddnull) break;
    n++;
  }
  BDDgraph1(0, 0, n, ptr);
}

} // namespace sapporobdd
