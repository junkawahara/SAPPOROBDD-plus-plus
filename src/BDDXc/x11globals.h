/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Global X11 resources)               *
 ****************************************/

#ifndef BDDXC_X11GLOBALS_H
#define BDDXC_X11GLOBALS_H

#include <X11/Xlib.h>

namespace sapporobdd {

extern Display     *disp;
extern Window       window;
extern GC           wingc;
extern XFontStruct *fontinfo;
extern int          scrn;

} // namespace sapporobdd

#endif /* BDDXC_X11GLOBALS_H */
