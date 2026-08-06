/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Internal header)                    *
 ****************************************/

#ifndef BDDXC_INTERNAL_H
#define BDDXC_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bddc.h"
#include "BDDException.h"
#include "defs.h"
#include "train.h"
#include "interface.h"
#include "reorder.h"

namespace sapporobdd {

/* ------- graph.cc ------- */
void SetRadix(int r);
int  GetRadix();
void PutNode(short level, int number);
void LeftEdge(short flevel, int fnumber, short tlevel, int tnumber);
void RightEdge(short flevel, int fnumber, short tlevel, int tnumber);
void TailEdge(short flevel, int fnumber, short tlevel, int tnumber);
void ExPochi(short level, int number);
void LeftExPochi(short level, int number);
void Show();
void Draw(const char *server, const char *fontname);
void BDDgraph0(const char *server, const char *fontname,
               int number, bddp bddps[]);
void BDDgraph1(const char *server, const char *fontname,
               int number, bddp bddps[]);
void BDDgraph2(const char *server, const char *fontname,
               int number, bddp bddps[]);
void SetWindowSize(int x, int y);

/* ------- location.cc ------- */
void LocationResetLocation(unsigned int w, unsigned int h);
void LocationSetMaximumSize(int x, int y);
void Center(short level, int number, int *x, int *y);
void Head(short level, int number, int *x, int *y);
void Top(short level, int number, int *x, int *y);
void LeftHand(short level, int number, int *x, int *y);
void RightHand(short level, int number, int *x, int *y);
void LeftLeg(short level, int number, int *x, int *y);
void RightLeg(short level, int number, int *x, int *y);
void LeftFoot(short level, int number, int *x, int *y);
void RightFoot(short level, int number, int *x, int *y);
void Hip(short level, int number, int *x, int *y);
void RightShoulder(short level, int number, int *x, int *y);
void LeftShoulder(short level, int number, int *x, int *y);

/* ------- parts11.cc ------- */
void Spark();
void DrawInt(int number, int x, int y);
void DrawBoldInt(int number, int x, int y);
void Circle(int x, int y, unsigned int r);
void Square(int x, int y, unsigned int r);
void Curve(int x0, int y0, int xf, int yf,
           int xp, int yp, int xt, int yt, int splineflag);
void Cross(int x, int y, int s);
void Line(int x1, int y1, int x2, int y2);

/* ------- window11.cc ------- */
extern unsigned int BDDIOwidth, BDDIOheight;

void SetCrippingWindow(unsigned int x0, unsigned int y0,
                       unsigned int x1, unsigned int y1);
void ResetWindowSize();
void QueryColor(int *foreground, int *background);
int  WindowOpen(const char *server, const char *fontname);
void WindowClose();
void Wait();
int  MeanFontWidth();
int  DontCrip(int x, int y);
int  Interrupt();

} // namespace sapporobdd

#endif /* BDDXC_INTERNAL_H */
