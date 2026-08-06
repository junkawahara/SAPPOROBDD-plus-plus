/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Drawing primitives for X11)         *
 ****************************************/

#include "bddxc_internal.h"
#include "x11globals.h"

#define EDGEBLUSH 1

namespace sapporobdd {

void Spark()
{
  int x, y;
  Window tmpw;
  int tmpi;
  unsigned int tmpm;

  XQueryPointer(disp, window,
                &tmpw, &tmpw,
                &tmpi, &tmpi,
                &x, &y,
                &tmpm);
  XDrawLine(disp, window, wingc,
            x - 20, y, x - 10, y);
  XDrawLine(disp, window, wingc,
            x + 10, y, x + 20, y);
  XDrawLine(disp, window, wingc,
            x, y - 20, x, y - 10);
  XDrawLine(disp, window, wingc,
            x, y + 10, x, y + 20);
  XDrawLine(disp, window, wingc,
            x - 20, y - 20, x - 10, y - 10);
  XDrawLine(disp, window, wingc,
            x + 10, y + 10, x + 20, y + 20);
  XDrawLine(disp, window, wingc,
            x - 20, y + 20, x - 10, y + 10);
  XDrawLine(disp, window, wingc,
            x + 10, y - 10, x + 20, y - 20);
}


void DrawInt(int number, int x, int y)
{
  int width, height;
  char letter[15];

  if(DontCrip(x, y))
  {
    snprintf(letter, sizeof(letter), "%d", number);
    width = XTextWidth(fontinfo, letter, (int)strlen(letter));
    height = fontinfo->ascent + fontinfo->descent;
    XDrawImageString(disp, window, wingc,
                     x - width / 2, y - height / 2 + fontinfo->ascent,
                     letter, (int)strlen(letter));
  }
}


void DrawBoldInt(int number, int x, int y)
{
  int width, height;
  char letter[15];

  if(DontCrip(x, y))
  {
    snprintf(letter, sizeof(letter), "%d", number);
    width = XTextWidth(fontinfo, letter, (int)strlen(letter));
    height = fontinfo->ascent + fontinfo->descent;
    XDrawImageString(disp, window, wingc,
                     x - width / 2, y - height / 2 + fontinfo->ascent,
                     letter, (int)strlen(letter));
    XDrawString(disp, window, wingc,
                x - width / 2 + 1, y - height / 2 + fontinfo->ascent,
                letter, (int)strlen(letter));
  }
}


void Circle(int x, int y, unsigned int r)
{
  if(DontCrip(x, y))
  {
    XDrawArc(disp, window, wingc, x - (int)r, y - (int)r, 2 * r, 2 * r,
             0, 23040);
  }
}


void Square(int x, int y, unsigned int r)
{
  if(DontCrip(x, y))
  {
    XDrawRectangle(disp, window, wingc,
                   x - (int)r, y - (int)r,
                   2 * r, 2 * r);
  }
}


void Curve(int x0, int y0, int xf, int yf,
           int xp, int yp, int xt, int yt, int splineflag)
{
  XPoint list[4];

  if(DontCrip(x0, y0) && DontCrip(xt, yt))
  {
#ifdef SPLINE
    if(splineflag)
    {
      list[0].x = xf;
      list[0].y = yf;
      list[0].flags = VertexCurved | VertexStartClosed;
      list[1].x = xp;
      list[1].y = yp;
      list[1].flags = VertexCurved;
      list[2].x = xt;
      list[2].y = yt;
      list[2].flags = VertexCurved | VertexEndClosed;
      XDraw(window, list, 3, EDGEBLUSH, EDGEBLUSH,
            BlackPixel, GXcopy, AllPlanes);
    }
    else
    {
      XLine(window, xf, yf, xt, yt,
            EDGEBLUSH, EDGEBLUSH, BlackPixel, GXcopy, AllPlanes);
    }
#else
#ifdef LINE
    if(splineflag)
    {
      list[0].x = (short)xf;
      list[0].y = (short)yf;
      list[1].x = (short)xp;
      list[1].y = (short)yp;
      list[2].x = (short)xt;
      list[2].y = (short)yt;
      XDrawLines(disp, window, wingc, list, 3, CoordModeOrigin);
    }
    else
    {
      XDrawLine(disp, window, wingc, xf, yf, xt, yt);
    }
#else
    XLine(window, xf, yf, xt, yt,
          EDGEBLUSH, EDGEBLUSH, BlackPixel, GXcopy, AllPlanes);
#endif
#endif
  }
}


void Cross(int x, int y, int s)
{
  if(DontCrip(x, y))
  {
    XDrawLine(disp, window, wingc, x - s, y - s, x + s, y + s);
    XDrawLine(disp, window, wingc, x - s, y + s, x + s, y - s);
  }
}


void Line(int x1, int y1, int x2, int y2)
{
  if(DontCrip(x1, y1) && DontCrip(x2, y2))
  {
    XDrawLine(disp, window, wingc, x1, y1, x2, y2);
  }
}

} // namespace sapporobdd
