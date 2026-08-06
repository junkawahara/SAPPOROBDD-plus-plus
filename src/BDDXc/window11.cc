/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Window management for X11)          *
 ****************************************/

#include "bddxc_internal.h"
#include "x11globals.h"

#define DEFAULTWIDTH  500
#define DEFAULTHEIGHT 600

namespace sapporobdd {

#include "yubi.ic"
#include "yubi_mask.ic"
#include "wait.ic"
#include "wait_mask.ic"

unsigned int BDDIOwidth, BDDIOheight;

       Display     *disp;
       Window       window;
       GC           wingc;
       XFontStruct *fontinfo;
static Cursor       yubicu, waitcu;
       int          scrn;
static int          cripx0, cripx1, cripy0, cripy1;


void SetCrippingWindow(unsigned int x0, unsigned int y0,
                       unsigned int x1, unsigned int y1)
{
  cripx0 = (int)x0;
  cripy0 = (int)y0;
  cripx1 = (int)x1;
  cripy1 = (int)y1;
}


static void ClearWindow()
{
  XClearWindow(disp, window);
}


void ResetWindowSize()
{
  XWindowAttributes info;

  XGetWindowAttributes(disp, window, &info);
  if(info.width != (int)BDDIOwidth || info.height != (int)BDDIOheight)
  {
    BDDIOwidth = (unsigned int)info.width;
    BDDIOheight = (unsigned int)info.height;
    SetCrippingWindow(0, 0, BDDIOwidth, BDDIOheight);
    ClearWindow();
  }
  LocationResetLocation(BDDIOwidth, BDDIOheight);
}


void QueryColor(int *foreground, int *background)
{
  *foreground = (int)BlackPixel(disp, scrn);
  *background = (int)WhitePixel(disp, scrn);
}


static void DefineCursor()
{
  static XColor frground = {0L, 0, 0, 0, 0, 0};
  static XColor bground = {0L, 65535, 65535, 65535, 0, 0};
  Pixmap pix, maskpix;

  pix = XCreateBitmapFromData(disp, window,
                              (const char *)yubi_bits, yubi_width, yubi_height);
  maskpix = XCreateBitmapFromData(disp, window,
                                  (const char *)yubi_mask_bits,
                                  yubi_mask_width, yubi_mask_height);
/*  pix = XCreatePixmapFromBitmapData( disp, window,
                                    yubi_bits, yubi_width, yubi_height,
                                    BlackPixel( disp, scrn ),
                                    WhitePixel( disp, scrn ),
                                    1 );
  maskpix = XCreatePixmapFromBitmapData( disp, window,
                                    yubi_bits, yubi_width, yubi_height,
                                    BlackPixel( disp, scrn ),
                                    WhitePixel( disp, scrn ),
                                    1 );
*/
  yubicu = XCreatePixmapCursor(disp, pix, maskpix,
                               &frground, &bground,
                               (unsigned int)yubi_x_hot,
                               (unsigned int)yubi_y_hot);
  XFreePixmap(disp, pix);
  XFreePixmap(disp, maskpix);

  pix = XCreateBitmapFromData(disp, window,
                              (const char *)wait_bits, wait_width, wait_height);
  maskpix = XCreateBitmapFromData(disp, window,
                                  (const char *)wait_mask_bits,
                                  wait_mask_width, wait_mask_height);
  waitcu = XCreatePixmapCursor(disp, pix, maskpix,
                               &frground, &bground,
                               (unsigned int)wait_x_hot,
                               (unsigned int)wait_y_hot);
  XFreePixmap(disp, pix);
  XFreePixmap(disp, maskpix);
}


static void KillCursor()
{
  XFreeCursor(disp, yubicu);
  XFreeCursor(disp, waitcu);
}


int WindowOpen(const char *server, const char *fontname)
{
  char fname[50];

  disp = XOpenDisplay(server);
  if(disp == NULL)
  {
    fprintf(stderr, "Can't open display '%s.'\n",
            (server == NULL)? "": server);
    return FALSE;
  }
  scrn = XDefaultScreen(disp);
  if(fontname == NULL || *fontname == '\0')
  {
    snprintf(fname, sizeof(fname), "%s", "fixed");
  }
  else
  {
    snprintf(fname, sizeof(fname), "%s", fontname);
  }
  fontinfo = XLoadQueryFont(disp, fname);
  if(fontinfo == NULL)
  {
    fprintf(stderr, "Can't open font '%s.'\n", fname);
    return FALSE;
  }
  if(BDDIOwidth == 0 || BDDIOheight == 0)
  {
    BDDIOwidth = DEFAULTWIDTH;
    BDDIOheight = DEFAULTHEIGHT;
    SetCrippingWindow(0, 0, BDDIOwidth, BDDIOheight);
    LocationResetLocation(BDDIOwidth, BDDIOheight);
  }
  window = XCreateSimpleWindow(disp, XRootWindow(disp, scrn),
                     0, 0, BDDIOwidth, BDDIOheight, 3,
                     BlackPixel(disp, scrn), WhitePixel(disp, scrn));
  XSelectInput(disp, window, ButtonPressMask | ExposureMask | KeyPressMask);

  XStoreName(disp, window, "BDD viewer");
  wingc = XCreateGC(disp, window, 0, NULL);
  XSetState(disp, wingc,
            BlackPixel(disp, scrn), WhitePixel(disp, scrn),
            GXcopy, AllPlanes);
  XSetFont(disp, wingc, fontinfo->fid);
  XSetArcMode(disp, wingc, ArcChord);
  DefineCursor();
  XDefineCursor(disp, window, waitcu);
  XMapWindow(disp, window);
  return TRUE;
}


void WindowClose()
{
  Spark();
  XSync(disp, 0);
  KillCursor();
  XDestroyWindow(disp, window);
  XFreeFont(disp, fontinfo);
  XCloseDisplay(disp);
}


void Wait()
{
  XEvent event;
  int exitf;

  exitf = 0;
  do
  {
    XNextEvent(disp, &event);
    switch(event.type)
    {
    case Expose:
      XDefineCursor(disp, window, waitcu);
      Show();
      XDefineCursor(disp, window, yubicu);
      break;
    case ButtonPress:
      exitf = 1;
      break;
    default:
      break;
    }
  }while(exitf == 0);
}


int MeanFontWidth()
{
  return fontinfo->min_bounds.width;
}


int DontCrip(int x, int y)
{
  return (x >= cripx0) & (x <= cripx1)
       & (y >= cripy0) & (y <= cripy1);
}


int Interrupt()
{
  XSync(disp, 0);
  return 0;
}

} // namespace sapporobdd
