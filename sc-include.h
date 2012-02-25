/* $Header: /CVSROOT/scylla-charybdis/sc-include.h,v 1.4 2003/11/02 23:11:02 tino Exp $
 *
 * $Log: sc-include.h,v $
 * Revision 1.4  2003/11/02 23:11:02  tino
 * More cygwin defines
 *
 * Revision 1.3  2003/11/02 22:54:05  tino
 * Cygwin special treatment
 *
 */

#define _GNU_SOURCE	1

#ifdef __CYGWIN__
#include "sc-cygwin.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>

#include <unistd.h>
#include <netdb.h>

#include <sys/stat.h>
#include <sys/time.h>

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/fcntl.h>

#include "version.h"

#ifdef DEBUGGING
#include <stdarg.h>
static void
#define DP(X)	do { debugprintf X; } while (0)
debugprintf(const char *s, ...)
{
  va_list list;

  fprintf(stderr, "[");
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "]\n");
}
#else
#define DP(X)	do { ; } while (0)
#endif
#define xDP(X)	do { ; } while (0)

static void	tino_ssl_errprint(FILE *);
