/* $Header: /CVSROOT/scylla-charybdis/sc-cygwin.h,v 1.3 2003/11/02 23:15:41 tino Exp $
 *
 * I really hate conditional compiles and hacks like this here.
 * this is why this is in a separate file.
 *
 * Why can't cygwin define stat64 directly?
 * Isn't the need for such a special define not too much already?
 * What if the #define later makes problems?
 *
 * TRY TO FIX CYGWIN ISSUES HERE.  I don't know how and why myself.
 *
 * $Log: sc-cygwin.h,v $
 * Revision 1.3  2003/11/02 23:15:41  tino
 * Real rude cygwin hacks *sigh*
 *
 * Revision 1.2  2003/11/02 23:11:02  tino
 * More cygwin defines
 *
 * Revision 1.1  2003/11/02 22:54:05  tino
 * Cygwin special treatment
 *
 */

#define __INSIDE_CYGWIN__

#define __stat64 stat64
#include <sys/stat.h>
#undef __stat64

#define O_LARGEFILE 0

extern __off64_t lseek64(int, __off64_t, int);
