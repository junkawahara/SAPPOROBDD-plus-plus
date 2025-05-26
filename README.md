# SAPPOROBDD++

[BDD Package SAPPORO-Edition  (Shin-ichi Minato, Kyoto Univ.)](https://github.com/Shin-ichi-Minato/SAPPOROBDD), modified by Jun Kawahara.

This package has been developed and published by Jun Kawahara under the MIT License. If you find a bug in this package, please report it from the [issue](https://github.com/junkawahara/SAPPOROBDD-plus-plus/issues) page (it is no problem to report issues of original SAPPOROBDD). Do not contact the developer of the original package, Prof. Shin-ichi Minato, about this package.

# Requirement

g++ (GCC) 7.3.0 or later

# Installation

First go to ./src (cd src) and execute a script (source INSTALL) or (source INSTALL32) to compile the BDD package into a library file ../lib/BDD64.a or ../lib/BDD32.a. Then go to a directory under ../app to make an application program using the BDD package.

# Change from SAPPOROBDD

- All the code is rewritten in C++.
  - If you want to use the C language, please use the original [SAPPOROBDD](https://github.com/Shin-ichi-Minato/SAPPOROBDD) package.
- Some compiler warnings are suppressed.
- We change default to 64-bit version and add B_32 macro for 32-bit version.
  - Defining B_64 does not affect the code.
- Namespace 'sapporobdd' is introduced.
  - Put `using namespace sapporobdd;` before using SAPPOROBDD++
- ZBDD is renamed to ZDD.
  - Functions including "ZBDD" are also renamed. For example, ZBDD_Meet is renamed to ZDD_Meet.
  - You can still use "ZBDD".
- The manual is converted to Markdown (and minor mistakes are fixed).
- Tests are added (in the "tests" directory).

# Manual

See [BDDplus.md](man/BDDplus.md) and [ZDD.md](man/classes/ZDD.md) (in Japanese).



# Author

The original [SAPPOROBDD](https://github.com/Shin-ichi-Minato/SAPPOROBDD) pacakge is developed by Shin-ichi MINATO  
at Graduate School of Informatics, Kyoto University.  
(https://www.lab2.kuis.kyoto-u.ac.jp/minato/)  SAPPOROBDD++ is developed by Jun Kawahara.

# License

SAPPOROBDD and SAPPOROBDD++ is under MIT license (https://en.wikipedia.org/wiki/MIT_License)

# Files

__include/*__ :      Header files.  
__src/*__ :          Source files of BDD package.  
__src/INSTALL__ :   Script for compiling all files in src/ .  
__src/INSTALL32__ : Script for compiling all files in src/ on 32bit machine.  
__src/INSTALL_LCM__ : Script for compiling all files in src/ with LCM-library.  
__src/INSTALL32_LCM__ : Script for compiling all files in src/ on 32bit machine with LCM-library.  
__src/CLEAN__ :     Script for deleting all object files in src/ .   
__src/BDDc/*__ :     Core of package written in C.  
__src/BDDXc/*__ :    Part of package related to X11, written in C.  
__src/BDDLCMc/*__ :  Part of package related to LCM, written in C.  
__src/BDD+/*__ :     Main part of package written in C++.  
__lib/*__ :           Compiled BDD library files are stored here.  
__app/*__ :          Source programs of BDD applications.  
__man/*__ :          Manuals of BDD package. (So far Japanese only.)  


