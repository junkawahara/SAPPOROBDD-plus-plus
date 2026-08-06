/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Common definitions)                 *
 ****************************************/

#ifndef BDDXC_DEFS_H
#define BDDXC_DEFS_H

#ifndef TRUE
#  define TRUE  1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#define EMPTY -1

#define NORMAL   0x00000000
#define NEGATIV  0x00000001
#define EXCHANGE 0x00000002

/* for further expanded edge attribute, name it as 0x4, 0x8, 0x10 ... */

namespace sapporobdd {

extern int edgemode; /* 0:Normal 1:Out-Inv 2:Out&In-Inv */

} // namespace sapporobdd

#endif /* BDDXC_DEFS_H */
