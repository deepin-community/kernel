/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
* OS.C --- SMI DDK
*
* OS or platform dependent files are conditionally compiled here.
*
*******************************************************************/
#include "ddk770_os.h"

/*
 * Use WATCOM DOS Extender compiler to implement the functions in OS.H
 */
#ifdef WDOSE
#include "WATCOM.C"
#endif


/* 
 * Use Linux compiler to implement the functions in OS.H
 */
#ifdef LINUX
#include "ddk770_linux.c"
#endif


/*
 * Use ARM interface functions.
 */
#ifdef SMI_ARM
/* Don't need this when using ARM DS 5 build environment.
   The DS 5 project builder will include armds5.c automatically.

   Other ARM compilers may be different.
*/

//#include "arm.c"
#endif


