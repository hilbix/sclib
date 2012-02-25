/* $Header: /CVSROOT/scylla-charybdis/example/read-nonblocking.c,v 1.3 2003/10/27 20:44:50 tino Exp $
 *
 * Read FD nonblocking and put result on stdout.
 * Return false if FD on EOF.
 *
 * $Log: read-nonblocking.c,v $
 * Revision 1.3  2003/10/27 20:44:50  tino
 * Missing LOG entry manually added
 *
 * Revision 1.2  2003/10/27 20:42:46  tino
 * CVS keywords added
 *
 * Revision 1.1  2003/10/27 20:41:58  tino
 * Helper to backuploophelper.sh
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int
main(void)
{
  char	buf[BUFSIZ];
  int	i, fl, got;

  if ((fl=fcntl(0, F_GETFL))<0)
    perror("stdin");
  fcntl(0, F_SETFL, fl|O_NONBLOCK);
  got	= read(0, buf, sizeof buf);
  fcntl(0, F_SETFL, fl);
  for (i=0; i<got; )
    {
      int	put;

      put = write(1, buf+i, got-i);
      if (put<0)
	{
	  if (errno==EINTR)
	    continue;
	  return 2;
	}
      i	+= put;
    }
  return got ? 0 : 1;
}
